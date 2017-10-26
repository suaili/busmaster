#ifndef _HD10AC_H
#define _HD10AC_H

#define KEYGENALGO_API extern "C" _declspec(dllimport)

typedef enum{
	KGRE_OK,
	KGRE_NOT_OK,
}VKeyGenResultEx;

 KEYGENALGO_API VKeyGenResultEx GenerateKeyEx(
	const unsigned char*  iSeedArray,		  /* Array for the seed [in] */
	unsigned short		    iSeedArraySize,	/* Length of the array for the seed [in] */
	const unsigned int	  iSecurityLevel,	/* Security level [in] */
	const char*			      iVariant,			  /* Name of the active variant [in] */
	unsigned char*		    ioKeyArray,		  /* Array for the key [in, out] */
	unsigned int		      iKeyArraySize,	/* Maximum length of the array for the key [in] */
	unsigned int&		      oSize				    /* Length of the key [out] */
	);

 typedef VKeyGenResultEx (*keyGen)(
	const unsigned char*  iSeedArray,		  /* Array for the seed [in] */
	unsigned short		    iSeedArraySize,	/* Length of the array for the seed [in] */
	const unsigned int	  iSecurityLevel,	/* Security level [in] */
	const char*			      iVariant,			  /* Name of the active variant [in] */
	unsigned char*		    ioKeyArray,		  /* Array for the key [in, out] */
	unsigned int		      iKeyArraySize,	/* Maximum length of the array for the key [in] */
	unsigned int&		      oSize				    /* Length of the key [out] */
	);

#endif