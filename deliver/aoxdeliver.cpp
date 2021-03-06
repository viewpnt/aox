// Copyright 2009 The Archiveopteryx Developers <info@aox.org>

#include "scope.h"
#include "event.h"
#include "estring.h"
#include "allocator.h"
#include "stderrlogger.h"
#include "configuration.h"
#include "permissions.h"
#include "logclient.h"
#include "eventloop.h"
#include "injector.h"
#include "mailbox.h"
#include "query.h"
#include "file.h"
#include "user.h"
#include "log.h"
#include "utf.h"

#include <stdlib.h>
#include <stdio.h>

// time, ctime
#include <time.h>
// all the exit codes
#include <sysexits.h>


static void quit( uint s, const EString & m )
{
    if ( !m.isEmpty() )
        fprintf( stderr, "aoxdeliver: %s\n", m.cstr() );
    exit( s );
}


class Deliverator
    : public EventHandler
{
public:
    Query * q;
    Injector * i;
    Injectee * m;
    UString mbn;
    EString un;
    Permissions * p;
    Mailbox * mb;

    Deliverator( Injectee * message,
                 const UString & mailbox, const EString & user )
        : q( 0 ), i( 0 ), m( message ), mbn( mailbox ), un( user ),
          p( 0 ), mb( 0 )
    {
        Allocator::addEternal( this, "deliver object" );
        q = new Query( "select al.mailbox, n.name as namespace, u.login "
                       "from aliases al "
                       "join addresses a on (al.address=a.id) "
                       "left join users u on (al.id=u.alias) "
                       "left join namespaces n on (u.parentspace=n.id) "
                       "where (a.localpart=$1 and a.domain=$2) "
                       "or (lower(u.login)=$3)", this );
        if ( user.contains( '@' ) ) {
            int at = user.find( '@' );
            q->bind( 1, user.mid( 0, at ) );
            q->bind( 2, user.mid( at + 1 ) );
        }
        else {
            q->bindNull( 1 );
            q->bindNull( 2 );
        }
        q->bind( 3, user.lower() );
        q->execute();
    }

    virtual ~Deliverator()
    {
        quit( EX_TEMPFAIL, "Delivery object unexpectedly deleted" );
    }

    void execute()
    {
        if ( q && !q->done() )
            return;

        if ( q && q->done() && !p ) {
            Row * r = q->nextRow();
            q = 0;
            if ( !r )
                quit( EX_NOUSER, "No such user: " + un );
            if ( !r->isNull( "login" ) &&
                 r->getEString( "login" ) == "anonymous" )
                quit( EX_DATAERR, "Cannot deliver to the anonymous user" );
            if ( mbn.isEmpty() ) {
                mb = Mailbox::find( r->getInt( "mailbox" ) );
            }
            else {
                UString pre;
                if ( !r->isNull( "namespace" ) && !mbn.startsWith( "/" ) )
                    pre = r->getUString( "namespace" ) + "/" +
                          r->getUString( "login" ) + "/";
                mb = Mailbox::find( pre + mbn );
                User * u = new User;
                UString anyone;
                anyone.append( "anyone" );
                u->setLogin( anyone );
                if ( mb )
                    p = new Permissions( mb, u, this );
            }
            if ( !mb )
                quit( EX_CANTCREAT, "No such mailbox" );
        }

        if ( p && !p->ready() )
            return;

        if ( p && !p->allowed( Permissions::Post ) )
            quit( EX_NOPERM,
                  "User 'anyone' does not have 'p' right on mailbox " +
                  mbn.ascii().quoted( '\'' ) );

        if ( !i ) {
            EStringList x;
            m->setFlags( mb, &x );
            i = new Injector( this );
            List<Injectee> y;
            y.append( m );
            i->addInjection( &y );
            i->execute();
        }

        if ( !i->done() )
            return;

        if ( i->failed() )
            quit( EX_SOFTWARE, "Injection error: " + i->error() );

        i = 0;
        EventLoop::shutdown();
    }
};


int main( int argc, char *argv[] )
{
    Scope global;

    EString sender;
    UString mailbox;
    EString recipient;
    EString filename;
    int verbose = 0;
    bool error = false;

    int n = 1;
    while ( n < argc ) {
        if ( argv[n][0] == '-' ) {
            switch ( argv[n][1] ) {
            case 'f':
                if ( argc - n > 1 )
                    sender = argv[++n];
                break;

            case 't':
                if ( argc - n > 1 ) {
                    Utf8Codec c;
                    mailbox = c.toUnicode( argv[++n] );
                    if ( !c.valid() )
                        error = true;
                }
                break;

            case 'v':
                {
                    int i = 1;
                    while ( argv[n][i] == 'v' ) {
                        verbose++;
                        i++;
                    }
                    if ( argv[n][i] != '\0' )
                        error = true;
                }
                break;

            default:
                error = true;
                break;
            }
        }
        else if ( recipient.isEmpty() ) {
            recipient = argv[n];
        }
        else if ( filename.isEmpty() ) {
            filename = argv[n];
        }
        else {
            error = true;
        }
        n++;
    }

    if ( error || recipient.isEmpty() ) {
        fprintf( stderr,
                 "Syntax: aoxdeliver [-v] [-f sender] recipient [filename]\n" );
        exit( -1 );
    }

    EString contents;
    if ( filename.isEmpty() ) {
        char s[128];
        while ( fgets( s, 128, stdin ) != 0 )
            contents.append( s );
    }
    else {
        File message( filename );
        if ( !message.valid() ) {
            fprintf( stderr, "Unable to open message file %s\n",
                     filename.cstr() );
            exit( -1 );
        }
        contents = message.contents();
    }

    Configuration::setup( "archiveopteryx.conf" );

    Injectee * message = new Injectee;
    message->parse( contents );
    if ( !message->error().isEmpty() ) {
        fprintf( stderr,
                 "Message parsing failed: %s", message->error().cstr( ) );
        exit( EX_DATAERR );
    }

    if ( verbose > 0 )
        fprintf( stderr, "Sending to <%s>\n", recipient.cstr() );

    EventLoop::setup();
    Database::setup( 1 );
    Log * l = new Log;
    Allocator::addEternal( l, "delivery log" );
    global.setLog( l );
    Allocator::addEternal( new StderrLogger( "aoxdeliver", verbose ),
                           "log object" );

    Configuration::report();
    Mailbox::setup();
    Deliverator * d = new Deliverator( message, mailbox, recipient );
    EventLoop::global()->start();
    if ( !d->i || d->i->failed() )
        return EX_UNAVAILABLE;

    if ( verbose )
        fprintf( stderr,
                 "aoxdeliver: Stored in %s as UID %d\n",
                 d->mb->name().utf8().cstr(),
                 d->m->uid( d->mb ) );
    return 0;
}
