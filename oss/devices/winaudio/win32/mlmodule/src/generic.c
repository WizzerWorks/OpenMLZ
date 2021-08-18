/***************************************************************************
 * License Applicability. Except to the extent portions of this file are 
 * made subject to an alternative license as permitted in the Khronos 
 * Free Software License Version 1.0 (the "License"), the contents of 
 * this file are subject only to the provisions of the License. You may 
 * not use this file except in compliance with the License. You may obtain 
 * a copy of the License at 
 * 
 * The Khronos Group Inc.:  PO Box 1019, Clearlake Park CA 95424 USA or at
 *  
 * http://www.Khronos.org/licenses/KhronosOpenSourceLicense1_0.pdf
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


#include <stdio.h>
#include <stdlib.h>
#include <ML/ml_didd.h>
#include "generic.h"

extern MLDDint32paramDetails* pathParams;

#define MY_PHYSICAL_DEVICE_VERSION 1

MLstatus ddIdentifyParam(MLDDint32paramDetails* pParam,
			 MLint64 deviceId, MLpv** retCap)
{
  MLpv detailParams[11];

  detailParams[0].param = ML_ID_INT64;
  detailParams[0].value.int64 = pParam->id;
  detailParams[0].length = 1;

  detailParams[1].param = ML_NAME_BYTE_ARRAY;
  detailParams[1].value.pByte = (MLbyte*) pParam->name;
  detailParams[1].length = (MLint32)strlen((const char *)detailParams[1].value.pByte) + 1;
  detailParams[1].maxLength = detailParams[1].length;

  detailParams[2].param = ML_PARENT_ID_INT64;
  detailParams[2].value.int64 = deviceId;

  detailParams[3].param = ML_PARAM_TYPE_INT32;
  detailParams[3].value.int32 = pParam->type;
  detailParams[3].length = 1;

  detailParams[4].param = ML_PARAM_ACCESS_INT32;
  detailParams[4].value.int32 = pParam->access;
  detailParams[4].length = 1;

  detailParams[5].param = ML_PARAM_DEFAULT | pParam->type;
  if( pParam->deflt != NULL && pParam->type == ML_TYPE_INT32)
    {
      detailParams[5].value.int32 = *(pParam->deflt); 
      detailParams[5].length = 1;
      detailParams[5].maxLength = 0;
    }
  else
    {
      detailParams[5].value.int64 = 0; 
      detailParams[5].length = 0;
      detailParams[5].maxLength = 0;
    }

  detailParams[6].param = ML_PARAM_MINS_ARRAY | 
    ML_PARAM_GET_TYPE_ELEM(pParam->id);
  detailParams[6].value.pInt32 = pParam->mins;
  detailParams[6].length = pParam->minsLength;
  detailParams[6].maxLength = detailParams[5].length;
  
  detailParams[7].param = ML_PARAM_MAXS_ARRAY | 
    ML_PARAM_GET_TYPE_ELEM(pParam->id);
  detailParams[7].value.pInt32 = pParam->maxs;
  detailParams[7].length = pParam->maxsLength;
  detailParams[7].maxLength = detailParams[6].length;

  detailParams[8].param = ML_PARAM_ENUM_VALUES_INT32_ARRAY;
  detailParams[8].value.pInt32 = pParam->enumValues;
  detailParams[8].length = pParam->enumValuesLength;
  detailParams[8].maxLength = detailParams[7].length;

  detailParams[9].param = ML_PARAM_ENUM_NAMES_BYTE_ARRAY;
  detailParams[9].value.pByte = (MLbyte*) pParam->enumNames;
  detailParams[9].length = pParam->enumNamesLength;
  detailParams[9].maxLength = detailParams[8].length;

  detailParams[10].param = ML_END;

  return mlDIcapDup( detailParams, retCap);
}

MLstatus ddIdentifyPhysicalDevice(MLDDdeviceDetails* pDevice,
				  MLint64 assignedDeviceId, MLpv** retCap)
{
  MLpv detailParams[10];
  MLint64 jackIds[10];
  MLint64 pathIds[10];
  MLint32 i;

  if( pDevice->jackLength > 10 || pDevice->pathLength > 10 )
    return ML_STATUS_INSUFFICIENT_RESOURCES;

  for( i=0; i< pDevice->jackLength; i++)
    jackIds[i] = mlDImakeJackId(assignedDeviceId, i);

  for( i=0; i< pDevice->pathLength; i++)
    pathIds[i] = mlDImakePathId(assignedDeviceId, i);

  // Fill out detailParams identifying the particular physical device

  detailParams[0].param = ML_ID_INT64;
  detailParams[0].value.int64 = assignedDeviceId;

  detailParams[1].param = ML_NAME_BYTE_ARRAY;
  detailParams[1].value.pByte = (MLbyte*)( pDevice->name );
  detailParams[1].length = (MLint32)strlen((const char *)pDevice->name) + 1;
  detailParams[1].maxLength = detailParams[1].length;

  detailParams[2].param = ML_PARENT_ID_INT64;
  detailParams[2].value.int64 = mlDIparentIdOfDeviceId(assignedDeviceId);

  detailParams[3].param = ML_DEVICE_VERSION_INT32;
  detailParams[3].value.int32 = pDevice->version;
  detailParams[3].length = 1;

  detailParams[4].param = ML_DEVICE_INDEX_INT32;
  detailParams[4].value.int32 = pDevice->index;
  detailParams[4].length = 1;

  detailParams[5].param = ML_DEVICE_LOCATION_BYTE_ARRAY;
  detailParams[5].value.pByte = (MLbyte*)( pDevice->location);
  detailParams[5].length = (MLint32)strlen(pDevice->location) + 1;
  detailParams[5].maxLength = detailParams[5].length;

  detailParams[6].param = ML_DEVICE_JACK_IDS_INT64_ARRAY;
  detailParams[6].value.pInt64 = jackIds;
  detailParams[6].length = pDevice->jackLength;
  detailParams[6].maxLength = pDevice->jackLength;

  detailParams[7].param = ML_DEVICE_PATH_IDS_INT64_ARRAY;
  detailParams[7].value.pInt64 = pathIds;
  detailParams[7].length = pDevice->pathLength;
  detailParams[7].maxLength = pDevice->pathLength;

  detailParams[8].param = ML_DEVICE_XCODE_IDS_INT64_ARRAY;
  detailParams[8].value.pInt64 = NULL;
  detailParams[8].length = 0;
  detailParams[8].maxLength = 0;

  detailParams[9].param = ML_END;

  return mlDIcapDup( detailParams, retCap);
}

MLstatus ddIdentifyJack(MLDDjackDetails* pJack,
			MLint64 objectId, MLpv** retCap)
{
  MLint64 physicalDeviceId = mlDIparentIdOfLogDevId(objectId);

  MLpv detailParams[10];
  MLint64 pathIds[10];
  MLint32 numPaths;

  for( numPaths=0; numPaths< pJack->pathLength && numPaths < 10; numPaths++)
    pathIds[numPaths] = mlDImakePathId(physicalDeviceId, 
				       pJack->pathIndexes[numPaths]);


  // Fill out detailParams identifying the particular physical device

  detailParams[0].param = ML_ID_INT64;
  detailParams[0].value.int64 = objectId;

  detailParams[1].param = ML_NAME_BYTE_ARRAY;
  detailParams[1].value.pByte = (MLbyte *)(pJack->name);
  detailParams[1].length = (MLint32)strlen(pJack->name) + 1;
  detailParams[1].maxLength = detailParams[1].length;

  detailParams[2].param = ML_PARENT_ID_INT64;
  detailParams[2].value.int64 = physicalDeviceId;

  detailParams[3].param = ML_JACK_TYPE_INT32;
  detailParams[3].value.int32 = pJack->type;

  detailParams[4].param = ML_JACK_DIRECTION_INT32;
  detailParams[4].value.int32 = pJack->direction;

  detailParams[5].param = ML_JACK_PATH_IDS_INT64_ARRAY;
  detailParams[5].value.pInt64 = pathIds;
  detailParams[5].length = numPaths;
  detailParams[5].maxLength = numPaths;

  detailParams[6].param = ML_PARAM_IDS_INT64_ARRAY;
  detailParams[6].value.pInt64 = NULL;
  detailParams[6].length = 0;
  detailParams[6].maxLength = 0;

  detailParams[7].param = ML_END;

  return mlDIcapDup( detailParams, retCap);
}

MLstatus ddIdentifyPath(MLDDpathDetails* pPath,
			MLint64 objectId, MLpv** retCap)
{
  MLint64 physicalDeviceId = mlDIparentIdOfLogDevId(objectId);
  MLint64 paramIds[100];

  MLpv detailParams[10];

  // Fill out the param ids
  {
    MLint32 i;
    for(i=0; i< pPath->paramsLength; i++)
	paramIds[i] = pPath->params[i]->id;
  }

  // Fill out detailParams identifying the particular physical device

  detailParams[0].param = ML_ID_INT64;
  detailParams[0].value.int64 = objectId;

  detailParams[1].param = ML_NAME_BYTE_ARRAY;
  detailParams[1].value.pByte = (MLbyte *)(pPath->name);
  detailParams[1].length = (MLint32)strlen(pPath->name) + 1;
  detailParams[1].maxLength = detailParams[1].length;

  detailParams[2].param = ML_PARENT_ID_INT64;
  detailParams[2].value.int64 = physicalDeviceId;

  detailParams[3].param = ML_PATH_TYPE_INT32;
  detailParams[3].value.int32 = pPath->type;

  detailParams[4].param = ML_PATH_COMPONENT_ALIGNMENT_INT32;
  detailParams[4].value.int32 = pPath->pixelLineAlignment;

  detailParams[5].param = ML_PATH_BUFFER_ALIGNMENT_INT32;
  detailParams[5].value.int32 = pPath->bufferAlignment;

  if( pPath->srcJackIndex != -1)
    {
      detailParams[6].param = ML_PATH_SRC_JACK_ID_INT64;
      detailParams[6].value.int64=mlDImakeJackId(physicalDeviceId, 
						 pPath->srcJackIndex);
    }
  else
    {
      detailParams[6].param = ML_PATH_SRC_JACK_ID_INT64;
      detailParams[6].value.int64 = 0;
    }

  if( pPath->dstJackIndex != -1)
    {
      detailParams[7].param = ML_PATH_DST_JACK_ID_INT64;
      detailParams[7].value.int64=mlDImakeJackId(physicalDeviceId, 
						 pPath->dstJackIndex);
    }
  else
    {
      detailParams[7].param = ML_PATH_DST_JACK_ID_INT64;
      detailParams[7].value.int64 = 0;
    }

  detailParams[8].param = ML_PARAM_IDS_INT64_ARRAY;
  detailParams[8].value.pInt64 = paramIds;
  detailParams[8].length = pPath->paramsLength;
  detailParams[8].maxLength = pPath->paramsLength;

  detailParams[9].param = ML_END;

  return mlDIcapDup( detailParams, retCap);
}


