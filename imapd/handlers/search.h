// Copyright Oryx Mail Systems GmbH. All enquiries to info@oryx.com, please.

#ifndef SEARCH_H
#define SEARCH_H

#include "command.h"
#include "messageset.h"
#include "ustring.h"


class Message;


class Search
    : public Command
{
public:
    Search( bool u );

    void parse();
    void execute();

private:
    void parseKey( bool alsoCharset = false );

private:
    enum Action {
        OnDate,
        SinceDate,
        BeforeDate,
        Contains,
        Larger,
        Smaller,
        And,
        Or,
        Not,
        All,
        None
    };
    enum Field { // moves? here for the moment
        InternalDate,
        Sent,
        Header,
        Body,
        Rfc822Size,
        Flags,
        Uid,
        NoField
    };

    struct Condition
    { // everything here is public. this may need changing at some point.
    public:
        Condition() : f( NoField ), a( All ), n( 0 ), l( 0 ), c( 0 ), d( 0 ) {}

        Field f;
        Action a;
        String a1;
        UString a2;
        MessageSet s;
        uint n;
        List<Condition> * l;
        Command * c;
        class SearchData * d;

        String where( class SearchData * ) const;
        String debugString() const;
        void simplify();
        enum MatchResult {
            Yes,
            No,
            Punt // really "ThrowHandsUpInAirAndDespair"
        };
        MatchResult match( Message *, uint );

    private:
        String whereInternalDate() const;
        String whereSent() const;
        String whereHeader() const;
        String whereHeaderField() const;
        String whereAddressField( const String & ) const;
    };

    Condition * add( Field, Action,
                     const String & = 0 );
    Condition * add( Field, Action,
                     const String &, const UString & );
    Condition * add( Field, Action, uint );
    Condition * add( const MessageSet & );

    Condition * push( Action );
    void pop();

    void prepare();

    String debugString();

    String date();

    void considerCache();

    UString uastring();

private:
    class SearchData * d;
    friend class SearchData;
};


#endif
