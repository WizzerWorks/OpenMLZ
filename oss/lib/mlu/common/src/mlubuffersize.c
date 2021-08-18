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
#include <string.h>
#include <math.h>
#include <ML/mlu.h>


#ifdef DEBUG
#define DBG(x) fprintf(stderr, x);
#else
#define DBG(x)
#endif


/* -------------------------------------------------------mluGetImageBufferSize
 */
MLstatus MLAPI mluGetImageBufferSize( MLopenid openPathOrPipe,
				      MLint32* bufferSize )
{
  MLpv pv[20];
  MLstatus stat;

  /* First ask the device for the answer...
   */
  memset( pv, 0, sizeof( pv ) );
  pv[0].param = ML_IMAGE_BUFFER_SIZE_INT32;
  pv[1].param = ML_END;
  if ( (mlGetControls( openPathOrPipe, pv ) == ML_STATUS_NO_ERROR) &&
       (pv[0].value.int32 != 0) ) {
    *bufferSize = pv[0].value.int32;
    return ML_STATUS_NO_ERROR;
  }
  
  /* If device didn't know, use some restricted builtin knowledge
   *
   * Check for the compression first
   */
  memset( pv, 0, sizeof( pv ) );
  pv[0].param = ML_IMAGE_COMPRESSION_INT32;
  pv[1].param = ML_END;
  
  if ( mlGetControls( openPathOrPipe, pv ) == ML_STATUS_NO_ERROR ) {
    if ( pv[0].value.int32 != ML_COMPRESSION_UNCOMPRESSED ) {
      return mluComputeImageBufferSize( pv, bufferSize );
    }

  } else {
    /* Device doesn't support ML_IMAGE_COMPRESSION control
     */
    DBG( "[mluGetImageBufferSize] the device does not support "
	 "ML_IMAGE_COMPRESSION control\n" );
    return ML_STATUS_INVALID_PARAMETER;
  }

  pv[1].param = ML_IMAGE_COLORSPACE_INT32;
  pv[2].param = ML_IMAGE_PACKING_INT32;
  pv[3].param = ML_IMAGE_SAMPLING_INT32;
  pv[4].param = ML_IMAGE_WIDTH_INT32;
  pv[5].param = ML_IMAGE_HEIGHT_INT32;
  pv[6].param = ML_IMAGE_HEIGHT_2_INT32;
  pv[7].param = ML_IMAGE_INTERLEAVE_MODE_INT32;
  pv[8].param = ML_IMAGE_SKIP_ROWS_INT32;
  pv[9].param = ML_IMAGE_SKIP_PIXELS_INT32;
  pv[10].param = ML_IMAGE_ROW_BYTES_INT32;
  pv[11].param = ML_END;

  if ( (stat = mlGetControls( openPathOrPipe, pv )) != ML_STATUS_NO_ERROR ) {
    if ( stat == ML_STATUS_INVALID_PARAMETER && pv[6].length == -1 ) {
      /* ML_IMAGE_HEIGHT_2_INT32 is not supported by this device!
       * set it to 0 just to be safe
       */
      pv[6].value.int32 = 0;

    } else {
      DBG( "[mluGetImageBufferSize] mlGetControls failed\n" );
      /* Either path/pipe is not open or it doesn't support the
       * required params
       */
      return ML_STATUS_INVALID_ID;
    }
  }

  return mluComputeImageBufferSize( pv, bufferSize );
}


/* ---------------------------------------------------mluComputeImageBufferSize
 */
MLstatus MLAPI mluComputeImageBufferSize( MLpv* imageParams,
					  MLint32* bufferSize )
{
  MLstatus stat;
  MLint32 packing;
  MLint32 sampling;
  MLint32 height = 0, height_f1 = 0, height_f2 = 0;
  MLint32 width;
  MLint32 bytesPerPixelDenomRet, bytesPerPixelNumRet;
  MLint32 skipRows, skipPixels, rowBytes;
  MLpv *pv;
  MLint32 bytesPerLine;

  /* Check if compression format is specified
   *
   * At this point only DV and uncompressed formats are supported
   */
  if ( (pv = mlPvFind( imageParams, ML_IMAGE_COMPRESSION_INT32 )) != NULL ) {
    switch ( pv->value.int32 ) {
    case ML_COMPRESSION_DV_625:
    case ML_COMPRESSION_DVCPRO_625:
      *bufferSize = 144000;
      return ML_STATUS_NO_ERROR;

    case ML_COMPRESSION_DV_525:
    case ML_COMPRESSION_DVCPRO_525:
      *bufferSize = 120000;
      return ML_STATUS_NO_ERROR;

    case ML_COMPRESSION_DVCPRO50_625:
      *bufferSize = 288000;
      return ML_STATUS_NO_ERROR;

    case ML_COMPRESSION_DVCPRO50_525:
      *bufferSize = 240000;
      return ML_STATUS_NO_ERROR;

    case ML_COMPRESSION_UNCOMPRESSED:
      break;

    default:
      DBG( "[mluComputeImageBufferSize] specified ML_IMAGE_COMPRESSION "
	   "is not currently supported.\n" );
      return ML_STATUS_INVALID_PARAMETER;
    }

  } else {
    DBG( "[mluComputeImageBufferSize] ML_IMAGE_COMPRESSION is not defined\n" );
    return ML_STATUS_INVALID_PARAMETER;
  }

  if ( (pv = mlPvFind( imageParams, ML_IMAGE_PACKING_INT32 )) == NULL ) {
    DBG( "[mluComputeImageBufferSize] ML_IMAGE_PACKING is not defined\n" );
    return ML_STATUS_INVALID_PARAMETER;
  }
  packing = pv->value.int32;

  if ( (pv = mlPvFind( imageParams, ML_IMAGE_SAMPLING_INT32 )) == NULL ) {
    DBG( "[mluComputeImageBufferSize] ML_IMAGE_SAMPLING is not defined\n" );
    return ML_STATUS_INVALID_PARAMETER;
  }
  sampling = pv->value.int32;

  if ( (pv = mlPvFind( imageParams, ML_IMAGE_WIDTH_INT32 )) == NULL ) {
    DBG( "[mluComputeImageBufferSize] ML_IMAGE_WIDTH is not defined\n" );
    return ML_STATUS_INVALID_PARAMETER;
  }
  width = pv->value.int32;

  if ( (pv = mlPvFind( imageParams, ML_IMAGE_HEIGHT_INT32 )) == NULL ) {
    DBG( "[mluComputeImageBufferSize] ML_IMAGE_HEIGHT is not defined\n" );
    return ML_STATUS_INVALID_PARAMETER;
  }
  height_f1 = pv->value.int32;

  if ( (pv = mlPvFind( imageParams, ML_IMAGE_SKIP_PIXELS_INT32 )) == NULL ) {
    DBG( "[mluComputeImageBufferSize] ML_IMAGE_SKIP_PIXELS is not defined\n" );
    return ML_STATUS_INVALID_PARAMETER;
  }
  skipPixels = pv->value.int32;
  
  if ( (pv = mlPvFind( imageParams, ML_IMAGE_SKIP_ROWS_INT32 )) == NULL ) {
    DBG( "[mluComputeImageBufferSize] ML_IMAGE_SKIP_ROWS is not defined\n" );
    return ML_STATUS_INVALID_PARAMETER;
  }
  skipRows = pv->value.int32;

  if ( (pv = mlPvFind( imageParams, ML_IMAGE_ROW_BYTES_INT32 )) == NULL ) {
    DBG( "[mluComputeImageBufferSize] ML_IMAGE_ROW_BYTES is not defined\n" );
    return ML_STATUS_INVALID_PARAMETER;
  }
  rowBytes = pv->value.int32;

  if ( rowBytes == 0 && skipPixels != 0 ) {
    DBG( "[mluComputeImageBufferSize] ML_IMAGE_SKIP_PIXELS cannot be "
	 "defined when ML_IMAGE_ROW_BYTES is equal to 0\n" );
    return ML_STATUS_INVALID_VALUE;
  }

  if ( (pv = mlPvFind( imageParams, ML_IMAGE_HEIGHT_2_INT32 )) != NULL ) {
    height_f2 = pv->value.int32;
  }

  if ( (pv = mlPvFind( imageParams, ML_IMAGE_INTERLEAVE_MODE_INT32) )
       != NULL ) {
    if ( pv->value.int32 == ML_INTERLEAVE_MODE_INTERLEAVED ) {
      height = height_f1 + height_f2;
    } else {
      if ( height_f1 > height_f2 ) {
	height = height_f1;
      } else {
	height = height_f2;
      }
    }
  } else {
    height = height_f1;
  }

  if ( rowBytes > 0 ) {
    *bufferSize = rowBytes * (height + skipRows);
    return ML_STATUS_NO_ERROR;
  }

  /* Get pixel size
   */
  {
    MLpv params[3];

    params[0].param = ML_IMAGE_PACKING_INT32;
    params[0].value.int32 = packing ;
    params[1].param = ML_IMAGE_SAMPLING_INT32;
    params[1].value.int32 = sampling ;
    params[2].param = ML_END;

    stat = mluComputeImagePixelSize( params, &bytesPerPixelNumRet, 
				     &bytesPerPixelDenomRet );
    if ( stat != ML_STATUS_NO_ERROR ) {
#ifdef DEBUG
      fprintf( stderr,
	       "[mluComputeImageBufferSize] cannot determine pixel size: %s\n",
	       mlStatusName( stat ) );
#endif
      return ML_STATUS_INVALID_VALUE;
    }
  }
  bytesPerLine = (MLint32) ceil( width * (double) bytesPerPixelNumRet/
				 (double) bytesPerPixelDenomRet );
  *bufferSize = bytesPerLine * (height + skipRows);

  return ML_STATUS_NO_ERROR;
}


/* -------------------------------------------------------mluGetAudioBufferSize
 *
 * Determine audio buffer size from sample width, channel count, and
 * queue size.
 */
MLstatus MLAPI  mluGetAudioBufferSize( MLint64 openDeviceHandle,
				       int frameCount, int* result )
{
  MLpv pv[4], *pvp = pv;
  MLstatus status = ML_STATUS_NO_ERROR;
  int width = 0;

  /* First ask the device for the answer...
   */
  memset( pv, 0, sizeof( pv ) );

  pvp->param = ML_AUDIO_BUFFER_SIZE_INT32;
  pvp->value.int32 = 0;
  pvp->length = pvp->maxLength = 0;
  pvp++;

  pvp->param = ML_END;

  if ( (mlGetControls( openDeviceHandle, pv ) == ML_STATUS_NO_ERROR ) &&
       (pv[0].value.int32 != 0) ) {
    *result = pv[0].value.int32;
    return ML_STATUS_NO_ERROR;
  }

  pv[0].param = ML_AUDIO_FORMAT_INT32;
  pv[1].param = ML_AUDIO_CHANNELS_INT32;
  pv[2].param = ML_END;
  status = mlGetControls( openDeviceHandle, pv );
  if ( status != ML_STATUS_NO_ERROR ) {
    *result = 0;
    return status;

  } else if ( (pv[0].value.int32 & ML_AUDIO_FORMAT_CLASS_MASK) ==
	      ML_AUDIO_FORMAT_COMPLEX ) {
    /* We cannot currently determine frame size for complex formats
     */
    *result = 0;
    return ML_STATUS_INVALID_VALUE;
  }
  width = ML_GET_AUDIO_FORMAT_STORAGE(pv[0].value.int32);
  *result = (width * pv[1].value.int32 * frameCount) / 8;
  return status;
}
