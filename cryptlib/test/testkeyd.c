/****************************************************************************
*																			*
*					cryptlib Database Keyset Test Routines					*
*						Copyright Peter Gutmann 1995-2003					*
*																			*
****************************************************************************/

#ifdef _MSC_VER
  #include "../cryptlib.h"
  #include "../test/test.h"
#else
  #include "cryptlib.h"
  #include "test/test.h"
#endif /* Braindamaged MSC include handling */

#if defined( __MVS__ ) || defined( __VMCMS__ )
  /* Suspend conversion of literals to ASCII. */
  #pragma convlit( suspend )
#endif /* EBCDIC systems */

/****************************************************************************
*																			*
*					Database Keyset Read/Write Tests						*
*																			*
****************************************************************************/

/* Read/write a certificate from a public-key keyset.  Returns 
   CRYPT_ERROR_NOTAVAIL if this keyset type isn't available from this 
   cryptlib build, CRYPT_ERROR_FAILED if the keyset/data source access 
   failed */

static int testKeysetRead( const CRYPT_KEYSET_TYPE keysetType,
						   const char *keysetName,
						   const char *keyName, 
						   const CRYPT_CERTTYPE_TYPE type )
	{
	CRYPT_KEYSET cryptKeyset;
	CRYPT_CERTIFICATE cryptCert;
	int value, status;

	/* Open the keyset with a check to make sure this access method exists 
	   so we can return an appropriate error message */
	status = cryptKeysetOpen( &cryptKeyset, CRYPT_UNUSED, keysetType, 
							  keysetName, CRYPT_KEYOPT_READONLY );
	if( status == CRYPT_ERROR_PARAM3 )
		/* This type of keyset access not available */
		return( CRYPT_ERROR_NOTAVAIL );
	if( cryptStatusError( status ) )
		{
		printf( "cryptKeysetOpen() failed with error code %d, line %d.\n",
				status, __LINE__ );
		return( CRYPT_ERROR_FAILED );
		}

	/* Read the certificate from the keyset */
	status = cryptGetPublicKey( cryptKeyset, &cryptCert, CRYPT_KEYID_NAME,
								keyName );
	if( cryptStatusError( status ) )
		{
		/* The access to network-accessible keysets can be rather 
		   temperamental and can fail at this point even though it's not a
		   fatal error.  The calling code knows this and will continue the
		   self-test with an appropriate warning, so we explicitly clean up 
		   after ourselves to make sure we don't get a CRYPT_ORPHAN on
		   shutdown */
		if( keysetType == CRYPT_KEYSET_HTTP && \
			status == CRYPT_ERROR_NOTFOUND )
			{
			/* 404's are relatively common, and non-fatal */
			extErrorExit( cryptKeyset, "cryptGetPublicKey()", status, __LINE__ );
			puts( "  (404 is a common HTTP error, and non-fatal)." );
			return( TRUE );
			}

		return( extErrorExit( cryptKeyset, "cryptGetPublicKey()", status, 
							  __LINE__ ) );
		}

	/* Make sure we got what we were expecting */
	cryptGetAttribute( cryptCert, CRYPT_CERTINFO_CERTTYPE, &value );
	if( value != type )
		{
		printf( "Expecting certificate object type %d, got %d.", type, value );
		return( FALSE );
		}
	if( value == CRYPT_CERTTYPE_CERTCHAIN || value == CRYPT_CERTTYPE_CRL )
		{
		const BOOLEAN isCertChain = ( value == CRYPT_CERTTYPE_CERTCHAIN ) ? \
									TRUE : FALSE;

		value = 0;
		cryptSetAttribute( cryptCert, CRYPT_CERTINFO_CURRENT_CERTIFICATE, 
						   CRYPT_CURSOR_FIRST );
		do
			value++;
		while( cryptSetAttribute( cryptCert,
								  CRYPT_CERTINFO_CURRENT_CERTIFICATE, 
								  CRYPT_CURSOR_NEXT ) == CRYPT_OK );
		printf( isCertChain ? "Cert chain length = %d.\n" : \
							  "CRL has %d entries.\n", value );
		}

	/* Check the cert against the CRL.  Any kind of error is a failure since
	   the cert isn't in the CRL */
	if( keysetType != CRYPT_KEYSET_LDAP && \
		keysetType != CRYPT_KEYSET_HTTP )
		{
		puts( "Checking certificate against CRL." );
		status = cryptCheckCert( cryptCert, cryptKeyset );
		if( cryptStatusError( status ) )
			return( extErrorExit( cryptKeyset, "cryptCheckCert() (for CRL "
								  "in keyset)", status, __LINE__ ) );
		}

	/* Close the keyset */
	status = cryptKeysetClose( cryptKeyset );
	if( cryptStatusError( status ) )
		{
		printf( "cryptKeysetClose() failed with error code %d, line %d.\n",
				status, __LINE__ );
		return( FALSE );
		}

	cryptDestroyCert( cryptCert );
	return( TRUE );
	}

static int testKeysetWrite( const CRYPT_KEYSET_TYPE keysetType,
							const char *keysetName )
	{
	CRYPT_KEYSET cryptKeyset;
	CRYPT_CERTIFICATE cryptCert;
	BYTE buffer[ BUFFER_SIZE ];
	char name[ CRYPT_MAX_TEXTSIZE + 1 ];
	int length, status;

	/* Import the certificate from a file - this is easier than creating one
	   from scratch.  We use one of the later certs in the text set, since 
	   this contains an email address, which the earlier ones don't */
	status = importCertFromTemplate( &cryptCert, CERT_FILE_TEMPLATE, 5 );
	if( cryptStatusError( status ) )
		{
		puts( "Couldn't read certificate from file, skipping test of keyset "
			  "write..." );
		return( TRUE );
		}

	/* Create the database keyset with a check to make sure this access
	   method exists so we can return an appropriate error message.  If the
	   database table already exists, this will return a duplicate data
	   error so we retry the open with no flags to open the existing database
	   keyset for write access */
	status = cryptKeysetOpen( &cryptKeyset, CRYPT_UNUSED, keysetType, 
							  keysetName, CRYPT_KEYOPT_CREATE );
	if( cryptStatusOK( status ) )
		printf( "Created new certificate database '%s'.\n", keysetName );
	if( status == CRYPT_ERROR_PARAM3 )
		{
		/* This type of keyset access isn't available, return a special error
		   code to indicate that the test wasn't performed, but that this
		   isn't a reason to abort processing */
		cryptDestroyCert( cryptCert );
		return( CRYPT_ERROR_NOTAVAIL );
		}
	if( status == CRYPT_ERROR_DUPLICATE )
		status = cryptKeysetOpen( &cryptKeyset, CRYPT_UNUSED, keysetType, 
								  keysetName, 0 );
	if( cryptStatusError( status ) )
		{
		cryptDestroyCert( cryptCert );
		printf( "cryptKeysetOpen() failed with error code %d, line %d.\n",
				status, __LINE__ );
		if( status == CRYPT_ERROR_OPEN )
			return( CRYPT_ERROR_FAILED );
		return( FALSE );
		}

	/* Write the key to the database */
	puts( "Adding certificate." );
	status = cryptAddPublicKey( cryptKeyset, cryptCert );
	if( status == CRYPT_ERROR_DUPLICATE )
		{
		/* The key is already present, delete it and retry the write */
		status = cryptGetAttributeString( cryptCert, 
								CRYPT_CERTINFO_COMMONNAME, name, &length );
		if( cryptStatusOK( status ) )
			{
			name[ length ] = '\0';
			status = cryptDeleteKey( cryptKeyset, CRYPT_KEYID_NAME, name );
			}
		if( cryptStatusError( status ) )
			return( extErrorExit( cryptKeyset, "cryptDeleteKey()", status, 
								  __LINE__ ) );
		status = cryptAddPublicKey( cryptKeyset, cryptCert );
		}
	if( cryptStatusError( status ) )
		{
		printExtError( cryptKeyset, "cryptAddPublicKey()", status, __LINE__ );

		/* LDAP writes can fail due to the chosen directory not supporting the
		   schema du jour, so we're a bit more careful about cleaning up since
		   we'll skip the error and continue processing */
		cryptDestroyCert( cryptCert );
		cryptKeysetClose( cryptKeyset );
		return( FALSE );
		}
	cryptDestroyCert( cryptCert );

	/* Add a second cert with C=US so that we've got enough certs to properly 
	   exercise the query code.  This cert is highly unusual in that it 
	   doesn't have a DN, so we have to move up the DN looking for higher-up
	   values, in this case the OU */
	if( keysetType != CRYPT_KEYSET_LDAP )
		{
		status = importCertFromTemplate( &cryptCert, CERT_FILE_TEMPLATE, 2 );
		if( cryptStatusError( status ) )
			{
			puts( "Couldn't read second certificate from file, skipping "
				  "remaining keyset write tests..." );
			cryptKeysetClose( cryptKeyset );
			return( TRUE );
			}
		status = cryptAddPublicKey( cryptKeyset, cryptCert );
		if( status == CRYPT_ERROR_DUPLICATE )
			{
			status = cryptGetAttributeString( cryptCert, 
							CRYPT_CERTINFO_COMMONNAME, name, &length );
			if( cryptStatusError( status ) )
				status = cryptGetAttributeString( cryptCert, 
							CRYPT_CERTINFO_ORGANIZATIONALUNITNAME, name, &length );
			if( cryptStatusOK( status ) )
				{
				name[ length ] = '\0';
				status = cryptDeleteKey( cryptKeyset, CRYPT_KEYID_NAME, name );
				}
			if( cryptStatusOK( status ) )
				status = cryptAddPublicKey( cryptKeyset, cryptCert );
			}
		if( cryptStatusError( status ) )
			return( extErrorExit( cryptKeyset, "cryptAddPublicKey()", 
								  status, __LINE__ ) );
		cryptDestroyCert( cryptCert );
		}

	/* Now try the same thing with a CRL.  This code also tests the 
	   duplicate-detection mechanism, if we don't get a duplicate error 
	   there's a problem */
	puts( "Adding CRL." );
	status = importCertFromTemplate( &cryptCert, CRL_FILE_TEMPLATE, 1 );
	if( cryptStatusError( status ) )
		{
		puts( "Couldn't read CRL from file, skipping test of keyset "
			  "write..." );
		return( TRUE );
		}
	status = cryptAddPublicKey( cryptKeyset, cryptCert );
	if( cryptStatusError( status ) && status != CRYPT_ERROR_DUPLICATE )
		return( extErrorExit( cryptKeyset, "cryptAddPublicKey()", status, 
							  __LINE__ ) );
	status = cryptAddPublicKey( cryptKeyset, cryptCert );
	if( status != CRYPT_ERROR_DUPLICATE )
		{
		puts( "Addition of duplicate item to keyset failed to produce "
			  "CRYPT_ERROR_DUPLICATE" );
		return( FALSE );
		}
	cryptDestroyCert( cryptCert );

	/* Finally, try it with a cert chain */
	puts( "Adding cert chain." );
	filenameFromTemplate( buffer, CERTCHAIN_FILE_TEMPLATE, 1 );
	status = importCertFile( &cryptCert, buffer );
	if( cryptStatusError( status ) )
		{
		puts( "Couldn't read cert chain from file." );
		return( FALSE );
		}
	status = cryptAddPublicKey( cryptKeyset, cryptCert );
	if( cryptStatusError( status ) && status != CRYPT_ERROR_DUPLICATE )
		return( extErrorExit( cryptKeyset, "cryptAddPublicKey()", status, 
							  __LINE__ ) );
	cryptDestroyCert( cryptCert );

	/* In addition to the other certs we also add the generic user cert, 
	   which is used later in other tests.  Since it may have been added 
	   earlier, we try and delete it first (we can't use the existing 
	   version since the issuerAndSerialNumber won't match the one in the 
	   private-key keyset) */
	status = getPublicKey( &cryptCert, USER_PRIVKEY_FILE, 
						   USER_PRIVKEY_LABEL );
	if( cryptStatusError( status ) )
		{
		puts( "Couldn't read user cert from file." );
		return( FALSE );
		}
	cryptGetAttributeString( cryptCert, CRYPT_CERTINFO_COMMONNAME,
							 name, &length );
	name[ length ] = '\0';
	do
		status = cryptDeleteKey( cryptKeyset, CRYPT_KEYID_NAME, name );
	while( cryptStatusOK( status ) );
	status = cryptAddPublicKey( cryptKeyset, cryptCert );
	if( status == CRYPT_ERROR_NOTFOUND )
		/* This can occur if a database keyset is defined but hasn't been
		   initialised yet so the necessary tables don't exist, it can be
		   opened but an attempt to add a key will return a not found error 
		   since it's the table itself rather than any item within it that 
		   isn't being found */
		status = CRYPT_OK;
	if( cryptStatusError( status ) )
		return( extErrorExit( cryptKeyset, "cryptAddPublicKey()", status, 
							  __LINE__ ) );
	cryptDestroyCert( cryptCert );

	/* Make sure the deletion code works properly.  This is an artifact of
	   the way RDBMS' work, the delete query can execute successfully but
	   not delete anything so we make sure the glue code correctly 
	   translates this into a CRYPT_DATA_NOTFOUND */
	status = cryptDeleteKey( cryptKeyset, CRYPT_KEYID_NAME,
							 "Mr.Not Appearing in this Keyset" );
	if( status != CRYPT_ERROR_NOTFOUND )
		{
		puts( "Attempt to delete a nonexistant key reports success, the "
			  "database backend glue\ncode needs to be fixed to handle this "
			  "correctly." );
		return( FALSE );
		}

	/* Close the keyset */
	status = cryptKeysetClose( cryptKeyset );
	if( cryptStatusError( status ) )
		printf( "cryptKeysetClose() failed with error code %d, line %d.\n",
				status, __LINE__ );

	return( TRUE );
	}

/* Perform a general keyset query */

int testQuery( const CRYPT_KEYSET_TYPE keysetType, const char *keysetName )
	{
	CRYPT_KEYSET cryptKeyset;
	int count = 0, status;

	/* Open the database keyset */
	status = cryptKeysetOpen( &cryptKeyset, CRYPT_UNUSED, keysetType, 
							  keysetName, CRYPT_KEYOPT_READONLY );
	if( cryptStatusError( status ) )
		{
		printf( "cryptKeysetOpen() failed with error code %d, line %d.\n",
				status, __LINE__ );
		if( status == CRYPT_ERROR_OPEN )
			return( CRYPT_ERROR_FAILED );
		return( FALSE );
		}

	/* Send the query to the database and read back the results */
	status = cryptSetAttributeString( cryptKeyset, CRYPT_KEYINFO_QUERY, 
									  "$C='US'", 7 );
	if( cryptStatusError( status ) )
		return( extErrorExit( cryptKeyset, "Keyset query", status, 
							  __LINE__ ) );
	do
		{
		CRYPT_CERTIFICATE cryptCert;

		status = cryptGetPublicKey( cryptKeyset, &cryptCert,
									CRYPT_KEYID_NONE, NULL );
		if( cryptStatusOK( status ) )
			{
			count++;
			cryptDestroyCert( cryptCert );
			}
		}
	while( cryptStatusOK( status ) );
	if( cryptStatusError( status ) && status != CRYPT_ERROR_COMPLETE )
		return( extErrorExit( cryptKeyset, "cryptGetPublicKey()", status, 
							  __LINE__ ) );
	if( count < 2 )
		{
		puts( "Only one certificate was returned, this indicates that the "
			  "database backend\nglue code isn't processing ongoing queries "
			  "correctly." );
		return( FALSE );
		}
	printf( "%d certificate(s) matched the query.\n", count );

	/* Close the keyset */
	status = cryptKeysetClose( cryptKeyset );
	if( cryptStatusError( status ) )
		{
		printf( "cryptKeysetClose() failed with error code %d, line %d.\n",
				status, __LINE__ );
		return( FALSE );
		}

	return( TRUE );
	}

/* Read/write/query a certificate from a database keyset */

int testReadCert( void )
	{
	CRYPT_CERTIFICATE cryptCert;
	char name[ CRYPT_MAX_TEXTSIZE + 1 ];
	int length, status;

	/* Get the DN from the cert that we wrote earlier */
	status = importCertFromTemplate( &cryptCert, CERT_FILE_TEMPLATE, 5 );
	if( cryptStatusError( status ) )
		{
		puts( "Couldn't read certificate from file, skipping test of keyset "
			  "write..." );
		return( TRUE );
		}
	status = cryptGetAttributeString( cryptCert, CRYPT_CERTINFO_COMMONNAME, 
									  name, &length );
	if( cryptStatusError( status ) )
		return( extErrorExit( cryptCert, "cryptGetAttributeString()", status, 
							  __LINE__ ) );
	cryptDestroyCert( cryptCert );
	name[ length ] = '\0';

	puts( "Testing certificate database read..." );
	status = testKeysetRead( DATABASE_KEYSET_TYPE, DATABASE_KEYSET_NAME,
	 						 name, CRYPT_CERTTYPE_CERTIFICATE );
	if( status == CRYPT_ERROR_NOTAVAIL )
		/* Database keyset access not available */
		return( CRYPT_ERROR_NOTAVAIL );
	if( status == CRYPT_ERROR_FAILED )
		{
		puts( "This is probably because you haven't set up a database or "
			  "data source for use\nas a key database.  For this test to "
			  "work, you need to set up a database/data\nsource with the "
			  "name '" DATABASE_KEYSET_NAME "'.\n" );
		return( TRUE );
		}
	if( !status )
		return( FALSE );
	puts( "Reading complete cert chain." );
	status = testKeysetRead( DATABASE_KEYSET_TYPE, DATABASE_KEYSET_NAME,
	 						 "Thawte Freemail Member", 
							 CRYPT_CERTTYPE_CERTCHAIN );
	if( !status )
		return( FALSE );
	puts( "Certificate database read succeeded.\n" );
	return( TRUE );
	}

int testWriteCert( void )
	{
	int status;

	puts( "Testing certificate database write..." );
	status = testKeysetWrite( DATABASE_KEYSET_TYPE, DATABASE_KEYSET_NAME );
	if( status == CRYPT_ERROR_NOTAVAIL )
		/* Database keyset access not available */
		return( CRYPT_ERROR_NOTAVAIL );
	if( status == CRYPT_ERROR_FAILED )
		{
		printf( "This may be because you haven't set up a data source "
				"called '" DATABASE_KEYSET_NAME "'\nof type %d that can be "
				"used for the certificate store.  You can configure\nthe "
				"data source type and name using the DATABASE_KEYSET_xxx "
				"settings in\ntest/test.h.\n", DATABASE_KEYSET_TYPE );
		return( FALSE );
		}
	if( !status )
		return( FALSE );
	puts( "Certificate database write succeeded.\n" );
	return( TRUE );
	}

int testKeysetQuery( void )
	{
	int status;

	puts( "Testing general certificate database query..." );
	status = testQuery( DATABASE_KEYSET_TYPE, DATABASE_KEYSET_NAME );
	if( status == CRYPT_ERROR_NOTAVAIL )
		/* Database keyset access not available */
		return( CRYPT_ERROR_NOTAVAIL );
	if( status == CRYPT_ERROR_FAILED )
		{
		puts( "This is probably because you haven't set up a database or "
			  "data source for use\nas a key database.  For this test to "
			  "work, you need to set up a database/data\nsource with the "
			  "name '" DATABASE_KEYSET_NAME "'.\n" );
		return( FALSE );
		}
	if( !status )
		return( FALSE );
	puts( "Certificate database query succeeded.\n" );
	return( TRUE );
	}

/* Read/write/query a certificate from a database keyset accessed via the 
   generic plugin interface */

int testWriteCertDbx( void )
	{
	int status;

	puts( "Testing certificate database write via plugin interface..." );
	status = testKeysetWrite( CRYPT_KEYSET_PLUGIN, 
							  DATABASE_PLUGIN_KEYSET_NAME );
	if( status == CRYPT_ERROR_NOTAVAIL )
		/* Database plugin keyset access not available */
		return( CRYPT_ERROR_NOTAVAIL );
	if( status == CRYPT_ERROR_FAILED )
		{
		puts( "This may be because you haven't set up a database plugin "
			  "available as\n'" DATABASE_PLUGIN_KEYSET_NAME "' that can be "
			  "used for the certificate store.\nYou can configure the "
			  "plugin URL using the DATABASE_PLUGIN_KEYSET_xxx\nsettings in "
			  "test/test.h.\n" );
		return( FALSE );
		}
	if( !status )
		return( FALSE );
	puts( "Certificate database write succeeded.\n" );
	return( TRUE );
	}

/* Read/write/query a certificate from an LDAP keyset */

int testReadCertLDAP( void )
	{
	CRYPT_KEYSET cryptKeyset;
	static const char *ldapErrorString = \
		"LDAP directory read failed, probably because the standard being "
		"used by the\ndirectory server differs from the one used by the "
		"LDAP client software (pick\na standard, any standard).  If you "
		"know how the directory being used is\nconfigured, you can try "
		"changing the CRYPT_OPTION_KEYS_LDAP_xxx settings to\nmatch those "
		"used by the server.  Processing will continue without treating\n"
		"this as a fatal error.\n";
	char *ldapKeysetName = LDAP_KEYSET_NAME1;
	char ldapAttribute1[ CRYPT_MAX_TEXTSIZE + 1 ];
	char ldapAttribute2[ CRYPT_MAX_TEXTSIZE + 1 ];
	char certName[ CRYPT_MAX_TEXTSIZE ], caCertName[ CRYPT_MAX_TEXTSIZE ];
	char crlName[ CRYPT_MAX_TEXTSIZE ];
	int length, status;

	/* LDAP directories come and go, check to see which one is currently 
	   around */
	puts( "Testing LDAP directory availability..." );
	status = cryptKeysetOpen( &cryptKeyset, CRYPT_UNUSED, CRYPT_KEYSET_LDAP, 
							  ldapKeysetName, CRYPT_KEYOPT_READONLY );
	if( status == CRYPT_ERROR_PARAM3 )
		/* LDAP keyset access not available */
		return( CRYPT_ERROR_NOTAVAIL );
	if( status == CRYPT_ERROR_OPEN )
		{
		puts( LDAP_KEYSET_NAME1 " not available, trying alternative "
			  "directory..." );
		ldapKeysetName = LDAP_KEYSET_NAME2;
		status = cryptKeysetOpen( &cryptKeyset, CRYPT_UNUSED, 
								  CRYPT_KEYSET_LDAP, ldapKeysetName, 
								  CRYPT_KEYOPT_READONLY );
		}
	if( status == CRYPT_ERROR_OPEN )
		{
		puts( LDAP_KEYSET_NAME2 " not available either." );
		puts( "None of the test LDAP directories are available.  If you need "
			  "to test the\nLDAP capabilities, you need to set up an LDAP "
			  "directory that can be used\nfor the certificate store.  You "
			  "can configure the LDAP directory using the\nLDAP_KEYSET_xxx "
			  "settings in test/test.h.  Alternatively, if this message\n"
			  "took a long time to appear you may be behind a firewall that "
			  "blocks LDAP\ntraffic.\n" );
		return( FALSE );
		}
	if( cryptStatusError( status ) )
		{
		printf( "cryptKeysetOpen() failed with error code %d, line %d.\n",
				status, __LINE__ );
		return( FALSE );
		}
	status = cryptGetAttributeString( CRYPT_UNUSED, 
									  CRYPT_OPTION_KEYS_LDAP_OBJECTCLASS,
									  ldapAttribute1, &length );
	if( cryptStatusOK( status ) )
		{
		ldapAttribute1[ length ] = '\0';
		status = cryptGetAttributeString( cryptKeyset, 
										  CRYPT_OPTION_KEYS_LDAP_OBJECTCLASS,
										  ldapAttribute2, &length );
		}
	if( cryptStatusOK( status ) )
		ldapAttribute2[ length ] = '\0';
	if( cryptStatusError( status ) || \
		strcmp( ldapAttribute1, ldapAttribute2 ) )
		{
		printf( "Failed to get/set keyset attribute via equivalent global "
				"attribute, error\ncode %d, value '%s', should be\n'%s', "
				"line %d.\n", status, ldapAttribute2, ldapAttribute1, 
				__LINE__ );
		return( FALSE );
		}
	cryptKeysetClose( cryptKeyset );
	printf( "  LDAP directory %s seems to be up, using that for read test.\n",
			ldapKeysetName );

	/* Now it gets tricky, we have to jump through all sorts of hoops to
	   run the tests in an automated manner since the innate incompatibility
	   of LDAP directory setups means that even though cryptlib's LDAP access
	   code retries failed queries with various options, we still need to
	   manually override some settings here.  The simplest option is a direct
	   read with no special-case handling */
	if( !strcmp( ldapKeysetName, LDAP_KEYSET_NAME1 ) )
		{
		puts( "Testing LDAP certificate read..." );
		status = testKeysetRead( CRYPT_KEYSET_LDAP, ldapKeysetName,
								 LDAP_CERT_NAME1, CRYPT_CERTTYPE_CERTIFICATE );
		if( !status )
			{
			/* Since we can never be sure about the LDAP schema du jour, we
			   don't treat a failure as a fatal error */
			puts( ldapErrorString );
			return( FALSE );
			}

		/* This directory doesn't contain CRLs (or at least not at any known
		   location) so we skip the CRL read test */
		puts( "LDAP certificate read succeeded (CRL read skipped).\n" );
		return( TRUE );
		}

	/* The LDAP directory we're using for these tests doesn't recognise the 
	   ';binary' modifier which is required by LDAP servers in order to get 
	   them to work properly, we have to change the attribute name around 
	   the read calls to the format expected by the server.
	   
	   In addition because the magic formula for fetching a CRL doesn't seem
	   to work for certificates, the CRL read is done first */
	puts( "Testing LDAP CRL read..." );
	cryptGetAttributeString( CRYPT_UNUSED, CRYPT_OPTION_KEYS_LDAP_CRLNAME, 
							 crlName, &length );
	certName[ length ] = '\0';
	cryptSetAttributeString( CRYPT_UNUSED, CRYPT_OPTION_KEYS_LDAP_CRLNAME, 
							 "certificateRevocationList", 25 );
	status = testKeysetRead( CRYPT_KEYSET_LDAP, ldapKeysetName,
							 LDAP_CRL_NAME2, CRYPT_CERTTYPE_CRL );
	cryptSetAttributeString( CRYPT_UNUSED, CRYPT_OPTION_KEYS_LDAP_CRLNAME, 
							 crlName, strlen( crlName ) );
	if( !status )
		{
		/* Since we can never be sure about the LDAP schema du jour, we
		   don't treat a failure as a fatal error */
		puts( ldapErrorString );
		return( FALSE );
		}

	puts( "Testing LDAP certificate read..." );
	cryptGetAttributeString( CRYPT_UNUSED, CRYPT_OPTION_KEYS_LDAP_CERTNAME, 
							 certName, &length );
	certName[ length ] = '\0';
	cryptSetAttributeString( CRYPT_UNUSED, CRYPT_OPTION_KEYS_LDAP_CERTNAME, 
							 "userCertificate", 15 );
	cryptGetAttributeString( CRYPT_UNUSED, CRYPT_OPTION_KEYS_LDAP_CACERTNAME, 
							 caCertName, &length );
	certName[ length ] = '\0';
	cryptSetAttributeString( CRYPT_UNUSED, CRYPT_OPTION_KEYS_LDAP_CACERTNAME, 
							 "cACertificate", 13 );
	status = testKeysetRead( CRYPT_KEYSET_LDAP, ldapKeysetName,
							 LDAP_CERT_NAME1, CRYPT_CERTTYPE_CERTIFICATE );
	cryptSetAttributeString( CRYPT_UNUSED, CRYPT_OPTION_KEYS_LDAP_CERTNAME, 
							 certName, strlen( certName ) );
	cryptSetAttributeString( CRYPT_UNUSED, CRYPT_OPTION_KEYS_LDAP_CACERTNAME, 
							 caCertName, strlen( caCertName ) );
	if( !status )
		{
		/* Since we can never be sure about the LDAP schema du jour, we
		   don't treat a failure as a fatal error */
		puts( "LDAP directory read failed, probably due to the magic "
			  "incantatation to fetch\na certificate from this server not "
			  "matching the one used to fetch a CRL.\nProcessing will "
			  "continue without treating this as a fatal error.\n" );
		return( FALSE );
		}
	puts( "LDAP certificate/CRL read succeeded.\n" );

	return( TRUE );
	}

int testWriteCertLDAP( void )
	{
	int status;

	puts( "Testing LDAP directory write..." );
	status = testKeysetWrite( CRYPT_KEYSET_LDAP, LDAP_KEYSET_NAME1 );
	if( status == CRYPT_ERROR_NOTAVAIL )
		/* LDAP keyset access not available */
		return( CRYPT_ERROR_NOTAVAIL );
	if( status == CRYPT_ERROR_FAILED )
		{
		puts( "This is probably because you haven't set up an LDAP "
			  "directory for use as the\nkey store.  For this test to work,"
			  "you need to set up a directory with the\nname '"
			  LDAP_KEYSET_NAME1 "'.\n" );
		return( FALSE );
		}
	if( !status )
		{
		/* Since we can never be sure about the LDAP schema du jour, we
		   don't treat a failure as a fatal error */
		puts( "LDAP directory write failed, probably due to the standard "
			  "being used by the\ndirectory differing from the one used "
			  "by cryptlib (pick a standard, any\nstandard).  Processing "
			  "will continue without treating this as a fatal error.\n" );
		return( FALSE );
		}
	puts( "LDAP directory write succeeded.\n" );
	return( TRUE );
	}

/* Read a certificate from a web page */

int testReadCertURL( void )
	{
	int status;

	/* Test fetching a cert from a URL via an HTTP proxy.  This requires
	   a random open proxy for testing (unless the site happens to be running
	   an HTTP proxy), www.openproxies.com will provide this */
#if 0	/* This is usually disabled because most people aren't behind HTTP 
		   proxies, and abusing other people's misconfigured HTTP caches/
		   proxies for testing isn't very nice */
	puts( "Testing HTTP certificate read from URL via proxy..." );
	cryptSetAttributeString( CRYPT_UNUSED, CRYPT_OPTION_NET_HTTP_PROXY,
							 "proxy.zetwuinwest.com.pl:80", 27 );
	status = testKeysetRead( CRYPT_KEYSET_HTTP, HTTP_KEYSET_CERT_NAME, 
							 "[none]", CRYPT_CERTTYPE_CERTIFICATE );
	if( status == CRYPT_ERROR_NOTAVAIL )	/* HTTP keyset access not avail.*/
		return( CRYPT_ERROR_NOTAVAIL );
	if( !status )
		return( FALSE );
#endif /* 0 */

	/* Test fetching a cert from a URL */
	puts( "Testing HTTP certificate read from URL..." );
	status = testKeysetRead( CRYPT_KEYSET_HTTP, HTTP_KEYSET_CERT_NAME, 
							 "[none]", CRYPT_CERTTYPE_CERTIFICATE );
	if( status == CRYPT_ERROR_NOTAVAIL )	/* HTTP keyset access not avail.*/
		return( CRYPT_ERROR_NOTAVAIL );
	if( status == CRYPT_ERROR_FAILED )
		{
		puts( "This is probably because the server isn't available or "
			  "inaccessible.\n" );
		return( TRUE );
		}
	if( !status )
		{
		puts( "If this message took a long time to appear, you may be "
			  "behind a firewall\nthat blocks HTTP traffic.\n" );
		return( FALSE );
		}

	/* Test fetching a CRL from a URL */
	puts( "Testing HTTP CRL read from URL..." );
	status = testKeysetRead( CRYPT_KEYSET_HTTP, HTTP_KEYSET_CRL_NAME, 
							 "[none]", CRYPT_CERTTYPE_CRL );
	if( status == CRYPT_ERROR_NOTAVAIL )	/* HTTP keyset access not avail.*/
		return( CRYPT_ERROR_NOTAVAIL );
	if( !status )
		return( FALSE );

	/* Test fetching a huge CRL from a URL, to check the ability to read 
	   arbitrary-length HTTP data */
#if 0	/* This is usually disabled because of the CRL size */
	puts( "Testing HTTP mega-CRL read from URL..." );
	status = testKeysetRead( CRYPT_KEYSET_HTTP, HTTP_KEYSET_HUGECRL_NAME,
							 "[none]", CRYPT_CERTTYPE_CRL );
	if( status == CRYPT_ERROR_NOTAVAIL )	/* HTTP keyset access not avail.*/
		return( CRYPT_ERROR_NOTAVAIL );
	if( !status )
		return( FALSE );
#endif /* 0 */

	puts( "HTTP certificate/CRL read from URL succeeded.\n" );
	return( TRUE );
	}

/* Read a certificate from an HTTP certificate store */

int testReadCertHTTP( void )
	{
	int status;

	puts( "Testing HTTP certificate store read..." );
	status = testKeysetRead( CRYPT_KEYSET_HTTP, HTTP_KEYSET_CERT_NAME, 
							 "Verisign", CRYPT_CERTTYPE_CERTIFICATE );
	if( status == CRYPT_ERROR_NOTAVAIL )	/* HTTP keyset access not avail.*/
		return( CRYPT_ERROR_NOTAVAIL );
	if( status == CRYPT_ERROR_FAILED )
		{
		puts( "This is probably because the server isn't available or "
			  "inaccessible.\n" );
		return( TRUE );
		}
	if( !status )
		{
		puts( "If this message took a long time to appear, you may be "
			  "behind a firewall\nthat blocks HTTP traffic.\n" );
		return( FALSE );
		}

	puts( "HTTP certificate store read succeeded.\n" );
	return( TRUE );
	}
