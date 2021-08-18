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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ML/ml.h>
#include <ML/mlu.h>

#ifdef _MSC_VER
/* With MSVC, strcasecmp does not exist. However, _stricmp seems to do
 * pretty much the same thing, so use that instead.
 */
#define strcasecmp _stricmp
#endif


/* ---------------------------------------------------------mluFindDeviceByName
 *
 * Search through system capabilites and find a device by name, parse
 * a ":" seperating the device name from the device index, reject NULL
 * device name.
 */
MLstatus MLAPI
mluFindDeviceByName( MLint64 sysId, const char* desiredName, 
		     MLint64* retDevId )
{
  MLstatus status;
  MLpv* sysCap;
  MLpv* deviceIds;
  char *dn;

  if ( ! desiredName || *desiredName == '\0' ) {
    return ML_STATUS_INVALID_ARGUMENT;
  }

  if ( (status = mlGetCapabilities( sysId, &sysCap )) != ML_STATUS_NO_ERROR ) {
    return status;
  }

  dn = strchr( desiredName, ':' );
  if ( dn ) {
    char deviceName[ 256 ];
    MLint32 index = atoi( &dn[1] );
    strncpy( deviceName, desiredName, dn - desiredName );
    deviceName[ dn - desiredName ] = '\0';
    return mluFindDeviceByNameAndIndex( sysId, deviceName, index, retDevId );
  }

  if ( (deviceIds = mlPvFind( sysCap, ML_SYSTEM_DEVICE_IDS_INT64_ARRAY )) !=
       NULL ) {
    int d;
    for ( d=0; d< deviceIds->length; d++) {
      MLpv* devCap = NULL;
      MLpv* deviceName;
      mlGetCapabilities( deviceIds->value.pInt64[d], &devCap );
      if ( devCap != NULL &&
	   (deviceName = mlPvFind( devCap, ML_NAME_BYTE_ARRAY )) != NULL &&
	   strcasecmp((const char*)deviceName->value.pByte, desiredName)==0 ) {
	*retDevId = deviceIds->value.pInt64[d];
	mlFreeCapabilities( sysCap );
	mlFreeCapabilities( devCap );
	return ML_STATUS_NO_ERROR;
      }
      mlFreeCapabilities( devCap );
    }
  }
  mlFreeCapabilities( sysCap );
  return ML_STATUS_INVALID_ARGUMENT;
}


/* -------------------------------------------------mluFindDeviceByNameAndIndex
 *
 * Search through system capabilites and find a device by name and
 * index (Used for selecting one out of a set of identical devices)
 */
MLstatus MLAPI
mluFindDeviceByNameAndIndex( MLint64 sysId, const char* desiredName, 
			     MLint32 desiredIndex, MLint64* retDevId )
{
  int d;
  MLstatus status;
  MLpv* sysCap;
  MLpv* deviceIds;

  if ( (status = mlGetCapabilities( sysId, &sysCap )) != ML_STATUS_NO_ERROR ) {
    return status;
  }

  if ( (deviceIds = mlPvFind( sysCap, ML_SYSTEM_DEVICE_IDS_INT64_ARRAY )) !=
       NULL ) {
    for ( d=0; d< deviceIds->length; d++ ) {
      MLpv* devCap = NULL;
      MLpv* deviceName, *deviceIndex;
      mlGetCapabilities( deviceIds->value.pInt64[d], &devCap );
      if ( devCap != NULL &&
	   (deviceName = mlPvFind( devCap, ML_NAME_BYTE_ARRAY )) != NULL &&
	   strcasecmp( (const char*)deviceName->value.pByte, desiredName)==0 &&
	   (deviceIndex = mlPvFind(devCap, ML_DEVICE_INDEX_INT32)) != NULL &&
	   deviceIndex->value.int32 == desiredIndex ) {
	*retDevId = deviceIds->value.pInt64[d];
	mlFreeCapabilities(sysCap);
	mlFreeCapabilities(devCap);
	return ML_STATUS_NO_ERROR;
      }
      mlFreeCapabilities( devCap );
    }
  }
  mlFreeCapabilities( sysCap );
  return ML_STATUS_INVALID_ARGUMENT;
}


/* ---------------------------------------------------findJackByNameOrDirection
 *
 * Search through device capabilites and find the first jack with a
 * desired name or direction.
 */
static MLstatus
findJackByNameOrDirection( MLint64 devId, const char* desiredName,
			   MLint32 desiredDirection, MLint64* retJackId )
{
  MLint32 i;
  MLstatus status;
  MLpv* devCap;
  MLint64 deviceId;
  MLpv* sysCap = 0;
  MLpv* jackIds;
  char jackName[ 256 ], *jn;
  MLpv* devIds = 0;
  MLint32 deviceIndex = 0;

  /* Device name prefixed to jackname?
   */
  if ( desiredName && (jn = strchr( desiredName, '.' )) ) {
    MLstatus s; MLint64 did; char deviceName[ 256 ];

    strncpy( deviceName, desiredName, jn - desiredName );
    deviceName[ jn - desiredName ] = '\0';
    s = mluFindDeviceByName( ML_SYSTEM_LOCALHOST, deviceName, &did );
    if ( s != ML_STATUS_NO_ERROR ) {
      return s;
    }
    strncpy( jackName, &jn[1], sizeof( jackName ) );
    desiredName = jackName;
    devId = did;
  }

  for ( ;; ) {
    if ( devId ) {
      deviceId = devId;
    } else {
      if ( !sysCap ) {
	status = mlGetCapabilities( ML_SYSTEM_LOCALHOST, &sysCap );
	if ( status != ML_STATUS_NO_ERROR ) {
	  return status;
	}
	devIds = mlPvFind( sysCap, ML_SYSTEM_DEVICE_IDS_INT64_ARRAY );
	if ( !devIds ) {
	  mlFreeCapabilities( sysCap );
	  return ML_STATUS_INVALID_ARGUMENT;
	}
	deviceIndex = 0;
      }
      if ( deviceIndex < devIds->length ) {
	deviceId = devIds->value.pInt64[ deviceIndex++ ];
      } else {
	return ML_STATUS_INVALID_ARGUMENT;
      }
    }
    status = mlGetCapabilities( deviceId, &devCap );
    if ( status != ML_STATUS_NO_ERROR ) {
      return status;
    }

    if ( (jackIds = mlPvFind(devCap, ML_DEVICE_JACK_IDS_INT64_ARRAY)) != NULL){
      for ( i = 0; i < jackIds->length; i++ ) {
	MLpv* jackCap = NULL;
	MLpv* jackDirection;
	MLpv* jackName;
	mlGetCapabilities( jackIds->value.pInt64[i], &jackCap );
	if ( jackCap != NULL &&
	     (jackName = mlPvFind( jackCap, ML_NAME_BYTE_ARRAY ))!= NULL &&
	     (jackDirection = mlPvFind( jackCap, ML_JACK_DIRECTION_INT32 )) !=
	     NULL &&
	     ((desiredName != NULL && 	   
	       strcasecmp( (const char*) jackName->value.pByte,
			   desiredName ) == 0 ) ||
	      (desiredName == NULL &&
	       jackDirection->value.int32 == desiredDirection))) {
	  *retJackId = jackIds->value.pInt64[i];
	  mlFreeCapabilities( jackCap );
	  mlFreeCapabilities( devCap );
	  if ( sysCap ) {
	    mlFreeCapabilities(sysCap);
	  }
	  return ML_STATUS_NO_ERROR;
	}
	mlFreeCapabilities( jackCap );
      } /* for i=0..jackIds->length */
    }
    mlFreeCapabilities( devCap );
    if ( devId ) {
      break;
    }
  } /* for ( ;; ) */
  if ( sysCap ) {
    mlFreeCapabilities(sysCap);
  }
  return ML_STATUS_INVALID_ARGUMENT;
}


/* -------------------------------------------------------findPathByJackAndType
 *
 * Starting with a jack, find a suitable path through it.
 */
static MLstatus
findPathByJackAndType( MLint64 jackId, MLint32 desiredPathType,
		       MLint64* retPathId, MLint32* retPathAlignment )
{
  MLint32 i;
  MLstatus status;
  MLpv* jackCap;
  MLpv* pathIds;

  if ( (status = mlGetCapabilities( jackId, &jackCap ))!=ML_STATUS_NO_ERROR ) {
    return status;
  }

  if ( (pathIds = mlPvFind( jackCap, ML_JACK_PATH_IDS_INT64_ARRAY )) != NULL ){
    for (i = 0; i < pathIds->length; i++ ) {
      MLpv* pathCap = NULL;
      MLpv* pathType;
      MLpv* pathAlignment;

      mlGetCapabilities( pathIds->value.pInt64[i], &pathCap );
      if ( pathCap != NULL &&
	   (pathType = mlPvFind( pathCap, ML_PATH_TYPE_INT32 )) != NULL &&
	   (pathAlignment=mlPvFind( pathCap,
				    ML_PATH_BUFFER_ALIGNMENT_INT32 ))!=NULL &&
	   pathType->value.int32 == desiredPathType ) {
	*retPathId = pathIds->value.pInt64[i];
	if ( retPathAlignment != NULL ) {
	    *retPathAlignment = pathAlignment->value.int32;
	}
	mlFreeCapabilities( pathCap );
	mlFreeCapabilities( jackCap );
	return ML_STATUS_NO_ERROR;
      }
      mlFreeCapabilities( pathCap );
    }
  }
  mlFreeCapabilities( jackCap );
  return ML_STATUS_INVALID_ARGUMENT;
}


/* ------------------------------------------------------mluFindJackByDirection
 */
MLstatus MLAPI
mluFindJackByDirection( MLint64 devId, MLint32 desiredDirection,
			MLint64* retJackId )
{
  return findJackByNameOrDirection( devId, NULL, desiredDirection, retJackId );
}


/* -----------------------------------------------------------mluFindJackByName
 */
MLstatus MLAPI
mluFindJackByName( MLint64 devId, const char* desiredName,
		   MLint64* retJackId )
{
  return findJackByNameOrDirection( devId, desiredName, 0, retJackId );
}


/* -----------------------------------------------------------mluFindPathByName
 */
MLstatus MLAPI
mluFindPathByName( MLint64 devId, const char* desiredName, MLint64* retPathId )
{
  MLint32 i;
  MLstatus status;
  MLpv* devCap;
  MLpv* pathIds;
  
  if ( (status = mlGetCapabilities( devId, &devCap )) != ML_STATUS_NO_ERROR ) {
    return status;
  }
  
  if ( (pathIds = mlPvFind( devCap, ML_DEVICE_PATH_IDS_INT64_ARRAY )) != NULL){
    for ( i = 0; i < pathIds->length; i++ ) {
      MLpv* pathCap = NULL;
      MLpv* pathName;
      mlGetCapabilities( pathIds->value.pInt64[i], &pathCap );
      if ( pathCap != NULL &&
	   (pathName = mlPvFind( pathCap,ML_NAME_BYTE_ARRAY )) != NULL &&
	   ((desiredName != NULL && 	   
	     strcasecmp((const char*)pathName->value.pByte, desiredName)==0 )||
	    desiredName == NULL) ) {
	*retPathId = pathIds->value.pInt64[i];
	mlFreeCapabilities( pathCap );
	mlFreeCapabilities( devCap );
	return ML_STATUS_NO_ERROR;
      }
      mlFreeCapabilities( pathCap );
    }
  }
  mlFreeCapabilities( devCap );
  return ML_STATUS_INVALID_ARGUMENT;
}


/* ----------------------------------------------------------mluFindXcodeByName
 */
MLstatus MLAPI
mluFindXcodeByName( MLint64 devId, const char* desiredName,
		    MLint64* retXcodeId )
{
  MLint32 i;
  MLstatus status;
  MLpv* devCap;
  MLpv* xcodeIds;
  
  if ( (status = mlGetCapabilities( devId, &devCap )) != ML_STATUS_NO_ERROR ) {
    return status;
  }
  
  if ((xcodeIds = mlPvFind( devCap, ML_DEVICE_XCODE_IDS_INT64_ARRAY)) != NULL){
    for ( i = 0; i < xcodeIds->length; i++ ) {
      MLpv* xcodeCap = NULL;
      MLpv* xcodeName;
      mlGetCapabilities( xcodeIds->value.pInt64[i], &xcodeCap );
      if ( xcodeCap != NULL &&
	   (xcodeName = mlPvFind( xcodeCap,ML_NAME_BYTE_ARRAY )) != NULL &&
	   ((desiredName != NULL && 	   
	     strcasecmp((const char*)xcodeName->value.pByte, desiredName)==0)||
	    desiredName == NULL) ) {
	*retXcodeId = xcodeIds->value.pInt64[i];
	mlFreeCapabilities( xcodeCap );
	mlFreeCapabilities( devCap );
	return ML_STATUS_NO_ERROR;
      }
      mlFreeCapabilities( xcodeCap );
    }
  }
  mlFreeCapabilities( devCap );
  return ML_STATUS_INVALID_ARGUMENT;
}


/* ------------------------------------------------------mluFindXcodePipeByName
 */
MLstatus MLAPI
mluFindXcodePipeByName( MLint64 xcodeId, const char* desiredName,
			MLint64* retPipeId )
{
  MLint32 i;
  MLstatus status;
  MLpv* xcodeCap;
  MLpv* srcPipeIds, *destPipeIds;
  
  if ( (status = mlGetCapabilities(xcodeId, &xcodeCap)) != ML_STATUS_NO_ERROR){
    return status;
  }
  
  if ((srcPipeIds = mlPvFind( xcodeCap, ML_XCODE_SRC_PIPE_IDS_INT64_ARRAY ))
      != NULL ) {
    for ( i = 0; i < srcPipeIds->length; i++ ) {
      MLpv* pipeCap = NULL;
      MLpv* pipeName;
      mlGetCapabilities( srcPipeIds->value.pInt64[i], &pipeCap );
      if ( pipeCap != NULL &&
	   (pipeName = mlPvFind( pipeCap, ML_NAME_BYTE_ARRAY )) != NULL &&
	   ((desiredName != NULL && 	   
	     strcasecmp((const char*)pipeName->value.pByte, desiredName)==0 )||
	    desiredName == NULL) ) {
	*retPipeId = srcPipeIds->value.pInt64[i];
	mlFreeCapabilities( pipeCap );
	mlFreeCapabilities( xcodeCap );
	return ML_STATUS_NO_ERROR;
      }
      mlFreeCapabilities( pipeCap );
    }
  }

  if ( (destPipeIds = mlPvFind( xcodeCap, ML_XCODE_DEST_PIPE_IDS_INT64_ARRAY ))
       != NULL ) {
    for (i = 0; i < destPipeIds->length; i++) {
      MLpv* pipeCap = NULL;
      MLpv* pipeName;
      mlGetCapabilities( destPipeIds->value.pInt64[i], &pipeCap );
      if ( pipeCap != NULL &&
	   (pipeName = mlPvFind( pipeCap, ML_NAME_BYTE_ARRAY )) != NULL &&
	   ((desiredName != NULL && 	   
	     strcasecmp((const char*)pipeName->value.pByte, desiredName)==0 )||
	    desiredName == NULL) ) {
	*retPipeId = destPipeIds->value.pInt64[i];
	mlFreeCapabilities( pipeCap );
	mlFreeCapabilities( xcodeCap );
	return ML_STATUS_NO_ERROR;
      }
      mlFreeCapabilities( pipeCap );
    }
  }

  mlFreeCapabilities( xcodeCap );
  return ML_STATUS_INVALID_ARGUMENT;
}


/* -------------------------------------------------------mluFindFirstInputJack
 */
MLstatus MLAPI
mluFindFirstInputJack( MLint64 devId, MLint64* retJackId )
{
  return findJackByNameOrDirection( devId, NULL, ML_DIRECTION_IN, retJackId );
}


/* ------------------------------------------------------mluFindFirstOutputJack
 */
MLstatus MLAPI
mluFindFirstOutputJack( MLint64 devId, MLint64* retJackId )
{
  return findJackByNameOrDirection( devId, NULL, ML_DIRECTION_OUT, retJackId );
}


/* ---------------------------------------------------------mluFindPathFromJack
 */
MLstatus MLAPI
mluFindPathFromJack( MLint64 jackId, 
		     MLint64* retPathId, MLint32* retPathAlignment )
{
  return findPathByJackAndType( jackId, ML_PATH_TYPE_DEV_TO_MEM,
				retPathId, retPathAlignment );
}


/* -----------------------------------------------------------mluFindPathToJack
 */
MLstatus MLAPI
mluFindPathToJack( MLint64 jackId, 
		   MLint64* retPathId, MLint32* retPathAlignment )
{
  return findPathByJackAndType( jackId, ML_PATH_TYPE_MEM_TO_DEV,
				retPathId, retPathAlignment );
}
