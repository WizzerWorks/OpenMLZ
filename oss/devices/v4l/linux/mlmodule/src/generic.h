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


#ifndef V4LGENERIC_H
#define V4LGENERIC_H

#include <stdio.h>
#include <stdlib.h>
#include <ML/ml_didd.h>

typedef struct
{
  MLint64  id;
  char*    name;
  MLint32  type;
  MLint32  access;
  MLint32* deflt;
  MLint32* mins; 
  MLint32  minsLength;
  MLint32* maxs; 
  MLint32  maxsLength;
  MLint32* enumValues; 
  MLint32  enumValuesLength;
  char*    enumNames; 
  MLint32  enumNamesLength;
} MLDDint32paramDetails;

typedef struct
{
  MLint32                jackIndex;
  char*                  name;
  MLint32                type;
  MLint32                direction;
  MLint32*               pathIndexes;
  MLint64*               pathIds;
  MLint32                pathLength;
} MLDDjackDetails;

typedef struct
{
  MLint32                pathIndex;
  char*                  name;
  MLint32                type;
  MLint32                pixelLineAlignment;
  MLint32                bufferAlignment;
  MLDDint32paramDetails** params;
  MLint32                paramsLength;
  MLint32                srcJackIndex;
  MLint32                dstJackIndex;
} MLDDpathDetails;

typedef struct
{
  char*                  name;
  MLint32                index;
  MLint32                version;
  char*                  location;
  MLint32                jackLength;
  MLint32                pathLength;
} MLDDdeviceDetails;

MLstatus ddIdentifyPhysicalDevice(MLDDdeviceDetails* pDevice,
				  MLint64 assignedDeviceId, MLpv** retCap);
MLstatus ddIdentifyJack(MLDDjackDetails* pJack,
			MLint64 objectId, MLpv** retCap);
MLstatus ddIdentifyPath(MLDDpathDetails* pPath,
			MLint64 objectId, MLpv** retCap);
MLstatus ddIdentifyParam(MLDDint32paramDetails* pParam,
			 MLint64 deviceId, MLpv** retCap);

#endif /* V4LGENERIC_H */
