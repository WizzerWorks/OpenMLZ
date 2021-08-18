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

#ifndef _ML_XCODE_H
#define _ML_XCODE_H
#ifdef __cplusplus
extern "C" {
#endif

#include <ML/mldefs.h>
#include <ML/mlparam.h>


/* mlXcode parameters used when obtaining the capabilities of a
 * transcoder Some parameter-value pairs returned by
 * mlXcodeGetCapabilities have a pre-defined location in the
 * list. Their indexes into the array are specified in mlXcodeIndexes
 *
 * See also the subsystem-independent capabilities defined in
 * mlparam.h
 */


/* Define's for default src and dst pipe
 */
#ifdef ML_XCODE_INPUT_PIPE_1_ID /* if mlcompat_1.0_2.0.h file is included */
#define ML_XCODE_SRC_PIPE ML_XCODE_INPUT_PIPE_1_ID
#define ML_XCODE_DST_PIPE ML_XCODE_OUTPUT_PIPE_1_ID
#else
#define ML_XCODE_SRC_PIPE ((MLint64)0x1 << 32) /* same as
						* ML_XCODE_INPUT_PIPE_1_ID */
#define ML_XCODE_DST_PIPE ((MLint64)0x8 << 32) /* same as
						* ML_XCODE_OUTPUT_PIPE_1_ID */
#endif
 
enum MLpvStaticClassXcodeEnum
{
  ML_XCODE_ENGINE_TYPE_INT32 =
        ML_PARAM_NAME( ML_CLASS_STATIC, ML_TYPE_INT32,
		       (ML_PV_STATIC_CLASS_BASE_XCODE + 1) ),
  ML_XCODE_IMPLEMENTATION_TYPE_INT32 =
        ML_PARAM_NAME( ML_CLASS_STATIC, ML_TYPE_INT32,
		       (ML_PV_STATIC_CLASS_BASE_XCODE + 2) ),
  ML_XCODE_COMPONENT_ALIGNMENT_INT32 =
        ML_PARAM_NAME( ML_CLASS_STATIC, ML_TYPE_INT32,
		       (ML_PV_STATIC_CLASS_BASE_XCODE + 12) ),
  ML_XCODE_BUFFER_ALIGNMENT_INT32 =
        ML_PARAM_NAME( ML_CLASS_STATIC, ML_TYPE_INT32,
		       (ML_PV_STATIC_CLASS_BASE_XCODE + 13) ),
  ML_XCODE_FEATURES_BYTE_ARRAY =
        ML_PARAM_NAME( ML_CLASS_STATIC, ML_TYPE_BYTE_ARRAY,
		       (ML_PV_STATIC_CLASS_BASE_XCODE + 14) ),
  ML_XCODE_SRC_PIPE_IDS_INT64_ARRAY =
        ML_PARAM_NAME( ML_CLASS_STATIC, ML_TYPE_INT64_ARRAY,
		       (ML_PV_STATIC_CLASS_BASE_XCODE + 16) ),
  ML_XCODE_DEST_PIPE_IDS_INT64_ARRAY =
        ML_PARAM_NAME( ML_CLASS_STATIC, ML_TYPE_INT64_ARRAY,
		       (ML_PV_STATIC_CLASS_BASE_XCODE + 17) )
};


/* The StaticClassPipeEnum below is using
 * (ML_PV_STATIC_CLASS_BASE_XCODE + X), where X is one larger than the
 * number for ML_XCODE_DEST_PIPE_IDS_INT64_ARRAY param above...
 */
enum MLpvStaticClassPipeEnum
{
  ML_PIPE_TYPE_INT32 =
        ML_PARAM_NAME( ML_CLASS_STATIC, ML_TYPE_INT32,
		       (ML_PV_STATIC_CLASS_BASE_XCODE + 18) )
};

enum mlPipeTypeEnum
{
  ML_PIPE_TYPE_MEM_TO_ENGINE,	
  ML_PIPE_TYPE_ENGINE_TO_MEM
};

enum mlXcodeStream{
  ML_XCODE_STREAM_SINGLE,
  ML_XCODE_STREAM_MULTI
};


/* ML_XCODE_IMPLEMENTATION_TYPE_INT32 enumerated values
 */
typedef enum {
  ML_XCODE_IMPLEMENTATION_TYPE_SW = 10,
  ML_XCODE_IMPLEMENTATION_TYPE_HW = 20
} MLxcodeimplementationtype;


/* ML_XCODE_MODE_INT32 enumerated values
 */
typedef enum __MLxcodemode{
  ML_XCODE_MODE_SYNCHRONOUS  = 10,
  ML_XCODE_MODE_ASYNCHRONOUS = 20
} MLxcodemode;


/* ML_XCODE_ENGINE_INT32 enumerated values
 */
typedef enum {
  ML_XCODE_ENGINE_TYPE_NULL = 1
} MLxcodeengine;


/* mlXcode parameters. For parameters with enumerated values, see
 * below for enumerations.
 */
enum mlParamsXcodeClassEnum
{
  /* Send/Receive Parameters
   * Spatial quality should be a real - use of int param is deprecated
   */
  ML_XCODE_SPATIAL_QUALITY_REAL32    = ML_PARAM_NAME( ML_CLASS_XCODE,
						      ML_TYPE_REAL32, 0x08 ),
  ML_XCODE_BITRATE_INT32             = ML_PARAM_NAME( ML_CLASS_XCODE,
						      ML_TYPE_INT32,  0x09 ),
  ML_XCODE_NUMBER_WORK_THREADS_INT32 = ML_PARAM_NAME( ML_CLASS_XCODE,
						      ML_TYPE_INT32,  0x0a )
};

#ifdef __cplusplus
}
#endif

#endif /* _ML_XCODE_H */
