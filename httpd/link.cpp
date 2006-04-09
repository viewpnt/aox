// Copyright Oryx Mail Systems GmbH. All enquiries to info@oryx.com, please.

#include "link.h"

#include "http.h"
#include "user.h"
#include "page.h"
#include "mailbox.h"
#include "message.h"
#include "stringlist.h"


class LinkData
    : public Garbage
{
public:
    LinkData()
        : type( Link::Unknown ), mailbox( 0 ), uid( 0 )
    {}

    String path;

    Link::Type type;
    Mailbox * mailbox;
    uint uid;
    String part;
};


/*! \class Link link.h

    The Link class is the basic Oryx web page class. It decides what
    needs to be done based on URL path components and tells a Page to
    do it, or makes a path based on other data.
*/


/*! Constructs an empty Link. */

Link::Link()
    : d( new LinkData )
{
}


/*! Constructs and parses a link with path \a s.
*/

Link::Link( const String &s )
    : d( new LinkData )
{
    parse( s );
}


/*!  Constructs a link generally like \a other that references \a
     mailbox.
*/

Link::Link( Link * other, Mailbox * mailbox )
    : d( new LinkData )
{
    if ( other->d->type == ArchiveMailbox ||
         other->d->type == ArchiveMessage ||
         other->d->type == ArchiveSearch ||
         other->d->type == ArchivePart )
        d->type = ArchiveMailbox;
    else
        d->type = WebmailMailbox;
    d->mailbox = mailbox;
}


/*!  Constructs a link generally like \a other that references \a uid
     in \a mailbox.
*/

Link::Link( Link * other, Mailbox * mailbox, uint uid )
    : d( new LinkData )
{
    if ( other->d->type == ArchiveMailbox ||
         other->d->type == ArchiveMessage ||
         other->d->type == ArchiveSearch ||
         other->d->type == ArchivePart )
        d->type = ArchiveMessage;
    else
        d->type = WebmailMessage;
    d->mailbox = mailbox;
    d->uid = uid;
}


/*!  Constructs a link generally like \a other that references part \a
     part of \a uid in \a mailbox.
*/

Link::Link( Link * other, Mailbox * mailbox, uint uid, const String & part )
    : d( new LinkData )
{
    if ( other->d->type == ArchiveMailbox ||
         other->d->type == ArchiveMessage ||
         other->d->type == ArchiveSearch ||
         other->d->type == ArchivePart )
        d->type = ArchivePart;
    else
        d->type = WebmailPart;
    d->mailbox = mailbox;
    d->uid = uid;
    d->part = part;
}


/*! Returns the type of this Link, which may be ArchiveMailbox,
    WebmailMailbox, Webmail, ArchiveMessage, WebmailMessage, or Unknown.
*/

Link::Type Link::type() const
{
    return d->type;
}


/*! Returns a pointer to the mailbox identified by this link, or 0 if
    there is no such mailbox, or if this link does not identify a
    mailbox.

    This value is meaningful only if type() is not Unknown.
*/

Mailbox *Link::mailbox() const
{
    return d->mailbox;
}


/*! Returns the UID, if this Link contains a UID, or 0 if not.

    This value is meaningful only if type() is not Unknown.
*/

uint Link::uid() const
{
    return d->uid;
}


/*! Returns the part number of the message identified by this Link, if
    there is one; or an empty string otherwise. The part number is a
    valid IMAP part number, but may not be valid for the message in
    question.

    This value is meaningful only if type() is not Unknown.
*/

String Link::part() const
{
    return d->part;
}


/*! Parses \a s as a http path. \a s must begin with a slash and
    cannot contain any escape sequences.
*/

void Link::parse( const String & s )
{
    StringList *l = StringList::split( '/', s );
    StringList::Iterator it( l );

    d->path = s;

    // We must assume that the path starts with a /.
    if ( !it || !it->isEmpty() || !++it )
        return;

    if ( it->isEmpty() ) {
        d->type = Webmail;
    }
    else if ( *it == "archive" ) {
        ++it;
        d->type = ArchiveMailbox;
        parseMailbox( it ); // XXX: use archive name, not mailbox ID
        ++it;
        if ( it && *it == "search" ) {
            d->type = ArchiveSearch;
        }
        else {
            if ( it )
                parseUid( ++it );
            if ( it )
                parsePart( ++it );
        }
    }
    else if ( *it == "favicon.ico" ) {
        d->type = Favicon;
    }
    else if ( *it == "logout" ) {
        d->type = Logout;
    }
    else if ( *it == "compose" ) {
        d->type = Compose;
    }
    else {
        d->type = WebmailMailbox;
        parseMailbox( it );
        ++it;
        if ( it && *it == "search" ) {
            d->type = WebmailSearch;
        }
        else {
            if ( it )
                parseUid( it );
            if ( it )
                parsePart( ++it );
        }
    }
}


/*! Generates a path that represents this Link object.
*/

String Link::string() const
{
    String s;
    switch( d->type ) {
    case ArchiveMailbox:
        s = "/archive/" + fn( d->mailbox->id() );
        break;
    case WebmailMailbox:
        s = "/" + fn( d->mailbox->id() );
        break;
    case Webmail:
        s = "/";
        break;
    case ArchiveMessage:
        s = "/archive/" + fn( d->mailbox->id() ) + "/" + fn( d->uid );
        break;
    case ArchivePart:
        s = "/archive/" + fn( d->mailbox->id() ) + "/" + fn( d->uid ) +
            "/" + d->part;
        break;
    case WebmailMessage:
        s = "/" + fn( d->mailbox->id() ) + "/" + fn( d->uid );
        break;
    case WebmailPart:
        s = "/" + fn( d->mailbox->id() ) + "/" + fn( d->uid ) + "/" + d->part;
        break;
    case WebmailSearch:
        s = "/" + fn( d->mailbox->id() ) + "/search";
        break;
    case ArchiveSearch:
        s = "/archive/" + fn( d->mailbox->id() ) + "/search";
        break;
    case Favicon:
        s = "/favicon.ico";
        break;
    case Logout:
        s = "/logout";
        break;
    case Compose:
        s = "/compose";
        break;
    case Unknown:
        s = d->path;
        break;
    }
    return s;
}


/*! Tries to parse \a s as a message uid. */

void Link::parseUid( const String *s )
{
    if ( !s )
        return;

    bool ok = false;
    d->uid = s->number( &ok );
    if ( ok ) {
        if ( d->type == ArchiveMailbox )
            d->type = ArchiveMessage;
        else if ( d->type == WebmailMailbox )
            d->type = WebmailMessage;
    }
}


/*! Tries to parse \a s as a mailbox id. */

void Link::parseMailbox( const String *s )
{
    Mailbox *m = 0;

    if ( !s )
        return;

    bool ok = false;
    uint id = s->number( &ok );
    if ( ok )
        m = Mailbox::find( id );
    if ( !m )
        d->type = Unknown;
    d->mailbox = m;
}


/*! Tries to parse \a s as an IMAP part number. */

void Link::parsePart( const String *s )
{
    if ( !s )
        return;

    char c;
    uint i = 0;
    while ( ( ( c = (*s)[i] ) >= '0' && c <= '9' ) || c == '.' ) {
        if ( c == '.' &&
             ( i == 0 || (*s)[i+1] == '.' || i == s->length()-1 ) )
        {
            d->type = Unknown;
            return;
        }
        i++;
    }

    if ( i < s->length()-1 ) {
        d->type = Unknown;
        return;
    }

    if ( d->type == ArchiveMessage )
        d->type = ArchivePart;
    else
        d->type = WebmailPart;
    d->part = *s;
}
