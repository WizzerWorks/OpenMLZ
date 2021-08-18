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

#ifndef MLU_TIMECODE_H
#define MLU_TIMECODE_H

#include <ML/ml.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Formats */
typedef enum MLU_TC_FORMAT_e {
  MLU_TC_FORMAT_NTSC = 0x10,
  MLU_TC_FORMAT_PAL  = 0x20,
  MLU_TC_FORMAT_FILM = 0x30,
  MLU_TC_FORMAT_HDTV = 0x40,

  MLU_TC_FORMAT_MASK = 0xf0
} MLU_TC_FORMAT;

/* Rates */
typedef enum MLU_TC_RATE_e {
  MLU_TC_RATE_2997 = 0x1,	/* NTSC, Brazilian PAL, dropframe  */
  MLU_TC_RATE_30,		/* NTSC, Brazilian PAL, non-dropframe */
  MLU_TC_RATE_24,		/* film                            */
  MLU_TC_RATE_25,		/* PAL                             */
  MLU_TC_RATE_60,		/* HDTV, experimental */
  MLU_TC_RATE_5994,		/* HDTV, experimental */

  MLU_TC_RATE_MASK = 0xf
} MLU_TC_RATE;

/* Dropframe flag */
typedef enum MLU_TC_DROPFRAME_e {
  MLU_TC_NODROP    = 0x000,
  MLU_TC_DROP      = 0x100,
  MLU_TC_DROP_MASK = 0x100
} MLU_TC_DROPFRAME;

/* common (fully specified) timecode types */
typedef enum MLU_TC_TYPE_e {
  MLU_TC_BAD = 0,

  MLU_TC_24_ND = (MLU_TC_NODROP | MLU_TC_FORMAT_FILM | MLU_TC_RATE_24),
  MLU_TC_25_ND = (MLU_TC_NODROP | MLU_TC_FORMAT_PAL  | MLU_TC_RATE_25),
  MLU_TC_30_ND = (MLU_TC_NODROP | MLU_TC_FORMAT_NTSC | MLU_TC_RATE_30),
  MLU_TC_60_ND = (MLU_TC_NODROP | MLU_TC_FORMAT_HDTV | MLU_TC_RATE_60),

  /* Note that 29.97 4-field drop is what is used in NTSC */
  /* 29.97 8-field drop is used in M-PAL (Brazil) */
  MLU_TC_2997_4FIELD_DROP = (MLU_TC_DROP|MLU_TC_FORMAT_NTSC|MLU_TC_RATE_2997),
  MLU_TC_2997_8FIELD_DROP = (MLU_TC_DROP|MLU_TC_FORMAT_PAL|MLU_TC_RATE_2997),
  MLU_TC_5994_8FIELD_DROP = (MLU_TC_DROP|MLU_TC_FORMAT_HDTV|MLU_TC_RATE_5994)

} MLU_TC_TYPE;

typedef struct MLUTimeCode_s {
  MLint8	hours;
  MLint8	minutes;
  MLint8	seconds;
  MLint8	frames;

  MLuint8	evenField  :1;	/* True if came from an even field */
  MLuint8	colorLock  :1;	/* Count locked to color framing */
  MLuint8	dropFrame  :1;	/* NTSC drop frame mode on */

  MLuint8	userType;	/* Binary Group flags for user data */
  MLuint8	userData[4];	/* User Group bytes */

  MLU_TC_TYPE	tc_type;

} MLUTimeCode;

MLstatus MLAPI mluTCToString( char * string,
			      const MLUTimeCode *smpteTimecode );

MLstatus MLAPI mluTCFromString( MLUTimeCode * result, const char * string,
				MLU_TC_TYPE tc_type );

MLstatus MLAPI mluTCFromSeconds( MLUTimeCode * result,
				 const MLU_TC_TYPE tc_type,
				 const double seconds );

MLstatus MLAPI mluTCToSeconds( const MLUTimeCode * a, double * seconds);

MLstatus MLAPI mluTCPack( MLint32* word, const MLUTimeCode *smpteTimecode );
MLstatus MLAPI mluTCUnpack( MLUTimeCode *result, MLint32 word );


int      MLAPI mluTCFramesPerDay( const int tc_type );

MLstatus MLAPI mluTCAddTC( MLUTimeCode * result, const MLUTimeCode *a,
			   const MLUTimeCode *b, int *overflow );

MLstatus MLAPI mluTCAddFrames( MLUTimeCode * result, const MLUTimeCode * a, 
			       int  b, int * overflowunderflow );

MLstatus MLAPI mluTCFramesBetween( int  * result, const MLUTimeCode * a, 
				   const MLUTimeCode * b );

#ifdef __cplusplus
}
#endif
#endif /* MLU_TIMECODE_H */
