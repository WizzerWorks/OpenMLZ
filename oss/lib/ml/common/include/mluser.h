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

#ifndef _ML_USER_H_
#define _ML_USER_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <ML/mlparam.h>

enum MLpvUserClassEnum
{
  ML_USERDATA_INT32  = ML_PARAM_NAME( ML_CLASS_USER, ML_TYPE_INT32,  0x1 ),
  ML_USERDATA_INT64  = ML_PARAM_NAME( ML_CLASS_USER, ML_TYPE_INT64,  0x2 ),
  ML_USERDATA_REAL32 = ML_PARAM_NAME( ML_CLASS_USER, ML_TYPE_REAL32, 0x3 ),
  ML_USERDATA_REAL64 = ML_PARAM_NAME( ML_CLASS_USER, ML_TYPE_REAL64, 0x4 ),
  ML_USERDATA_BYTE_POINTER = ML_PARAM_NAME( ML_CLASS_USER,
					    ML_TYPE_BYTE_POINTER,    0x5 )
};


/* The following is used by the user to define user specific controls
 *
 * "ML_USERDATA_BASE_VALUE" insures that the parameter index will not
 * conflict with any internally defined values.
 */
enum MLpvUserBaseValue
{
  ML_USERDATA_BASE_VALUE = 0x20,
  ML_USERDATA_MAX_VALUE  = 0xff
};

#define	ML_USERDATA_DEFINED(type,value)	\
	ML_PARAM_NAME( ML_CLASS_USER, type, ML_USERDATA_BASE_VALUE+(value) )

#ifdef __cplusplus 
}
#endif

#endif /* _ML_USER_H_ */
