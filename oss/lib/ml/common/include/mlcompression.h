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

#ifndef MLCOMPRESSION_H
#define MLCOMPRESSION_H

#ifdef __cplusplus
extern "C" {
#endif

#include <ML/mldefs.h>
#include <ML/mlparam.h>

/* ML_IMAGE_COMPRESSION_INT32 or ML_AUDIO_COMPRESSION_INT32: specifies
 * the compression of pixels in the image buffer or of samples in the
 * audio buffer
 */
enum mlCompressionEnum
{
  ML_COMPRESSION_UNCOMPRESSED  = 0,
  ML_COMPRESSION_UNKNOWN       = 1,

  ML_COMPRESSION_MU_LAW        = ML_CLASS_AUDIO | 0x10,
  ML_COMPRESSION_A_LAW         = ML_CLASS_AUDIO | 0x11,
  ML_COMPRESSION_IMA_ADPCM     = ML_CLASS_AUDIO | 0x20,
  ML_COMPRESSION_AC3           = ML_CLASS_AUDIO | 0x30,

  ML_COMPRESSION_BASELINE_JPEG = ML_CLASS_IMAGE | 0x0101,
  ML_COMPRESSION_LOSSLESS_JPEG = ML_CLASS_IMAGE | 0x0102,

  ML_COMPRESSION_DV_625        = ML_CLASS_IMAGE | 0x0201,
  ML_COMPRESSION_DV_525        = ML_CLASS_IMAGE | 0x0202,
  ML_COMPRESSION_DVCPRO_625    = ML_CLASS_IMAGE | 0x0203,
  ML_COMPRESSION_DVCPRO_525    = ML_CLASS_IMAGE | 0x0204,
  ML_COMPRESSION_DVCPRO50_625  = ML_CLASS_IMAGE | 0x0205,
  ML_COMPRESSION_DVCPRO50_525  = ML_CLASS_IMAGE | 0x0206,

  ML_COMPRESSION_SLIM          = ML_CLASS_IMAGE | 0x0300,

  ML_COMPRESSION_MPEG1         = ML_CLASS_MPEG  | 0x1000,
  ML_COMPRESSION_MPEG2         = ML_CLASS_MPEG  | 0x2000,
  ML_COMPRESSION_MPEG2I        = ML_CLASS_MPEG  | 0x3000
};

#ifdef __cplusplus
}
#endif

#endif /* MLCOMPRESSION_H */

