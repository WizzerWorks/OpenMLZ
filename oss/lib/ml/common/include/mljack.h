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

#ifndef _ML_JACK_H
#define _ML_JACK_H
#ifdef __cplusplus
extern "C" {
#endif

#include <ML/mldefs.h>
#include <ML/mlparam.h>

/* mlJackparameters used when obtaining the capabilities of a jack
 * Some parameter-value pairs returned by mlJackGetCapabilities have a
 * pre-defined location in the list. Their indexes into the array are
 * specified in mlJackIndexes
 *
 * See also the subsystem-independent capabilities defined in mlparam.h
 */
enum MLpvStaticClassJackEnum
{
  ML_JACK_TYPE_INT32 =
          ML_PARAM_NAME( ML_CLASS_STATIC, ML_TYPE_INT32,
			 (ML_PV_STATIC_CLASS_BASE_JACK + 1) ),
  ML_JACK_DIRECTION_INT32 =
          ML_PARAM_NAME( ML_CLASS_STATIC, ML_TYPE_INT32,
			 (ML_PV_STATIC_CLASS_BASE_JACK + 2) ),
  ML_JACK_COMPONENT_SIZE_INT32 =
          ML_PARAM_NAME( ML_CLASS_STATIC, ML_TYPE_INT32,
			 (ML_PV_STATIC_CLASS_BASE_JACK + 3) ),
  ML_JACK_ALIAS_IDS =
          ML_PARAM_NAME( ML_CLASS_STATIC, ML_TYPE_INT64_ARRAY,
			 (ML_PV_STATIC_CLASS_BASE_JACK + 4) ),
  ML_JACK_PATH_IDS_INT64_ARRAY =
          ML_PARAM_NAME( ML_CLASS_STATIC, ML_TYPE_INT64_ARRAY,
			 (ML_PV_STATIC_CLASS_BASE_JACK + 5) ),
  ML_JACK_FEATURES_BYTE_ARRAY =
          ML_PARAM_NAME( ML_CLASS_STATIC, ML_TYPE_BYTE_ARRAY,
			 (ML_PV_STATIC_CLASS_BASE_JACK + 7) ),
  ML_JACK_INTERNAL_ID =
          ML_PARAM_NAME( ML_CLASS_STATIC, ML_TYPE_INT32,
			 (ML_PV_STATIC_CLASS_BASE_JACK + 8) )
};


/* ML_JACK_TYPE enumerated values
 */
enum mlJackTypeEnum
{
  ML_JACK_TYPE_COMPOSITE,
  ML_JACK_TYPE_SVIDEO,
  ML_JACK_TYPE_SDI,
  ML_JACK_TYPE_DUALLINK,
  ML_JACK_TYPE_SYNC,
  ML_JACK_TYPE_GENLOCK,
  ML_JACK_TYPE_GPI,
  ML_JACK_TYPE_SERIAL,
  ML_JACK_TYPE_ANALOG_AUDIO,
  ML_JACK_TYPE_AES,
  ML_JACK_TYPE_ADAT,
  ML_JACK_TYPE_GFX,
  ML_JACK_TYPE_AUX,
  ML_JACK_TYPE_VIDEO,
  ML_JACK_TYPE_AUDIO
};


/* ML_JACK_DIRECTION enumerated values
 */
enum mlDirectionEnum
{
  ML_DIRECTION_IN,
  ML_DIRECTION_OUT,
  ML_DIRECTION_BOTH
};

#ifdef __cplusplus 
}
#endif

#endif /* _ML_JACK_H */


