// Copyright Oryx Mail Systems GmbH. All enquiries to info@oryx.com, please.

#include "threader.h"

#include "transaction.h"
#include "messageset.h"
#include "mailbox.h"
#include "message.h"
#include "field.h"
#include "query.h"
#include "dict.h"


class ThreaderData
    : public Garbage
{
public:
    ThreaderData()
        : state( 0 ), mailbox( 0 ), largestUid( 0 ), largestAtStart( 0 ),
          users( 0 ),
          complete( 0 ), findnew( 0 ), findthreads( 0 ), newishThreads( 0 ),
          createThreads( 0 ), savepoint( 0 ), create( 0 )
    {}

    uint state;
    const Mailbox * mailbox;
    uint largestUid;
    uint largestAtStart;
    List<EventHandler> * users;
    Dict<Thread> threads;
    List<Thread> threadList;

    class NewMessage
        : public Garbage
    {
    public:
        NewMessage(): Garbage(), thread( 0 ), uid( 0 ) {}
        Thread * thread;
        uint uid;
    };
    List<NewMessage> newMessages;

    Query * complete;
    Query * findnew;
    Query * findthreads;
    Query * newishThreads;
    Query * createThreads;
    uint savepoint;
    Transaction * create;
};


/*! \class Threader threader.h

    The Threader class looks at a mailbox, builds an in-memory
    structure of the messages based on a table, and if necessary
    updates that table.
*/



/*! Constructs a threader for \a mailbox, which must not be null. */

Threader::Threader( const Mailbox * mailbox )
    : EventHandler(), d( new ThreaderData )
{
    d->mailbox = mailbox;
}


void Threader::execute()
{
    if ( d->state == 0 && !updated() ) {
        d->state = 1;
        d->largestAtStart = d->largestUid;
    }

    // we need to do something. what?
    Row * r = 0;
    // state 1: find any new thread_members rows for this mailbox
    if ( d->state == 1 ) {
        if ( !d->complete ) {
            d->complete
                = new Query( "select tm.uid, tm.thread, "
                             " t.subject "
                             "from thread_members tm "
                             "join threads t on (t.id=tm.thread) "
                             "where tm.mailbox=$1 and tm.uid>$2",
                             this );
            d->complete->bind( 1, d->mailbox->id() );
            d->complete->bind( 2, d->largestUid );
            d->complete->execute();
        }
        while ( (r=d->complete->nextRow()) ) {
            uint uid = r->getInt( "uid" );
            uint tid = r->getInt( "thread" );
            String subject = r->getString( "subject" );
            Thread * t = d->threads.find( subject );
            if ( !t ) {
                t = new Thread;
                t->setId( tid );
                t->setSubject( subject );
                d->threads.insert( subject, t );
                d->threadList.append( t );
            }
            t->add( uid );
            if ( uid > d->largestUid )
                d->largestUid = uid;
        }
        if ( !d->complete->done() )
            return;
        d->complete = 0;
        d->state = 2;
    }

    // state 2: find the base subjects of new messages and construct
    // the in-ram threads.
    if ( d->state == 2 ) {
        if ( !d->findnew ) {
            d->findnew
                = new Query( "select mm.uid, hf.value "
                             "from header_fields hf "
                             "join mailbox_messages mm using (message) "
                             "left join thread_members tm using (mailbox,uid) "
                             "where mm.mailbox=$1 and hf.field=$2 "
                             " and hf.part='' and tm.thread is null", this );
            d->findnew->bind( 1, d->mailbox->id() );
            d->findnew->bind( 2, HeaderField::Subject );
            d->findnew->execute();
            d->newMessages.clear();
        }
        while ( (r=d->findnew->nextRow()) ) {
            String subject = Message::baseSubject( r->getString( "value" ) );
            uint uid = r->getInt( "uid" );
            Thread * t = d->threads.find( subject );
            if ( !t ) {
                t = new Thread;
                t->setSubject( subject );
                d->threads.insert( subject, t );
                d->threadList.append( t );
            }
            t->add( uid );
            ThreaderData::NewMessage * nm = new ThreaderData::NewMessage;
            nm->uid = uid;
            nm->thread = t;
            d->newMessages.append( nm );
            if ( uid > d->largestUid )
                d->largestUid = uid;
        }
        if ( !d->findnew->done() )
            return;
        d->findnew = 0;
        d->state = 3;
    }

    // state 3: notify the users. the database can be updated later.

    if ( d->state == 3 ) {
        d->state = 4;

        List<EventHandler>::Iterator o( d->users );
        d->users = 0;
        while ( o ) {
            o->execute();
            ++o;
        }
    }

    // state 4: grab the lock on the threads table
    if ( d->state == 4 ) {
        d->create = new Transaction( this );
        Query * q = new Query( "lock threads in exclusive mode", 0 );
        d->create->enqueue( q );
        d->create->execute();
        d->state = 5;
    }

    // state 5/7: look for new threads
    if ( d->state == 5 || d->state == 7 ) {
        if ( d->createThreads ) {
            if ( !d->createThreads->done() )
                return;

            if ( d->createThreads->failed() ) {
                // XXX: We should fail here, so as to break the loop.
                Query * q =
                    new Query( "rollback to b" + fn( d->savepoint ), this );
                d->create->enqueue( q );
            }

            d->createThreads = 0;
            d->savepoint++;
        }

        d->newishThreads = new Query( "", this );
        List<Thread>::Iterator i( d->threadList );
        String s( "select id, subject from threads "
                  "where mailbox=$1 and (" );
        d->newishThreads->bind( 1, d->mailbox->id() );
        uint n = 2;
        while ( i ) {
            Thread * t = i;
            ++i;
            if ( !t->id() ) {
                if ( n > 2 )
                    s.append( " or " );
                d->newishThreads->bind( n, t->subject() );
                s.append( "subject=$" );
                s.append( fn( n ) );
                n++;
            }
        }
        s.append( ")" );
        if ( n == 2 ) {
            d->state += 2;
            d->newishThreads = 0;
        }
        else {
            d->newishThreads->setString( s );
            d->create->enqueue( d->newishThreads );
            d->create->execute();
            d->state++;
        }
    }

    // state 6/8: fetch what we asked for in 5/7
    if ( d->state == 6 || d->state == 8 ) {
        Row * r = d->newishThreads->nextRow();
        while ( r ) {
            Thread * t = d->threads.find( r->getString( "subject" ) );
            if ( t )
                t->setId( r->getInt( "id" ) );
            r = d->newishThreads->nextRow();
        }

        if ( !d->newishThreads->done() )
            return;

        if ( d->state == 8 ) {
            d->state = 9;
        }
        else {
            d->state = 7;

            List<Thread>::Iterator i( d->threadList );
            Query * q = 0;
            while ( i ) {
                Thread * t = i;
                ++i;
                if ( !t->id() ) {
                    if ( !q )
                        q = new Query( "copy threads (mailbox,subject) "
                                       "from stdin with binary", this );
                    q->bind( 1, d->mailbox->id(), Query::Binary );
                    q->bind( 2, t->subject(), Query::Binary );
                    q->submitLine();
                }
            }
            if ( q ) {
                d->createThreads = q;
                q = new Query( "savepoint b" + fn( d->savepoint ), this );
                d->create->enqueue( q );
                d->create->enqueue( d->createThreads );
                d->create->execute();
            }
        }
    }

    // state 9: insert the new thread_members rows
    if ( d->state == 9 ) {
        if ( !d->newMessages.isEmpty() ) {
            Query * q 
                = new Query( "copy thread_members (thread,mailbox,uid) "
                             "from stdin with binary", this );
            List<ThreaderData::NewMessage>::Iterator i( d->newMessages );
            while ( i ) {
                ThreaderData::NewMessage * nm = i;
                ++i;
                q->bind( 1, nm->thread->id(), Query::Binary );
                q->bind( 2, d->mailbox->id(), Query::Binary );
                q->bind( 3, nm->uid, Query::Binary );
                q->submitLine();
            }
            d->create->enqueue( q );
            d->create->execute();
            d->newMessages.clear();
        }
        d->create->commit();
        d->state = 10;
    }

    if ( d->state == 10 ) {
        if ( !d->create->done() )
            return;
        d->state = 0;
        d->create = 0;
        List<EventHandler>::Iterator o( d->users );
        d->users = 0;
        while ( o ) {
            o->execute();
            ++o;
        }
    }
}


/*! Returns true if this Threader has complete data for mailbox(), and
    false if refresh() needs to be called or is working. If \a
    alsoOnDisk is true, updated() additionally checks whether the
    database tables are completely updated.
*/

bool Threader::updated( bool alsoOnDisk ) const
{
    // is the state being updated?
    if ( d->state > 1 && d->state < 4 )
        return false;
    // are we currently writing to disk?
    if ( d->state > 1 && alsoOnDisk )
        return false;
    // do we have all the information?
    if ( d->largestUid + 1 >= d->mailbox->uidnext() )
        return true;
    return false;
}


/*! Returns a pointer to the Mailbox mapped by this Threader. This
    cannot be a null pointer in a valid object.
*/

const Mailbox * Threader::mailbox() const
{
    return d->mailbox;
}


/*! Starts updating the thread data for mailbox(), if that's
    necessary. If refresh() causes any work to be done, the \a user
    will be notified of completion using EventHandler::execute().
*/

void Threader::refresh( EventHandler * user )
{
    if ( updated( true ) )
        return;
    if ( !d->users )
        d->users = new List<EventHandler>;
    if ( !d->users->find( user ) )
        d->users->append( user );
    if ( d->state == 0 )
        execute();
}


class ThreadData
    : public Garbage
{
public:
    ThreadData(): id( 0 ) {}

    uint id;
    String subject;
    MessageSet members;
};


/*! \class Thread threader.h

    The Thread class models a simple thread. Not a pretty tree or even
    DAG, just a set of messages and a subject.

    The Thread class is meant to be small, small and small: Sometimes
    (perhaps often) we need to keep Thread objects for an entire
    Mailbox in RAM. Size is more important than functionality.

    If an IMAP THREAD command needs to return a tree, it has to
    compute the tree itself. This class can help make that simpler,
    that's all.

    If an ArchiveThread needs to display information about some/all
    threads, this class can help make it simpler, but it isn't
    sufficient in and of itself.

    The Threader creates, owns and updates Thread objects.
*/



/*! Constructs an empty Thread. */

Thread::Thread()
    : d( new ThreadData )
{
}


/*! Returns all the members of this thread. This may include deleted
    messages. */

MessageSet Thread::members() const
{
    return d->members;
}


/*! Records that \a uid is a member of this thread. */

void Thread::add( uint uid )
{
    d->members.add( uid );
}


/*! Records that \a subject is the base subject of this thread
    (ie. without "re", "fwd" or similar).
*/

void Thread::setSubject( const String & subject )
{
    d->subject = subject;
}


/*! Returns whatever was set by setSubject(), or an empty string
    initially.
*/

String Thread::subject() const
{
    return d->subject;
}


/*! Returns the database ID of this thread, or 0 if the thread still
    isn't recorded in the database. 0 is perfectly possible - Threader
    will notify its users as soon as it can, even if the thread IDs
    aren't known yet.
*/

uint Thread::id() const
{
    return d->id;
}


/*! Records that \a id is the database ID of this thread. */

void Thread::setId( uint id )
{
    d->id = id;
}


/*! Returns a pointer to an unsorted list of all threads. Never
    returns a null pointer. The returned list should not be modified.
*/

List<Thread> * Threader::allThreads() const
{
    return &d->threadList;
}
