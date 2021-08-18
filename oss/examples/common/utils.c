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
 ***************************************************************************/

/*
	utils.c

	Utilities sample programs
*/

#include "utils.h"

#include <stdio.h>
#include <errno.h>
#include <malloc.h>
#include <string.h>

#ifndef	ML_OS_IRIX
#define	oserror()	errno
#endif


/*-------------------------------------------------------------------event_wait
 */
MLstatus event_wait( MLwaitable pathWaitHandle )
{
#ifdef	ML_OS_NT
  if ( WaitForSingleObject( pathWaitHandle, INFINITE ) != WAIT_OBJECT_0 ) {
    fprintf( stderr, "Error waiting for reply\n" );
    return ML_STATUS_INTERNAL_ERROR;
  }

#else	/* ML_OS_UNIX */
  for ( ;; ) {
    fd_set fdset;
    int rc;

    FD_ZERO( &fdset );
    FD_SET( pathWaitHandle, &fdset );

    rc = select( pathWaitHandle+1, &fdset, NULL, NULL, NULL );
    if ( rc < 0 ) {
      fprintf( stderr, "select: %s\n", strerror( oserror() ) );
      return ML_STATUS_INTERNAL_ERROR;
    }

    if ( rc == 1 ) {
      break;
    }
    /* anything else, loop again */
  }
#endif

  return ML_STATUS_NO_ERROR;
}


/*------------------------------------------------------------------printParams
 */
void printParams( MLint64 pathId, MLpv* msg, FILE* fp )
{
  MLpv *p;
  char buff[256];

  for ( p = msg; p->param != ML_END; ++p ) {
    MLint32 size = sizeof( buff );
    MLstatus stat = mlPvToString( pathId, p, buff, &size );

    if ( stat == ML_STATUS_NO_ERROR ) {
      fprintf( fp, "\t%s", buff );

    } else {
      /* If the value was invalid, at least attempt to print the param
       * name.
       */
      if ( stat == ML_STATUS_INVALID_VALUE ) {
	size = sizeof( buff );
	stat = mlPvParamToString( pathId, p, buff, &size );
      }

      if ( stat == ML_STATUS_NO_ERROR ) {
	fprintf( fp, "\t%s (invalid value)", buff );

      } else if ( (stat == ML_STATUS_INVALID_PARAMETER) &&
		  (ML_PARAM_GET_CLASS(p->param) == ML_CLASS_USER) ) {
	fprintf( fp, "\tUser-defined parameter '0x%" FORMAT_LLX "' = ",
		 p->param );

	if ( ML_PARAM_IS_POINTER( p->param ) ||
	     ML_PARAM_IS_ARRAY( p->param ) ) {
	  fprintf( fp, "%spointer %p",
		   ML_PARAM_IS_ARRAY( p->param ) ? "array " : "",
		   p->value.pByte );

	} else {
	  /* Scalar param */
	  switch( ML_PARAM_GET_TYPE( p->param ) ) {

	  case ML_TYPE_BYTE:
	    fprintf( fp, "'%c' (%d)", p->value.byte, p->value.byte );
	    break;

	  case ML_TYPE_INT32:
	    fprintf( fp, "%d (0x%x)", p->value.int32, p->value.int32 );
	    break;

	  case ML_TYPE_INT64:  
	    fprintf( fp, "%" FORMAT_LLD " (0x%" FORMAT_LLX ")",
		     p->value.int64, p->value.int64 );
	    break;

	  case ML_TYPE_REAL32: 
	    fprintf( fp, "%f", (double) p->value.real32 );
	    break;

	  case ML_TYPE_REAL64: 
	    fprintf( fp, "%f", (double) p->value.real64 );
	    break;

	  default:
	    fprintf( fp, "(value not printable)" );
	  } /* switch */
	}

      } else {
	fprintf( fp, "\t(error '%s' while decoding param '0x%" FORMAT_LLX
		 "')", mlStatusName( stat ), p->param );
      }
    }

    /* If the length is non-standard (ie: not 1), print it too */
    if ( p->length != 1 ) {
      fprintf( fp, " (length %d)", p->length );
    }
    fprintf( fp, "\n" );
  }
}


/*----------------------------------------------------------------printJackName
 */
void printJackName( MLint64 jackId, FILE* fp )
{
  MLpv* jackCaps;
  MLstatus status;

  status = mlGetCapabilities( jackId, &jackCaps );
  if ( status != ML_STATUS_NO_ERROR ) {
    fprintf( stdout, "Can not determine jack name: mlGetCapabilities failed:"
	     " %s\n", mlStatusName( status ) );
  } else {
    MLpv* pv = mlPvFind( jackCaps, ML_NAME_BYTE_ARRAY );
    if ( pv == NULL ) {
      fprintf( stdout, "Can not determine jack name: ML_NAME_BYTE_ARRAY "
	       "not found in caps\n" );

    } else {
      fprintf( stdout, "Using jack: %s\n", pv->value.pByte );
    }
    mlFreeCapabilities( jackCaps );
  }
}


/*-------------------------------------------------------checkPathSupportsParam
 */
int checkPathSupportsParam( MLint64 pathId, MLint64 param,
			    MLint32 desiredAccess )
{
  int supported = 0;
  MLpv* capabilities;
  MLpv* paramIds = 0;
  int i;
  MLstatus status = ML_STATUS_NO_ERROR;

  status = mlGetCapabilities( pathId, &capabilities );
  if ( status != ML_STATUS_NO_ERROR ) {
    fprintf( stderr, "[checkPathSupportsParam] Couldn't get path "
	     "capabilities: %s\n", mlStatusName( status ) );
    return 0;
  }

  paramIds = mlPvFind( capabilities, ML_PARAM_IDS_INT64_ARRAY );
  if ( paramIds == NULL ) {
    fprintf( stderr, "[checkPathSupportsParam] Couldn't find Param "
	     "list in capabilities\n" );
    mlFreeCapabilities( capabilities );
    return 0;
  }

  for ( i=0; i < paramIds->length; ++i ) {
    if ( paramIds->value.pInt64[i] == param ) {
      /* The control is supported */
      supported = 1;
      break;
    }
  } // for i=0..paramIds.length
  mlFreeCapabilities( capabilities );

  /* Test the access mode of the param if requested to do so -- and if
   * the parameter was found.
   */
  if ( (supported != 0) && (desiredAccess != 0) ) {
    MLpv* paramAccess = 0;
    MLint32 perms;

    status = mlPvGetCapabilities( pathId, param, &capabilities );
    if ( status != ML_STATUS_NO_ERROR ) {
      fprintf( stderr, "[checkPathSupportsParam] Couldn't get capabilities "
	       "for param: %s\n", mlStatusName( status ) );
      /* Assume the param does NOT support the access, and return
       * failure
       */
      return 0;
    }

    paramAccess = mlPvFind( capabilities, ML_PARAM_ACCESS_INT32 );
    if ( paramAccess == NULL ) {
      fprintf( stderr, "[checkPathSupportsParam] Couldn't find access "
	       "capabilities for param\n" );
      mlFreeCapabilities( capabilities );
      return 0;
    }

    perms = paramAccess->value.int32;
    mlFreeCapabilities( capabilities );

    /* Now test the access permissions */
    if ( (perms & desiredAccess) != desiredAccess ) {
      supported = 0;
    }
  } /* if supported != 0 && desiredAccess != 0 */

  return supported;
}


/*--------------------------------------------------------------allocateBuffers
 *
 * NOTE: on Windows, there is no "memalign" function. We could write a
 * work-around -- which would involve allocating a larger chunk than
 * requested, and adjusting the pointer to satisfy the alignment
 * request -- but that would complicate matters (freeBuffers would
 * also need to be made more complex, in order to properly de-allocate
 * the block of memory).
 *
 * So for now, we simply use "malloc", and test the alignment of the
 * returned pointer. If it doesn't satisfy the request, we return an
 * error -- we can then see if we really need to code a work-around.
 */
MLint32 allocateBuffers( void** buffers, MLint32 bufferSize,
			 MLint32 maxBuffers, MLint32 memAlignment )
{
  int i;

  /* Should we check the alignment value? In some example programs,
   * the alignment used to be checked and made at least 8. Is this
   * necessary?
   */

  /* Start by zero-ing out all buffer pointers -- useful in case the
   * allocation fails part-way through the list of buffers. That way,
   * freeBuffers can safely be called.
   */
  for ( i = 0; i < maxBuffers; ++i ) {
    buffers[i] = 0;
  }

  for ( i = 0; i < maxBuffers; ++i ) {

#ifdef ML_OS_NT
    // No memalign on NT -- see above
    buffers[i] = malloc( bufferSize );
#else
    buffers[i] = memalign( memAlignment, bufferSize );
#endif
    if ( buffers[i] == NULL ) {
      perror( "Memory allocation failed" );
      return -1;
    }

#ifdef ML_OS_NT
    // Test that the alignment is OK
    if ( ((long) buffers[i] % memAlignment) != 0 ) {
      fprintf( stderr, "Allocated memory at address 0x%x, not aligned to %d\n",
	       buffers[i], memAlignment );
      return -1;
    }
#endif

    /* Here we touch the buffers, forcing the buffer memory to be
     * mapped. This avoids the need to map the buffers the first time
     * they're used.  We could go the extra step and mpin them, but
     * choose not to here, trying a simpler approach first.
     */
    memset( buffers[i], 0xf0f0f0f0, bufferSize );
  }
  return 0;
}


/*------------------------------------------------------------------freeBuffers
 */ 
MLint32 freeBuffers( void** buffers, MLint32 maxBuffers )
{
  int i;

  for ( i = 0; i < maxBuffers; ++i ) {
    if ( buffers[i] != 0 ) {
      free( buffers[i] );
    }
  }

  return 0;
}
