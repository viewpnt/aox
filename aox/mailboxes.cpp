// Copyright Oryx Mail Systems GmbH. All enquiries to info@oryx.com, please.

#include "mailboxes.h"

#include "user.h"
#include "query.h"
#include "mailbox.h"
#include "occlient.h"
#include "stringlist.h"
#include "transaction.h"
#include "addresscache.h"

#include <stdio.h>


/*! \class ListMailboxes mailboxes.h
    This class handles the "aox list mailboxes" command.
*/

ListMailboxes::ListMailboxes( StringList * args )
    : AoxCommand( args ), q( 0 )
{
}


void ListMailboxes::execute()
{
    if ( !q ) {
        String owner;
        String p( next() );

        while ( p[0] == '-' ) {
            if ( p == "-d" ) {
                setopt( 'd' );
            }
            else if ( p == "-s" ) {
                setopt( 's' );
            }
            else if ( p == "-o" ) {
                setopt( 'o' );
                owner = next();
                if ( owner.isEmpty() )
                    error( "No username specified with -o." );
            }
            else {
                error( "Bad option name: '" + p + "'" );
            }

            p = next();
        }

        String pattern( p );
        end();

        database();

        String s( "select name,login as owner" );
        if ( opt( 's' ) > 0 )
            s.append( ",(select count(*) from messages "
                      "where mailbox=m.id)::int as messages,"
                      "(select sum(rfc822size) from messages "
                      "where mailbox=m.id)::int as size" );
        s.append( " from mailboxes m left join users u on (m.owner=u.id)" );

        int n = 1;
        StringList where;
        if ( opt( 'd' ) == 0 )
            where.append( "not deleted" );
        if ( !pattern.isEmpty() )
            where.append( "name like $" + fn( n++ ) );
        if ( opt( 'o' ) > 0 )
            where.append( "login like $" + fn( n ) );

        if ( !where.isEmpty() ) {
            s.append( " where " );
            s.append( where.join( " and " ) );
        }

        q = new Query( s, this );
        if ( !pattern.isEmpty() )
            q->bind( 1, sqlPattern( pattern ) );
        if ( !owner.isEmpty() )
            q->bind( n, owner );
        q->execute();
    }

    while ( q->hasResults() ) {
        Row * r = q->nextRow();

        String s( r->getString( "name" ) );
        if ( opt( 's' ) > 0 ) {
            int messages = r->getInt( "messages" );
            int size = r->getInt( "size" );
            s.append( " (" );
            s.append( fn( messages ) );
            if ( messages == 1 )
                s.append( " message, " );
            else
                s.append( " messages, " );
            s.append( String::humanNumber( size ) );
            s.append( " bytes" );
            s.append( ")" );
        }

        printf( "%s\n", s.cstr() );
    }

    if ( !q->done() )
        return;

    finish();
}



class CreateMailboxData
    : public Garbage
{
public:
    CreateMailboxData()
        : user( 0 ), m( 0 ), t( 0 ), q( 0 )
    {}

    String name;
    User * user;
    Mailbox * m;
    Transaction * t;
    Query * q;
};


/*! \class CreateMailbox mailboxes.h
    This class handles the "aox create mailbox" command.
*/

CreateMailbox::CreateMailbox( StringList * args )
    : AoxCommand( args ), d( new CreateMailboxData )
{
}


void CreateMailbox::execute()
{
    if ( d->name.isEmpty() ) {
        parseOptions();
        d->name = next();
        String owner = next();
        end();

        if ( d->name.isEmpty() )
            error( "No mailbox name supplied." );

        database( true );
        OCClient::setup();
        AddressCache::setup();
        Mailbox::setup( this );

        if ( !owner.isEmpty() ) {
            d->user = new User;
            d->user->setLogin( owner );
            d->user->refresh( this );
        }
    }

    if ( !choresDone() )
        return;

    if ( d->user && d->user->state() == User::Unverified )
        return;

    if ( !d->t ) {
        if ( d->user && d->user->state() == User::Nonexistent )
            error( "No user named " + d->user->login() );

        if ( d->user && !d->name.startsWith( "/" ) )
            d->name = d->user->home()->name() + "/" + d->name;

        d->m = Mailbox::obtain( d->name );
        if ( !d->m )
            error( "Can't create mailbox named " + d->name );

        d->t = new Transaction( this );
        if ( d->m->create( d->t, d->user ) == 0 )
            error( "Couldn't create mailbox " + d->name );
        d->t->commit();
    }

    if ( !d->t->done() )
        return;

    if ( d->t->failed() )
        error( "Couldn't create mailbox: " + d->t->error() );

    OCClient::send( "mailbox " + d->m->name().quoted() + " new" );
    finish();
}



class DeleteMailboxData
    : public Garbage
{
public:
    DeleteMailboxData()
        : m( 0 ), t( 0 )
    {}

    String name;
    Mailbox * m;
    Transaction * t;
};


/*! \class DeleteMailbox mailboxes.h
    This class handles the "aox delete mailbox" command.
*/

DeleteMailbox::DeleteMailbox( StringList * args )
    : AoxCommand( args ), d( new DeleteMailboxData )
{
}


void DeleteMailbox::execute()
{
    if ( d->name.isEmpty() ) {
        parseOptions();
        d->name = next();
        end();

        if ( d->name.isEmpty() )
            error( "No mailbox name supplied." );

        database( true );
        OCClient::setup();
        AddressCache::setup();
        Mailbox::setup( this );
    }

    if ( !choresDone() )
        return;

    if ( !d->t ) {
        d->m = Mailbox::obtain( d->name, false );
        if ( !d->m )
            error( "No mailbox named " + d->name );
        d->t = new Transaction( this );
        if ( d->m->remove( d->t ) == 0 )
            error( "Couldn't delete mailbox " + d->name );
        d->t->commit();
    }

    if ( d->t && !d->t->done() )
        return;

    if ( d->t->failed() )
        error( "Couldn't delete mailbox: " + d->t->error() );

    OCClient::send( "mailbox " + d->m->name().quoted() + " deleted" );
    finish();
}