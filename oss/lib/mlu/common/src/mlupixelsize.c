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

#include <stdio.h>
#ifndef WIN32
#include <strings.h>
#endif
#include <math.h>
#include <ML/ml.h>


/* ------------------------------------------------------------
 * Set Pixel Size
 *
 * The Pixel Size is expressed as #bytes per pixel / #pixels
 *
 * This is to allow non-integral pixel sizes (e.g., 40 bits per pixel
 * would be 10/2 or 10 bytes per 2 pixels)
 *
 * Note that the denominator also specifies the minimum increment of #
 * of pixels in a line to have an integral # of pixels total in that
 * line.
 *
 * the following macros abbreviate this:
 * PixSize( # bytes (per) # pixels )
 */
#define	setFract(v,n,d)	v ## Num = n, v ## Denom = d
#define	pixelSize(n,v)	setFract( bytesPerPixel, n, v )


/* For example:
 *
 * pixelSize( 4, 1 )  is  4 bytes per 1 pixels
 * pixelSize( 5, 2 )  is  5 bytes per 2 pixels
 */

#ifdef DEBUG
#define DBG(x) fprintf(stderr, x);
#else
#define DBG(x)
#endif


/* ----------------------------------------------------mluComputeImagePixelSize
 */
MLstatus MLAPI
mluComputeImagePixelSize( MLpv *params,
			  MLint32 *bytesPerPixelNumRet,
			  MLint32 *bytesPerPixelDenomRet )
{
  MLstatus error = 0;
  MLint32 bytesPerPixelNum = 0;
  MLint32 bytesPerPixelDenom = 0;
  MLint32 packing;
  MLint32 sampling;
  MLpv *pv;

  if ( (pv = mlPvFind( params, ML_IMAGE_PACKING_INT32 )) == NULL ) {
    DBG( "[mluComputeImagePixelSize] ML_IMAGE_PACKING_INT32 is not defined\n");
    return ML_STATUS_INVALID_PARAMETER;
  }
  packing = pv->value.int32;

  if ( (pv = mlPvFind( params, ML_IMAGE_SAMPLING_INT32 )) == NULL ) {
    DBG("[mluComputeImagePixelSize] ML_IMAGE_SAMPLING_INT32 is not defined\n");
    return ML_STATUS_INVALID_PARAMETER;
  }
  sampling = pv->value.int32;

  /* Assume YCrCb for now
   */
  switch ( packing ) {
  default:
    error = 1;
    break;

  case ML_PACKING_8: 
  case ML_PACKING_8_R: 
  case ML_PACKING_8_4123:
  case ML_PACKING_8_3214:
    switch ( sampling ) {
    case ML_SAMPLING_4444:		pixelSize( 4, 1 ); break;
    case ML_SAMPLING_4224:		pixelSize( 6, 2 ); break;
    case ML_SAMPLING_444:		pixelSize( 3, 1 ); break;
    case ML_SAMPLING_422:		pixelSize( 4, 2 ); break;

    case ML_SAMPLING_420_MPEG1:
    case ML_SAMPLING_420_MPEG2:
    case ML_SAMPLING_420_DVC625:        pixelSize( 5, 4 ); break;
    case ML_SAMPLING_411_DVC:	        pixelSize( 3, 2 ); break;

    case ML_SAMPLING_4004:              pixelSize( 2, 1 ); break;
    case ML_SAMPLING_400:		pixelSize( 1, 1 ); break;

    default:			        error = 1; break;
    }
    break;

  case ML_PACKING_10: 
  case ML_PACKING_10_R: 
  case ML_PACKING_10_3214:
    switch ( sampling ) {
    case ML_SAMPLING_422 :		pixelSize( 5, 2 ); break;
    default:			        error = 1; break;
    }
    break;

  case ML_PACKING_S12: 
    switch ( sampling ) {
    case ML_SAMPLING_4444:		pixelSize( 6, 1 ); break;
    case ML_SAMPLING_4224:		pixelSize( 9, 2 ); break;
    case ML_SAMPLING_444:		pixelSize( 9, 2 ); break;
    default:			        error = 1; break;
    }
    break;

  case ML_PACKING_10_10_10_2:
  case ML_PACKING_10_10_10_2_R: 
  case ML_PACKING_10_10_10_2_3214:
    switch ( sampling ) {
    case ML_SAMPLING_444:               pixelSize( 4, 1 ); break;
    case ML_SAMPLING_4444:		pixelSize( 4, 1 ); break;
    case ML_SAMPLING_4224:		pixelSize( 8, 2 ); break;
    default:			        error = 1; break;
    }
    break;

  case ML_PACKING_10_10_10in32L:
    switch ( sampling ) {
    case ML_SAMPLING_4444:		pixelSize( 16, 3 ); break;
    case ML_SAMPLING_4224:		pixelSize(  8, 2 ); break;
    case ML_SAMPLING_444:		pixelSize(  4, 1 ); break;
    case ML_SAMPLING_422:		pixelSize( 16, 6 ); break;
    default:			        error = 1; break;
    }
    break;

  case ML_PACKING_10in16L:
  case ML_PACKING_10in16L_R: 
  case ML_PACKING_10in16L_3214:
  case ML_PACKING_10in16R:
  case ML_PACKING_10in16R_R: 
  case ML_PACKING_10in16R_3214:
  case ML_PACKING_S12in16L:
  case ML_PACKING_S12in16R:
  case ML_PACKING_S13in16L:
  case ML_PACKING_S13in16R:
    switch ( sampling ) {
    case ML_SAMPLING_411_DVC:        	pixelSize( 3, 1 ); break;
    case ML_SAMPLING_4444:		pixelSize( 8, 1 ); break;
    case ML_SAMPLING_4224:		pixelSize( 6, 1 ); break;
    case ML_SAMPLING_422:		pixelSize( 8, 2 ); break;
    case ML_SAMPLING_444:		pixelSize( 6, 1 ); break;
    default:			        error = 1; break;
    }
    break;
  }

  /* punt ...
   */
  if ( error == 1 ) {
    return ML_STATUS_INVALID_ARGUMENT;
  }

  /* all's ok
   */
  *bytesPerPixelNumRet = bytesPerPixelNum;
  *bytesPerPixelDenomRet = bytesPerPixelDenom;

  return ML_STATUS_NO_ERROR;
}


/* --------------------------------------------------------mluGetImagePixelSize
 */
MLstatus MLAPI mluGetImagePixelSize( MLopenid openPathOrPipe, 
				     MLint32 *bytesPerPixelNumRet,
				     MLint32 *bytesPerPixelDenomRet )
{
  MLpv pv[3];
  MLstatus stat;

  pv[0].param = ML_IMAGE_PACKING_INT32;
  pv[1].param = ML_IMAGE_SAMPLING_INT32;
  pv[2].param = ML_END;

  if ( (stat = mlGetControls( openPathOrPipe, pv )) != ML_STATUS_NO_ERROR ) {
#ifdef DEBUG
    fprintf( stderr, "[mluGetImageBufferSize] mlGetControls failed\n" );
#endif

    /* either path/pipe is not open or it doesn't support the required
     * params
     */
    return stat;
  }

  return mluComputeImagePixelSize( pv, bytesPerPixelNumRet,
				   bytesPerPixelDenomRet );
}


