#include "imap.h"

#include "command.h"

#include <test.h>
#include <buffer.h>
#include <arena.h>
#include <list.h>
#include "handlers/capability.h"

#include <time.h>


class IMAPData {
public:
    IMAPData():
        cmdArena( 0 ),
        readingLiteral( false ), literalSize( 0 ),
        args( 0 ),
        state( IMAP::NotAuthenticated ),
        grabber( 0 ),
        idle( false )
    {}

    Arena * cmdArena;
    bool readingLiteral;
    uint literalSize;
    List<String> * args;
    IMAP::State state;
    Command * grabber;
    List<Command> commands;
    bool idle;
};


/*! \class IMAP

  \brief The IMAP class implements the IMAP server seen by clients.

  Most of IMAP functionality is in the command handlers, but this
  class contains the top-level responsibility and functionality. This
  class reads commands from its network connection, does basic
  parsing and creates command handlers as necessary.

  The IMAP state (RFC 3501 section 3) and Idle state (RFC 2177) are
  both encoded here, exactly as in the specification.
*/


/*! Creates a basic IMAP connection */

IMAP::IMAP(int s)
    : Connection(s), d(0)
{
    d = new IMAPData;

    setReadBuffer( new Buffer( fd() ) );
    setWriteBuffer( new Buffer( fd() ) );
    writeBuffer()->append("* OK [");
    writeBuffer()->append( Capability::capabilities() );
    writeBuffer()->append( "]\r\n");
    setTimeout( time(0) + 20 );
}


/*! Destroys the object and closes its network connection. */

IMAP::~IMAP()
{
    delete d;
}


/*! Handles incoming data and timeouts. */

int IMAP::react(Event e)
{
    int result = 1;
    switch (e) {
    case Connection::Read:
        result = parse();
        break;
    case Connection::Timeout:
        writeBuffer()->append("* BAD autologout\r\n");
        result = 0;
        break;
    }
    runCommands();
    if ( state() == Logout )
        result = 0;
    if ( result )
        setTimeout( time(0) + 20 );
    else
        writeBuffer()->write();
    return result;
}


int IMAP::parse()
{
    if ( !d->cmdArena )
        d->cmdArena = new Arena;
    if ( !d->args )
        d->args = new List<String>;
    Arena::push( d->cmdArena );
    Buffer * r = readBuffer();
    while( true ) {
        if ( d->grabber ) {
            d->grabber->read();
            // still grabbed? must wait for more.
            if ( d->grabber )
                return true;
        }
        else if ( d->readingLiteral ) {
            if ( r->size() >= d->literalSize ) {
                d->args->append( r->string( d->literalSize ) );
                r->remove( d->literalSize );
                d->readingLiteral = false;
            }
            else {
                return true; // better luck next time
            }
        }
        else {
            // this is a little evil, isn't it? Buffer::canReadLine()
            // sounds like a good idea after all.
            uint i = 0;
            while( i < r->size() && (*r)[i] != 10 )
                i++;
            if ( (*r)[i] == 10 ) {
                // we have a line; read it and consider literals
                uint j = i;
                if ( i > 0 && (*r)[i-1] == 13 )
                    j--;
                String * s = r->string( j );
                d->args->append( s );
                r->remove( i + 1 ); // string + trailing lf
                if ( s->endsWith( "}" ) ) {
                    i = s->length()-2;
                    bool plus = false;
                    if ( (*s)[i] == '+' ) {
                        plus = true;
                        i--;
                    }
                    j = i;
                    while( i > 0 && (*s)[i] >= '0' && (*s)[i] <= '9' )
                        i--;
                    if ( (*s)[i] == '{' ) {
                        d->readingLiteral = true;
                        bool ok;
                        d->literalSize = s->mid( i+1, j-i-1 ).number( &ok );
                        // if ( ok && size > 99999999 ) ok = false; ? perhaps?
                        if ( !ok ) {
                            writeBuffer()->append( "* BAD literal, BAD\r\n" );
                            return false;
                        }
                        if ( ok && !plus )
                            writeBuffer()->append( "+\r\n" );
                    }
                }
                if ( !d->readingLiteral ) {
                    addCommand();
                    Arena::pop();
                    d->cmdArena = new Arena;
                    Arena::push( d->cmdArena );
                }
            }
            else {
                return true; // better luck next time
            }
        }
    }
    Arena::pop();
}


/*! Does preliminary parsing and adds a new Command object. At some
  point, that object may be executed - we don't care about that for
  the moment.

  During execution of this function, the command's Arena must be used.
*/

void IMAP::addCommand()
{
    List<String> * args = d->args;
    d->args = new List<String>;

    String * s = args->first();

    // pick up the tag
    uint i = (uint)-1;
    uchar c;
    do {
        i++;
        c = (*s)[i];
    } while( i < s->length() &&
             c < 128 && c > ' ' && c != '+' &&
             c != '(' && c != ')' && c != '{' &&
             c != '%' && c != '%' );
    if ( i < 1 || c != ' ' ) {
        writeBuffer()->append( "* BAD tag\r\n" );
        return;
    }
    String tag = s->mid( 0, i );

    // pick up the command
    uint j = i+1;
    do {
        i++;
        c = (*s)[i];
    } while( i < s->length() &&
             c < 128 && 
             ( c > ' ' || 
               ( c == ' ' && s->mid( j, i-j ).lower() == "uid" ) ) &&
             c != '(' && c != ')' && c != '{' &&
             c != '%' && c != '%' &&
             c != '"' && c != '\\' &&
             c != ']' );
    if ( i == j ) {
        writeBuffer()->append( "* BAD no command\r\n" );
        return;
    }
    String command = s->mid( j, i-j );

    // evil hack: skip past a space if there is one, for ease of
    // parsing by the Command classes.
    if ( (*s)[i] == ' ' )
        i++;

    // write the new string into the one in the list
    *s = s->mid( i );

    Command * cmd = Command::create( this, command, tag, args, d->cmdArena );
    if ( cmd ) {
        cmd->parse();
        if ( cmd->ok() && cmd->state() == Command::Executing &&
            !d->commands.isEmpty() ) {
            // we're already executing one or more commands. can this
            // one be started concurrently?
            if ( cmd->group() == 0 ) {
                // no, it can't.
                cmd->setState( Command::Blocked );
            }
            else {
                // do all other commands belong to the same command group?
                d->commands.first();
                while( d->commands.current() && 
                       d->commands.current()->group() == cmd->group() )
                    d->commands.next();
                if ( d->commands.current() )
                    // no, d->commands.current() does not
                    cmd->setState( Command::Blocked );
            }
        }
        d->commands.append( cmd );
    }
    else {
        String tmp( tag );
        tmp += " BAD command unknown: ";
        tmp += command;
        tmp += "\r\n";
        writeBuffer()->append( tmp );
    }
}


static class IMAPTest : public Test {
public:
    IMAPTest() : Test( 500 ) {}
    void test() {

    }
} imapTest;




/*! Returns the current state of this IMAP session, which is one of
  NotAuthenticated, Authenticated, Selected and Logout.
*/

IMAP::State IMAP::state() const
{
    return d->state;
}


/*! Sets this IMAP connection to be in state \a s. The initial value
  is NotAuthenticated.
*/

void IMAP::setState( State s )
{
    d->state = s;
}


/*! Notifies this IMAP connection that it is idle if \a i is true, and
  not idle if \a i is false. An idle connection (see RFC 2177) is one
  in which e.g. EXPUNGE/EXISTS responses may be sent at any time. If a
  connection is not idle, such responses must be delayed until the
  client can listen to them.
*/

void IMAP::setIdle( bool i )
{
    d->idle = i;
}


/*! Returns true if this connection is idle, and false if it is
  not. The initial (and normal) state is false.
*/

bool IMAP::idle() const
{
    return d->idle;
}


/*! Reserves input from the connection for \a command.

  When more input is available, \a Command::read() is called, and as
  soon as the command has read enough, it must call reserve( 0 ) to
  hand the connection back to the general IMAP parser.

  Most commands should never need to call this; it is provided for
  commands that need to read more input after parsing has completed,
  such as IDLE and AUTHENTICATE.

  There is a nasty gotcha: If a command reserves the input stream and
  calls Command::error() while in Blocked state, the command is
  deleted, but there is no way to hand the input stream back to the
  IMAP object. Only the relevant Command knows when it can hand the
  input stream back.
  
  Therefore, Commands that call reserve() simply must hand it back properly
  before calling Command::error() or Command::setState().
*/

void IMAP::reserve( Command * command )
{
    d->grabber = command;
}


/*! Calls execute() on all currently operating commands, and if
  possible calls emitResponses() and retires those which can be
  retired.
*/

void IMAP::runCommands()
{
    Command * c;
    bool more = true;
    while( more ) {
        more = false;
        d->commands.first();
        while( (c=d->commands.current()) != 0 ) {
                Arena::push( c->arena() );
                if ( c->ok() && c->state() == Command::Executing )
                    c->execute();
                if ( !c->ok() )
                    c->setState( Command::Finished );
                if ( c->state() == Command::Finished )
                    c->emitResponses();
                Arena::pop();
                d->commands.next();
            }
            d->commands.first();
            while( (c=d->commands.current()) != 0 ) {
                if ( c->state() == Command::Finished ) {
                    d->commands.take();
                    delete c;
                }
                else {
                    d->commands.next();
                }
            }
        c = d->commands.first();
        if ( c && c->ok() && c->state() == Command::Blocked ) {
            c->setState( Command::Executing );
            more = true;
        }
    };
}
