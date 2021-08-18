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

#ifndef __INC_MLAUDIO_H__
#define __INC_MLAUDIO_H__  

#ifdef __cplusplus
extern "C" {
#endif

#include <ML/mldefs.h>
#include <ML/mlparam.h>

enum MLpvAudioClassEnum
{
  /* The following parameters are supported by all audio devices
   */
  ML_AUDIO_FORMAT_INT32       = ML_PARAM_NAME( ML_CLASS_AUDIO,
					       ML_TYPE_INT32, 0x1 ),
  ML_AUDIO_CHANNELS_INT32     = ML_PARAM_NAME( ML_CLASS_AUDIO,
					       ML_TYPE_INT32, 0x2 ),
  ML_AUDIO_SAMPLE_RATE_REAL64 = ML_PARAM_NAME( ML_CLASS_AUDIO,
					       ML_TYPE_REAL64, 0x3 ),
  ML_AUDIO_PRECISION_INT32    = ML_PARAM_NAME( ML_CLASS_AUDIO,
					       ML_TYPE_INT32, 0x4 ),
  ML_AUDIO_GAINS_REAL64_ARRAY = ML_PARAM_NAME( ML_CLASS_AUDIO,
					       ML_TYPE_REAL64_ARRAY, 0x5 ),
  ML_AUDIO_COMPRESSION_INT32  = ML_PARAM_NAME( ML_CLASS_AUDIO,
					       ML_TYPE_INT32, 0x6 ),
  ML_AUDIO_FRAME_SIZE_INT32   = ML_PARAM_NAME( ML_CLASS_AUDIO,
					       ML_TYPE_INT32, 0x7 ),

  /* Audio buffer parameter
   */

  /* Buffer pointers are in different include files.  use 'A' to
   * distinguish "Audio" BP.
   */
  ML_AUDIO_BUFFER_POINTER = ML_PARAM_NAME( ML_CLASS_BUFFER,
					   ML_TYPE_BYTE_POINTER, 'A' ),
  ML_AUDIO_BUFFER_SIZE_INT32 = ML_PARAM_NAME( ML_CLASS_AUDIO,
					      ML_TYPE_INT32, 0x10 ),

  /* UST/MSC support
   */
  ML_AUDIO_UST_INT64          = ML_PARAM_NAME( ML_CLASS_AUDIO,
				      ML_TYPE_INT64, 0x101),
  ML_AUDIO_MSC_INT64          = ML_PARAM_NAME( ML_CLASS_AUDIO,
				      ML_TYPE_INT64, 0x102),
  ML_AUDIO_ASC_INT64          = ML_PARAM_NAME( ML_CLASS_AUDIO,
				      ML_TYPE_INT64, 0x103),
  ML_WAIT_FOR_AUDIO_UST_INT64 = ML_PARAM_NAME( ML_CLASS_AUDIO,
					       ML_TYPE_INT64, 0x104),
  ML_WAIT_FOR_AUDIO_MSC_INT64 = ML_PARAM_NAME( ML_CLASS_AUDIO,
					       ML_TYPE_INT64, 0x105),

  /* The following parameters are currently supported only by SGI
   * audio devices
   */
  ML_AUDIO_CLOCK_GEN_INT32        = ML_PARAM_NAME( ML_CLASS_AUDIO,
						   ML_TYPE_INT32, 0x201 ),
  ML_AUDIO_JITTER_INT32           = ML_PARAM_NAME( ML_CLASS_AUDIO,
						   ML_TYPE_INT32, 0x202 ),
  ML_AUDIO_MASTER_CLOCK_INT32     = ML_PARAM_NAME( ML_CLASS_AUDIO,
						   ML_TYPE_INT32, 0x203 ),
  ML_AUDIO_GAIN_REFERENCE_INT32   = ML_PARAM_NAME( ML_CLASS_AUDIO,
						   ML_TYPE_INT32, 0x204 ),
  ML_AUDIO_VIDEO_SYNC_INT32       = ML_PARAM_NAME( ML_CLASS_AUDIO,
						   ML_TYPE_INT32, 0x205 ),
  ML_AUDIO_SUBCODE_FRAMESIZE_INT32 = ML_PARAM_NAME( ML_CLASS_AUDIO,
						    ML_TYPE_INT32, 0x206 ),
  ML_AUDIO_SUBCODE_FORMAT_INT32_ARRAY = ML_PARAM_NAME( ML_CLASS_AUDIO,
						       ML_TYPE_INT32_ARRAY,
						       0x207 ),
  ML_AUDIO_SUBCODE_CHANNELS_INT32 = ML_PARAM_NAME( ML_CLASS_AUDIO,
						   ML_TYPE_INT32, 0x208 ),
  ML_AUDIO_AES_CHANNEL_STATUS_BYTE_ARRAY = ML_PARAM_NAME( ML_CLASS_AUDIO,
							  ML_TYPE_BYTE_ARRAY,
							  0x209 ),
  ML_AUDIO_AES_USER_BYTE_ARRAY    = ML_PARAM_NAME( ML_CLASS_AUDIO,
						   ML_TYPE_BYTE_ARRAY, 0x210 ),
  ML_AUDIO_AES_VALIDITY_BYTE_ARRAY = ML_PARAM_NAME( ML_CLASS_AUDIO,
						    ML_TYPE_BYTE_ARRAY,
						    0x211 ),
  ML_AUDIO_FLOAT_MAX_REAL32       = ML_PARAM_NAME( ML_CLASS_AUDIO,
						   ML_TYPE_REAL32, 0x212 ),
  ML_AUDIO_FP_LIMITING_INT32      = ML_PARAM_NAME( ML_CLASS_AUDIO,
						   ML_TYPE_INT32, 0x213 )
};

enum mlAudioFormatBuildingEnum
{
  ML_AUDIO_FORMAT_SIMPLE	= 0x00000000,
  ML_AUDIO_FORMAT_COMPLEX	= 0x10000000,
  ML_AUDIO_FORMAT_CLASS_MASK	= 0x70000000,

  ML_AUDIO_FORMAT_SHIFT_L	= 0x00010000,
  ML_AUDIO_FORMAT_SHIFT_R	= 0x00020000,
  ML_AUDIO_FORMAT_SHIFT_MASK	= 0x000f0000,

  ML_AUDIO_FORMAT_IN8		= 0x00000800,
  ML_AUDIO_FORMAT_IN16	= 0x00001000,
  ML_AUDIO_FORMAT_IN32	= 0x00002000,
  ML_AUDIO_FORMAT_IN64	= 0x00004000,
  ML_AUDIO_FORMAT_STORAGE_MASK	= 0x0000ff00,

  ML_AUDIO_FORMAT_SIGNED	= 0x00100000,	/* Two's complement */
  ML_AUDIO_FORMAT_UNSIGNED	= 0x00200000,
  ML_AUDIO_FORMAT_REAL	= 0x00400000, /* Can be single- or double-precision */
  ML_AUDIO_FORMAT_TYPE_MASK	= 0x00f00000,

  ML_AUDIO_FORMAT_DETAIL_MASK	= 0x000000ff,

  ML_AUDIO_FORMAT_IN8L	= ML_AUDIO_FORMAT_IN8 | ML_AUDIO_FORMAT_SHIFT_L,
  ML_AUDIO_FORMAT_IN8R	= ML_AUDIO_FORMAT_IN8 | ML_AUDIO_FORMAT_SHIFT_R,
  ML_AUDIO_FORMAT_IN16L	= ML_AUDIO_FORMAT_IN16 | ML_AUDIO_FORMAT_SHIFT_L,
  ML_AUDIO_FORMAT_IN16R	= ML_AUDIO_FORMAT_IN16 | ML_AUDIO_FORMAT_SHIFT_R,
  ML_AUDIO_FORMAT_IN32L	= ML_AUDIO_FORMAT_IN32 | ML_AUDIO_FORMAT_SHIFT_L,
  ML_AUDIO_FORMAT_IN32R	= ML_AUDIO_FORMAT_IN32 | ML_AUDIO_FORMAT_SHIFT_R
};

enum mlAudioFormatEnum
{
  ML_AUDIO_FORMAT_S8      = ML_AUDIO_FORMAT_SIMPLE | ML_AUDIO_FORMAT_SIGNED |
                            ML_AUDIO_FORMAT_IN8 | 8,
  ML_AUDIO_FORMAT_U8      = ML_AUDIO_FORMAT_SIMPLE | ML_AUDIO_FORMAT_UNSIGNED |
                            ML_AUDIO_FORMAT_IN8 | 8,
  ML_AUDIO_FORMAT_S16     = ML_AUDIO_FORMAT_SIMPLE | ML_AUDIO_FORMAT_SIGNED |
                            ML_AUDIO_FORMAT_IN16 | 16,
  ML_AUDIO_FORMAT_U16     = ML_AUDIO_FORMAT_SIMPLE | ML_AUDIO_FORMAT_UNSIGNED |
                            ML_AUDIO_FORMAT_IN16 | 16,
  ML_AUDIO_FORMAT_S24in32R = ML_AUDIO_FORMAT_SIMPLE | ML_AUDIO_FORMAT_SIGNED |
                             ML_AUDIO_FORMAT_IN32R  | 24,
  ML_AUDIO_FORMAT_S20in32L = ML_AUDIO_FORMAT_SIMPLE | ML_AUDIO_FORMAT_SIGNED |
                             ML_AUDIO_FORMAT_IN32L  | 20,
  ML_AUDIO_FORMAT_R32     = ML_AUDIO_FORMAT_SIMPLE | ML_AUDIO_FORMAT_REAL |
                            ML_AUDIO_FORMAT_IN32 | 32,
  ML_AUDIO_FORMAT_R64     = ML_AUDIO_FORMAT_SIMPLE | ML_AUDIO_FORMAT_REAL |
                            ML_AUDIO_FORMAT_IN64 | 64,

  ML_AUDIO_FORMAT_AES_CHANNEL_STATUS = (ML_AUDIO_FORMAT_COMPLEX | 47),
  ML_AUDIO_FORMAT_AES_USER           = (ML_AUDIO_FORMAT_COMPLEX | 48),
  ML_AUDIO_FORMAT_AES_VALIDITY       = (ML_AUDIO_FORMAT_COMPLEX | 49),
  ML_AUDIO_FORMAT_ADAT_USER0         = (ML_AUDIO_FORMAT_COMPLEX | 50),
  ML_AUDIO_FORMAT_ADAT_USER1         = (ML_AUDIO_FORMAT_COMPLEX | 51),
  ML_AUDIO_FORMAT_ADAT_USER2         = (ML_AUDIO_FORMAT_COMPLEX | 52),
  ML_AUDIO_FORMAT_ADAT_USER3         = (ML_AUDIO_FORMAT_COMPLEX | 53)
};

#define ML_GET_AUDIO_FORMAT_TYPE(f)     ((f) & ML_AUDIO_FORMAT_TYPE_MASK)
#define ML_GET_AUDIO_FORMAT_BITS(f)     ((f) & ML_AUDIO_FORMAT_DETAIL_MASK)
#define ML_GET_AUDIO_FORMAT_STORAGE(f) (((f)&ML_AUDIO_FORMAT_STORAGE_MASK)>>8)

enum mlAudioChannelsEnum
{
  ML_CHANNELS_MONO   = 1,
  ML_CHANNELS_STEREO = 2,
  ML_CHANNELS_4      = 4,
  ML_CHANNELS_6      = 6,
  ML_CHANNELS_8      = 8,
  ML_CHANNELS_16     = 16
};

enum mlAudioFPLimitingEnum
{
  ML_FP_LIMITING_OFF = 0,
  ML_FP_LIMITING_ON  = 1
};

enum mlAudioVideoSyncEnum
{
  ML_VID_EXTERNAL = 0,
  ML_VID_INTERNAL = 1
};

/* The sample rate of the signal can not be determined
 */
#define ML_SAMPLE_RATE_UNKNOWN 0.0	

/* define standard uppercase only equivalents
 */
#define ML_AUDIO_FORMAT_S24IN32R	ML_AUDIO_FORMAT_S24in32R
#define ML_AUDIO_FORMAT_S20IN32L	ML_AUDIO_FORMAT_S20in32L

#ifdef __cplusplus
}
#endif

#endif
