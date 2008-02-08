// Copyright Oryx Mail Systems GmbH. All enquiries to info@oryx.com, please.

#ifndef SESSION_H
#define SESSION_H

#include "global.h"
#include "messageset.h"
#include "permissions.h"
#include "event.h"
#include "list.h"

class Mailbox;
class Message;
class Select;
class IMAP;


class Session
    : public Garbage
{
public:
    Session( Mailbox *, bool );
    virtual ~Session();

    void end();

    bool initialised() const;
    void refresh( class EventHandler * );
    bool isEmpty() const;

    Mailbox * mailbox() const;
    bool readOnly() const;

    Permissions *permissions() const;
    void setPermissions( Permissions * );
    bool allows( Permissions::Right );

    uint uidnext() const;
    uint uidvalidity() const;
    void setUidnext( uint );

    int64 nextModSeq() const;
    void setNextModSeq( int64 ) const;

    uint uid( uint ) const;
    uint msn( uint ) const;
    uint count() const;

    void insert( uint );
    void insert( uint, uint );
    void remove( uint );

    MessageSet recent() const;
    bool isRecent( uint ) const;
    void addRecent( uint );

    const MessageSet & expunged() const;
    const MessageSet & messages() const;

    void expunge( const MessageSet &, int64 );
    void clearExpunged();

    enum ResponseType { New, Modified, Deleted };

    virtual bool responsesNeeded( ResponseType ) const;
    virtual bool responsesReady( ResponseType ) const;
    virtual bool responsesPermitted( ResponseType ) const;

    virtual void emitResponses();
    void emitResponses( ResponseType );
    virtual void emitExpunges();
    virtual void emitExists( uint );
    virtual void emitModifications();

    MessageSet unannounced() const;
    void addUnannounced( uint );
    void addUnannounced( const MessageSet & );

    void setSessionInitialiser( class SessionInitialiser * );
    class SessionInitialiser * sessionInitialiser() const;

private:
    friend class SessionInitialiser;
    class SessionData *d;
};


class SessionInitialiser
    : public EventHandler
{
public:
    SessionInitialiser( Mailbox * );

    void execute();

    void addWatcher( EventHandler * );

private:
    class SessionInitialiserData * d;

    void findSessions();
    void eliminateGoodSessions();
    void restart();
    void grabLock();
    void releaseLock();
    void findRecent();
    void findUidnext();
    void findViewChanges();
    void writeViewChanges();
    void findMailboxChanges();
    void recordMailboxChanges();
    void emitResponses();
    void addToSessions( uint, int64 );
    void submit( class Query * );
};


#endif
