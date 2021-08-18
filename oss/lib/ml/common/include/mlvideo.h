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

#ifndef _ML_VIDEO_H
#define _ML_VIDEO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <ML/mldefs.h>
#include <ML/mlparam.h>


/* Video  parameters
 */

enum MLpvVideoClassEnum
{
  /* Video jack parameters */
  ML_VIDEO_TIMING_INT32                = ML_PARAM_NAME( ML_CLASS_VIDEO,
							ML_TYPE_INT32,  0x01 ),
  ML_VIDEO_COLORSPACE_INT32            = ML_PARAM_NAME( ML_CLASS_VIDEO,
							ML_TYPE_INT32,  0x02 ),
  ML_VIDEO_PRECISION_INT32             = ML_PARAM_NAME( ML_CLASS_VIDEO,
							ML_TYPE_INT32,  0x03 ),
  ML_VIDEO_SIGNAL_PRESENT_INT32        = ML_PARAM_NAME( ML_CLASS_VIDEO,
							ML_TYPE_INT32,  0x04 ),
  ML_VIDEO_GENLOCK_SOURCE_TIMING_INT32 = ML_PARAM_NAME( ML_CLASS_VIDEO,
							ML_TYPE_INT32,  0x05 ),
  ML_VIDEO_GENLOCK_TYPE_INT32          = ML_PARAM_NAME( ML_CLASS_VIDEO,
							ML_TYPE_INT32,  0x06 ),
  ML_VIDEO_GENLOCK_SIGNAL_PRESENT_INT32= ML_PARAM_NAME( ML_CLASS_VIDEO,
							ML_TYPE_INT32,  0x07 ),
  ML_VIDEO_GENLOCK_STATE_INT32         = ML_PARAM_NAME( ML_CLASS_VIDEO,
							ML_TYPE_INT32,  0x08 ),
  ML_VIDEO_GENLOCK_ERROR_STATUS_INT32  = ML_PARAM_NAME( ML_CLASS_VIDEO,
							ML_TYPE_INT32,  0x09 ),

  ML_VIDEO_BRIGHTNESS_INT32            = ML_PARAM_NAME( ML_CLASS_VIDEO,
							ML_TYPE_INT32,  0x30 ),
  ML_VIDEO_CONTRAST_INT32              = ML_PARAM_NAME( ML_CLASS_VIDEO,
							ML_TYPE_INT32,  0x31 ),
  ML_VIDEO_HUE_INT32                   = ML_PARAM_NAME( ML_CLASS_VIDEO,
							ML_TYPE_INT32,  0x32 ),
  ML_VIDEO_SATURATION_INT32            = ML_PARAM_NAME( ML_CLASS_VIDEO,
							ML_TYPE_INT32,  0x33 ),
  
  ML_VIDEO_RED_SETUP_INT32             = ML_PARAM_NAME( ML_CLASS_VIDEO,
							ML_TYPE_INT32,  0x34 ),
  ML_VIDEO_GREEN_SETUP_INT32           = ML_PARAM_NAME( ML_CLASS_VIDEO,
							ML_TYPE_INT32,  0x35 ),
  ML_VIDEO_BLUE_SETUP_INT32            = ML_PARAM_NAME( ML_CLASS_VIDEO,
							ML_TYPE_INT32,  0x36 ),
  ML_VIDEO_ALPHA_SETUP_INT32           = ML_PARAM_NAME( ML_CLASS_VIDEO,
							ML_TYPE_INT32,  0x37 ),
  
  ML_VIDEO_H_PHASE_INT32               = ML_PARAM_NAME( ML_CLASS_VIDEO,
							ML_TYPE_INT32,  0x38 ),
  ML_VIDEO_V_PHASE_INT32               = ML_PARAM_NAME( ML_CLASS_VIDEO,
							ML_TYPE_INT32,  0x39 ),
  
  ML_VIDEO_FLICKER_FILTER_INT32        = ML_PARAM_NAME( ML_CLASS_VIDEO,
							ML_TYPE_INT32,  0x3a ),
  ML_VIDEO_DITHER_FILTER_INT32         = ML_PARAM_NAME( ML_CLASS_VIDEO,
							ML_TYPE_INT32,  0x3b ),
  ML_VIDEO_NOTCH_FILTER_INT32          = ML_PARAM_NAME( ML_CLASS_VIDEO,
							ML_TYPE_INT32,  0x3c ),
  
  ML_VIDEO_FILL_Y_REAL32               = ML_PARAM_NAME( ML_CLASS_VIDEO,
							ML_TYPE_REAL32, 0x60 ),
  ML_VIDEO_FILL_Cr_REAL32              = ML_PARAM_NAME( ML_CLASS_VIDEO,
							ML_TYPE_REAL32, 0x61 ),
  ML_VIDEO_FILL_Cb_REAL32              = ML_PARAM_NAME( ML_CLASS_VIDEO,
							ML_TYPE_REAL32, 0x62 ),
  ML_VIDEO_FILL_RED_REAL32             = ML_PARAM_NAME( ML_CLASS_VIDEO,
							ML_TYPE_REAL32, 0x63 ),
  ML_VIDEO_FILL_GREEN_REAL32           = ML_PARAM_NAME( ML_CLASS_VIDEO,
							ML_TYPE_REAL32, 0x64 ),
  ML_VIDEO_FILL_BLUE_REAL32            = ML_PARAM_NAME( ML_CLASS_VIDEO,
							ML_TYPE_REAL32, 0x65 ),
  ML_VIDEO_FILL_ALPHA_REAL32           = ML_PARAM_NAME( ML_CLASS_VIDEO,
							ML_TYPE_REAL32, 0x66 ),

  /* Video Jack Multiplexor Controls */
  ML_VIDEO_INPUT_DEFAULT_SIGNAL_INT64  = ML_PARAM_NAME( ML_CLASS_VIDEO,
							ML_TYPE_INT64,  0x40 ),
  ML_VIDEO_OUTPUT_DEFAULT_SIGNAL_INT64 = ML_PARAM_NAME( ML_CLASS_VIDEO,
							ML_TYPE_INT64,  0x41 ),

  /* Video raster parameters */
  ML_VIDEO_START_X_INT32               = ML_PARAM_NAME( ML_CLASS_VIDEO,
							ML_TYPE_INT32,  0x11 ),
  ML_VIDEO_START_Y_F1_INT32            = ML_PARAM_NAME( ML_CLASS_VIDEO,
							ML_TYPE_INT32,  0x12 ),
  ML_VIDEO_START_Y_F2_INT32            = ML_PARAM_NAME( ML_CLASS_VIDEO,
							ML_TYPE_INT32,  0x13 ),
  ML_VIDEO_WIDTH_INT32                 = ML_PARAM_NAME( ML_CLASS_VIDEO,
							ML_TYPE_INT32,  0x14 ),
  ML_VIDEO_HEIGHT_F1_INT32             = ML_PARAM_NAME( ML_CLASS_VIDEO,
							ML_TYPE_INT32,  0x15 ),
  ML_VIDEO_HEIGHT_F2_INT32             = ML_PARAM_NAME( ML_CLASS_VIDEO,
							ML_TYPE_INT32,  0x16 ),
  ML_VIDEO_OUTPUT_REPEAT_INT32         = ML_PARAM_NAME( ML_CLASS_VIDEO,
							ML_TYPE_INT32,  0x17 ),
  ML_VIDEO_SAMPLING_INT32              = ML_PARAM_NAME( ML_CLASS_VIDEO,
							ML_TYPE_INT32,  0x19 ),

  /* ust/msc support */
  ML_WAIT_FOR_VIDEO_UST_INT64          = ML_PARAM_NAME( ML_CLASS_VIDEO,
							ML_TYPE_INT64,  0x25 ),
  ML_WAIT_FOR_VIDEO_MSC_INT64          = ML_PARAM_NAME( ML_CLASS_VIDEO,
							ML_TYPE_INT64,  0x26 ),

  ML_VIDEO_UST_INT64                   = ML_PARAM_NAME( ML_CLASS_VIDEO,
							ML_TYPE_INT64,  0x27 ),
  ML_VIDEO_MSC_INT64                   = ML_PARAM_NAME( ML_CLASS_VIDEO,
							ML_TYPE_INT64,  0x28 ),
  ML_VIDEO_ASC_INT64                   = ML_PARAM_NAME( ML_CLASS_VIDEO,
							ML_TYPE_INT64,  0x29 )
};


/* Backward compatibility
 */
#define	ML_VIDEO_GENLOCK_SOURCE_ID_INT32 ML_VIDEO_GENLOCK_SOURCE_TIMING_INT32

/* Equates to maintain upper-case only macro standards
 */
#define ML_VIDEO_FILL_CR_REAL32 ML_VIDEO_FILL_Cr_REAL32
#define ML_VIDEO_FILL_CB_REAL32 ML_VIDEO_FILL_Cb_REAL32

/* ML_VIDEO_OUTPUT_DEFAULT_SIGNAL enumerations
 */
enum mlVideoOutputDefaultSignalEnum
{
  ML_SIGNAL_NOTHING,    /* no signal or timing  */
  ML_SIGNAL_BLACK,      /* black screen */
  ML_SIGNAL_COLORBARS,  /* device dependant definition of colorbars */
  ML_SIGNAL_INPUT_VIDEO /* whatever the device defaults the input to be */
};


/* Backward compatibility
 */
#define ML_SIGNAL_COLOR_BARS ML_SIGNAL_COLORBARS


/* ML_VIDEO_TIMING_INT32 enumerations
 */
enum mlVideoTimingEnum
{
  /* The following two params are meaningful only when
   * querying the sensed timing on a video device
   */
  ML_TIMING_NONE,    /* no signal is present */                   /* 0 */
  ML_TIMING_UNKNOWN, /* the timing of the signal can not be determined */

  /* Common SD video timings 
   */
  ML_TIMING_525,        /* NTSC 13.5 MHz; 720x486 or 720x487 */  /* 2 */
  ML_TIMING_625,        /* PAL  13.5 Mhz; 720x576 */
  ML_TIMING_525_SQ_PIX, /* 12.27 MHz; 646x486 */
  ML_TIMING_625_SQ_PIX, /* 14.75 MHz; 768x576 */

  /* SMPTE 274M formats: 1125 total lines, 1920x1080 active picture,
   * square pixels, 16:9 picture aspect ratio.
   * 'p' suffix means progressive scanning, 
   * 'i' suffix means 2:1 interlaced,
   * 'PsF' suffix means progressive split field.
   */
  ML_TIMING_1125_1920x1080_60p,    /* 148.5 MHz */               /* 6 */
  ML_TIMING_1125_1920x1080_5994p,  /* 148.5/1.001 MHz */
  ML_TIMING_1125_1920x1080_50p,    /* 148.5 MHz */
  ML_TIMING_1125_1920x1080_60i,    /* 74.25 MHz */
  ML_TIMING_1125_1920x1080_5994i,  /* 74.25/1.001 MHz */         /* a */
  ML_TIMING_1125_1920x1080_50i,    /* 74.25 MHz */
  ML_TIMING_1125_1920x1080_30p,    /* 74.25 MHz */
  ML_TIMING_1125_1920x1080_2997p,  /* 74.25/1.001 MHz */
  ML_TIMING_1125_1920x1080_25p,    /* 74.25 MHz */
  ML_TIMING_1125_1920x1080_24p,    /* 74.25 MHz */               /* f */
  ML_TIMING_1125_1920x1080_2398p,  /* 74.25/1.001 MHz */         /* 10 */

  ML_TIMING_1125_1920x1080_24PsF,  /* Sony proposed 274M 74.25 MHz */
  ML_TIMING_1125_1920x1080_2398PsF,/* Sony proposed 274M 74.25/1.001 MHz */
  ML_TIMING_1125_1920x1080_30PsF,  /* Sony proposed 274M 74.25 MHz */
  ML_TIMING_1125_1920x1080_2997PsF,/* Sony proposed 274M 74.25/1.001 MHz */
  ML_TIMING_1125_1920x1080_25PsF,  /* Sony proposed 274M 74.25 MHz */
  
  /* other "high-definition" formats
   */
  ML_TIMING_1250_1920x1080_50p,    /* SMPTE 295M 148.5 MHz */    /* 16 */
  ML_TIMING_1250_1920x1080_50i,    /* SMPTE 295M 74.25 MHz */
  ML_TIMING_1125_1920x1035_60i,    /* SMPTE 260M/240M 74.25 MHz */
  ML_TIMING_1125_1920x1035_5994i,  /* SMPTE 260M/240M 74.25/1.001 MHz*/
  ML_TIMING_750_1280x720_60p,      /* SMPTE 296M 74.25 MHz */    /* 1a */
  ML_TIMING_750_1280x720_5994p,    /* SMPTE 296M 74.25/1.001 MHz */
  ML_TIMING_525_720x483_5994p,     /* SMPTE 293M 27.0 MHz */

  ML_TIMING_750_1280x720_50p,      /* SMPTE 296M 74.25 MHz */    /* 1d */
  ML_TIMING_750_1280x720_30p,      /* SMPTE 296M 74.25/1.001 MHz */
  ML_TIMING_750_1280x720_2997p,    /* SMPTE 296M 74.25 MHz */
  ML_TIMING_750_1280x720_25p,      /* SMPTE 296M 74.25/1.001 MHz */ /* 20 */
  ML_TIMING_750_1280x720_24p,      /* SMPTE 296M 74.25 MHz */
  ML_TIMING_750_1280x720_2398p,    /* SMPTE 296M 74.25/1.001 MHz */

  /* HSDL formats
   */
  ML_TIMING_FILM2K_1556_20PsF,     /* HSDL */                    /* 23 */
  ML_TIMING_FILM2K_1556_1915PsF,   /* HSDL */
  ML_TIMING_FILM2K_1556_15PsF,     /* HSDL */
  ML_TIMING_FILM2K_1556_14PsF,     /* HSDL */
                                                                                
  ML_TIMING_FILM2K_1080_24p,       /* HSDL */                    /* 27 */
  ML_TIMING_FILM2K_1080_2398p,     /* HSDL */

  ML_TIMING_FILM2K_1080_24PsF,     /* HSDL */                    /* 29 */
  ML_TIMING_FILM2K_1080_2398PsF,   /* HSDL */               

  ML_NUMBER_TIMINGS
};


/* ML_VIDEO_OUTPUT_REPEAT_INT32 enumerations
 */
enum mlVideoOutputRepeatEnum
{
  ML_VIDEO_REPEAT_NONE,
  ML_VIDEO_REPEAT_FIELD,
  ML_VIDEO_REPEAT_FRAME
};

/* ML_VIDEO_GENLOCK_TYPE_INT32 enumerations
 */
enum mlVideoGenlockTypeEnum
{
  ML_VIDEO_GENLOCK_SRC_INTERNAL,
  ML_VIDEO_GENLOCK_SRC_VIDEO_INPUT,
  ML_VIDEO_GENLOCK_SRC_REF_BILEVEL,
  ML_VIDEO_GENLOCK_SRC_REF_TRILEVEL
};

/* ML_VIDEO_GENLOCK_STATE_INT32 enumerations
 */
enum mlVideoGenlockStateEnum
{
  ML_VIDEO_GENLOCK_STATE_IS_UNKNOWN,
  ML_VIDEO_GENLOCK_STATE_IS_UNLOCKED,
  ML_VIDEO_GENLOCK_STATE_IS_LOCKED
};

/* ML_VIDEO_GENLOCK_ERROR_STATUS_INT32 enumerations
 */
enum mlVideoGenlockErrorStatus
{
  ML_VIDEO_GENLOCK_ERROR_STATUS_NONE,
  ML_VIDEO_GENLOCK_ERROR_STATUS_NO_SIGNAL,
  ML_VIDEO_GENLOCK_ERROR_STATUS_UNKNOWN_SIGNAL,
  ML_VIDEO_GENLOCK_ERROR_STATUS_ILLEGAL_COMBINATION,
  ML_VIDEO_GENLOCK_ERROR_STATUS_TIMING_MISMATCH,
  ML_VIDEO_GENLOCK_ERROR_STATUS_UNKNOWN
};

#ifdef __cplusplus 
}
#endif

#endif /* _ML_VIDEO_H */


