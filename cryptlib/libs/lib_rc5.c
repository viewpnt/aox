/****************************************************************************
*																			*
*						cryptlib RC5 Encryption Routines					*
*						Copyright Peter Gutmann 1997-2003					*
*																			*
****************************************************************************/

#include <stdlib.h>
#if defined( INC_ALL )
  #include "crypt.h"
  #include "rc5.h"
  #include "context.h"
  #include "libs.h"
#elif defined( INC_CHILD )
  #include "../crypt.h"
  #include "../crypt/rc5.h"
  #include "../misc/context.h"
  #include "libs.h"
#else
  #include "crypt.h"
  #include "crypt/rc5.h"
  #include "misc/context.h"
  #include "libs/libs.h"
#endif /* Compiler-specific includes */

#ifdef USE_RC5

/* The default number of RC5 rounds */

#define RC5_DEFAULT_ROUNDS	RC5_12_ROUNDS

/* Defines to map from EAY to native naming */

#define RC5_BLOCKSIZE		RC5_32_BLOCK
#define RC5_KEY				RC5_32_KEY

/****************************************************************************
*																			*
*								RC5 Self-test Routines						*
*																			*
****************************************************************************/

/* RC5 test vectors from RC5 specification */

static const FAR_BSS struct RC5_TEST {
	const BYTE key[ 16 ];
	const BYTE plainText[ 8 ];
	const BYTE cipherText[ 8 ];
	} testRC5[] = {
	{ { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	  { 0x21, 0xA5, 0xDB, 0xEE, 0x15, 0x4B, 0x8F, 0x6D } },
	{ { 0x91, 0x5F, 0x46, 0x19, 0xBE, 0x41, 0xB2, 0x51,
		0x63, 0x55, 0xA5, 0x01, 0x10, 0xA9, 0xCE, 0x91 },
	  { 0x21, 0xA5, 0xDB, 0xEE, 0x15, 0x4B, 0x8F, 0x6D },
	  { 0xF7, 0xC0, 0x13, 0xAC, 0x5B, 0x2B, 0x89, 0x52 } },
	{ { 0x78, 0x33, 0x48, 0xE7, 0x5A, 0xEB, 0x0F, 0x2F,
		0xD7, 0xB1, 0x69, 0xBB, 0x8D, 0xC1, 0x67, 0x87 },
	  { 0xF7, 0xC0, 0x13, 0xAC, 0x5B, 0x2B, 0x89, 0x52 },
	  { 0x2F, 0x42, 0xB3, 0xB7, 0x03, 0x69, 0xFC, 0x92 } },
	{ { 0xDC, 0x49, 0xDB, 0x13, 0x75, 0xA5, 0x58, 0x4F,
		0x64, 0x85, 0xB4, 0x13, 0xB5, 0xF1, 0x2B, 0xAF },
	  { 0x2F, 0x42, 0xB3, 0xB7, 0x03, 0x69, 0xFC, 0x92 },
	  { 0x65, 0xC1, 0x78, 0xB2, 0x84, 0xD1, 0x97, 0xCC } },
	{ { 0x52, 0x69, 0xF1, 0x49, 0xD4, 0x1B, 0xA0, 0x15,
		0x24, 0x97, 0x57, 0x4D, 0x7F, 0x15, 0x31, 0x25 },
	  { 0x65, 0xC1, 0x78, 0xB2, 0x84, 0xD1, 0x97, 0xCC },
	  { 0xEB, 0x44, 0xE4, 0x15, 0xDA, 0x31, 0x98, 0x24 } }
	};

/* Test the RC5 code against the RC5 test vectors */

int rc5SelfTest( void )
	{
	BYTE temp[ RC5_BLOCKSIZE ];
	RC5_KEY key;
	int i;

	for( i = 0; i < sizeof( testRC5 ) / sizeof( struct RC5_TEST ); i++ )
		{
		memcpy( temp, testRC5[ i ].plainText, RC5_BLOCKSIZE );
		RC5_32_set_key( &key, 16, testRC5[ i ].key, 12 );
		RC5_32_ecb_encrypt( temp, temp, &key, RC5_ENCRYPT );
		if( memcmp( testRC5[ i ].cipherText, temp, RC5_BLOCKSIZE ) )
			return( CRYPT_ERROR );
		}

	return( CRYPT_OK );
	}

/****************************************************************************
*																			*
*								Control Routines							*
*																			*
****************************************************************************/

/* Return context subtype-specific information */

int rc5GetInfo( const CAPABILITY_INFO_TYPE type, 
				void *varParam, const int constParam )
	{
	if( type == CAPABILITY_INFO_STATESIZE )
		return( sizeof( RC5_KEY ) );

	return( getInfo( type, varParam, constParam ) );
	}

/****************************************************************************
*																			*
*							RC5 En/Decryption Routines						*
*																			*
****************************************************************************/

/* Encrypt/decrypt data in ECB mode */

int rc5EncryptECB( CONTEXT_INFO *contextInfoPtr, BYTE *buffer, int noBytes )
	{
	CONV_INFO *convInfo = contextInfoPtr->ctxConv;
	RC5_KEY *rc5Key = ( RC5_KEY * ) convInfo->key;
	int blockCount = noBytes / RC5_BLOCKSIZE;

	while( blockCount-- )
		{
		/* Encrypt a block of data */
		RC5_32_ecb_encrypt( buffer, buffer, rc5Key, RC5_ENCRYPT );

		/* Move on to next block of data */
		buffer += RC5_BLOCKSIZE;
		}

	return( CRYPT_OK );
	}

int rc5DecryptECB( CONTEXT_INFO *contextInfoPtr, BYTE *buffer, int noBytes )
	{
	CONV_INFO *convInfo = contextInfoPtr->ctxConv;
	RC5_KEY *rc5Key = ( RC5_KEY * ) convInfo->key;
	int blockCount = noBytes / RC5_BLOCKSIZE;

	while( blockCount-- )
		{
		/* Decrypt a block of data */
		RC5_32_ecb_encrypt( buffer, buffer, rc5Key, RC5_DECRYPT );

		/* Move on to next block of data */
		buffer += RC5_BLOCKSIZE;
		}

	return( CRYPT_OK );
	}

/* Encrypt/decrypt data in CBC mode */

int rc5EncryptCBC( CONTEXT_INFO *contextInfoPtr, BYTE *buffer, int noBytes )
	{
	CONV_INFO *convInfo = contextInfoPtr->ctxConv;

	RC5_32_cbc_encrypt( buffer, buffer, noBytes, convInfo->key,
						convInfo->currentIV, RC5_ENCRYPT );

	return( CRYPT_OK );
	}

int rc5DecryptCBC( CONTEXT_INFO *contextInfoPtr, BYTE *buffer, int noBytes )
	{
	CONV_INFO *convInfo = contextInfoPtr->ctxConv;

	RC5_32_cbc_encrypt( buffer, buffer, noBytes, convInfo->key,
						convInfo->currentIV, RC5_DECRYPT );

	return( CRYPT_OK );
	}

/* Encrypt/decrypt data in CFB mode */

int rc5EncryptCFB( CONTEXT_INFO *contextInfoPtr, BYTE *buffer, int noBytes )
	{
	CONV_INFO *convInfo = contextInfoPtr->ctxConv;
	RC5_KEY *rc5Key = ( RC5_KEY * ) convInfo->key;
	int i, ivCount = convInfo->ivCount;

	/* If there's any encrypted material left in the IV, use it now */
	if( ivCount )
		{
		int bytesToUse;

		/* Find out how much material left in the encrypted IV we can use */
		bytesToUse = RC5_BLOCKSIZE - ivCount;
		if( noBytes < bytesToUse )
			bytesToUse = noBytes;

		/* Encrypt the data */
		for( i = 0; i < bytesToUse; i++ )
			buffer[ i ] ^= convInfo->currentIV[ i + ivCount ];
		memcpy( convInfo->currentIV + ivCount, buffer, bytesToUse );

		/* Adjust the byte count and buffer position */
		noBytes -= bytesToUse;
		buffer += bytesToUse;
		ivCount += bytesToUse;
		}

	while( noBytes )
		{
		ivCount = ( noBytes > RC5_BLOCKSIZE ) ? RC5_BLOCKSIZE : noBytes;

		/* Encrypt the IV */
		RC5_32_ecb_encrypt( convInfo->currentIV, convInfo->currentIV, rc5Key, 
							RC5_ENCRYPT );

		/* XOR the buffer contents with the encrypted IV */
		for( i = 0; i < ivCount; i++ )
			buffer[ i ] ^= convInfo->currentIV[ i ];

		/* Shift the ciphertext into the IV */
		memcpy( convInfo->currentIV, buffer, ivCount );

		/* Move on to next block of data */
		noBytes -= ivCount;
		buffer += ivCount;
		}

	/* Remember how much of the IV is still available for use */
	convInfo->ivCount = ( ivCount % RC5_BLOCKSIZE );

	return( CRYPT_OK );
	}

/* Decrypt data in CFB mode.  Note that the transformation can be made
   faster (but less clear) with temp = buffer, buffer ^= iv, iv = temp
   all in one loop */

int rc5DecryptCFB( CONTEXT_INFO *contextInfoPtr, BYTE *buffer, int noBytes )
	{
	CONV_INFO *convInfo = contextInfoPtr->ctxConv;
	RC5_KEY *rc5Key = ( RC5_KEY * ) convInfo->key;
	BYTE temp[ RC5_BLOCKSIZE ];
	int i, ivCount = convInfo->ivCount;

	/* If there's any encrypted material left in the IV, use it now */
	if( ivCount )
		{
		int bytesToUse;

		/* Find out how much material left in the encrypted IV we can use */
		bytesToUse = RC5_BLOCKSIZE - ivCount;
		if( noBytes < bytesToUse )
			bytesToUse = noBytes;

		/* Decrypt the data */
		memcpy( temp, buffer, bytesToUse );
		for( i = 0; i < bytesToUse; i++ )
			buffer[ i ] ^= convInfo->currentIV[ i + ivCount ];
		memcpy( convInfo->currentIV + ivCount, temp, bytesToUse );

		/* Adjust the byte count and buffer position */
		noBytes -= bytesToUse;
		buffer += bytesToUse;
		ivCount += bytesToUse;
		}

	while( noBytes )
		{
		ivCount = ( noBytes > RC5_BLOCKSIZE ) ? RC5_BLOCKSIZE : noBytes;

		/* Encrypt the IV */
		RC5_32_ecb_encrypt( convInfo->currentIV, convInfo->currentIV, rc5Key, 
							RC5_ENCRYPT );

		/* Save the ciphertext */
		memcpy( temp, buffer, ivCount );

		/* XOR the buffer contents with the encrypted IV */
		for( i = 0; i < ivCount; i++ )
			buffer[ i ] ^= convInfo->currentIV[ i ];

		/* Shift the ciphertext into the IV */
		memcpy( convInfo->currentIV, temp, ivCount );

		/* Move on to next block of data */
		noBytes -= ivCount;
		buffer += ivCount;
		}

	/* Remember how much of the IV is still available for use */
	convInfo->ivCount = ( ivCount % RC5_BLOCKSIZE );

	/* Clear the temporary buffer */
	zeroise( temp, RC5_BLOCKSIZE );

	return( CRYPT_OK );
	}

/* Encrypt/decrypt data in OFB mode */

int rc5EncryptOFB( CONTEXT_INFO *contextInfoPtr, BYTE *buffer, int noBytes )
	{
	CONV_INFO *convInfo = contextInfoPtr->ctxConv;
	RC5_KEY *rc5Key = ( RC5_KEY * ) convInfo->key;
	int i, ivCount = convInfo->ivCount;

	/* If there's any encrypted material left in the IV, use it now */
	if( ivCount )
		{
		int bytesToUse;

		/* Find out how much material left in the encrypted IV we can use */
		bytesToUse = RC5_BLOCKSIZE - ivCount;
		if( noBytes < bytesToUse )
			bytesToUse = noBytes;

		/* Encrypt the data */
		for( i = 0; i < bytesToUse; i++ )
			buffer[ i ] ^= convInfo->currentIV[ i + ivCount ];

		/* Adjust the byte count and buffer position */
		noBytes -= bytesToUse;
		buffer += bytesToUse;
		ivCount += bytesToUse;
		}

	while( noBytes )
		{
		ivCount = ( noBytes > RC5_BLOCKSIZE ) ? RC5_BLOCKSIZE : noBytes;

		/* Encrypt the IV */
		RC5_32_ecb_encrypt( convInfo->currentIV, convInfo->currentIV, rc5Key, 
							RC5_ENCRYPT );

		/* XOR the buffer contents with the encrypted IV */
		for( i = 0; i < ivCount; i++ )
			buffer[ i ] ^= convInfo->currentIV[ i ];

		/* Move on to next block of data */
		noBytes -= ivCount;
		buffer += ivCount;
		}

	/* Remember how much of the IV is still available for use */
	convInfo->ivCount = ( ivCount % RC5_BLOCKSIZE );

	return( CRYPT_OK );
	}

/* Decrypt data in OFB mode */

int rc5DecryptOFB( CONTEXT_INFO *contextInfoPtr, BYTE *buffer, int noBytes )
	{
	CONV_INFO *convInfo = contextInfoPtr->ctxConv;
	RC5_KEY *rc5Key = ( RC5_KEY * ) convInfo->key;
	int i, ivCount = convInfo->ivCount;

	/* If there's any encrypted material left in the IV, use it now */
	if( ivCount )
		{
		int bytesToUse;

		/* Find out how much material left in the encrypted IV we can use */
		bytesToUse = RC5_BLOCKSIZE - ivCount;
		if( noBytes < bytesToUse )
			bytesToUse = noBytes;

		/* Decrypt the data */
		for( i = 0; i < bytesToUse; i++ )
			buffer[ i ] ^= convInfo->currentIV[ i + ivCount ];

		/* Adjust the byte count and buffer position */
		noBytes -= bytesToUse;
		buffer += bytesToUse;
		ivCount += bytesToUse;
		}

	while( noBytes )
		{
		ivCount = ( noBytes > RC5_BLOCKSIZE ) ? RC5_BLOCKSIZE : noBytes;

		/* Encrypt the IV */
		RC5_32_ecb_encrypt( convInfo->currentIV, convInfo->currentIV, rc5Key, 
							RC5_ENCRYPT );

		/* XOR the buffer contents with the encrypted IV */
		for( i = 0; i < ivCount; i++ )
			buffer[ i ] ^= convInfo->currentIV[ i ];

		/* Move on to next block of data */
		noBytes -= ivCount;
		buffer += ivCount;
		}

	/* Remember how much of the IV is still available for use */
	convInfo->ivCount = ( ivCount % RC5_BLOCKSIZE );

	return( CRYPT_OK );
	}

/****************************************************************************
*																			*
*							RC5 Key Management Routines						*
*																			*
****************************************************************************/

/* Key schedule a RC5 key */

int rc5InitKey( CONTEXT_INFO *contextInfoPtr, const void *key, 
				const int keyLength )
	{
	CONV_INFO *convInfo = contextInfoPtr->ctxConv;
	RC5_KEY *rc5Key = ( RC5_KEY * ) convInfo->key;

	/* Copy the key to internal storage */
	if( convInfo->userKey != key )
		memcpy( convInfo->userKey, key, keyLength );
	convInfo->userKeyLength = keyLength;

	RC5_32_set_key( rc5Key, keyLength, key, RC5_DEFAULT_ROUNDS );
	return( CRYPT_OK );
	}
#endif /* USE_RC5 */
