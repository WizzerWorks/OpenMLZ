/***************************************************************************
 * License Applicability. Except to the extent portions of this file are
 * made subject to an alternative license as permitted in the SGI Free 
 * Software License C, Version 1.0 (the "License"), the contents of this 
 * file are subject only to the provisions of the License. You may not use 
 * this file except in compliance with the License. You may obtain a copy 
 * of the License at Silicon Graphics, Inc., attn: Legal Services, 
 * 1500 Crittenden Lane, Mountain View, CA 94043, or at: 
 *   
 * http://oss.sgi.com/projects/FreeC 
 *
 * Note that, as provided in the License, the Software is distributed 
 * on an "AS IS" basis, with ALL EXPRESS AND IMPLIED WARRANTIES AND 
 * CONDITIONS DISCLAIMED, INCLUDING, WITHOUT LIMITATION, ANY IMPLIED 
 * WARRANTIES AND CONDITIONS OF MERCHANTABILITY, SATISFACTORY QUALITY, 
 * FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT. 
 * 
 * Original Code. The Original Code is: OpenML ML Library, 1.1, 12/13/2001,
 * developed by Silicon Graphics, Inc. 
 * ML1.1 is Copyright (c) 2001 Silicon Graphics, Inc. 
 * Copyright in any portions created by third parties is as indicated 
 * elsewhere herein. All Rights Reserved. 
 *
 * Additional Notice Provisions:
 * License Applicability. Except to the extent portions of this file are 
 * made subject to an alternative license as permitted in the Khronos 
 * Free Software License Version 1.0 (the "License"), the contents of 
 * this file are subject only to the provisions of the License. You may 
 * not use this file except in compliance with the License. You may obtain 
 * a copy of the License at 
 * 
 * The Khronos Group Inc.:  PO Box 1019, Clearlake Park CA 95424 USA or at
 *  
 * http://www.Khronos.org/licenses/KhronosOpenSourceLicense1_0.pdf
 * 
 * Note that, as provided in the License, the Software is distributed 
 * on an "AS IS" basis, with ALL EXPRESS AND IMPLIED WARRANTIES AND 
 * CONDITIONS DISCLAIMED, INCLUDING, WITHOUT LIMITATION, ANY IMPLIED 
 * WARRANTIES AND CONDITIONS OF MERCHANTABILITY, SATISFACTORY QUALITY, 
 * FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT. 
 * 
 * Original Code. The Original Code is: OpenML ML Library, 1.1, 12/13/2001,
 * developed by Silicon Graphics, Inc. 
 * ML1.1 is Copyright (c) 2001 Silicon Graphics, Inc. 
 * Copyright in any portions created by third parties is as indicated 
 * elsewhere herein. All Rights Reserved. 
 *
 ***************************************************************************/

#include <ML/mltypes.h>

#ifdef ML_OS_NT
#include <stddef.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>

#define MLGLOBAL_EXPORTS
#include <ML/getopt.h>
char* optarg = NULL;
int optind = 0;

/* getopt for NT
 * implementation based on the text copy example from VC++6.0 documentation
 *
 *      Get next command line option and parameter
 *
 *  PARAMETERS:
 *
 *      argc - count of command line arguments
 *      argv - array of command line argument strings
 *      pszValidOpts - string of valid, case-sensitive option characters,
 *                     a colon ':' following a given character means that
 *                     option can take a parameter
 *      ppszParam - pointer to a pointer to a string for output
 *
 *  RETURNS:
 *
 *      If valid option is found, the character value of that option
 *          is returned, and *ppszParam points to the parameter if given,
 *          or is NULL if no param
 *      If standalone parameter (with no option) is found, -1 is returned,
 *          and *ppszParam points to the standalone parameter
 *      If option is found, but it is not in the list of valid options,
 *          '?' is returned, and *ppszParam points to the invalid argument
 *      When end of argument list is reached, -1 is returned, and
 *          *ppszParam is NULL
 *
 *  COMMENTS:
 *
 */

/* An NT specific variant that passes back data as a parameter because
 * we were having a problem accessing a global
 */
int MLAPI getoptNT( int argc, char* argv[], const char* pszValidOpts,
		    char **ppoptarg )
{
  static int iArg = 1;
  char chOpt;
  char* psz = NULL;
  char* pszParam = NULL;

  if ( iArg < argc ) {
    psz = &(argv[iArg][0]);
    if ( *psz == '-' || *psz == '/' ) {
      /* We have an option specifier
       */
      chOpt = argv[iArg][1];
      if ( isalnum( chOpt ) || ispunct( chOpt ) ) {
	/* We have an option character
	 */
	psz = strchr( pszValidOpts, chOpt );
	if ( psz != NULL ) {
	  /* Option is valid, we want to return chOpt
	   */
	  if ( psz[1] == ':' ) {
	    /* Option can have a parameter
	     */
	    psz = &(argv[iArg][2]);
	    if ( *psz == '\0' ) {
	      /* Must look at next argv for param
	       */
	      if ( iArg+1 < argc ) {
		psz = &(argv[iArg+1][0]);
		if ( *psz == '-' ) {
		  /* Next argv is a new option, so param not given for
		   * current option
		   */

		} else {
		  /* Next argv is the param
		   */
		  iArg++;
		  pszParam = psz;
		}

	      } else {
		/* Reached end of args looking for param
		 */
		fprintf( stderr, "Option requires an argument -- %c\n",
			 chOpt );
		chOpt = '?';
	      }

	    } else {
	      /* Param is attached to option
	       */
	      pszParam = psz;
	    }

	  } else {
	    /* Option is alone, has no parameter
	     */
	    pszParam = &(argv[iArg][0]);
	  }

	} else {
	  /* Option specified is not in list of valid options
	   */
	  fprintf( stderr, "Illegal option -- %c\n", chOpt );
	  chOpt = '?';
	  pszParam = &(argv[iArg][0]);
	}

      } else {
	/* Though option specifier was given, option character is not
	 * alpha or was was not specified
	 */
	chOpt = '?';
	pszParam = &(argv[iArg][0]);
      }

    } else {
      /* Standalone arg given with no option specifier
       */
      chOpt = 1;
      pszParam = &(argv[iArg][0]);
    }

  } else {
    /* End of argument list
     */
    chOpt = -1;
  }

  iArg++;
  *ppoptarg = pszParam;
  return chOpt;
}


/* The original variant that passes back data in a global
 */
int MLAPI getopt( int argc, char* argv[], const char* pszValidOpts )
{
  static int iArg = 1;
  char chOpt;
  char* psz = NULL;
  char* pszParam = NULL;

  /* Assume there are no "independant" args, ie: non-option args, set
   * optind to point beyond last command-line arg.
   */
  optind = argc;

  if ( iArg < argc ) {
    psz = &(argv[iArg][0]);
    if ( *psz == '-' || *psz == '/' ) {
      /* We have an option specifier
       */
      chOpt = argv[iArg][1];
      if ( isalnum( chOpt ) || ispunct( chOpt ) ) {
	/* We have an option character
	 */
	psz = strchr( pszValidOpts, chOpt );
	if ( psz != NULL ) {
	  /* Option is valid, we want to return chOpt
	   */
	  if ( psz[1] == ':' ) {
	    /* Option can have a parameter
	     */
	    psz = &(argv[iArg][2]);
	    if ( *psz == '\0' ) {
	      /* Must look at next argv for param
	       */
	      if ( iArg+1 < argc ) {
		psz = &(argv[iArg+1][0]);
		if ( *psz == '-' ) {
		  /* Next argv is a new option, so param not given for
		   * current option
		   */

		} else {
		  /* Next argv is the param
		   */
		  iArg++;
		  pszParam = psz;
		}

	      } else {
		/* Reached end of args looking for param
		 */
		fprintf( stderr, "Option requires an argument -- %c\n",
			 chOpt );
		chOpt = '?';
	      }

	    } else {
	      /* Param is attached to option
	       */
	      pszParam = psz;
	    }

	  } else {
	    /* Option is alone, has no parameter
	     */
	    pszParam = &(argv[iArg][0]);
	  }

	} else {
	  /* Option specified is not in list of valid options
	   */
	  fprintf( stderr, "Illegal option -- %c\n", chOpt );
	  chOpt = '?';
	  pszParam = &(argv[iArg][0]);
	}

      } else {
	/* Though option specifier was given, option character is not
	 * alpha or was was not specified
	 */
	chOpt = '?';
	pszParam = &(argv[iArg][0]);
      }

    } else {
      /* Standalone arg given with no option specifier. Assume this
       * means we have reached the end of the option list
       */
      chOpt = -1;
      pszParam = &(argv[iArg][0]);
      optind = iArg;
    }

  } else {
    /* End of argument list
     */
    chOpt = -1;
  }

  iArg++;
  optarg = pszParam;
  return chOpt;
}

#endif /* ML_OS_NT */

