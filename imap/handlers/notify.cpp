// Copyright 2009 The Archiveopteryx Developers <info@aox.org>

#include "notify.h"

#include "mailboxgroup.h"
#include "transaction.h"
#include "imapparser.h"
#include "eventmap.h"
#include "status.h"
#include "fetch.h"


class NotifyData
    : public Garbage
{
public:
    NotifyData(): status( false ), events( new EventMap ) {}

    bool status;
    EventMap * events;
};


/*! \class Notify notify.h

    The Notify class implements the IMAP NOTIFY extension, RFC 5465.

    It doesn't actually do very much, just parse the MUA's wishes and
    sets a variable or two or three or forty-four for IMAP to use.
*/



Notify::Notify()
    : Command(), d( new NotifyData )
{
    // nothing more
}


void Notify::parse()
{
    space();
    if ( present( "none" ) ) {
        end();
        return;
    }
    require( "set" );
    space();
    if ( present( "status" ) ) {
        d->status = true;
        space();
    }
    parseEventGroup();
    while ( ok() && !parser()->atEnd() ) {
        space();
        parseEventGroup();
    }
    end();
}


/*! Parses an even group and updates state. */

void Notify::parseEventGroup()
{
    EventFilterSpec * s = new EventFilterSpec;
    require( "(" );
    if ( present( "selected" ) ) {
        s->setType( EventFilterSpec::Selected );
    }
    else if ( present( "selected-delayed" ) ) {
        s->setType( EventFilterSpec::SelectedDelayed );
    }
    else if ( present( "inboxes" ) ) {
        s->setType( EventFilterSpec::Inboxes );
    }
    else if ( present( "personal" ) ) {
        s->setType( EventFilterSpec::Personal );
    }
    else if ( present( "subscribed" ) ) {
        s->setType( EventFilterSpec::Subscribed );
    }
    else if ( present( "subtree" ) ) {
        s->setType( EventFilterSpec::Subtree );
        space();
        s->setMailboxes( parseMailboxes() );
    }
    else if ( present( "mailboxes" ) ) {
        s->setType( EventFilterSpec::Mailboxes );
        space();
        s->setMailboxes( parseMailboxes() );
    }
    else {
        error( Bad, "Expected SELECTED, INBOXES, etc." );
    }
    space();
    if ( present( "none" ) ) {
        //huh
        require( ")" );
        return;
    }

    if ( !ok() )
        return;

    require( "(" );
    parseEvent( s );
    while ( ok() && present( " " ) )
        parseEvent( s );
    require( ")" );
    require( ")" );
    d->events->add( s );
}


/*! Parses a single event description and records it using \a s. */

void Notify::parseEvent( EventFilterSpec * s )
{
    if ( present( "messagenew" ) ) {
        // "MessageNew" [SP "(" fetch-att *(SP fetch-att) ")" ]
        if ( present( " " ) ) {
            require( "(" );
            Fetch * f = new Fetch( false );
            f->setParser( parser() );
            f->parseAttribute( false );
            while( parser()->ok() && present( " " ) )
                f->parseAttribute( false );
            require( ")" );
            s->setNewMessageFetcher( f );
        }
    } else if ( present( "messageexpunge" ) ) {
        s->setNotificationWanted( EventFilterSpec::Expunge, true );
    } else if ( present( "flagchange" ) ) {
        space();
        s->setNotificationWanted( EventFilterSpec::FlagChange, true );
    } else if ( present( "annotationchange" ) ) {
        space();
        s->setNotificationWanted( EventFilterSpec::AnnotationChange, true );
    } else if ( present( "mailboxname" ) ) {
        s->setNotificationWanted( EventFilterSpec::MailboxName, true );
    } else if ( present( "subscriptionchange" ) ) {
        s->setNotificationWanted( EventFilterSpec::Subscription, true );
    } else if ( present( "mailboxmetadatachange" ) ) {
        // N/A until we implement METADATA
    } else if ( present( "servermetadatachange" ) ) {
        // do we ever notify about this? no? we can ignore it for now.
    } else {
        EString a = atom();
        error( Bad, "Unknown event type: " + a );
    }
}


/*! Parses the one-or-many-mailbox item and returns a pointer to a
    list of mailboxes. The return value will not be a null pointer,
    but can point to an empty list, and in case of errors the list
    mail contain null pointers.
*/

List<Mailbox> * Notify::parseMailboxes()
{
    List<Mailbox> * l = new List<Mailbox>;
    if ( present( "(" ) ) {
        l->append( mailbox() );
        while ( ok() && present( " " ) )
            l->append( mailbox() );
        require( ")" );
    }
    else {
        l->append( mailbox() );
    }
    return l;
}


/*! Activates the parsed notification. May run a bunch of STATUS
    queries.
*/

void Notify::execute()
{
    if ( state() != Executing || !imap()->user() )
        return;
    if ( !transaction() ) {
        setTransaction( new Transaction( this ) );
        d->events->refresh( transaction(), imap()->user() );
        transaction()->commit();
    }
    if ( !transaction()->done() )
        return;
    if ( d->status ) {
        List<Mailbox> * m = d->events->mailboxes();
        (void)new MailboxGroup( m, imap() );
        List<Mailbox>::Iterator i( m );
        while ( i ) {
            (void)new Status( this, i );
            ++i;
        }
    }
    imap()->setEventMap( d->events );
    finish();
}
