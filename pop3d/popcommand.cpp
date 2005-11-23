// Copyright Oryx Mail Systems GmbH. All enquiries to info@oryx.com, please.

#include "popcommand.h"

#include "tls.h"
#include "plain.h"
#include "query.h"
#include "mechanism.h"
#include "stringlist.h"


class PopCommandData
    : public Garbage
{
public:
    PopCommandData()
        : pop( 0 ), args( 0 ), done( false ),
          tlsServer( 0 ), m( 0 ), q( 0 )
    {}

    POP * pop;
    PopCommand::Command cmd;
    StringList * args;

    bool done;

    TlsServer * tlsServer;
    SaslMechanism * m;
    Query * q;
};


/*! \class PopCommand popcommand.h
    This class represents a single POP3 command. It is analogous to an
    IMAP Command, except that it does all the work itself, rather than
    leaving it to subclasses.
*/


/*! Creates a new PopCommand object representing the command \a cmd, for
    the POP server \a pop.
*/

PopCommand::PopCommand( POP * pop, Command cmd, StringList * args )
    : d( new PopCommandData )
{
    d->pop = pop;
    d->cmd = cmd;
    d->args = args;
}


/*! Marks this command as having finished execute()-ing. Any responses
    are written to the client, and the POP server is instructed to move
    on to processing the next command.
*/

void PopCommand::finish()
{
    d->done = true;
    d->pop->write();
    d->pop->runCommands();
}


/*! Returns true if this PopCommand has finished executing, and false if
    execute() hasn't been called, or if it has work left to do. Once the
    work is done, execute() calls finish() to signal completion.
*/

bool PopCommand::done()
{
    return d->done;
}


void PopCommand::execute()
{
    switch ( d->cmd ) {
    case Quit:
        log( "Closing connection due to QUIT command", Log::Debug );
        d->pop->setState( POP::Update );
        d->pop->ok( "Goodbye." );
        break;

    case Capa:
        d->pop->ok( "Supported capabilities:" );
        // d->pop->enqueue( "TOP\r\n" );
        d->pop->enqueue( "SASL\r\n" );
        d->pop->enqueue( "STLS\r\n" );
        d->pop->enqueue( "USER\r\n" );
        d->pop->enqueue( "RESP-CODES\r\n" );
        d->pop->enqueue( "PIPELINING\r\n" );
        // d->pop->enqueue( "UIDL\r\n" );
        d->pop->enqueue( "IMPLEMENTATION Oryx POP3 Server.\r\n" );
        d->pop->enqueue( ".\r\n" );
        break;

    case Stls:
        if ( !startTls() )
            return;
        break;

    case Auth:
        d->pop->err( "Unimplemented." );
        break;

    case User:
        d->pop->setUser( nextArg() );
        d->pop->ok( "Send PASS." );
        break;

    case Pass:
        if ( !pass() )
            return;
        break;

    case Noop:
        d->pop->ok( "Done" );
        break;
    }

    finish();
}


/*! Handles the STLS command. */

bool PopCommand::startTls()
{
    if ( !d->tlsServer ) {
        d->tlsServer = new TlsServer( this, d->pop->peer(), "POP" );
        d->pop->reserve( 1 );
    }

    if ( !d->tlsServer->done() )
        return false;

    d->pop->ok( "Begin TLS negotiation." );
    d->pop->reserve( 0 );
    d->pop->write();
    d->pop->startTls( d->tlsServer );
    return true;
}


/*! Handles the PASS command. */

bool PopCommand::pass()
{
    if ( !d->m ) {
        d->m = new Plain( this );
        d->m->setLogin( d->pop->user() );
        d->m->setSecret( nextArg() );
    }

    d->m->query();
    if ( !d->m->done() )
        return false;

    if ( d->m->state() == SaslMechanism::Succeeded ) {
        d->pop->ok( "Authentication succeeded." );
        d->pop->setState( POP::Transaction );
    }
    else {
        d->pop->err( "Authentication failed." );
    }

    return true;
}


/*! This function returns the next argument supplied by the client for
    this command, or an empty string if there are no more arguments.
    (Should we assume that nextArg will never be called more times
    than there are arguments? The POP parser does enforce this.)
*/

String PopCommand::nextArg()
{
    if ( d->args && !d->args->isEmpty() )
        return *d->args->take( d->args->first() );
    return "";
}
