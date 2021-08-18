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

#ifndef _ML_PATH_H
#define _ML_PATH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <ML/mldefs.h>
#include <ML/mlparam.h>


/* mlPath parameters used when obtaining the capabilities of a path.
 * Some parameter-value pairs returned by mlPathGetCapabilities have a
 * pre-defined location in the list. Their indexes into the array are
 * specified in mlPathIndexes
 *
 * See also the subsystem-independent capabilities defined in
 * mlparam.h
 */
enum MLpvStaticClassPathEnum
{
  ML_PATH_TYPE_INT32 =
        ML_PARAM_NAME( ML_CLASS_STATIC, ML_TYPE_INT32,
		       (ML_PV_STATIC_CLASS_BASE_PATH + 1) ),
  ML_PATH_SRC_JACK_ID_INT64 =
        ML_PARAM_NAME( ML_CLASS_STATIC, ML_TYPE_INT64,
		       (ML_PV_STATIC_CLASS_BASE_PATH + 2) ),
  ML_PATH_DST_JACK_ID_INT64 =
        ML_PARAM_NAME( ML_CLASS_STATIC, ML_TYPE_INT64,
		       (ML_PV_STATIC_CLASS_BASE_PATH + 3) ),
  ML_PATH_INTERNAL_ID =
        ML_PARAM_NAME( ML_CLASS_STATIC, ML_TYPE_INT32,
		       (ML_PV_STATIC_CLASS_BASE_PATH + 5) ),
  ML_PATH_COMPONENT_ALIGNMENT_INT32 =
        ML_PARAM_NAME( ML_CLASS_STATIC, ML_TYPE_INT32,
		       (ML_PV_STATIC_CLASS_BASE_PATH + 6) ),
  ML_PATH_BUFFER_ALIGNMENT_INT32 =
        ML_PARAM_NAME( ML_CLASS_STATIC, ML_TYPE_INT32,
		       (ML_PV_STATIC_CLASS_BASE_PATH + 7) ),
  ML_PATH_FEATURES_BYTE_ARRAY =
        ML_PARAM_NAME( ML_CLASS_STATIC, ML_TYPE_BYTE_ARRAY,
		       (ML_PV_STATIC_CLASS_BASE_PATH + 8) )
};


/* ML_PATH_TYPE enumerated values
 */
enum mlPathTypeEnum
{
  ML_PATH_TYPE_DEV_TO_MEM,
  ML_PATH_TYPE_MEM_TO_DEV,
  ML_PATH_TYPE_DEV_TO_DEV
};


/* For backwards compatibility 
 */
#define ML_PATH_PIXEL_LINE_ALIGNMENT_INT32 ML_PATH_COMPONENT_ALIGNMENT_INT32

#ifdef __cplusplus 
}
#endif

#endif /* _ML_PATH_H */

