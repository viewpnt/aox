/****************************************************************************
*																			*
*								ACL Definitions								*
*						Copyright Peter Gutmann 1998-2003					*
*																			*
****************************************************************************/

#ifndef _CRYPTACD_DEFINED

#define _CRYPTACD_DEFINED

/* Various includes and defines needed for range checking */

#include <limits.h>		/* For INT_MAX */

/****************************************************************************
*																			*
*						Object/Parameter Type Information					*
*																			*
****************************************************************************/

/* The attribute's type, for attribute ACLs.  The basic values are boolean,
   numeric, or byte string, there are also some special types such as object
   handles that place extra constraints on the attribute */

typedef enum {
	ATTRIBUTE_VALUE_NONE,			/* Non-value */
	ATTRIBUTE_VALUE_BOOLEAN,		/* Boolean flag */
	ATTRIBUTE_VALUE_NUMERIC,		/* Numeric value */
	ATTRIBUTE_VALUE_STRING,			/* Byte string */
	ATTRIBUTE_VALUE_WCSTRING,		/* (Possible) widechar string */
	ATTRIBUTE_VALUE_OBJECT,			/* Object handle */
	ATTRIBUTE_VALUE_TIME,			/* Timestamp */
	ATTRIBUTE_VALUE_SPECIAL			/* Special-case value with sub-ACLs */
	} ATTRIBUTE_VALUE_TYPE;

/* The parameter's type, for mechanism ACLs.  The basic values are boolean, 
   numeric, or byte string, there are also some special types such as object 
   handles that place extra constraints on the attribute */

typedef enum {
	MECHPARAM_VALUE_NONE,			/* Non-value */
	MECHPARAM_VALUE_BOOLEAN,		/* Boolean flag */
	MECHPARAM_VALUE_NUMERIC,		/* Numeric value */
	MECHPARAM_VALUE_STRING,			/* Byte string */
	MECHPARAM_VALUE_STRING_OPT,		/* Byte string or (NULL, 0) */
	MECHPARAM_VALUE_STRING_NONE,	/* Empty (NULL, 0) string */
	MECHPARAM_VALUE_OBJECT,			/* Object handle */
	MECHPARAM_VALUE_UNUSED			/* CRYPT_UNUSED */
	} MECHPARAM_VALUE_TYPE;

/* Bit flags for specifying valid object subtypes.  Since the full field names 
   are rather long, we define a shortened form (only visible within the ACL
   definitions) that reduces the space required to define them */

#define ST_CTX_CONV				SUBTYPE_CTX_CONV
#define ST_CTX_PKC				SUBTYPE_CTX_PKC
#define ST_CTX_HASH				SUBTYPE_CTX_HASH
#define ST_CTX_MAC				SUBTYPE_CTX_MAC
#define ST_CTX_ANY				( ST_CTX_CONV | ST_CTX_PKC | ST_CTX_HASH | \
								  ST_CTX_MAC )

#define ST_CERT_CERT			SUBTYPE_CERT_CERT
#define ST_CERT_CERTREQ			SUBTYPE_CERT_CERTREQ
#define ST_CERT_REQ_CERT		SUBTYPE_CERT_REQ_CERT
#define ST_CERT_REQ_REV			SUBTYPE_CERT_REQ_REV
#define ST_CERT_CERTCHAIN		SUBTYPE_CERT_CERTCHAIN
#define ST_CERT_ATTRCERT		SUBTYPE_CERT_ATTRCERT
#define ST_CERT_CRL				SUBTYPE_CERT_CRL
#define ST_CERT_CMSATTR			SUBTYPE_CERT_CMSATTR
#define ST_CERT_RTCS_REQ		SUBTYPE_CERT_RTCS_REQ
#define ST_CERT_RTCS_RESP		SUBTYPE_CERT_RTCS_RESP
#define ST_CERT_OCSP_REQ		SUBTYPE_CERT_OCSP_REQ
#define ST_CERT_OCSP_RESP		SUBTYPE_CERT_OCSP_RESP
#define ST_CERT_PKIUSER			SUBTYPE_CERT_PKIUSER
#define ST_CERT_ANY_CERT		( ST_CERT_CERT | ST_CERT_CERTREQ | \
								  SUBTYPE_CERT_REQ_CERT | ST_CERT_CERTCHAIN )
#define ST_CERT_ANY				( ST_CERT_ANY_CERT | ST_CERT_ATTRCERT | \
								  ST_CERT_REQ_REV | ST_CERT_CRL | \
								  ST_CERT_CMSATTR | ST_CERT_RTCS_REQ | \
								  ST_CERT_RTCS_RESP | ST_CERT_OCSP_REQ | \
								  ST_CERT_OCSP_RESP | ST_CERT_PKIUSER )

#define ST_KEYSET_FILE			SUBTYPE_KEYSET_FILE
#define ST_KEYSET_FILE_PARTIAL	SUBTYPE_KEYSET_FILE_PARTIAL
#define ST_KEYSET_DBMS			SUBTYPE_KEYSET_DBMS
#define ST_KEYSET_DBMS_STORE	SUBTYPE_KEYSET_DBMS_STORE
#define ST_KEYSET_HTTP			SUBTYPE_KEYSET_HTTP
#define ST_KEYSET_LDAP			SUBTYPE_KEYSET_LDAP
#define ST_KEYSET_ANY			( ST_KEYSET_FILE | ST_KEYSET_FILE_PARTIAL | \
								  ST_KEYSET_DBMS | ST_KEYSET_DBMS_STORE | \
								  ST_KEYSET_HTTP | ST_KEYSET_LDAP )

#define ST_ENV_ENV				SUBTYPE_ENV_ENV
#define ST_ENV_ENV_PGP			SUBTYPE_ENV_ENV_PGP
#define ST_ENV_DEENV			SUBTYPE_ENV_DEENV
#define ST_ENV_ANY				( SUBTYPE_ENV_ENV | SUBTYPE_ENV_ENV_PGP | \
								  SUBTYPE_ENV_DEENV )

#define ST_DEV_SYSTEM			SUBTYPE_DEV_SYSTEM
#define ST_DEV_FORT				SUBTYPE_DEV_FORTEZZA
#define ST_DEV_P11				SUBTYPE_DEV_PKCS11
#define ST_DEV_CAPI				SUBTYPE_DEV_CRYPTOAPI
#define ST_DEV_ANY_STD			( SUBTYPE_DEV_FORTEZZA | SUBTYPE_DEV_PKCS11 | \
								  SUBTYPE_DEV_CRYPTOAPI )
#define ST_DEV_ANY				( ST_DEV_ANY_STD | SUBTYPE_DEV_SYSTEM )

#define ST_SESS_SSH				SUBTYPE_SESSION_SSH
#define ST_SESS_SSH_SVR			SUBTYPE_SESSION_SSH_SVR
#define ST_SESS_SSL				SUBTYPE_SESSION_SSL
#define ST_SESS_SSL_SVR			SUBTYPE_SESSION_SSL_SVR
#define ST_SESS_RTCS			SUBTYPE_SESSION_RTCS
#define ST_SESS_RTCS_SVR		SUBTYPE_SESSION_RTCS_SVR
#define ST_SESS_OCSP			SUBTYPE_SESSION_OCSP
#define ST_SESS_OCSP_SVR		SUBTYPE_SESSION_OCSP_SVR
#define ST_SESS_TSP				SUBTYPE_SESSION_TSP
#define ST_SESS_TSP_SVR			SUBTYPE_SESSION_TSP_SVR
#define ST_SESS_CMP				SUBTYPE_SESSION_CMP
#define ST_SESS_CMP_SVR			SUBTYPE_SESSION_CMP_SVR
#define ST_SESS_SCEP			SUBTYPE_SESSION_SCEP
#define ST_SESS_SCEP_SVR		SUBTYPE_SESSION_SCEP_SVR
#define ST_SESS_ANY_SVR			( ST_SESS_SSH_SVR | ST_SESS_SSL_SVR | \
								  ST_SESS_RTCS_SVR | ST_SESS_OCSP_SVR | \
								  ST_SESS_TSP_SVR | ST_SESS_CMP_SVR | \
								  ST_SESS_SCEP_SVR )
#define ST_SESS_ANY_CLIENT		( ST_SESS_SSH | ST_SESS_SSL | ST_SESS_RTCS | \
								  ST_SESS_OCSP | ST_SESS_TSP | ST_SESS_CMP | \
								  ST_SESS_SCEP )
#define ST_SESS_ANY_DATA		( ST_SESS_SSH | ST_SESS_SSH_SVR | \
								  ST_SESS_SSL | ST_SESS_SSL_SVR )
#define ST_SESS_ANY_REQRESP		( ST_SESS_RTCS | ST_SESS_RTCS_SVR | \
								  ST_SESS_OCSP | ST_SESS_OCSP_SVR | \
								  ST_SESS_TSP | ST_SESS_TSP_SVR | \
								  ST_SESS_CMP | ST_SESS_CMP_SVR | \
								  ST_SESS_SCEP | ST_SESS_SCEP_SVR )
#define ST_SESS_ANY_SEC			( ST_SESS_ANY_DATA | \
								  ST_SESS_CMP | ST_SESSION_CMP_SVR )
#define ST_SESS_ANY				( ST_SESS_ANY_CLIENT | ST_SESS_ANY_SVR )

#define ST_USER_NORMAL			SUBTYPE_USER_NORMAL
#define ST_USER_SO				SUBTYPE_USER_SO
#define ST_USER_CA				SUBTYPE_USER_CA
#define ST_USER_ANY				( SUBTYPE_USER_NORMAL | SUBTYPE_USER_SO | \
								  SUBTYPE_USER_CA )

/* A subtype value that allows access for any object subtype and for no
   object subtypes */

#define ST_ANY					0x7FFFFFFFL
#define ST_NONE					0

/* A data type to store subtype values */

#if INT_MAX <= 65535L
  typedef unsigned long OBJECT_SUBTYPE;
#else
  typedef unsigned int OBJECT_SUBTYPE;
#endif /* 16- vs.32-bit systems */

/****************************************************************************
*																			*
*							Access Permission Information					*
*																			*
****************************************************************************/

/* Read/write/delete permission flags.  Each object can have two modes, "low" 
   and "high", whose exact definition depends on the object type.  At some 
   point an operation on an object (loading a key for a context, signing a 
   cert) will move it from the low to the high state, at which point a much
   more restricted set of permissions apply.  The permissions are given as
   RWD_RWD with the first set being for the object in the high state and the
   second for the object in the low state.

   In addition to the usual external-access permssions, some attributes are
   only visible internally.  Normal attributes have matching internal-access
   and external-access permssions but the internal-access-only ones have the
   external-access permissions turned off.

   Some of the odder combinations arise from ACLs with sub-ACLs, for which 
   the overall access permission is the union of the permissions in all the
   sub-ACLs.  For example if one sub-ACL has xxx_RWx and another has xWD_xxx,
   the parent ACL will have xWD_RWx.  Finally, there are a small number of 
   special-case permissions in which internal access differs from external 
   access.  This is used for attributes that are used for control purposes 
   (e.g. identifier information in cert requests) and can be set internally 
   but are read-only externally.

			  Internal low ----++---- External high
			  Internal high --+||+--- External low */
#define ACCESS_xxx_xxx		0x0000	/* No access */
#define ACCESS_xxx_xWx		0x0202	/* Low: Write-only */
#define ACCESS_xxx_xWD		0x0303	/* Low: Write/delete */
#define ACCESS_xxx_Rxx		0x0404	/* Low: Read-only */
#define ACCESS_xxx_RWx		0x0606	/* Low: Read/write */
#define ACCESS_xxx_RWD		0x0707	/* Low: All access */
#define ACCESS_xWx_xWx		0x2222	/* High: Write-only, Low: Write-only */
#define ACCESS_xWD_xWD		0x3333	/* High: Write/delete, Low: Write/delete */
#define ACCESS_xWx_xxx		0x2020	/* High: Write-only, Low: None */
#define ACCESS_Rxx_xxx		0x4040	/* High: Read-only, Low: None */
#define ACCESS_Rxx_xWx		0x4242	/* High: Read-only, Low: Write-only */
#define ACCESS_Rxx_Rxx		0x4444	/* High: Read-only, Low: Read-only */
#define ACCESS_Rxx_RxD		0x4545	/* High: Read-only, Low: Read/delete */
#define ACCESS_Rxx_RWx		0x4646	/* High: Read-only, Low: Read/write */
#define ACCESS_Rxx_RWD		0x4747	/* High: Read-only, Low: All access */
#define ACCESS_RxD_RxD		0x5555	/* High: Read/delete, Low: Read/delete */
#define ACCESS_RWx_xxx		0x6060	/* High: Read/write, Low: None */
#define ACCESS_RWx_xWx		0x6262	/* High: Read/write, Low: Write-only */
#define ACCESS_RWx_Rxx		0x6464	/* High: Read/write, Low: Read-only */
#define ACCESS_RWx_RWx		0x6666	/* High: Read/write, Low: Read/write */
#define ACCESS_RWx_RWD		0x6767	/* High: Read/write, Low: All access */
#define ACCESS_RWD_xxx		0x7070	/* High: All access, Low: None */
#define ACCESS_RWD_xWD		0x7373	/* High: All access, Low: Write/delete */
#define ACCESS_RWD_RWD		0x7777	/* High: All access, Low: All access */

#define ACCESS_INT_xxx_xxx	0x0200	/* Internal: No access */
#define ACCESS_INT_xxx_xWx	0x0200	/* Internal: None, write-only */
#define ACCESS_INT_xxx_Rxx	0x0400	/* Internal: None, read-only */
#define ACCESS_INT_xWx_xxx	0x2000	/* Internal: Write-only, none */
#define ACCESS_INT_xWx_xWx	0x2200	/* Internal: Write-only, write-only */
#define ACCESS_INT_Rxx_xxx	0x4000	/* Internal: Read-only, none */
#define ACCESS_INT_Rxx_xWx	0x4200	/* Internal: Read-only, write-only */
#define ACCESS_INT_Rxx_Rxx	0x4400	/* Internal: Read-only, read-only */
#define ACCESS_INT_Rxx_RWx	0x4600	/* Internal: Read-only, read/write */
#define ACCESS_INT_RWx_xxx	0x6000	/* Internal: Read/write, none */
#define ACCESS_INT_RWx_RWx	0x6600	/* Internal: Read/write, read/write */

#define ACCESS_SPECIAL_Rxx_RWx_Rxx_Rxx \
							0x4644	/* Internal = Read-only, read/write, 
									   External = Read-only, read-only */

#define ACCESS_FLAG_R		0x0004	/* Read access permitted */
#define ACCESS_FLAG_W		0x0002	/* Write access permitted */
#define ACCESS_FLAG_D		0x0001	/* Delete access permitted */
#define ACCESS_FLAG_H_R		0x0040	/* Read access permitted in high mode */
#define ACCESS_FLAG_H_W		0x0020	/* Write access permitted in high mode */
#define ACCESS_FLAG_H_D		0x0010	/* Delete access permitted in high mode */

#define ACCESS_MASK_EXTERNAL 0x0077	/* External-access flags mask */
#define ACCESS_MASK_INTERNAL 0x7700	/* Internal-access flags mask */

#define MK_ACCESS_INTERNAL( value )	( ( value ) << 8 )

/* The basic RWD access flags are also used for checking some parameters
   passed with keyset mechanism messages, in addition to these we have flags
   for getFirst/getNext functions that are only used with keysets.  Note
   that although these partially overlap with the high-mode access flags for
   attributes this isn't a problem since keysets don't distinguish between
   high and low states.  In addition some of the combinations may seem a bit
   odd, but that's because they're for mechanism parameters such as key ID
   information which is needed for reads and deletes but not writes, since
   it's implicitly included with the key which is being written.  Finally,
   one type of mechanism has parameter semantics that are too complex to
   express via a simple ACL entry, these are given a different-looking ACL
   entry xxXXxx to indicate to readers that this isn't the same as a normal
   entry with the same value.  In addition to this, the semantics of some
   of the getFirst/Next accesses are complex enough that we need to hardcode
   them into the ACL checking, leaving only a representative entry on the ACL
   definition itself (see cryptack for more details) */

#define ACCESS_KEYSET_xxxxx	0x0000	/* No access */
#define ACCESS_KEYSET_xxXXx	0x0006	/* Special-case values (params optional) */
#define ACCESS_KEYSET_xxRxD	0x0005	/* Read and delete */
#define ACCESS_KEYSET_xxRWx	0x0006	/* Read/write */
#define ACCESS_KEYSET_xxRWD	0x0007	/* Read/write and delete */
#define ACCESS_KEYSET_FxRxD	0x0015	/* GetFirst, read, and delete */
#define ACCESS_KEYSET_FNxxx	0x0018	/* GetFirst/Next */
#define ACCESS_KEYSET_FNRWD	0x001F	/* All access */

#define ACCESS_FLAG_F		0x0010	/* GetFirst access permitted */
#define ACCESS_FLAG_N		0x0008	/* GetNext access permitted */

/****************************************************************************
*																			*
*								Routing Information							*
*																			*
****************************************************************************/

/* Routing types, which specify the routing used for the message.  This 
   routing applies not only for attribute manipulation messages but for all 
   messages in general, some of the routing types defined below only apply 
   for non-attribute messages.  The routing types are:

	ROUTE_NONE
		Not routed (the message or attribute is valid for any object type).

	ROUTE( target )
	ROUTE_ALT( target, altTarget )
	ROUTE_ALT2( target, altTarget1, altTarget2 )
		Fixed-target messages always routed to a particular object type or
		set of types (e.g. a certificate attribute is always routed to a 
		certificate object; a generate key message is always routed to a 
		context).

	ROUTE_FIXED( target )
	ROUTE_FIXED_ALT( target, altTarget )
		Not routed, but checked to make sure they're addressed to the
		required target type.  These message types aren't routed because
		they're specific to a particular object and are explicitly unroutable
		(for example a get key message sent to a cert or context tied to a
		device shouldn't be forwarded on to the device, since it would result
		in the cert acting as a keyset.  This is theoretically justifiable -
		"Get me another cert from the same place this one came from" - but
		it's stretching the orthogonality of objects a bit far).

	ROUTE_IMPLICIT
		For object attribute manipulation messages, implicitly routed by
		attribute type.

	ROUTE_SPECIAL( routingFunction )
		Special-case, message-dependent routing */

#define ROUTE_NONE \
		OBJECT_TYPE_NONE, NULL
#define ROUTE( target ) \
		( target ), findTargetType
#define ROUTE_ALT( target, altTarget ) \
		( target ) | ( ( altTarget ) << 8 ), findTargetType
#define ROUTE_ALT2( target, altTarget1, altTarget2 ) \
		( target ) | ( ( altTarget1 ) << 8 ) | ( ( altTarget2 ) << 16 ), findTargetType
#define ROUTE_FIXED( target ) \
		( target ), checkTargetType
#define ROUTE_FIXED_ALT( target, altTarget ) \
		( target ) | ( ( altTarget ) << 8 ), checkTargetType
#define ROUTE_IMPLICIT \
		OBJECT_TYPE_LAST, findTargetType
#define ROUTE_SPECIAL( function ) \
		OBJECT_TYPE_NONE, ( function )

/* Macros to determine which type of routing to apply */

#define isImplicitRouting( target ) \
		( ( target ) == OBJECT_TYPE_LAST )
#define isExplicitRouting( target ) \
		( ( target ) == OBJECT_TYPE_NONE )

/* Prototypes for routing functions used with the above definitions */

static int findTargetType( const int objectHandle, const int arg );
static int checkTargetType( const int objectHandle, const int targets );

/****************************************************************************
*																			*
*								Value Range Information						*
*																			*
****************************************************************************/

/* The value range (for numeric or boolean values) or length range (for 
   variable-length data).  Some values aren't amenable to a simple range 
   check so we also allow various extended types of checking.  To denote that 
   an extended check needs to be performed, we set the low range value to 
   RANGE_EXT_MARKER and the high range value to an indicator of the type of 
   check to be performed.  The range types are:

	RANGE_ANY
		Allow any value
	RANGE_ALLOWEDVALUES
		extendedInfo contains int [] of allowed values, terminated by
		CRYPT_ERROR
	RANGE_SUBRANGES
		extendedInfo contains subrange [] of allowed subranges, terminated
		by { CRYPT_ERROR, CRYPT_ERROR }
	RANGE_SUBTYPED
		extendedInfo contains sub-acl [] of object-type-specific sub-ACLs.
		Most checking is done by the main ACL, the sub-ACL is then
		recursively applied.
	RANGE_SELECTVALUE
		Special-case value that would normally be a RANGE_SUBTYPED value but
		is used frequently enough that it's handled specially.  On write this
		value is CRYPT_UNUSED to select this attribute, on read it's a 
		presence check and returns TRUE or FALSE.  This is used for composite
		attributes with sub-components, e.g. DNs */

typedef enum {
	RANGEVAL_NONE,					/* No range type */
	RANGEVAL_ANY,					/* Any value allowed */
	RANGEVAL_ALLOWEDVALUES,			/* List of permissible values */
	RANGEVAL_SUBRANGES,				/* List of permissible subranges */
	RANGEVAL_SUBTYPED,				/* Object-subtype-specific sub-ACL */
	RANGEVAL_SELECTVALUE,			/* Write = select attr, read = pres.chk */
	RANGEVAL_LAST					/* Last valid range type */
	} RANGEVAL_TYPE;

#define RANGE_EXT_MARKER	-1000	/* Marker to denote extended range value */

#define RANGE_ANY			RANGE_EXT_MARKER, RANGEVAL_ANY
#define RANGE_ALLOWEDVALUES	RANGE_EXT_MARKER, RANGEVAL_ALLOWEDVALUES
#define RANGE_SUBRANGES		RANGE_EXT_MARKER, RANGEVAL_SUBRANGES
#define RANGE_SUBTYPED		RANGE_EXT_MARKER, RANGEVAL_SUBTYPED
#define RANGE_SELECTVALUE	RANGE_EXT_MARKER, RANGEVAL_SELECTVALUE
#define RANGE( low, high )	( low ), ( high )

/* The maximum possible integer value, used to indicate that any value is
   allowed (e.g. when returning device-specific error codes).  Note that this
   differs from MAX_INTLENGTH from crypt.h, which defines the maximum data
   length value that can be safely specified by a signed integer */

#define RANGE_MAX			( INT_MAX - 128 )

/* Data structures to contain special-case range information */

typedef struct { int lowRange, highRange; } RANGE_SUBRANGE_TYPE;

/* Macro to check whether it's an extended range and to extract the special
   range type */

#define isSpecialRange( attributeACL ) \
		( ( attributeACL )->lowRange == RANGE_EXT_MARKER )
#define getSpecialRangeType( attributeACL )	( ( attributeACL )->highRange )
#define getSpecialRangeInfo( attributeACL )	( ( attributeACL )->extendedInfo )

/****************************************************************************
*																			*
*								ACL Entry Definitions						*
*																			*
****************************************************************************/

/* Attribute ACL entry.  If the code is compiled in debug mode, we also add
   the attribute type, which is used for an internal consistency check */

typedef struct {
#ifndef NDEBUG
	/* The attribute type, used for consistency checking */
	const CRYPT_ATTRIBUTE_TYPE attribute;/* Attribute */
#endif /* NDEBUG */

	/* Attribute type checking information: The attribute value type and
	   object subtypes for which the attribute is valid */
	const ATTRIBUTE_VALUE_TYPE valueType;/* Attribute value type */
	const OBJECT_SUBTYPE subTypeA, subTypeB;
									/* Object subtypes for which attr.valid */

	/* Access information: The type of access and object states that are
	   permitted, and attribute flags for this attribute */
	const int access;				/* Permitted access type */
	const int flags;				/* Attribute flags */

	/* Routing information: The object type the attribute applies to, and the
	   routing function applied to the attribute message */
	const OBJECT_TYPE routingTarget;	/* Target type if routable */
	int ( *routingFunction )( const int objectHandle, const int arg );

	/* Attribute value checking information */
	const int lowRange;				/* Min/max allowed if numeric/boolean, */
	const int highRange;			/*	length if string */
	const void *extendedInfo;		/* Extended access information */
	} ATTRIBUTE_ACL;

/* Key management ACL entry.  If the code is compiled in debug mode, we also
   add the item type, which is used for an internal consistency check */

typedef struct {
#ifndef NDEBUG
	/* The item type, used for consistency checking */
	const KEYMGMT_ITEM_TYPE itemType;/* Key management item type */
#endif /* NDEBUG */

	/* Valid keyset types and access types for this item type.  This is a
	   matrix giving keyset types for which read/write/delete (R/W/D),
	   getFirst/Next (FN), and query (Q) access are valid */
	const OBJECT_SUBTYPE keysetR_subTypeA, keysetR_subTypeB;
	const OBJECT_SUBTYPE keysetW_subTypeA, keysetW_subTypeB;
	const OBJECT_SUBTYPE keysetD_subTypeA, keysetD_subTypeB;
	const OBJECT_SUBTYPE keysetFN_subTypeA, keysetFN_subTypeB;
	const OBJECT_SUBTYPE keysetQ_subTypeA, keysetQ_subTypeB;

	/* Permitted object types and key management flags for this item type */
	const OBJECT_SUBTYPE objSubTypeA, objSubTypeB;
									/* Permitted object types for item */
	const int allowedFlags;			/* Permitted key management flags */

	/* Parameter flags for the mechanism information.  These define which
	   types of optional/mandatory parameters can and can't be present.
	   These use an extended form of the ACCESS_xxx flags to indicate
	   whether the parameter is required or not/permitted or not for
	   read, write, and delete messages */
	const int idUseFlags;			/* ID required/not permitted */
	const int pwUseFlags;			/* Password required/not permitted */

	/* In the case of public/private keys the general-purpose ACL entries
	   aren't quite specific enough since some keysets require specific
	   types of certificates while others require generic public-key objects.
	   In the latter case they can have almost any kind of certificate object
	   attached, which doesn't matter when we're interested only in the
	   public key but does matter if we want a specific type of cert.  If we
	   want a specific cert type, we specify the subset of keysets that this
	   applies to and the cert type(s) here */
	const OBJECT_SUBTYPE specificKeysetSubTypeA, specificKeysetSubTypeB;
	const OBJECT_SUBTYPE specificObjSubTypeA, specificObjSubTypeB;
	} KEYMGMT_ACL;

/* Object ACL entry for object parameters */

typedef struct {
	const OBJECT_SUBTYPE subTypeA, subTypeB;
									/* Object subtypes for which attr.valid */
	const int flags;				/* ACL flags */
	} OBJECT_ACL;

/* Mechanism ACL parameter entry, which defines the type and valid values for 
   that mechanism.  A list of these constitutes a mechanism ACL */

typedef struct {
	const MECHPARAM_VALUE_TYPE valueType;	/* Parameter value type */
	const int lowRange, highRange;	/* Min/max value or length */
	const int flags;				/* ACL flags */
	const OBJECT_SUBTYPE subTypeA, subTypeB;
									/* Object subtypes for which param.valid */
	} MECHANISM_PARAM_ACL;

typedef struct {
	const MECHANISM_TYPE type;		/* Mechanism type */
	const MECHANISM_PARAM_ACL paramACL[ 5 ];
									/* Parameter ACL information */
	} MECHANISM_ACL;

/****************************************************************************
*																			*
*							ACL Initialisation Macros						*
*																			*
****************************************************************************/

/* Macros to make it easy to set up ACL's.  We have one for each of the
   basic types and two general-purpose ones that provide more control over
   the values */

#ifndef NDEBUG
  /* Standard ACL entries */
  #define MKACL_B( attribute, subTypeA, subTypeB, access, routing ) \
			{ attribute, ATTRIBUTE_VALUE_BOOLEAN, subTypeA, subTypeB, access, \
			  0, routing, FALSE, TRUE, NULL }
  #define MKACL_N( attribute, subTypeA, subTypeB, access, routing, range ) \
			{ attribute, ATTRIBUTE_VALUE_NUMERIC, subTypeA, subTypeB, access, \
			  0, routing, range, NULL }
  #define MKACL_S( attribute, subTypeA, subTypeB, access, routing, range ) \
			{ attribute, ATTRIBUTE_VALUE_STRING, subTypeA, subTypeB, access, \
			  0, routing, range, NULL }
  #define MKACL_WCS( attribute, subTypeA, subTypeB, access, routing, range ) \
			{ attribute, ATTRIBUTE_VALUE_WCSTRING, subTypeA, subTypeB, access, \
			  0, routing, range, NULL }
  #define MKACL_O( attribute, subTypeA, subTypeB, access, routing, type ) \
			{ attribute, ATTRIBUTE_VALUE_OBJECT, subTypeA, subTypeB, access, \
			  0, routing, 0, 0, type }
  #define MKACL_T( attribute, subTypeA, subTypeB, access, routing ) \
			{ attribute, ATTRIBUTE_VALUE_TIME, subTypeA, subTypeB, access, \
			  0, routing, 0, 0, NULL }
  #define MKACL_X( attribute, subTypeA, subTypeB, access, routing, subACL ) \
			{ attribute, ATTRIBUTE_VALUE_SPECIAL, subTypeA, subTypeB, access, \
			  0, routing, RANGE_SUBTYPED, subACL }

  /* Extended types */
  #define MKACL_B_EX( attribute, subTypeA, subTypeB, access, flags, routing ) \
			{ attribute, ATTRIBUTE_VALUE_BOOLEAN, subTypeA, subTypeB, access, \
			  flags, routing, FALSE, TRUE, NULL }
  #define MKACL_N_EX( attribute, subTypeA, subTypeB, access, flags, routing, range ) \
			{ attribute, ATTRIBUTE_VALUE_NUMERIC, subTypeA, subTypeB, access, \
			  flags, routing, range, NULL }
  #define MKACL_S_EX( attribute, subTypeA, subTypeB, access, flags, routing, range ) \
			{ attribute, ATTRIBUTE_VALUE_STRING, subTypeA, subTypeB, access, \
			  flags, routing, range, NULL }
  #define MKACL_O_EX( attribute, subTypeA, subTypeB, access, flags, routing, type ) \
			{ attribute, ATTRIBUTE_VALUE_OBJECT, subTypeA, subTypeB, access, \
			  flags, routing, 0, 0, type }

  /* General-purpose ACL macros */
  #define MKACL( attribute, valueType, subTypeA, subTypeB, access, flags, routing, range ) \
			{ attribute, valueType, subTypeA, subTypeB, access, flags, \
			  routing, range, NULL }
  #define MKACL_EX( attribute, valueType, subTypeA, subTypeB, access, flags, routing, range, allowed ) \
			{ attribute, valueType, subTypeA, subTypeB, access, flags, \
			  routing, range, allowed }

  /* End-of-ACL canary.  Note that the comma is necessary in order to allow
     the non-debug version to evaluate to nothing */
  #define MKACL_END() \
			, { CRYPT_ERROR, ATTRIBUTE_VALUE_NONE, 0, 0, ACCESS_xxx_xxx, \
				0, 0, NULL, 0, 0, NULL }

  /* End-of-ACL marker, used to terminate variable-length sub-ACL lists.  The
     ST_ANY match ensures it matches any object types */
  #define MKACL_END_SUBACL() \
			{ CRYPT_ERROR, ATTRIBUTE_VALUE_NONE, ST_ANY, ST_ANY, ACCESS_xxx_xxx, \
			  0, 0, NULL, 0, 0, NULL }
#else
  /* Standard ACL entries */
  #define MKACL_B( attribute, subTypeA, subTypeB, access, routing ) \
			{ ATTRIBUTE_VALUE_BOOLEAN, subTypeA, subTypeB, access, 0, \
			  routing, FALSE, TRUE, NULL }
  #define MKACL_N( attribute, subTypeA, subTypeB, access, routing, range ) \
			{ ATTRIBUTE_VALUE_NUMERIC, subTypeA, subTypeB, access, 0, \
			  routing, range, NULL }
  #define MKACL_S( attribute, subTypeA, subTypeB, access, routing, range ) \
			{ ATTRIBUTE_VALUE_STRING, subTypeA, subTypeB, access, 0, \
			  routing, range, NULL }
  #define MKACL_WCS( attribute, subTypeA, subTypeB, access, routing, range ) \
			{ ATTRIBUTE_VALUE_WCSTRING, subTypeA, subTypeB, access, 0, \
			  routing, range, NULL }
  #define MKACL_O( attribute, subTypeA, subTypeB, access, routing, type ) \
			{ ATTRIBUTE_VALUE_OBJECT, subTypeA, subTypeB, access, 0, \
			  routing, 0, 0, type }
  #define MKACL_T( attribute, subTypeA, subTypeB, access, routing ) \
			{ ATTRIBUTE_VALUE_TIME, subTypeA, subTypeB, access, 0, \
			  routing, 0, 0, NULL }
  #define MKACL_X( attribute, subTypeA, subTypeB, access, routing, subACL ) \
			{ ATTRIBUTE_VALUE_SPECIAL, subTypeA, subTypeB, access, 0, \
			  routing, RANGE_SUBTYPED, subACL }

  /* Extended types */
  #define MKACL_B_EX( attribute, subTypeA, subTypeB, access, flags, routing ) \
			{ ATTRIBUTE_VALUE_BOOLEAN, subTypeA, subTypeB, access, flags, \
			  routing, FALSE, TRUE, NULL }
  #define MKACL_N_EX( attribute, subTypeA, subTypeB, access, flags, routing, range ) \
			{ ATTRIBUTE_VALUE_NUMERIC, subTypeA, subTypeB, access, flags, \
			  routing, range, NULL }
  #define MKACL_S_EX( attribute, subTypeA, subTypeB, access, flags, routing, range ) \
			{ ATTRIBUTE_VALUE_STRING, subTypeA, subTypeB, access, flags, \
			  routing, range, NULL }
  #define MKACL_O_EX( attribute, subTypeA, subTypeB, access, flags, routing, type ) \
			{ ATTRIBUTE_VALUE_OBJECT, subTypeA, subTypeB, access, flags, \
			  routing, 0, 0, type }

  /* General-purpose ACL macros */
  #define MKACL( attribute, valueType, subTypeA, subTypeB, access, flags, routing, range ) \
			{ valueType, subTypeA, subTypeB, access, flags, routing, range, NULL }
  #define MKACL_EX( attribute, valueType, subTypeA, subTypeB, access, flags, routing, range, allowed ) \
			{ valueType, subTypeA, subTypeB, access, flags, routing, range, allowed }

  /* End-of-ACL canary.  Note that the comma is necessary in order to allow
     the non-debug version to evaluate to nothing */
  #define MKACL_END()

  /* End-of-ACL marker, used to terminate variable-length sub-ACL lists.  The
     ST_ANY match ensures it matches any object types */
  #define MKACL_END_SUBACL() \
			{ ATTRIBUTE_VALUE_NONE, ST_ANY, ST_ANY, ACCESS_xxx_xxx, \
			  0, 0, NULL, 0, 0, NULL }
#endif /* NDEBUG */

/* Mechanism ACLs */

#define MKACM_B() \
			{ MECHPARAM_VALUE_BOOLEAN, 0, 0, 0, 0, 0 }
#define MKACM_N( min, max ) \
			{ MECHPARAM_VALUE_NUMERIC, min, max, 0, 0, 0 }
#define MKACM_S( minLen, maxLen ) \
			{ MECHPARAM_VALUE_STRING, minLen, maxLen, 0, 0, 0 }
#define MKACM_S_OPT( minLen, maxLen ) \
			{ MECHPARAM_VALUE_STRING_OPT, minLen, maxLen, 0, 0, 0 }
#define MKACM_S_NONE() \
			{ MECHPARAM_VALUE_STRING_NONE, 0, 0, 0, 0, 0 }
#define MKACM_O( subTypeA, flags ) \
			{ MECHPARAM_VALUE_OBJECT, 0, 0, flags, subTypeA, ST_NONE }
#define MKACM_UNUSED() \
			{ MECHPARAM_VALUE_UNUSED, 0, 0, 0, 0, 0 }

/* End-of-mechanism-ACL marker */

#define MKACM_END() \
			{ MECHPARAM_VALUE_NONE, 0, 0, 0, 0 }

/* Key management ACLs.  The basic form treats the RWD and FnQ groups as one
   value, the _RWD form specifies individual RWD and FnQ values, and the _EX
   form adds special-case checking for specific object types that must be
   written to some keyset types */

#ifndef NDEBUG
  #define MK_KEYACL( itemType, keysetRWDSubType, keysetFNQSubType, \
					 objectSubType, flags, idUseFlags, pwUseFlags ) \
			{ itemType, keysetRWDSubType, ST_NONE, keysetRWDSubType, ST_NONE, \
			  keysetRWDSubType, ST_NONE, keysetFNQSubType, ST_NONE, \
			  keysetFNQSubType, ST_NONE, objectSubType, ST_NONE, \
			  flags, idUseFlags, pwUseFlags, ST_NONE, ST_NONE }
  #define MK_KEYACL_RWD( itemType, keysetR_SubType, keysetW_SubType, keysetD_SubType, \
  					keysetFN_SubType, keysetQ_SubType, objectSubType, flags, \
  					idUseFlags, pwUseFlags ) \
			{ itemType, keysetR_SubType, ST_NONE, keysetW_SubType, ST_NONE, \
			  keysetD_SubType, ST_NONE, keysetFN_SubType, ST_NONE, \
			  keysetQ_SubType, ST_NONE, objectSubType, ST_NONE, \
			  flags, idUseFlags, pwUseFlags, ST_NONE, ST_NONE }
  #define MK_KEYACL_EX( itemType, keysetR_SubType, keysetW_SubType, keysetD_SubType, \
  					keysetFN_SubType, keysetQ_SubType, objectSubType, flags, \
  					idUseFlags, pwUseFlags, specificKeysetType, specificObjectType ) \
			{ itemType, keysetR_SubType, ST_NONE, keysetW_SubType, ST_NONE, \
			  keysetD_SubType, ST_NONE, keysetFN_SubType, ST_NONE, \
			  keysetQ_SubType, ST_NONE, objectSubType, ST_NONE, \
			  flags, idUseFlags, pwUseFlags, specificKeysetType, ST_NONE, \
			  specificObjectType, ST_NONE }
#else
  #define MK_KEYACL( itemType, keysetRWDSubType, keysetFNQSubType, \
					 objectSubType, flags, idUseFlags, pwUseFlags ) \
			{ keysetRWDSubType, ST_NONE, keysetRWDSubType, ST_NONE, \
			  keysetRWDSubType, ST_NONE, keysetFNQSubType, ST_NONE, \
			  keysetFNQSubType, ST_NONE, objectSubType, ST_NONE, \
			  flags, idUseFlags, pwUseFlags, ST_NONE, ST_NONE }
  #define MK_KEYACL_RWD( itemType, keysetR_SubType, keysetW_SubType, keysetD_SubType, \
  					keysetFN_SubType, keysetQ_SubType, objectSubType, flags, \
  					idUseFlags, pwUseFlags ) \
			{ keysetR_SubType, ST_NONE, keysetW_SubType, ST_NONE, \
			  keysetD_SubType, ST_NONE, keysetFN_SubType, ST_NONE, \
			  keysetQ_SubType, ST_NONE, objectSubType, ST_NONE, \
			  flags, idUseFlags, pwUseFlags, ST_NONE, ST_NONE }
  #define MK_KEYACL_EX( itemType, keysetR_SubType, keysetW_SubType, keysetD_SubType, \
  					keysetFN_SubType, keysetQ_SubType, objectSubType, flags, \
  					idUseFlags, pwUseFlags, specificKeysetType, specificObjectType ) \
			{ keysetR_SubType, ST_NONE, keysetW_SubType, ST_NONE, \
			  keysetD_SubType, ST_NONE, keysetFN_SubType, ST_NONE, \
			  keysetQ_SubType, ST_NONE, objectSubType, ST_NONE, \
			  flags, idUseFlags, pwUseFlags, specificKeysetType, ST_NONE, \
			  specificObjectType, ST_NONE }
#endif /* NDEBUG */

/****************************************************************************
*																			*
*							Miscellaneous Information						*
*																			*
****************************************************************************/

/* Flags for attribute ACLs:

	FLAG_OBJECTPROPERTY
		This is an object property attribute which is handled by the kernel
		rather than being forwarded to the object.

	FLAG_TRIGGER
		Successfully setting this attribute triggers a change from the low to
		the high state */

#define ATTRIBUTE_FLAG_NONE		0x00
#define ATTRIBUTE_FLAG_PROPERTY	0x01
#define ATTRIBUTE_FLAG_TRIGGER	0x02
#define ATTRIBUTE_FLAG_LAST		0x04

/* Miscellaneous ACL flags:

	FLAG_LOW_STATE
	FLAG_HIGH_STATE
	FLAG_ANY_STATE
		Whether the object should be in a particular state.

	FLAG_ROUTE_TO_CTX
	FLAG_ROUTE_TO_CERT
		Whether routing should be applied to an object to locate an 
		underlying object (e.g. a PKC object for a certificate or a 
		certificate for a PKC object).  The need to apply routing is 
		unfortunate but is required in order to apply the subtype check to 
		PKC/cert objects, sorting out which (pre-routed) object types are 
		permissible is beyond the scope of the ACL validation routines that 
		would have to take into consideration the intricacies of all manner 
		of certificate objects paired with public and private keys */

#define ACL_FLAG_NONE			0x00
#define ACL_FLAG_LOW_STATE		0x01
#define ACL_FLAG_HIGH_STATE		0x02
#define ACL_FLAG_ANY_STATE		0x03
#define ACL_FLAG_ROUTE_TO_CTX	0x04
#define ACL_FLAG_ROUTE_TO_CERT	0x08

#define ACL_FLAG_STATE_MASK		0x03

/* Macros to check the misc.ACL flags */

#define checkObjectState( flags, objectHandle ) \
		( ( ( flags & ACL_FLAG_LOW_STATE ) && \
			  !isInHighState( objectHandle ) ) || \
		  ( ( flags & ACL_FLAG_HIGH_STATE ) && \
			  isInHighState( objectHandle ) ) )

/* Macro to access the mechanism ACL information for a given parameter in a 
   list of mechanism parameter ACLs, and to get the subtype of an object */

#define paramInfo( mechanismACL, paramNo )	mechanismACL->paramACL[ paramNo ]
#define objectST( objectHandle )			objectTable[ objectHandle ].subType

/* Macros to check each mechanism parameter against an ACL entry */

#define checkMechParamNumeric( paramACL, value ) \
		( ( paramACL.valueType == MECHPARAM_VALUE_BOOLEAN && \
			( value == TRUE || value == FALSE ) ) || \
		  ( paramACL.valueType == MECHPARAM_VALUE_NUMERIC && \
			( value >= paramACL.lowRange && value <= paramACL.highRange ) ) )

#define checkMechParamString( paramACL, data, dataLen ) \
		( ( ( paramACL.valueType == MECHPARAM_VALUE_STRING_NONE || \
			  paramACL.valueType == MECHPARAM_VALUE_STRING_OPT ) && \
			data == NULL && dataLen == 0 ) || \
		  ( ( paramACL.valueType == MECHPARAM_VALUE_STRING || \
			  paramACL.valueType == MECHPARAM_VALUE_STRING_OPT ) && \
			data != NULL && ( dataLen >= paramACL.lowRange && \
							  dataLen <= paramACL.highRange ) ) )

#define checkMechParamObject( paramACL, objectHandle ) \
		( ( paramACL.valueType == MECHPARAM_VALUE_UNUSED && \
			objectHandle == CRYPT_UNUSED ) || \
		  ( paramACL.valueType == MECHPARAM_VALUE_OBJECT && \
			( ( paramACL.subTypeA & objectST( objectHandle ) ) == \
									objectST( objectHandle ) || \
			  ( paramACL.subTypeB & objectST( objectHandle ) ) == \
									objectST( objectHandle ) ) && \
			checkObjectState( paramACL.flags, objectHandle ) ) )

#endif /* _CRYPTACD_DEFINED */
