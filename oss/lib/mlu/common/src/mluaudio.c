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
#include <stdlib.h>

#include <ML/ml.h>

#define _MLU_COMPUTE_AUDIO_FRAME_REQUIRED_PARAMS 2


/* ----------------------------------------------------mluComputeAudioFrameSize
 */
MLstatus MLAPI mluComputeAudioFrameSize( MLpv *params, MLint32 *frameSize )
{
  MLint32 format = 0;
  MLint32 valid_params = 0;
  MLint32 channels = 0;
  MLint32 n;

  for ( n = 0; params[n].param != ML_END; n++ )	{
    switch ( params[n].param ) {
    case ML_AUDIO_FORMAT_INT32:
      format = params[n].value.int32;
      valid_params++;
      break;

    case ML_AUDIO_CHANNELS_INT32:
      channels = params[n].value.int32;
      valid_params++;
      break;
    }
  }

  if ( valid_params != _MLU_COMPUTE_AUDIO_FRAME_REQUIRED_PARAMS ) {
    return ML_STATUS_INVALID_PARAMETER;
  }

  /* All is ok. */
  *frameSize = ML_GET_AUDIO_FORMAT_STORAGE(format) / 8 * channels;

  return ML_STATUS_NO_ERROR;
}


/* --------------------------------------------------------mluGetAudioFrameSize
 */
MLstatus MLAPI mluGetAudioFrameSize( MLopenid openPathOrPipe,
				     MLint32 *frameSize )
{
  MLpv pv[3];
  MLstatus stat;

  pv[0].param = ML_AUDIO_FORMAT_INT32;
  pv[1].param = ML_AUDIO_CHANNELS_INT32;
  pv[2].param = ML_END;

  if ( (stat = mlGetControls( openPathOrPipe, pv )) != ML_STATUS_NO_ERROR ) {
#ifdef DEBUG
    fprintf( stderr, "[mluGetAudioFrameSize] mlGetControls failed\n" );
#endif
    /* Either path/pipe is not open or it doesn't support the required
     * params.
     */
    return stat;
  }

  return mluComputeAudioFrameSize( pv, frameSize );
}
