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

#ifndef __INC_MLANCDATA_H__
#define __INC_MLANCDATA_H__  


#ifdef __cplusplus
extern "C" {
#endif

#include <ML/mldefs.h>
#include <ML/mlparam.h>


/* Ancillary data params
 */

enum MLpvAncDataClassEnum {
  /* VITC 32 bit Timecode and Userdata Bytes
   */
  ML_VITC_TIMECODE_INT32 = ML_PARAM_NAME( ML_CLASS_ANC_DATA,
					  ML_TYPE_INT32, 0x01 ),
  ML_VITC_USERDATA_INT32 = ML_PARAM_NAME( ML_CLASS_ANC_DATA,
					  ML_TYPE_INT32, 0x02 ),

  /* This is the VITC code for the 2nd field of an Interleaved
   * frame...
   */
  ML_VITC2_TIMECODE_INT32 = ML_PARAM_NAME( ML_CLASS_ANC_DATA,
					   ML_TYPE_INT32, 0x03 ),
  ML_VITC2_USERDATA_INT32  = ML_PARAM_NAME( ML_CLASS_ANC_DATA,
					    ML_TYPE_INT32, 0x04 ),

  /* Which horizontal line VITC should be sent on output
   * Which horizontal line VITC should be checked on input
   * See "MLVitcLineNumberEnum" for preset values
   */
  ML_VITC_LINE_NUMBER_INT32 = ML_PARAM_NAME( ML_CLASS_ANC_DATA,
					     ML_TYPE_INT32, 0x05 ),

  /* Specify on which line the 2nd (identical) VITC code should be
   * placed...
   */
  ML_VITC_LINE_NUMBER_2_INT32 = ML_PARAM_NAME( ML_CLASS_ANC_DATA,
					       ML_TYPE_INT32, 0x06 ),

  /* Which horizontal line VITC was detected on input
   */
  ML_VITC_INCOMING_LINE_NUMBER_INT32 = ML_PARAM_NAME( ML_CLASS_ANC_DATA,
						      ML_TYPE_INT32, 0x07 ),

  /* LTC 32 bit Timecode and Userdata Bytes
   */
  ML_LTC_TIMECODE_INT32	= ML_PARAM_NAME( ML_CLASS_ANC_DATA,
					 ML_TYPE_INT32, 0x10 ),
  ML_LTC_USERDATA_INT32	= ML_PARAM_NAME( ML_CLASS_ANC_DATA,
					 ML_TYPE_INT32, 0x11 )
};


/* Preset values for ML_VITC_LINE_NUMBER_INT32
 */
enum MLVitcLineNumberEnum {
  /* Set ML_VITC_LINE_NUMBER_DEFAULT to send/check VITC on the default lines:
   * ML_TIMING_525: 14/16
   * ML_TIMING_625: 19/21
   * (others: TBD)
   */
  ML_VITC_LINE_NUMBER_DEFAULT = 10000,

  /* Set ML_VITC_LINE_NUMBER_NONE to turn off sending/checking VITC
   */
  ML_VITC_LINE_NUMBER_NONE = 10001

  /* Other values are device dependent but are usually in the range
   * 4..23
   */
};


/* PACKED RAW Timecode format:
 */
typedef union MLTimeCodePackedRaw_u {
  MLuint32 word;
  struct {
    MLuint32 bit75        : 1 ;   /* bit 75    */
    MLuint32 bit74        : 1 ;   /* bit 74    */
    MLuint32 hrs1         : 2 ;   /* bit 73-72 */
    MLuint32 hrs2         : 4 ;   /* bit 65-62 */
    MLuint32 bit55        : 1 ;   /* bit 55    */
    MLuint32 mins1        : 3 ;   /* bit 54-52 */
    MLuint32 mins2        : 4 ;   /* bit 45-42 */
    MLuint32 bit35        : 1 ;   /* bit 35    */
    MLuint32 secs1        : 3 ;   /* bit 34-32 */
    MLuint32 secs2        : 4 ;   /* bit 25-22 */
    MLuint32 color        : 1 ;   /* bit 15    */
    MLuint32 drop         : 1 ;   /* bit 14    */
    MLuint32 frames1      : 2 ;   /* bit 13-12 */
    MLuint32 frames2      : 4 ;   /* bit 5-2   */
  } bits;
} MLTimeCodePackedRaw;

#ifdef __cplusplus
}
#endif

#endif
