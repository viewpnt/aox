// Copyright 2009 The Archiveopteryx Developers <info@aox.org>

#include "idle.h"

#include "imap.h"
#include "buffer.h"
#include "mailbox.h"
#include "imapsession.h"


/*! \class Idle idle.h
    Implements the RFC 2177 IDLE extension.

    The IDLE extension permits IMAP clients to remain idle, while the
    IMAP server may send EXPUNGE, EXISTS and flag updates at any time.

    This implementation differs from that implied by the RFC in that
    +/DONE is not actually part of the command; Idle prints the +
    itself and waits for DONE during command execution. Thus, "parse
    errors" are not handled in parse().

    For some reason, RFC 2177 permits IDLE to be called in
    authenticated state. We must be careful not to assume otherwise.
*/


/*! Switches to IDLE mode and grabs the input, such that the DONE can
    be properly processed.
*/

void Idle::execute()
{
    // find the mailbox we're looking at, if any
    Mailbox * m = 0;
    if ( imap()->session() )
        m = imap()->session()->mailbox();

    // if the connection went away while we were idling, finish off.
    if ( !m || imap()->Connection::state() != Connection::Connected )
        read();

    if ( idling )
        return;

    imap()->reserve( this );
    imap()->enqueue( "+ idling\r\n" );
    idling = true;
}


/*! Reads the "DONE" line and switches off IDLE mode. */

void Idle::read()
{
    if ( imap()->Connection::state() != Connection::Connected ) {
        error( Bad, "Leaving idle mode due to connection state change" );
    }
    else {
        EString * s = imap()->readBuffer()->removeLine();
        if ( !s )
            return;

        EString r = s->lower();
        if ( r != "done" )
            error( Bad, "Leaving idle mode due to syntax error: " + r );
        else
            finish();
    }
}
