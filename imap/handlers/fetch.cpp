// Copyright Oryx Mail Systems GmbH. All enquiries to info@oryx.com, please.

#include "fetch.h"

#include "imapsession.h"
#include "transaction.h"
#include "annotation.h"
#include "messageset.h"
#include "stringlist.h"
#include "mimefields.h"
#include "imapparser.h"
#include "bodypart.h"
#include "occlient.h"
#include "address.h"
#include "mailbox.h"
#include "message.h"
#include "ustring.h"
#include "section.h"
#include "listext.h"
#include "fetcher.h"
#include "codec.h"
#include "query.h"
#include "scope.h"
#include "store.h"
#include "imap.h"
#include "flag.h"
#include "date.h"
#include "user.h"
#include "dict.h"
#include "utf.h"



static const char * legalAnnotationAttributes[] = {
    "value",
    "value.priv",
    "value.shared",
    "size",
    "size.priv",
    "size.shared",
    0
};


class FetchData
    : public Garbage
{
public:
    FetchData()
        : state( 0 ), peek( true ),
          changedSince( 0 ), notThose( 0 ),
          uid( false ),
          flags( false ), envelope( false ),
          body( false ), bodystructure( false ),
          internaldate( false ), rfc822size( false ),
          annotation( false ), modseq( false ),
          needHeader( false ), needBody( false )
    {}

    int state;
    bool peek;
    MessageSet set;
    MessageSet expunged;
    List<Message> requested;
    uint changedSince;
    Query * notThose;

    // we want to ask for...
    bool uid;
    bool flags;
    bool envelope;
    bool body;
    bool bodystructure;
    bool internaldate;
    bool rfc822size;
    bool annotation;
    bool modseq;
    List<Section> sections;

    // and the sections imply that we...
    bool needHeader;
    bool needBody;

    StringList entries;
    StringList attribs;

    class SeenFlagSetter
        : public EventHandler
    {
    public:
        SeenFlagSetter( ImapSession *, const MessageSet & m,
                        EventHandler * owner );
        void execute();

        MessageSet messages;
        Transaction * t;
        Flag * seen;
        Query * f;
        Query * ms;
        ImapSession * session;
        EventHandler * o;
        Mailbox * mailbox;
        int64 modseq;
    };
};


/*! \class Fetch fetch.h
    Returns message data (RFC 3501, section 6.4.5).

    Our parser used to be slightly more permissive than the RFC. This is
    a bug, and many of the problems have been corrected (but not tested).

    There's quite a bit of support for RFC 4551, CONDSTORE.
*/


/*! Creates a new handler for FETCH if \a u is false, or for UID FETCH
    if \a u is true.
*/

Fetch::Fetch( bool u )
    : Command(), d( new FetchData )
{
    d->uid = u;
    if ( u )
        setGroup( 1 );
    else
        setGroup( 2 );
}


// fetch           = "FETCH" SP set SP ("ALL" / "FULL" / "FAST" / fetch-att /
//                   "(" fetch-att *(SP fetch-att) ")")
// fetch-att       = "ENVELOPE" / "FLAGS" / "INTERNALDATE" /
//                   "RFC822" [".HEADER" / ".SIZE" / ".TEXT"] /
//                   "BODY" ["STRUCTURE"] / "UID" /
//                   "BODY" [".PEEK"] section ["<" number "." nz-number ">"]
//                 / "MODSEQ" ; 4551
// section         = "[" [section-spec] "]"
// section-spec    = section-msgtext / (section-part ["." section-text])
// section-msgtext = "HEADER" / "HEADER.FIELDS" [".NOT"] SP header-list /
//                   "TEXT"
// section-part    = nz-number *("." nz-number)
// section-text    = section-msgtext / "MIME"
// header-list     = "(" header-fld-name *(SP header-fld-name) ")"
// header-fld-name = astring


void Fetch::parse()
{
    space();
    d->set = set( !d->uid );
    space();
    if ( nextChar() == '(' ) {
        // "(" fetch-att *(SP fetch-att) ")")
        step();
        parseAttribute( false );
        while( nextChar() == ' ' ) {
            step();
            parseAttribute( false );
        }
        require( ")" );
    }
    else {
        // single fetch-att, or the macros
        parseAttribute( true );
    }
    if ( present( " (" ) ) {
        // RFC 4466 fetch-modifiers
        parseFetchModifier();
        while ( present( " " ) )
            parseFetchModifier();
        require( ")" );
    }
    end();
    if ( d->envelope || d->body || d->bodystructure )
        d->needHeader = true;
    if ( d->body || d->bodystructure )
        d->needBody = true;
    if ( d->needBody )
        d->needHeader = true;
    if ( !ok() )
        return;
    StringList l;
    l.append( new String( "Fetch " + fn( d->set.count() ) + " messages: " ) );
    if ( d->needHeader )
        l.append( "header/address" );
    if ( d->needBody )
        l.append( "body" );
    if ( d->flags )
        l.append( "flags" );
    if ( d->rfc822size || d->internaldate || d->modseq )
        l.append( "trivia" );
    if ( d->annotation )
        l.append( "annotations" );
    log( l.join( " " ) );
}


/*! This helper is responsible for parsing a single attribute from the
    fetch arguments. If \a alsoMacro is true, this function parses a
    macro as well as a single attribute.
*/

void Fetch::parseAttribute( bool alsoMacro )
{
    String keyword = dotLetters( 3, 13 ).lower(); // UID/ALL, RFC822.HEADER
    if ( alsoMacro && keyword == "all" ) {
        // equivalent to: (FLAGS INTERNALDATE RFC822.SIZE ENVELOPE)
        d->flags = true;
        d->envelope = true;
        d->internaldate = true;
        d->rfc822size = true;
    }
    else if ( alsoMacro && keyword == "full" ) {
        // equivalent to: (FLAGS INTERNALDATE RFC822.SIZE ENVELOPE BODY)
        d->flags = true;
        d->envelope = true;
        d->body = true;
        d->internaldate = true;
        d->rfc822size = true;
    }
    else if ( alsoMacro && keyword == "fast" ) {
        // equivalent to: (FLAGS INTERNALDATE RFC822.SIZE)
        d->flags = true;
        d->internaldate = true;
        d->rfc822size = true;
    }
    else if ( keyword == "envelope" ) {
        d->envelope = true;
    }
    else if ( keyword == "flags" ) {
        d->flags = true;
    }
    else if ( keyword == "internaldate" ) {
        d->internaldate = true;
    }
    else if ( keyword == "rfc822" ) {
        d->peek = false;
        d->needHeader = true;
        d->needBody = true;
        Section * s = new Section;
        s->id = keyword;
        d->sections.append( s );
    }
    else if ( keyword == "rfc822.header" ) {
        d->needHeader = true;
        Section * s = new Section;
        s->id = keyword;
        d->sections.append( s );
    }
    else if ( keyword == "rfc822.size" ) {
        d->rfc822size = true;
    }
    else if ( keyword == "annotation" ) {
        d->annotation = true;
        require( " " );
        parseAnnotation();
    }
    else if ( keyword == "rfc822.text" ) {
        d->peek = false;
        d->needBody = true;
        Section * s = new Section;
        s->id = keyword;
        d->sections.append( s );
    }
    else if ( keyword == "body.peek" && nextChar() == '[' ) {
        step();
        parseBody( false );
    }
    else if ( keyword == "body" ) {
        if ( nextChar() == '[' ) {
            d->peek = false;
            step();
            parseBody( false );
        }
        else {
            d->body = true;
            // poor man's bodystructure
        }
    }
    else if ( keyword == "bodystructure" ) {
        d->bodystructure = true;
        // like body, but with bells and whistles
    }
    else if ( keyword == "uid" ) {
        d->uid = true;
    }
    else if ( keyword == "binary.peek" && nextChar() == '[' ) {
        step();
        parseBody( true );
    }
    else if ( keyword == "binary" && nextChar() == '[' ) {
        d->peek = false;
        step();
        parseBody( true );
    }
    else if ( keyword == "binary.size" && nextChar() == '[' ) {
        step();
        parseBody( true );
        Section * s = d->sections.last();
        s->id = "size";
        if ( s->partial )
            error( Bad, "Fetching partial BINARY.SIZE is not meaningful" );
        if ( s->part.isEmpty() )
            d->rfc822size = true;
    }
    else if ( keyword == "modseq" ) {
        d->modseq = true;
    }
    else {
        error( Bad, "expected fetch attribute, saw word " + keyword );
    }
}


/*! This utility function fetches at least \a min, at most \a max
    characters, all of which must be a letter, a digit or a dot.
    Consecutive dots ARE allowed.
*/

String Fetch::dotLetters( uint min, uint max )
{
    String r( parser()->dotLetters( min, max ) );
    if ( !parser()->ok() )
        error( Bad, parser()->error() );
    return r;
}


/*! Uses the ImapParser \a ip to parse a section-text production, and
    returns a pointer to a suitably constructed Section object. Upon
    return, the ImapParser's cursor is advanced to point past the end
    of the section-text. \a ip must not be 0; and the return value of
    this function is also guaranteed to be non-zero.

    If \a binary is false (the default), then the BINARY extensions of
    RFC 3516 are summarily ignored.

    If there were any parsing errors, Section::error will be non-empty.
*/

Section * Fetch::parseSection( ImapParser * ip, bool binary )
{
    Section * s = new Section;
    s->binary = binary;

    // section-spec    = section-msgtext / (section-part ["." section-text])
    // section-msgtext = "HEADER" /
    //                   "HEADER.FIELDS" [".NOT"] SP header-list /
    //                   "TEXT"
    // section-part    = nz-number *("." nz-number)
    // section-text    = section-msgtext / "MIME"

    // Parse a section-part.
    bool dot = false;
    if ( ip->nextChar() >= '0' && ip->nextChar() <= '9' ) {
        String part;
        part.append( fn( ip->nzNumber() ) );
        while ( ip->nextChar() == '.' ) {
            ip->step();
            if ( ip->nextChar() >= '0' && ip->nextChar() <= '9' ) {
                part.append( "." );
                part.append( fn( ip->nzNumber() ) );
            }
            else {
                dot = true;
                break;
            }
        }
        s->part = part;
        if ( !dot )
            return s;
    }

    // Parse any section-text.
    String item = ip->dotLetters( 0, 17 ).lower();
    if ( binary && !item.isEmpty() ) {
        s->error = "BINARY with section-text is not legal, saw " + item;
    }
    else if ( item == "text" ) {
        if ( s->part.isEmpty() )
            s->needsHeader = false;
    }
    else if ( item == "header" ) {
        if ( s->part.isEmpty() )
            s->needsBody = false;
    }
    else if ( item == "header.fields" ||
              item == "header.fields.not" )
    {
        if ( s->part.isEmpty() )
            s->needsBody = false;
        ip->require( " (" );
        s->fields.append( new String( ip->astring().headerCased() ) );
        while ( ip->nextChar() == ' ' ) {
            ip->require( " " );
            s->fields.append( new String( ip->astring().headerCased() ) );
        }
        ip->require( ")" );
    }
    else if ( item == "mime" ) {
        if ( s->part.isEmpty() )
            s->error = "MIME requires a section-part.";
    }
    else if ( !item.isEmpty() || dot ) {
        s->error =
            "Expected text, header, header.fields etc, not " + item +
            ip->following();
    }

    s->id = item;
    return s;
}


/*! Parses a bodypart description - the bit following "body[" in an
    attribute. The cursor must be after '[' on entry, and is left
    after the trailing ']'.

    If \a binary is true, the parsed section will be sent using the
    BINARY extension (RFC 3516). If not, it'll be sent using a normal
    BODY.
*/

void Fetch::parseBody( bool binary )
{
    Section * s = parseSection( parser(), binary );
    if ( !s->error.isEmpty() ) {
        error( Bad, s->error );
        return;
    }

    require( "]" );

    // Parse any range specification.
    if ( nextChar() == '<' ) {
        s->partial = true;
        step();
        s->offset = number();
        require( "." );
        s->length = nzNumber();
        require( ">" );
    }

    d->sections.append( s );
    if ( s->needsHeader )
        d->needHeader = true;
    if ( s->needsBody )
        d->needBody = true;
}


void record( StringList & l, Dict<void> & d, const String & a )
{
    if ( !d.contains( a.lower() ) )
        l.append( new String( a ) );
    d.insert( a.lower(), (void *)1 );
}


/*! Parses the entries and attributes from an ANNOTATION fetch-att.
    Expects the cursor to be on the first parenthesis, and advances
    it to past the last one.
*/

void Fetch::parseAnnotation()
{
    bool atEnd;
    bool paren;

    // Simplified ABNF from draft-ietf-imapext-annotate-15:
    //
    //  fetch-att =/ "ANNOTATION" SP "(" entries SP attribs ")"
    //  entries   = list-mailbox /
    //              "(" list-mailbox *(SP list-mailbox) ")"
    //  attribs   = astring /
    //              "(" astring *(SP astring) ")"

    require( "(" );

    paren = false;
    if ( nextChar() == '(' ) {
        step();
        paren = true;
    }

    atEnd = false;
    while ( !atEnd ) {
        d->entries.append( new String( listMailbox() ) );

        if ( paren ) {
            if ( nextChar() == ')' ) {
                step();
                atEnd = true;
            }
            else {
                space();
            }
        }
        else {
            atEnd = true;
        }
    }

    require( " " );

    paren = false;
    if ( nextChar() == '(' ) {
        step();
        paren = true;
    }

    Dict<void> attribs;

    atEnd = false;
    while ( !atEnd ) {
        String a( astring() );

        // XXX: This check (and the legalAnnotationAttributes table) is
        // duplicated in Search::parseKey(). But where should a common
        // attribute-checking function live?
        uint i = 0;
        while ( ::legalAnnotationAttributes[i] &&
                a != ::legalAnnotationAttributes[i] )
            i++;
        if ( !::legalAnnotationAttributes[i] )
            error( Bad, "Unknown annotation attribute: " + a );

        if ( a.endsWith( ".priv" ) || a.endsWith( ".shared" ) ) {
            record( d->attribs, attribs, a );
        }
        else {
            record( d->attribs, attribs, a + ".priv" );
            record( d->attribs, attribs, a + ".shared" );
        }

        if ( paren ) {
            if ( nextChar() == ')' ) {
                step();
                atEnd = true;
            }
            else {
                space();
            }
        }
        else {
            atEnd = true;
        }
    }

    require( ")" );
}


void Fetch::execute()
{
    if ( state() != Executing )
        return;

    ImapSession * s = imap()->session();

    if ( !d->peek && s->readOnly() )
        d->peek = true;

    if ( d->state == 0 ) {
        if ( d->changedSince ) {
            if ( !d->notThose ) {
                d->notThose = new Query( "select uid from modsequences "
                                         "where mailbox=$1 and modseq<=$2 "
                                         "and " + d->set.where(),
                                         this );
                d->notThose->bind( 1, s->mailbox()->id() );
                d->notThose->bind( 2, d->changedSince );
                d->notThose->execute();
            }
            Row * r;
            while ( (r=d->notThose->nextRow()) != 0 )
                d->set.remove( r->getInt( "uid" ) );
            if ( !d->notThose->done() )
                return;
        }
        d->state = 1;
    }

    if ( d->state == 1 ) {
        d->state = 2;
        if ( !d->peek ) {
            (void)new FetchData::SeenFlagSetter( s, d->set, this );
            return;
        }
    }

    if ( d->state == 2 ) {
        if ( group() == 2 ) // then RFC 2180 section 4.1.2 applies
            d->expunged = s->expunged().intersection( d->set );
        shrink( &d->set );
        d->state = 3;
    }

    while ( d->state == 3 || d->state == 4 ) {
        if ( d->state == 3 ) {
            d->state = 4;
            sendFetchQueries();
        }

        bool ok = true;
        while ( ok && !d->requested.isEmpty() ) {
            Message * m = d->requested.first();
            uint msn = s->msn( m->uid() );
            if ( ( !d->annotation || m->hasAnnotations() ) &&
                 ( !d->needHeader || ( m->hasHeaders() &&
                                       m->hasAddresses() ) ) &&
                 ( !d->needBody || m->hasBodies() ) &&
                 ( !d->flags || m->hasFlags() ) &&
                 ( ( !d->rfc822size && !d->internaldate && !d->modseq )
                   || m->hasTrivia() ) &&
                 m->uid() > 0 && msn > 0 )
            {
                imap()->enqueue( fetchResponse( m, m->uid(), msn ) );
                d->requested.shift();
            }
            else {
                log( "Stopped processing at UID " + fn( m->uid() ) +
                     " (" + fn( d->requested.count() ) + " messages to go)",
                     Log::Debug );
                ok = false;
            }
        }

        // in the case of fetch, we sometimes have thousands of responses,
        // so it's important to push the first responses to the client as
        // quickly as possible.
        imap()->write();

        if ( !d->requested.isEmpty() )
            return;
        else if ( imap()->Connection::state() != Connection::Connected )
            d->state = 5; // hack to cope with client close
        else if ( d->set.isEmpty() )
            d->state = 5;
        else
            d->state = 3;
    }

    if ( !d->expunged.isEmpty() ) {
        s->recordExpungedFetch( d->expunged );
        error( No, "UID(s) " + d->expunged.set() + " has/have been expunged" );
    }
    finish();
}


/*! Issues queries to resolve any questions this FETCH needs to answer.
*/

void Fetch::sendFetchQueries()
{
    Mailbox * mb = imap()->session()->mailbox();

    List<Message> * l = new List<Message>;

    uint n = 0;
    while ( n < 1024 && !d->set.isEmpty() ) {
        uint uid = d->set.value( 1 );
        d->set.remove( uid );
        Message * m = new Message;
        m->setUid( uid );
        d->requested.append( m );
        l->append( m );
        n++;
    }

    if ( d->needHeader ) {
        Fetcher * f =
            new MessageAddressFetcher( mb, l, this );
        f->execute();
        f = new MessageHeaderFetcher( mb, l, this );
        f->execute();
    }
    if ( d->needBody ) {
        MessageBodyFetcher * mbf =
            new MessageBodyFetcher( mb, l, this );
        mbf->execute();
    }
    if ( d->flags ) {
        MessageFlagFetcher * mff =
            new MessageFlagFetcher( mb, l, this );
        mff->execute();
    }
    if ( d->rfc822size || d->internaldate || d->modseq ) {
        MessageTriviaFetcher * mtf =
            new MessageTriviaFetcher( mb, l, this );
        mtf->execute();
    }
    if ( d->annotation ) {
        MessageAnnotationFetcher * maf =
            new MessageAnnotationFetcher( mb, l, this );
        maf->execute();
    }
}


/*! This function returns the text of that portion of the Message \a m
    that is described by the Section \a s. It is publicly available so
    that Append may use it for CATENATE.
*/

String Fetch::sectionData( Section * s, Message * m )
{
    String item, data;

    if ( s->id == "rfc822" ) {
        item = s->id.upper();
        data = m->rfc822();
    }

    else if ( s->id == "mime" ||
              s->id == "rfc822.header" ||
              s->id.startsWith( "header" ) ) {
        bool rfc822 = s->id == "rfc822.header";
        bool fields = s->id.startsWith( "header.fields" );
        bool exclude = s->id.endsWith( ".not" );

        Header * hdr = m->header();
        if ( !s->part.isEmpty() ) {
            Bodypart * bp = m->bodypart( s->part, false );
            if ( bp && bp->header() )
                hdr = bp->header();
            else
                hdr = 0;
        }

        List< HeaderField >::Iterator it;
        if ( hdr )
            it = hdr->fields()->first();
        while ( it ) {
            bool include = false;
            if ( !fields ) {
                include = true;
            }
            else {
                bool listed = s->fields.find( it->name() );
                if ( exclude )
                    include = !listed;
                else
                    include = listed;
            }
            if ( include ) {
                String n = it->name().headerCased();
                data.append( n + ": " + it->value() + "\r\n" );
            }
            ++it;
        }

        item = s->id.upper();
        if ( !rfc822 ) {
            if ( !s->part.isEmpty() )
                item = s->part + "." + item;
            item = "BODY[" + item;
            if ( fields )
                item.append( " (" + s->fields.join( " " ) + ")" );
            item.append( "]" );
        }
        data.append( "\r\n" );
    }

    else if ( s->id == "rfc822.text" ) {
        item = s->id.upper();
        data = m->body();
    }

    else if ( s->id == "text" ) {
        if ( s->part.isEmpty() ) {
            item = "TEXT";
            data = m->body();
        }
        else {
            item = s->part + ".TEXT";
            Bodypart *bp = m->bodypart( s->part, false );
            if ( bp && bp->message() )
                data = bp->message()->body();
        }
        item = "BODY[" + item + "]";
    }

    else if ( ( s->id.isEmpty() || s->id == "size" ) &&
              s->part.isEmpty() )
    {
        if ( s->id == "size" ) {
            item = "BINARY.SIZE[]";
            data = fn( m->rfc822Size() );
        }
        else {
            item = "BODY[]";
            data = m->rfc822();
        }
    }

    else if ( s->id.isEmpty() || s->id == "size" ) {
        item = "BODY";
        Bodypart * bp = m->bodypart( s->part, false );
        if ( !bp ) {
            // nonexistent part number
            if ( s->binary )
                item = "BINARY";
            // should we report an error?  the fetch responses will be
            // sent anyway.
            // error( No, "No such bodypart: " + s->part );
        }
        else if ( bp->message() ) {
            // message/rfc822 part
            data = bp->message()->rfc822();
        }
        else if ( bp->children()->isEmpty() ) {
            // leaf part
            data = bp->data();

            ContentType * ct = bp->contentType();
            if ( !ct || ct->type() == "text" ) {
                UString text;

                if ( data.isEmpty() ) {
                    text = bp->text();
                }
                else {
                    Codec * c = new Utf8Codec;
                    text = c->toUnicode( data );
                }

                Codec * c = 0;
                if ( ct )
                    c = Codec::byName( ct->parameter( "charset" ) );
                if ( !c )
                    c = new Utf8Codec;
                data = c->fromUnicode( text );
            }
            if ( !s->binary )
                data = data.encode( bp->contentTransferEncoding(), 70 );
        }
        else {
            // nonleaf part. probably wrong - this might use the wrong
            // content-transfer-encoding.
            data = bp->asText();
        }

        if ( s->binary )
            item = "BINARY";

        if ( s->id == "size" ) {
            item = "BINARY.SIZE";
            data = fn( data.length() );
        }

        item = item + "[" + s->part + "]";
    }

    if ( s->partial ) {
        item.append( "<" + fn( s->offset ) + ">" );
        data = data.mid( s->offset, s->length );
    }

    s->item = item;
    return data;
}


/* This function returns the response data for an element in
   d->sections, to be included in the FETCH response by
   fetchResponses() below.
*/

static String sectionResponse( Section * s, Message * m )
{
    String data( Fetch::sectionData( s, m ) );
    if ( !s->item.startsWith( "BINARY.SIZE" ) )
        data = Command::imapQuoted( data, Command::NString );
    return s->item + " " + data;
}


/*! Emits a single FETCH response for the messae \a m, which is
    trusted to have UID \a uid and MSN \a msn.

    The message must have all necessary content.
*/

String Fetch::fetchResponse( Message * m, uint uid, uint msn )
{
    StringList l;
    if ( d->uid )
        l.append( "UID " + fn( uid ) );
    if ( d->rfc822size )
        l.append( "RFC822.SIZE " + fn( m->rfc822Size() ) );
    if ( d->flags )
        l.append( "FLAGS (" + flagList( m, uid, imap()->session() ) + ")" );
    if ( d->internaldate )
        l.append( "INTERNALDATE " + internalDate( m ) );
    if ( d->envelope )
        l.append( "ENVELOPE " + envelope( m ) );
    if ( d->body )
        l.append( "BODY " + bodyStructure( m, false ) );
    if ( d->bodystructure )
        l.append( "BODYSTRUCTURE " + bodyStructure( m, true ) );
    if ( d->annotation )
        l.append( "ANNOTATION " + annotation( m, imap()->user(),
                                              d->entries, d->attribs ) );
    if ( d->modseq )
        l.append( "MODSEQ (" + fn( m->modSeq() ) + ")" );

    List< Section >::Iterator it( d->sections );
    while ( it ) {
        l.append( sectionResponse( it, m ) );
        ++it;
    }

    String r = "* " + fn( msn ) + " FETCH (" + l.join( " " ) + ")\r\n";
    return r;
}


/*! Returns a string containing all the flags that are set for message
    \a m, which has UID \a uid and is interpreted within \a session.
*/

String Fetch::flagList( Message * m, uint uid, Session * session )
{
    String r;

    if ( session && session->isRecent( uid ) )
        r = "\\recent";

    List<Flag> * f = m->flags();
    if ( f && !f->isEmpty() ) {
        List<Flag>::Iterator it( f );
        while ( it ) {
            if ( !r.isEmpty() )
                r.append( " " );
            r.append( it->name() );
            ++it;
        }
    }

    return r;
}


/*! Returns the internaldate of \a m in IMAP format. */

String Fetch::internalDate( Message * m )
{
    Date date;
    date.setUnixTime( m->internalDate() );
    return "\"" + date.imap() + "\"";
}


static String hf( Header * f, HeaderField::Type t )
{
    List<Address> * a = f->addresses( t );
    if ( !a || a->isEmpty() )
        return "NIL ";
    String r( "(" );
    List<Address>::Iterator it( a );
    while ( it ) {
        r.append( "(" );
        if ( it->type() == Address::EmptyGroup ) {
            r.append( "NIL NIL " );
            r.append( Command::imapQuoted( it->name(), Command::NString ) );
            r.append( " NIL)(NIL NIL NIL NIL" );
        } else if ( it->type() == Address::Local ||
                    it->type() == Address::Normal ) {
            r.append( Command::imapQuoted( HeaderField::encodePhrase( it->uname().utf8() ),
                                           Command::NString ) );
            r.append( " NIL " );
            r.append( Command::imapQuoted( it->localpart(), Command::NString ) );
            r.append( " " );
            if ( it->domain().isEmpty() )
                r.append( "\" \"" ); // see RFC 3501 page 77, second-to-last paragraph
            else
                r.append( Command::imapQuoted( it->domain(), Command::NString ) );
        }
        r.append( ")" );
        ++it;
    }
    r.append( ") " );
    return r;
}


/*! Returns the IMAP envelope for \a m. */

String Fetch::envelope( Message * m )
{
    Header * h = m->header();

    // envelope = "(" env-date SP env-subject SP env-from SP
    //                env-sender SP env-reply-to SP env-to SP env-cc SP
    //                env-bcc SP env-in-reply-to SP env-message-id ")"

    String r( "(" );

    Date * date = h->date();
    if ( date )
        r.append( imapQuoted( date->rfc822(), NString ) );
    else
        r.append( "NIL" );
    r.append( " " );

    r.append( imapQuoted( h->subject(), NString ) + " " );
    r.append( hf( h, HeaderField::From ) );
    r.append( hf( h, HeaderField::Sender ) );
    r.append( hf( h, HeaderField::ReplyTo ) );
    r.append( hf( h, HeaderField::To ) );
    r.append( hf( h, HeaderField::Cc ) );
    r.append( hf( h, HeaderField::Bcc ) );
    r.append( imapQuoted( h->inReplyTo(), NString ) + " " );
    r.append( imapQuoted( h->messageId(), NString ) );

    r.append( ")" );
    return r;
}


static String parameterString( MimeField *mf )
{
    StringList *p = 0;

    if ( mf )
        p = mf->parameters();
    if ( !mf || !p || p->isEmpty() )
        return "NIL";

    StringList l;
    StringList::Iterator it( p );
    while ( it ) {
        l.append( Command::imapQuoted( *it ) );
        l.append( Command::imapQuoted( mf->parameter( *it ) ) );
        ++it;
    }

    return "(" + l.join( " " ) + ")";
}


static String dispositionString( ContentDisposition *cd )
{
    if ( !cd )
        return "NIL";

    String s;
    switch ( cd->disposition() ) {
    case ContentDisposition::Inline:
        s = "inline";
        break;
    case ContentDisposition::Attachment:
        s = "attachment";
        break;
    }

    return "(\"" + s + "\" " + parameterString( cd ) + ")";
}


static String languageString( ContentLanguage *cl )
{
    if ( !cl )
        return "NIL";

    StringList m;
    const StringList *l = cl->languages();
    StringList::Iterator it( l );
    while ( it ) {
        m.append( Command::imapQuoted( *it ) );
        ++it;
    }

    if ( l->count() == 1 )
        return *m.first();
    return "(" + m.join( " " ) + ")";
}


/*! Returns either the IMAP BODY or BODYSTRUCTURE production for \a
    m. If \a extended is true, BODYSTRUCTURE is returned. If it's
    false, BODY.
*/

String Fetch::bodyStructure( Multipart * m, bool extended )
{
    String r;

    Header * hdr = m->header();
    ContentType * ct = hdr->contentType();

    if ( ct && ct->type() == "multipart" ) {
        StringList children;
        List< Bodypart >::Iterator it( m->children() );
        while ( it ) {
            children.append( bodyStructure( it, extended ) );
            ++it;
        }

        r = "(" + children.join( "" ) +
            " " + imapQuoted( ct->subtype() );

        if ( extended ) {
            r.append( " " );
            r.append( parameterString( ct ) );
            r.append( " " );
            r.append( dispositionString( hdr->contentDisposition() ) );
            r.append( " " );
            r.append( languageString( hdr->contentLanguage() ) );
            r.append( " " );
            r.append( imapQuoted( hdr->contentLocation(), NString ) );
        }

        r.append( ")" );
    }
    else {
        r = singlePartStructure( (Bodypart*)m, extended );
    }

    return r;
}


/*! Returns the structure of the single-part bodypart \a mp.

    If \a extended is true, extended BODYSTRUCTURE attributes are
    included.
*/

String Fetch::singlePartStructure( Multipart * mp, bool extended )
{
    StringList l;

    if ( !mp )
        return "";

    ContentType * ct = mp->header()->contentType();

    if ( ct ) {
        l.append( imapQuoted( ct->type() ) );
        l.append( imapQuoted( ct->subtype() ) );
    }
    else {
        // XXX: What happens to the default if this is a /digest?
        l.append( "\"text\"" );
        l.append( "\"plain\"" );
    }

    l.append( parameterString( ct ) );
    l.append( imapQuoted( mp->header()->messageId( HeaderField::ContentId ),
                          NString ) );
    l.append( imapQuoted( mp->header()->contentDescription(), NString ) );

    if ( mp->header()->contentTransferEncoding() ) {
        switch( mp->header()->contentTransferEncoding()->encoding() ) {
        case String::Binary:
            l.append( "\"8BIT\"" ); // hm. is this entirely sound?
            break;
        case String::Uuencode:
            l.append( "\"x-uuencode\"" ); // should never happen
            break;
        case String::Base64:
            l.append( "\"BASE64\"" );
            break;
        case String::QP:
            l.append( "\"QUOTED-PRINTABLE\"" );
            break;
        }
    }
    else {
        l.append( "\"7BIT\"" );
    }

    Bodypart * bp = 0;
    if ( mp->isBodypart() )
        bp = (Bodypart*)mp;
    else if ( mp->isMessage() )
        bp = ((Message*)mp)->children()->first();

    if ( bp ) {
        l.append( fn( bp->numEncodedBytes() ) );
        if ( ct && ct->type() == "message" && ct->subtype() == "rfc822" ) {
            // body-type-msg   = media-message SP body-fields SP envelope
            //                   SP body SP body-fld-lines
            l.append( envelope( bp->message() ) );
            l.append( bodyStructure( bp->message(), extended ) );
            l.append( fn( bp->numEncodedLines() ) );
        }
        else if ( !ct || ct->type() == "text" ) {
            // body-type-text  = media-text SP body-fields SP body-fld-lines
            l.append( fn( bp->numEncodedLines() ) );
        }
    }

    if ( extended ) {
        String md5;
        HeaderField *f = mp->header()->field( HeaderField::ContentMd5 );
        if ( f )
            md5 = f->value();

        l.append( imapQuoted( md5, NString ) );
        l.append( dispositionString( mp->header()->contentDisposition() ) );
        l.append( languageString( mp->header()->contentLanguage() ) );
        l.append( imapQuoted( mp->header()->contentLocation(), NString ) );
    }

    return "(" + l.join( " " ) + ")";
}


/*! Returns the IMAP ANNOTATION production for \a m, from the point of
    view of \a u (0 for no user, only public annotations). \a
    entrySpecs is a list of the entries to be matched, each of which
    can contain the * and % wildcards. \a attributes is a list of
    attributes to be returned (each including the .priv or .shared
    suffix).
*/

String Fetch::annotation( Multipart * m, User * u,
                          const StringList & entrySpecs,
                          const StringList & attributes )
{
    if ( !m->isMessage() )
        return "";

    typedef Dict< String > AttributeDict;
    Dict< AttributeDict > entries;

    StringList entryNames;

    uint user = 0;
    if ( u )
        user = u->id();
    List<Annotation>::Iterator i( ((Message*)m)->annotations() );
    while ( i ) {
        Annotation * a = i;
        ++i;

        String entry( a->entryName()->name() );
        bool entryWanted = false;
        StringList::Iterator e( entrySpecs );
        while ( e ) {
            if ( Listext::match( *e, 0, entry, 0 ) == 2 ) {
                if ( !entries.find( entry ) )
                    entryNames.append( new String( entry ) );
                entryWanted = true;
                break;
            }
            ++e;
        }

        if ( ( a->ownerId() == 0 || a->ownerId() == user ) &&
             entryWanted )
        {
            AttributeDict * atts = entries.find( entry );
            if ( !atts ) {
                atts = new AttributeDict;
                entries.insert( entry, atts );
            }

            const char * suffix = ".shared";
            if ( a->ownerId() )
                suffix = ".priv";

            String * v = new String( a->value() );
            String * s = new String( fn( v->length() ) );

            atts->insert( String( "value" ) + suffix, v );
            atts->insert( String( "size" ) + suffix, s );
        }
    }

    String r( "(" );
    StringList::Iterator e( entryNames );
    while ( e ) {
        String entry( *e );

        String tmp;
        StringList::Iterator a( attributes );
        while ( a ) {
            String attrib( *a );

            String * value = 0;
            AttributeDict * atts = entries.find( entry );
            if ( atts )
                value = atts->find( attrib );

            tmp.append( attrib );
            tmp.append( " " );
            if ( value )
                tmp.append( imapQuoted( *value ) );
            else if ( attrib.startsWith( "size." ) )
                tmp.append( "\"0\"" );
            else
                tmp.append( "NIL" );
            ++a;
            if ( a )
                tmp.append( " " );
        }

        r.append( entry );
        if ( !tmp.isEmpty() ) {
            r.append( " (" );
            r.append( tmp );
            r.append( ")" );
        }

        ++e;
        if ( e )
            r.append( " " );
    }
    r.append( ")" );
    return r;
}


/*! Parses a single RFC 4466 fetch-modifier. At the moment only RFC
    4551 is supported.
*/

void Fetch::parseFetchModifier()
{
    String name = atom().lower();
    if ( name == "changedsince" ) {
        space();
        d->changedSince = number();
        d->modseq = true;
    }
    else {
        error( Bad, "Unknown fetch modifier: " + name );
    }
}


FetchData::SeenFlagSetter::SeenFlagSetter( ImapSession * s,
                                           const MessageSet & ms,
                                           EventHandler * owner )
    : EventHandler(),
      t( 0 ), seen( 0 ), f( 0 ), ms( 0 ), session( s ), o( owner ),
      mailbox( s->mailbox() ), modseq( 0 )
{
    if ( mailbox->view() ) {
        messages.add( mailbox->sourceUids( ms ) );
        mailbox = mailbox->source();
    }
    else {
        messages.add( ms );
    }
    execute();
}


void FetchData::SeenFlagSetter::execute()
{
    if ( !t ) {
        seen = Flag::find( "\\seen" );
        if ( !seen )
            return;

        t = new Transaction( this );
        ms = new Query( "select nextmodseq from mailboxes "
                        "where id=$1 for update", this );
        ms->bind( 1, mailbox->id() );
        t->enqueue( ms );

        f = new Query( "select uid from flags "
                       "where mailbox=$1 and flag=$2 and uid>=$3 and uid<=$4",
                       this );
        f->bind( 1, mailbox->id() );
        f->bind( 2, seen->id() );
        f->bind( 3, messages.smallest() );
        f->bind( 4, messages.largest() );
        t->enqueue( f );

        t->execute();
    }

    Row * r = f->nextRow();
    while ( r ) {
        messages.remove( r->getInt( "uid" ) );
        r = f->nextRow();
    }

    if ( !f->done() )
        return;

    if ( messages.isEmpty() ) {
        t->rollback();
        if ( o )
            o->execute();
        o = 0;
        return;
    }

    if ( !ms->done() )
        return;

    r = ms->nextRow();
    if ( r ) {
        modseq = r->getBigint( "nextmodseq" );
        Query * q = new Query( "update modsequences "
                               "set modseq=$1 "
                               "where mailbox=$2 and " + messages.where(),
                               0 );
        q->bind( 1, modseq );
        q->bind( 2, mailbox->id() );
        t->enqueue( q );


        q = Store::addFlagsQuery( seen, mailbox, messages, 0 );
        t->enqueue( q );
        q = new Query( "update mailboxes set nextmodseq=$1 "
                       "where id=$2", 0 );
        q->bind( 1, modseq + 1 );
        q->bind( 2, mailbox->id() );
        t->enqueue( q );
        t->commit();
    }

    if ( !t->done() )
        return;

    if ( mailbox->nextModSeq() <= modseq ) {
        mailbox->setNextModSeq( modseq + 1 );
        OCClient::send( "mailbox " + mailbox->name().quoted() + " "
                        "nextmodseq=" + fn( modseq+1 ) );
    }
    if ( o )
        o->execute();
    modseq = 0;
    o = 0;
}
