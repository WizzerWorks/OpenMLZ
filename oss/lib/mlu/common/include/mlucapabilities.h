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

#ifndef _MLUCAPABILITIES_H
#define _MLUCAPABILITIES_H

#ifdef __cplusplus
extern "C" {
#endif

MLstatus MLAPI
mluFindDeviceByName( MLint64 sysId, const char* name, MLint64* retDevId );

MLstatus MLAPI
mluFindDeviceByNameAndIndex( MLint64 sysId, const char* name,
			     MLint32 index, MLint64* retDevId );

MLstatus MLAPI
mluFindJackByName( MLint64 devId, const char* name, MLint64* retDevId );

MLstatus MLAPI
mluFindPathByName( MLint64 devId, const char* name, MLint64* retPathId );

MLstatus MLAPI
mluFindXcodeByName( MLint64 devId, const char* name, MLint64* retXcodeId );

MLstatus MLAPI
mluFindXcodePipeByName( MLint64 xcodeId, const char* name,
			MLint64* retPipeId );

MLstatus MLAPI
mluFindJackByDirection( MLint64 devId, MLint32 direction, MLint64* retJackId );

MLstatus MLAPI
mluFindFirstInputJack( MLint64 devId, MLint64* retJackId );

MLstatus MLAPI
mluFindFirstOutputJack( MLint64 devId, MLint64* retJackId );

MLstatus MLAPI
mluFindPathFromJack( MLint64 jackId,
		     MLint64* retPathId, MLint32* retPathAlignment );

MLstatus MLAPI
mluFindPathToJack( MLint64 jackId,
		   MLint64* retPathId, MLint32* retPathAlignment );

#ifdef __cplusplus 
}
#endif

#endif /* _MLUCAPABILITIES_H */
