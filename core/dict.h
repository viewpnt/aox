// Copyright 2009 The Archiveopteryx Developers <info@aox.org>

#ifndef DICT_H
#define DICT_H

#include "patriciatree.h"
#include "ustring.h"
#include "estring.h"


template<class T>
class Dict: public PatriciaTree<T> {
public:
    Dict(): PatriciaTree<T>() {}

    T * find( const EString & s ) const {
        return PatriciaTree<T>::find( s.data(), s.length() * 8 );
    }
    void insert( const EString & s, T* r ) {
        PatriciaTree<T>::insert( s.data(), s.length() * 8, r );
    }
    T* remove( const EString & s ) {
        return PatriciaTree<T>::remove( s.data(), s.length() * 8 );
    }
    bool contains( const EString & s ) const {
        return find( s ) != 0;
    }

private:
    // operators explicitly undefined because there is no single
    // correct way to implement them.
    Dict< T > &operator =( const Dict< T > & ) { return *this; }
    bool operator ==( const Dict< T > & ) const { return false; }
    bool operator !=( const Dict< T > & ) const { return false; }
};


template<class T>
class UDict: public PatriciaTree<T> {
public:
    UDict(): PatriciaTree<T>() {}

    T * find( const UString & s ) const {
        return PatriciaTree<T>::find( (const char *)s.data(), s.length() * 8 * sizeof( uint ) );
    }
    void insert( const UString & s, T* r ) {
        PatriciaTree<T>::insert( (const char *)s.data(), s.length() * 8 * sizeof( uint ), r );
    }
    T* remove( const UString & s ) {
        return PatriciaTree<T>::remove( (const char *)s.data(), s.length() * 8 * sizeof( uint ) );
    }
    bool contains( const UString & s ) const {
        return find( s ) != 0;
    }

private:
    // operators explicitly undefined because there is no single
    // correct way to implement them.
    UDict< T > &operator =( const UDict< T > & ) { return *this; }
    bool operator ==( const UDict< T > & ) const { return false; }
    bool operator !=( const UDict< T > & ) const { return false; }
};


#endif
