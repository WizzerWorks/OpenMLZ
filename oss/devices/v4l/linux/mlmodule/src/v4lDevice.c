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
#include <malloc.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <linux/types.h>
#include <linux/videodev.h>

#include <ML/ml_didd.h>
#include "v4lDef.h"
#include "generic.h"
#include <ML/ml_private.h>

/* forward declaration
 */
int ddkAdvanceState(v4lOpen *pOpen, int oldState, int newState, int noaction);

/* ----------------------------------------------------convenient abbreviations
 */
#define CASE_RO(PARAM,VALUE) \
case PARAM: \
 if( VALUE != pv->value.int32 ) \
    { \
     fprintf(stderr,"[v4l param verification] error, param " #PARAM " has"\
           " value %d, but should have value %d\n",pv->value.int32,VALUE); \
     pv->length=-1; return ML_STATUS_INVALID_VALUE; \
    } \
  break

#define CASE_R(PARAM,VALUE) \
case PARAM: \
  pv->value.int32 = VALUE; \
  break

#define CASE_W(PARAM,VALUE) \
case PARAM: \
  VALUE = pv->value.int32; \
  break

#ifdef DEBUG
#include <stdlib.h>
static int debugLevel = 0;
/* error printouts */
#define DEBG1(block) if (debugLevel >= 1) { block; fflush(stdout); }
/* basic function printouts */
#define DEBG2(block) if (debugLevel >= 2) { block; fflush(stdout); }
/* detail printouts */
#define DEBG3(block) if (debugLevel >= 3) { block; fflush(stdout); }
/* extreme debug printouts */
#define DEBG4(block) if (debugLevel >= 4) { block; fflush(stdout); }
#else
#define DEBG1(block)
#define DEBG2(block)
#define DEBG3(block)
#define DEBG4(block)
#endif

/* ----------------------------------------------------------------deviceParams
 */
MLDDint32paramDetails v4lsendCount[] =
{{
  ML_QUEUE_SEND_COUNT_INT32,       
  "ML_QUEUE_SEND_COUNT_INT32",     
  ML_TYPE_INT32,   
  ML_ACCESS_RI, 
  NULL, 
  NULL, 0,
  NULL, 0,
  NULL, 0,
  NULL, 0
}};

MLDDint32paramDetails v4lreceiveCount[] =
{{
  ML_QUEUE_RECEIVE_COUNT_INT32,       
  "ML_QUEUE_RECEIVE_COUNT_INT32",     
  ML_TYPE_INT32,   
  ML_ACCESS_RI, 
  NULL, 
  NULL, 0,
  NULL, 0,
  NULL, 0,
  NULL, 0
}};

MLDDint32paramDetails v4lsendWaitable[] =
{{
  ML_QUEUE_SEND_WAITABLE_INT64,       
  "ML_QUEUE_SEND_WAITABLE_INT64",     
  ML_TYPE_INT64,
  ML_ACCESS_RI, 
  NULL, 
  NULL, 0,
  NULL, 0,
  NULL, 0,
  NULL, 0
}};

MLDDint32paramDetails v4lreceiveWaitable[] =
{{
  ML_QUEUE_RECEIVE_WAITABLE_INT64,       
  "ML_QUEUE_RECEIVE_WAITABLE_INT64",     
  ML_TYPE_INT64,
  ML_ACCESS_RI, 
  NULL, 
  NULL, 0,
  NULL, 0,
  NULL, 0,
  NULL, 0
}};

MLint32 v4lStateEnums[] =
{
  ML_DEVICE_STATE_TRANSFERRING,
  ML_DEVICE_STATE_WAITING,
  ML_DEVICE_STATE_ABORTING,
  ML_DEVICE_STATE_FINISHING,
  ML_DEVICE_STATE_READY
};

char v4lStateNames[] = 
  "ML_DEVICE_STATE_TRANSFERRING\0"
  "ML_DEVICE_STATE_WAITING\0"
  "ML_DEVICE_STATE_ABORTING\0"
  "ML_DEVICE_STATE_FINISHING\0"
  "ML_DEVICE_STATE_READY\0"
;

MLDDint32paramDetails v4ldeviceState[] =
{{
  ML_DEVICE_STATE_INT32,       
  "ML_DEVICE_STATE_INT32",     
  ML_TYPE_INT32,   
  ML_ACCESS_RWI, 
  NULL, 
  NULL, 0,
  NULL, 0,
  v4lStateEnums, sizeof(v4lStateEnums)/sizeof(MLint32),
  v4lStateNames, sizeof(v4lStateNames)
}};

/* -----------------------------------------------------------------Jack params
 */
MLint32 v4lTimingEnums[] =
{
  ML_TIMING_525,     /* NTSC */
};

char v4lTimingNames[] = 
  "ML_TIMING_525"
;

MLDDint32paramDetails v4lVideoTiming[] =
{{
  ML_VIDEO_TIMING_INT32,       
  "ML_VIDEO_TIMING_INT32",     
  ML_TYPE_INT32,   
  ML_ACCESS_RWI, 
  v4lTimingEnums, 
  NULL, 0,
  NULL, 0,
  v4lTimingEnums, sizeof(v4lTimingEnums)/sizeof(MLint32),
  v4lTimingNames, sizeof(v4lTimingNames)
}};

MLint32 v4lVideoColorspaceEnums[] =
{
  ML_COLORSPACE_CbYCr_601_HEAD,
};

char v4lVideoColorspaceNames[] =
  "ML_COLORSPACE_CbYCr_601_HEAD"
;

MLDDint32paramDetails v4lVideoColorspace[] =
{{
  ML_VIDEO_COLORSPACE_INT32,   
  "ML_VIDEO_COLORSPACE_INT32", 
  ML_TYPE_INT32,   
  ML_ACCESS_RWI,
  v4lVideoColorspaceEnums,
  NULL, 0,
  NULL, 0,
  v4lVideoColorspaceEnums, sizeof(v4lVideoColorspaceEnums)/sizeof(MLint32),
  v4lVideoColorspaceNames, sizeof(v4lVideoColorspaceNames)
}};

MLint32 v4lPrecisionVal[] = 
{ 
  8
};

MLDDint32paramDetails v4lVideoPrecision[] =
{{
  ML_VIDEO_PRECISION_INT32,     
  "ML_VIDEO_PRECISION_INT32", 
  ML_TYPE_INT32, 
  ML_ACCESS_RWI,
  v4lPrecisionVal,
  v4lPrecisionVal, sizeof(v4lPrecisionVal)/sizeof(MLint32),
  v4lPrecisionVal, sizeof(v4lPrecisionVal)/sizeof(MLint32),
  NULL, 0,
  NULL, 0
}};

/* ----------------------------------------------------------------Image params
 */
MLDDint32paramDetails v4lImageBuffer[] =
{{
  ML_IMAGE_BUFFER_POINTER,     
  "ML_IMAGE_BUFFER_POINTER", 
  ML_TYPE_BYTE_POINTER, 
  ML_ACCESS_RWBT,
  NULL,
  NULL, 0,
  NULL, 0,
  NULL, 0,
  NULL, 0
}};

MLDDint32paramDetails v4lVideoUST[] =
{{
  ML_VIDEO_UST_INT64,     
  "ML_VIDEO_UST_INT64", 
  ML_TYPE_INT64, 
  ML_ACCESS_RBT,
  NULL,
  NULL, 0,
  NULL, 0,
  NULL, 0,
  NULL, 0
}};

MLDDint32paramDetails v4lVideoMSC[] =
{{
  ML_VIDEO_MSC_INT64,     
  "ML_VIDEO_MSC_INT64", 
  ML_TYPE_INT64,
  ML_ACCESS_RBT,
  NULL,
  NULL, 0,
  NULL, 0,
  NULL, 0,
  NULL, 0
}};

MLDDint32paramDetails v4lImageBufferSize[] =
{{
  ML_IMAGE_BUFFER_SIZE_INT32,     
  "ML_IMAGE_BUFFER_SIZE_INT32", 
  ML_TYPE_INT32, 
  ML_ACCESS_RI,
  NULL,
  NULL, 0,
  NULL, 0,
  NULL, 0,
  NULL, 0
}};

MLint32 v4lImageWidthMinVal[] = 
{ 
  640
};

MLint32 v4lImageWidthMaxVal[] = 
{ 
  640
};

MLDDint32paramDetails v4lImageWidth[] =
{{
  ML_IMAGE_WIDTH_INT32,     
  "ML_IMAGE_WIDTH_INT32", 
  ML_TYPE_INT32, 
  ML_ACCESS_RWI | ML_ACCESS_RWQ,
  v4lImageWidthMinVal,
  v4lImageWidthMinVal, sizeof(v4lImageWidthMinVal)/sizeof(MLint32),
  v4lImageWidthMaxVal, sizeof(v4lImageWidthMaxVal)/sizeof(MLint32),
  NULL, 0,
  NULL, 0
}};

MLint32 v4lImageHeight1MinVal[] = 
{ 
  480
};

MLint32 v4lImageHeight1MaxVal[] = 
{ 
  480
};

MLDDint32paramDetails v4lImageHeight1[] =
{{
  ML_IMAGE_HEIGHT_1_INT32,     
  "ML_IMAGE_HEIGHT_1_INT32", 
  ML_TYPE_INT32, 
  ML_ACCESS_RWI | ML_ACCESS_RWQ,
  v4lImageHeight1MinVal,
  v4lImageHeight1MinVal, sizeof(v4lImageHeight1MinVal)/sizeof(MLint32),
  v4lImageHeight1MaxVal, sizeof(v4lImageHeight1MaxVal)/sizeof(MLint32),
  NULL, 0,
  NULL, 0
}};

MLint32 v4lZero[] = { 0 };

MLDDint32paramDetails v4lImageHeight2[] =
{{
  ML_IMAGE_HEIGHT_2_INT32,     
  "ML_IMAGE_HEIGHT_2_INT32", 
  ML_TYPE_INT32, 
  ML_ACCESS_RI | ML_ACCESS_RQ,
  v4lZero,
  v4lZero, 1,
  v4lZero, 1,
  NULL, 0,
  NULL, 0
}};

MLDDint32paramDetails v4lImageRowBytes[] =
{{
  ML_IMAGE_ROW_BYTES_INT32,     
  "ML_IMAGE_ROW_BYTES_INT32", 
  ML_TYPE_INT32, 
  ML_ACCESS_RI | ML_ACCESS_RQ,
  v4lZero,
  v4lZero, 1,
  v4lZero, 1,
  NULL, 0,
  NULL, 0
}};

MLDDint32paramDetails v4lImageSkipPixels[] =
{{
  ML_IMAGE_SKIP_PIXELS_INT32,     
  "ML_IMAGE_SKIP_PIXELS_INT32", 
  ML_TYPE_INT32, 
  ML_ACCESS_RI | ML_ACCESS_RQ,
  v4lZero,
  v4lZero, 1,
  v4lZero, 1,
  NULL, 0,
  NULL, 0
}};

MLDDint32paramDetails v4lImageSkipRows[] =
{{
  ML_IMAGE_SKIP_ROWS_INT32,     
  "ML_IMAGE_SKIP_ROWS_INT32", 
  ML_TYPE_INT32, 
  ML_ACCESS_RI | ML_ACCESS_RQ,
  v4lZero,
  v4lZero, 1,
  v4lZero, 1,
  NULL, 0,
  NULL, 0
}};

MLint32 v4lOrientationEnums[] = 
{
  ML_ORIENTATION_TOP_TO_BOTTOM,
  ML_ORIENTATION_BOTTOM_TO_TOP
};

char v4lOrientationNames[] =
{
  "ML_ORIENTATION_TOP_TO_BOTTOM\0"
  "ML_ORIENTATION_BOTTOM_TO_TOP\0"
};

MLDDint32paramDetails v4lImageOrientation[] =
{{
  ML_IMAGE_ORIENTATION_INT32,     
  "ML_IMAGE_ORIENTATION_INT32", 
  ML_TYPE_INT32, 
  ML_ACCESS_RWI | ML_ACCESS_RWQT,
  v4lOrientationEnums,
  NULL, 0,
  NULL, 0,
  v4lOrientationEnums, sizeof(v4lOrientationEnums)/sizeof(MLint32),
  v4lOrientationNames, sizeof(v4lOrientationNames)
}};

MLint32 v4lImageCompressionEnums[] = 
{
  ML_COMPRESSION_UNCOMPRESSED,
};

char v4lImageCompressionNames[] =
{
  "ML_COMPRESSION_UNCOMPRESSED"
};

MLDDint32paramDetails v4lImageCompression[] =
{{
  ML_IMAGE_COMPRESSION_INT32,     
  "ML_IMAGE_COMPRESSION_INT32", 
  ML_TYPE_INT32, 
  ML_ACCESS_RI | ML_ACCESS_RQ,
  v4lImageCompressionEnums,
  NULL, 0,
  NULL, 0,
  v4lImageCompressionEnums, sizeof(v4lImageCompressionEnums)/sizeof(MLint32),
  v4lImageCompressionNames, sizeof(v4lImageCompressionNames)
}};

MLint32 v4lTemporalSamplingEnums[] = 
{
  ML_TEMPORAL_SAMPLING_FIELD_BASED,
};

char v4lTemporalSamplingNames[] =
{
  "ML_TEMPORAL_SAMPLING_FIELD_BASED"
};

MLDDint32paramDetails v4lImageTemporalSampling[] =
{{
  ML_IMAGE_TEMPORAL_SAMPLING_INT32,     
  "ML_IMAGE_TEMPORAL_SAMPLING_INT32", 
  ML_TYPE_INT32, 
  ML_ACCESS_RI | ML_ACCESS_RQ,
  v4lTemporalSamplingEnums,
  NULL, 0,
  NULL, 0,
  v4lTemporalSamplingEnums, sizeof(v4lTemporalSamplingEnums)/sizeof(MLint32),
  v4lTemporalSamplingNames, sizeof(v4lTemporalSamplingNames)
}};

MLint32 v4lInterleaveModeEnums[] = 
{
  ML_INTERLEAVE_MODE_INTERLEAVED
};

char v4lInterleaveModeNames[] =
{
  "ML_INTERLEAVE_MODE_INTERLEAVED"
};

MLDDint32paramDetails v4lImageInterleaveMode[] =
{{
  ML_IMAGE_INTERLEAVE_MODE_INT32,     
  "ML_IMAGE_INTERLEAVE_MODE_INT32", 
  ML_TYPE_INT32, 
  ML_ACCESS_RI | ML_ACCESS_RWQ,
  v4lInterleaveModeEnums,
  NULL, 0,
  NULL, 0,
  v4lInterleaveModeEnums, sizeof(v4lInterleaveModeEnums)/sizeof(MLint32),
  v4lInterleaveModeNames, sizeof(v4lInterleaveModeNames)
}};

MLint32 v4lImagePackingEnums[] = 
{
  ML_PACKING_8,
  ML_PACKING_8_R
};

char v4lImagePackingNames[] =
{
  "ML_PACKING_8\0"
  "ML_PACKING_8_R\0"
};

MLDDint32paramDetails v4lImagePacking[] =
{{
  ML_IMAGE_PACKING_INT32,     
  "ML_IMAGE_PACKING_INT32", 
  ML_TYPE_INT32, 
  ML_ACCESS_RWI | ML_ACCESS_RWQT,
  v4lImagePackingEnums,
  NULL, 0,
  NULL, 0,
  v4lImagePackingEnums, sizeof(v4lImagePackingEnums)/sizeof(MLint32),
  v4lImagePackingNames, sizeof(v4lImagePackingNames)
}};

MLint32 v4lImageSamplingEnums[] = 
{
  ML_SAMPLING_444
};

char v4lImageSamplingNames[] =
{
  "ML_SAMPLING_444"
};

MLDDint32paramDetails v4lImageSampling[] =
{{
  ML_IMAGE_SAMPLING_INT32,     
  "ML_IMAGE_SAMPLING_INT32", 
  ML_TYPE_INT32, 
  ML_ACCESS_RI | ML_ACCESS_RQ,
  v4lImageSamplingEnums,
  NULL, 0,
  NULL, 0,
  v4lImageSamplingEnums, sizeof(v4lImageSamplingEnums)/sizeof(MLint32),
  v4lImageSamplingNames, sizeof(v4lImageSamplingNames)
}};

MLint32 v4lImageColorspaceEnums[] =
{
  ML_COLORSPACE_RGB_601_FULL
};

char v4lImageColorspaceNames[] =
  "ML_COLORSPACE_RGB_601_FULL"
;

MLDDint32paramDetails v4lImageColorspace[] =
{{
  ML_IMAGE_COLORSPACE_INT32,     
  "ML_IMAGE_COLORSPACE_INT32", 
  ML_TYPE_INT32, 
  ML_ACCESS_RI | ML_ACCESS_RWQ,
  v4lImageColorspaceEnums,
  NULL, 0,
  NULL, 0,
  v4lImageColorspaceEnums, sizeof(v4lImageColorspaceEnums)/sizeof(MLint32),
  v4lImageColorspaceNames, sizeof(v4lImageColorspaceNames)
}};

MLint32 v4lEventEnums[] = 
{
  ML_EVENT_VIDEO_SEQUENCE_LOST
};

char v4lEventNames[] =
{
  "ML_EVENT_VIDEO_SEQUENCE_LOST\0"
};

MLDDint32paramDetails v4lDeviceEvent[] =
{{
  ML_DEVICE_EVENTS_INT32_ARRAY,     
  "ML_DEVICE_EVENTS_INT32_ARRAY", 
  ML_TYPE_INT32_ARRAY, 
  ML_ACCESS_RWI,
  NULL,
  NULL, 0,
  NULL, 0,
  v4lEventEnums, sizeof(v4lEventEnums)/sizeof(MLint32),
  v4lEventNames, sizeof(v4lEventNames)
}};

MLDDint32paramDetails* pathParams[] =
{
  v4ldeviceState,
  v4lsendCount,
  v4lreceiveCount,
  v4lsendWaitable,
  v4lreceiveWaitable,
  v4lVideoTiming,
  v4lVideoColorspace,
  v4lVideoPrecision,
  v4lImageBuffer,
  v4lVideoUST,
  v4lVideoMSC,
  v4lImageBufferSize,
  v4lImageWidth,
  v4lImageHeight1,
  v4lImageHeight2,
  v4lImageRowBytes,
  v4lImageSkipPixels,
  v4lImageSkipRows,
  v4lImageOrientation,
  v4lImageCompression,
  v4lImageTemporalSampling,
  v4lImageInterleaveMode,
  v4lImagePacking,
  v4lImageSampling,
  v4lImageColorspace,
  v4lDeviceEvent
};


/* -----------------------------------------------------------ddGetCapabilities
 */
MLstatus ddGetCapabilities(MLbyte* ddDevicePriv,
			   MLint64 staticObjectId,
			   MLpv** capabilities)
{
  v4lDeviceInfo* pDevice = (v4lDeviceInfo*)ddDevicePriv;

  DEBG3(printf("[v4l mlmodule] in ddGetCap, object %" FORMAT_LLX "\n",
	       staticObjectId));

  switch( mlDIextractIdType(staticObjectId) )
    {
    case ML_REF_TYPE_DEVICE:
      {
	MLDDdeviceDetails device;
	device.name = pDevice->name;
	device.index = pDevice->index;
	device.version = 1;
	device.location = pDevice->location;
	device.jackLength = pDevice->numChannels;
	device.pathLength = pDevice->numChannels;
	return ddIdentifyPhysicalDevice(&device, staticObjectId, capabilities);
      }
    case ML_REF_TYPE_JACK:
      {
	MLDDjackDetails jack;
	MLint32 jackIndex = mlDIextractJackIndex(staticObjectId);
	if( jackIndex >= pDevice->numChannels )
	  return ML_STATUS_INVALID_ID;
	jack.name = pDevice->channels[jackIndex].name;
	jack.type = pDevice->channels[jackIndex].type;
	jack.direction = ML_DIRECTION_IN;
	jack.pathIndexes = &jackIndex;
	jack.pathLength = 1;
	return ddIdentifyJack(&jack, staticObjectId, capabilities);
      }
    case ML_REF_TYPE_PATH:
      {
	MLDDpathDetails path;
	MLint32 pathIndex = mlDIextractPathIndex(staticObjectId);
	if( pathIndex >= pDevice->numChannels )
	  return ML_STATUS_INVALID_ID;
	path.name = pDevice->channels[pathIndex].name;
	path.type = ML_PATH_TYPE_DEV_TO_MEM;
	path.pixelLineAlignment = 4;
	path.bufferAlignment = 4;
	path.params = pathParams;
	path.paramsLength = sizeof(pathParams)/sizeof(void*);
	path.srcJackIndex = pathIndex;
	path.dstJackIndex = -1;
	return ddIdentifyPath(&path, staticObjectId, capabilities);
      }
    default:
      return ML_STATUS_INVALID_ID;
    }
}


/* ---------------------------------------------------------ddPvGetCapabilities
 */
MLstatus ddPvGetCapabilities(MLbyte* ddDevicePriv,
			     MLint64 staticObjectId, MLint64 paramId,
			     MLpv** capabilities)
{
  v4lDeviceInfo* pDevice = (v4lDeviceInfo*)ddDevicePriv;
  MLint32 maxParams = sizeof(pathParams)/sizeof(MLDDint32paramDetails*);
  MLint32 p;

  if( mlDIextractIdType(staticObjectId) != ML_REF_TYPE_PATH )
    return ML_STATUS_INVALID_ID;

  if( mlDIextractPathIndex(staticObjectId) > pDevice->numChannels )
    return ML_STATUS_INVALID_ID;
  
  for(p=0; p< maxParams; p++)
    if( pathParams[p]->id == paramId )
      return ddIdentifyParam(pathParams[p], staticObjectId, capabilities);
  return ML_STATUS_INVALID_ID;
}


/* ---------------------------------------------------------processPathControls
 */
static MLstatus processPathControls(v4lOpen* pOpen, MLpv *msg)
{ 
  MLpv* pv = msg;
  v4lPathParams newParams;

  pthread_mutex_lock(&(pOpen->mutex));

  newParams = pOpen->pathParams;

  while( pv->param != ML_END )
    {
      if( ML_PARAM_GET_CLASS( pv->param) != ML_CLASS_USER )
	{
	  switch( pv->param )
	    {
	    case ML_DEVICE_EVENTS_INT32_ARRAY:
	      {
		int i;
		newParams.eventCount=0;
		for(i=0; i< pv->length; i++)
		  {
		    MLint32 newEvent = pv->value.pInt32[i];
		    switch(pv->value.pInt32[i])
		      {
		      case ML_EVENT_VIDEO_SEQUENCE_LOST:
			newParams.events[newParams.eventCount++] = newEvent;
			break;
		      default:
			fprintf(stderr, "[v4l] unsupported event\n");
			pthread_mutex_unlock(&(pOpen->mutex));
			return ML_STATUS_INVALID_VALUE;
		      }
		  }
	      }
	      break;

	    CASE_W(ML_DEVICE_STATE_INT32, newParams.deviceState);
	    CASE_W(ML_IMAGE_ORIENTATION_INT32,newParams.imageorientation);
	    CASE_W(ML_IMAGE_PACKING_INT32, newParams.imagepacking);

	    case ML_IMAGE_WIDTH_INT32:
	      if( pOpen->pathParams.deviceState != ML_DEVICE_STATE_READY &&
		  newParams.imagewidth != pv->value.int32 )
		{
		  fprintf(stderr,"[v4l param verification] error, "
			  " can't set width during transfer\n");
		  pv->length=-1; 
		  return ML_STATUS_INVALID_VALUE;
		}
	      else
		newParams.imagewidth = pv->value.int32;
	      break;
	    case ML_IMAGE_HEIGHT_1_INT32:
	      if( pOpen->pathParams.deviceState != ML_DEVICE_STATE_READY &&
		  newParams.imageheight1 != pv->value.int32 )
		{
		  fprintf(stderr,"[v4l param verification] error, "
			  " can't set height during transfer\n");
		  pv->length=-1; 
		  return ML_STATUS_INVALID_VALUE;
		}
	      else
		newParams.imageheight1 = pv->value.int32;
	      break;

	    CASE_RO(ML_IMAGE_COLORSPACE_INT32, newParams.imagecolorspace);
	    CASE_RO(ML_IMAGE_SAMPLING_INT32, newParams.imagesampling);
	    CASE_RO(ML_IMAGE_INTERLEAVE_MODE_INT32, newParams.imageinterleave);
	    CASE_RO(ML_IMAGE_HEIGHT_2_INT32,newParams.imageheight2);
	    CASE_RO(ML_IMAGE_COMPRESSION_INT32, newParams.imagecompression);
	    CASE_RO(ML_IMAGE_TEMPORAL_SAMPLING_INT32, newParams.imagetemporal);
	    CASE_RO(ML_IMAGE_ROW_BYTES_INT32,newParams.imagerowbytes);
	    CASE_RO(ML_IMAGE_SKIP_PIXELS_INT32,newParams.imageskippixels);
	    CASE_RO(ML_IMAGE_SKIP_ROWS_INT32,newParams.imageskiprows);
	    CASE_RO(ML_VIDEO_TIMING_INT32,newParams.videotiming);
	    CASE_RO(ML_VIDEO_COLORSPACE_INT32,newParams.videocolorspace);
	    CASE_RO(ML_VIDEO_PRECISION_INT32,newParams.videoprecision);
	      
	    default:
	      pv->length = -1;
			pthread_mutex_unlock( &(pOpen->mutex));
	      return ML_STATUS_INVALID_PARAMETER;
	    }
	}
      pv++;
    }

  /* Now check the new device state
   */
  {
    MLint32 newState = newParams.deviceState;
    MLint32 oldState = pOpen->pathParams.deviceState;

    if( newState != oldState )
      {
	if( ddkAdvanceState(pOpen, oldState, newState, 
			    1 /* don't actually advance, just check */) )
	  {
	    pv = mlPvFind( msg, ML_DEVICE_STATE_INT32 );
	    if( pv != NULL) 
	      pv->length = -1;
	    fprintf(stderr,"[mlDDK v4lAdvanceState] failed error check.\n");
	    pthread_mutex_unlock( &(pOpen->mutex));
	    return ML_STATUS_INVALID_VALUE;
	  }
      }

    /* All is well, so commit the param changes
     */
    pOpen->pathParams = newParams;

    pthread_mutex_unlock( &(pOpen->mutex));

    if( newState != oldState )
      {
	if( ddkAdvanceState(pOpen, oldState, newState, 
			    0 /* commit the new state */ ) )
	  {
	    fprintf(stderr,"[mlDDK v4lAdvanceState] internal error.\n");
	    return ML_STATUS_INTERNAL_ERROR;
	  }
      }
  }
  return ML_STATUS_NO_ERROR;
}

/* ----------------------------------------------------------------processFrame
 *
 * send a frame of video data to the application - this function is
 * called from the transfer thread.
 */
static int processMessage( v4lOpen* pOpen, char* buffer,
			 int size, MLint64 ust, MLint64 msc)
{
  MLint32 status;
  MLqueueEntry* entry;
  MLint32 msgType;
  MLpv* msg;
  MLpv* pv;

  status = mlDIQueueNextMessage(pOpen->pQueue, &entry,
				(enum mlMessageTypeEnum*)&msgType,
				&msg, NULL, NULL);

  /* 
   * Process any control messages which may precede the next
   * buffers message 
   */
  while(status == ML_STATUS_NO_ERROR &&
	(msgType == ML_CONTROLS_IN_PROGRESS ||
	 msgType == ML_CONTROLS_FAILED))
    {
      int reply;
      if( msgType == ML_CONTROLS_FAILED)
	{
	  /* nothing we can do except skip the message */
	  reply = ML_CONTROLS_FAILED;
	}
      else
	{
	  if( processPathControls(pOpen, msg))
	    reply = ML_CONTROLS_FAILED;
	  else
	    reply = ML_CONTROLS_COMPLETE;
	}

      if (mlDIQueueUpdateMessage(entry, reply) != ML_STATUS_NO_ERROR)
	{
	  fprintf(stderr,"[mlSDK v4lTransferThread] internal error"
		  " - process message failed to update control message\n");
	  return -1;
	}

      if (mlDIQueueAdvanceMessages(pOpen->pQueue) != ML_STATUS_NO_ERROR)
	{
	  fprintf(stderr,"[mlSDK v4lTransferThread] internal error"
		  " - process message failed to advance message\n");
	  return -1;
	}

      status = mlDIQueueNextMessage(pOpen->pQueue, &entry,
				    (enum mlMessageTypeEnum*)&msgType,
				    &msg, NULL, NULL);
    }

  if( pOpen->pathParams.deviceState == ML_DEVICE_STATE_FINISHING &&
      status == ML_STATUS_RECEIVE_QUEUE_EMPTY)
    return 1; /* all done */

  pthread_mutex_lock(&(pOpen->mutex));

  /*
   * If we're still doing a transfer, and there isn't any
   * message, then we're underflowing - send sequence lost event.
   */
  if( status == ML_STATUS_RECEIVE_QUEUE_EMPTY )
    {
      int i;
      for(i=0; i< pOpen->pathParams.eventCount; i++)
	if( pOpen->pathParams.events[i] == ML_EVENT_VIDEO_SEQUENCE_LOST)
	  {
	    MLpv eventRecord[3];
	    eventRecord[0].param = ML_VIDEO_UST_INT64;
	    eventRecord[0].value.int64 = ust;
	    eventRecord[0].length = 1;
	    eventRecord[1].param = ML_VIDEO_MSC_INT64;
	    eventRecord[1].value.int64 = msc;
	    eventRecord[1].length = 1;
	    eventRecord[2].param = ML_END;
	    mlDIQueueReturnEvent(pOpen->pQueue, 
				  ML_EVENT_VIDEO_SEQUENCE_LOST,
				  		  eventRecord);
	    break;
	  }
      pthread_mutex_unlock(&(pOpen->mutex));
      return 0;
    }

  if( status != ML_STATUS_NO_ERROR || 
      entry->messageType != ML_BUFFERS_IN_PROGRESS )
    {
      pthread_mutex_unlock(&(pOpen->mutex));
      fprintf(stderr,"[mlSDK v4lTransferThread] internal error"
	      " - unexpected message\n");
      return -1;
    }

  msg = _mlDIFifoAtOffset(q_message(pOpen->pQueue), 
			  entry->payloadDataByteOffset);
  if( msg == NULL)
    {
      fprintf(stderr,"[mlSDK v4lTransferThread] Expected msg in payload\n");
      pthread_mutex_unlock(&(pOpen->mutex));
      return -1;
    }

  if( (pv = mlPvFind(msg, ML_IMAGE_BUFFER_POINTER)) != NULL)
    {
      if( pv->maxLength < size ) 
	size = pv->maxLength;
      pv->length = size;

      if( pOpen->pathParams.imageorientation == ML_ORIENTATION_TOP_TO_BOTTOM
	  && pOpen->pathParams.imagepacking == ML_PACKING_8_R)
	{
	  /* optimal case for v4l hardware - copy whole buffer */
	  bcopy((const void*)buffer, (void*)pv->value.pByte, size);
	}
      else
	{
	  /* This is awkward, but we have no choice - v4l provides pixels
	   * in BGR format (even though it calls it RGB) and Mesa doesn't
	   * support the ABGR_EXT, so we need a conversion somewhere
	   * and, since we need to at least do a copy here anyway...
	   */
	  int lineSize = pOpen->pathParams.imagewidth*3;
	  char* from = buffer;
	  char* to = pv->value.pByte;
	  int tostep = lineSize;
	  int y;

	  if(pOpen->pathParams.imageorientation== ML_ORIENTATION_BOTTOM_TO_TOP)
	    {
	      to += lineSize*(pOpen->pathParams.imageheight1-1);
	      tostep = -lineSize;
	    }
	  for( y=0; y<pOpen->pathParams.imageheight1; y++)
	    {
	      if( pOpen->pathParams.imagepacking == ML_PACKING_8_R)
		{
		  bcopy((const void*)from, (void*)to, lineSize);
		}
	      else
		{
		  int x;
		  for(x=0; x<pOpen->pathParams.imagewidth*3; x+=3)
		    {
		      *(to+x) = *(from+x+2);
		      *(to+x+1) = *(from+x+1);
		      *(to+x+2) = *(from+x);
		    }
		}
	      from +=lineSize;
	      to += tostep;
	    }
	}
    }

  pthread_mutex_unlock(&(pOpen->mutex));

  if( (pv = mlPvFind(msg, ML_VIDEO_UST_INT64)) != NULL)
    {
      pv->value.int64 = ust;
      pv->length=1;
    }

  if( (pv = mlPvFind(msg, ML_VIDEO_MSC_INT64)) != NULL)
    {
      pv->value.int64 = msc;
      pv->length=1;
    }
	      
  if (mlDIQueueUpdateMessage(entry, ML_BUFFERS_COMPLETE) != ML_STATUS_NO_ERROR)
    {
      fprintf(stderr,"[mlSDK v4lTransferThread] internal error"
	      " - process message failed to update message\n");
      return -1;
    }

  status = mlDIQueueAdvanceMessages(pOpen->pQueue);
  if(status)
    {
      fprintf(stderr,"[mlSDK v4lTransferThread] internal error"
	      " - process message failed to advance message\n");
      fprintf(stderr,"return code %d\n", status);
      return -1;
    }
  return 0;
}


/* -----------------------------------------------------------v4lTransferThread
 */
static void* v4lTransferThread(void* mydata)
{
  v4lOpen* pOpen = (v4lOpen*)mydata;
  struct video_channel vchannel;
  struct video_picture vpicture;
  struct video_mbuf vmbuf;
  int vfd;
  
  char* buffers;
  const int vmmap_absolute_max = 20;
  struct video_mmap vmmap[vmmap_absolute_max];
  char* buffer[vmmap_absolute_max];
  int max_frames;
  int frame;

  if( (vfd = open("/dev/video0", O_RDWR)) < 0 )
    {
      perror("[mlSDK v4lTransferThread] open failed:");
      pthread_exit((void*)-1);
    }
    
  /* Select desired video channel for input
   */
  vchannel.channel=1;
  vchannel.norm = VIDEO_MODE_NTSC;

  if (ioctl(vfd, VIDIOCSCHAN, &vchannel) < 0) 
    {
      perror("[mlSDK v4lTransferThread] set channel failed:");
      pthread_exit((void*)-1);
    }

  if (ioctl(vfd, VIDIOCGPICT, &vpicture) < 0) 
    {
      perror("get picture");
      fprintf(stderr, "[v4l mlmodule] unable to get picture params\n");
      pthread_exit((void*)-1);
    }
  vpicture.depth = 24;
  vpicture.palette = VIDEO_PALETTE_RGB24;
  if (ioctl(vfd, VIDIOCSPICT, &vpicture) < 0) 
    {
      perror("set depth");
      fprintf(stderr, "[v4l mlmodule] unable to use 24-bit mode\n");
      pthread_exit((void*)-1);
    }

  /* Prepare buffers
   */
  if (ioctl(vfd,VIDIOCGMBUF, &vmbuf))
    {
      perror("[mlSDK v4lTransferThread] VIDIOCGMBUF failed:");
      pthread_exit((void*)-1);
    }

  if( vmbuf.frames < vmmap_absolute_max )
    max_frames = vmbuf.frames;
  else
    max_frames = vmmap_absolute_max;

  buffers = mmap(0, vmbuf.size, PROT_READ|PROT_WRITE, MAP_SHARED, vfd, 0);

  if( buffers == NULL )
    {
      perror("[mlSDK v4lTransferThread] mmap failed:");
      pthread_exit((void*)-1);
    }

  /* Process any control changes waiting on the send queue */
  /* processMessage(pOpen, NULL, 0, 0, 0);*/
  
  /* Preroll buffers
   */
  for(frame=0; frame<max_frames; frame++)
    {
      pthread_mutex_lock(&(pOpen->mutex));
      vmmap[frame].width=pOpen->pathParams.imagewidth;
      vmmap[frame].height=pOpen->pathParams.imageheight1;
      vmmap[frame].format=VIDEO_PALETTE_RGB24;
      vmmap[frame].frame=frame;
      pthread_mutex_unlock(&(pOpen->mutex));

      buffer[frame] = buffers+vmbuf.offsets[frame];

      /* Ask for a capture into this buffer
       */
      if (ioctl(vfd,VIDIOCMCAPTURE, &vmmap[frame]))
	{
	  perror("[mlSDK v4lTransferThread] VIDIOCMCAPTURE failed");
	  pthread_exit((void*)-1);
	}
    }
	
  /* Now begin the transfer.
   */
  {
    MLint64 ust;
    MLint64 nanoseconds = 1e9;
    MLint64 timePerFrame = (MLint64)((double)1.0/(double)29.97*nanoseconds);
    MLint64 msc=100; /* arbitrary starting value */
    frame=0;
    while( pOpen->pathParams.deviceState == ML_DEVICE_STATE_TRANSFERRING ||
	   pOpen->pathParams.deviceState == ML_DEVICE_STATE_FINISHING )
      {
	/* wait for the desired frame to complete
	 */
	if( ioctl(vfd,VIDIOCSYNC, &vmmap[frame]))
	  perror("VIDIOCSYNC");

	/* We don't have a real UST clock in HW on v4l devices, so the
	 * best we can do is guess - take the current system ust time
	 * now (after the frame is done), subtract the time for one
	 * frame to guess the time for the start of this frame
	 */
	mlGetSystemUST(ML_SYSTEM_LOCALHOST, &ust);
	ust -= timePerFrame;

	/* iff this frame matches the size the user requested, then
	 * send this completed frame to the user app.
	 *
	 * A side-effect of this is that, if the user changes image
	 * size during a transfer, then there may be a delay (and
	 * frames may be dropped) but its guaranteed that the next
	 * buffer to be received will match up.
	 */
	if( processMessage(pOpen, buffer[frame],
			 vmbuf.size/vmbuf.frames, ust, msc))
	  break;

	/* now send the frame back to be reused
	 */
	pthread_mutex_lock(&(pOpen->mutex));
	vmmap[frame].width=pOpen->pathParams.imagewidth;
	vmmap[frame].height=pOpen->pathParams.imageheight1;
	pthread_mutex_unlock(&(pOpen->mutex));
	if (ioctl(vfd,VIDIOCMCAPTURE, &vmmap[frame]))
	  {
	    perror("[mlSDK v4lTransferThread] VIDIOCMCAPTURE failed");
	    pthread_exit((void*)-1);
	  }

	msc += 2; /* msc counts fields, not buffers or frames. */
	frame = (frame+1) % max_frames;
      }
  }

  /* Tidy up and exit
   */
  munmap(buffers, vmbuf.size);

  close(vfd);

  pthread_exit((void*)0);
}


/* -------------------------------------------------------------ddkAdvanceState
 */
int ddkAdvanceState(v4lOpen *pOpen, int oldState, int newState, int noaction)
{
  enum actions 
  { 
    FINISH=1, 
    ABORT=2, 
    STARTWAIT=3, 
    STARTTRANSFER=4, 
    SUSPENDTRANSFER=5, 
    RESUMETRANSFER=6,
    FLUSH = 7,
  };
  MLint32 requiredAction=0;

  switch( oldState)
    {
    case ML_DEVICE_STATE_TRANSFERRING:
      if( newState == ML_DEVICE_STATE_ABORTING )
	requiredAction = ABORT; 
      else if( newState == ML_DEVICE_STATE_FINISHING )
	requiredAction = FINISH;
      else if( newState == ML_DEVICE_STATE_WAITING )
	requiredAction = SUSPENDTRANSFER;
      else
	{
	  fprintf(stderr,
		  "[mlSDK v4ladvanceState] can't switch from %d to %d\n",
		  oldState, newState);
	  return -1;
	}
      break;

    case ML_DEVICE_STATE_WAITING:
      if( newState == ML_DEVICE_STATE_ABORTING )
	requiredAction = ABORT; 
      else if( newState == ML_DEVICE_STATE_FINISHING )
	requiredAction = FINISH;
      else if( newState == ML_DEVICE_STATE_TRANSFERRING )
	requiredAction = RESUMETRANSFER;
      else
	{
	  fprintf(stderr,
		  "[mlSDK v4ladvanceState] can't switch from %d to %d\n",
		  oldState, newState);
	  return -1;
	}
      break;

    case ML_DEVICE_STATE_ABORTING:
    case ML_DEVICE_STATE_FINISHING:
      fprintf(stderr,"[mlSDK v4ladvanceState] can't switch from %d to %d\n",
	      oldState, newState);
      return -1;

    case ML_DEVICE_STATE_READY:
      if( newState == ML_DEVICE_STATE_WAITING )
	requiredAction = STARTWAIT;
      else if( newState == ML_DEVICE_STATE_TRANSFERRING )
	requiredAction = STARTTRANSFER;
      else if( newState == ML_DEVICE_STATE_ABORTING )
	requiredAction = FLUSH;
      else if( newState == ML_DEVICE_STATE_FINISHING )
	return 0;
      else
	{
	  fprintf(stderr,
		  "[mlSDK v4ladvanceState] can't switch from %d to %d\n",
		  oldState, newState);
	  return -1;
	}
      break;
    default: 
      {
	fprintf(stderr,"[mlSDK v4ladvanceState] unknown state %d\n",
		oldState);
	return -1;
      }
    }

  if( noaction )
    return 0;

  switch( requiredAction )
    {
    case STARTTRANSFER:
      {
	if( pthread_create(&(pOpen->thread), NULL, 
			   v4lTransferThread, (void*)pOpen))
	  {
	    perror("[mlSDK v4lAdvanceState] pthread_create failed\n");
	    return ML_STATUS_INTERNAL_ERROR;
	  }
      }
      break;
    case SUSPENDTRANSFER:
      /* Don't need to do anything here - the transfer thread will
       * find out when the params change
       */
      break;
    case RESUMETRANSFER:
      /* Don't need to do anything here - the transfer thread will
       * find out when the params change
       */
      break;
    case FINISH:
    case ABORT:
      {
	/* device state has been set already, so we just need
	 * to let the transfer thread detect the change and
	 * tidy up
	 */
	void* ret;
	pthread_join(pOpen->thread, &ret);
      }
      /* no break */
    case FLUSH:
      {
	mlDIQueueAbortMessages(pOpen->pQueue);
	pOpen->pathParams.deviceState = ML_DEVICE_STATE_READY;
      }
      break;
    default:
      break;
    }
  return 0; 
}


/* ----------------------------------------------------------------------ddOpen
 */
MLstatus ddOpen(MLbyte* ddDevicePriv,
		MLint64  staticObjectId,
		MLopenid openObjectId,
		MLpv*    openOptions,
		MLbyte** retddPriv)
{
  MLqueueOptions qOpt;
  MLopenOptions oOpt;
  MLint32 pathIndex;
  v4lOpen* pOpen;

  if( mlDIextractIdType( staticObjectId ) != ML_REF_TYPE_PATH )
    return ML_STATUS_INVALID_ID; /* this module only supports opening paths */

  pOpen = (v4lOpen*)malloc( sizeof( v4lOpen) );
  if( pOpen == NULL )
    return ML_STATUS_OUT_OF_MEMORY;

  pathIndex = mlDIextractPathIndex( staticObjectId );

  /* Parse open options
   */
  if (mlDIparseOpenOptions(openObjectId, openOptions, &oOpt
			   ) != ML_STATUS_NO_ERROR) {
    return ML_STATUS_INVALID_PARAMETER;
  }

  /* Setup a single queue between us and the DI layer.
   * Get queue options from open options.
   */
  memset(&qOpt, 0, sizeof(qOpt));
  qOpt.sendSignalCount = oOpt.sendSignalCount;
  qOpt.sendMaxCount = oOpt.sendQueueCount;
  qOpt.receiveMaxCount = oOpt.receiveQueueCount;
  qOpt.eventMaxCount = oOpt.eventPayloadCount;
  qOpt.messagePayloadSize = oOpt.messagePayloadSize;
  qOpt.ddMessageSize = 0;
  qOpt.ddEventSize = sizeof(MLpv) * 4;
  qOpt.ddAlignment = 4;
    
  /* Now we can make a queue.
   */
  if (mlDIQueueCreate(0, 0, &qOpt, &(pOpen->pQueue)) != ML_STATUS_NO_ERROR) 
    {
      free(pOpen);
      return ML_STATUS_OUT_OF_MEMORY;
    }
    
  /* open any dd stuff, creating the queue */

  pOpen->pathParams.videotiming = *(v4lVideoTiming->deflt);
  pOpen->pathParams.videoprecision = *(v4lVideoPrecision->deflt);
  pOpen->pathParams.videocolorspace = *(v4lVideoColorspace->deflt);

  pOpen->pathParams.imagewidth = *(v4lImageWidth->deflt);
  pOpen->pathParams.imageheight1 = *(v4lImageHeight1->deflt);
  pOpen->pathParams.imageheight2 = *(v4lImageHeight2->deflt);
  pOpen->pathParams.imagecompression = *(v4lImageCompression->deflt);
  pOpen->pathParams.imagetemporal = *(v4lImageTemporalSampling->deflt);
  pOpen->pathParams.imageinterleave = *(v4lImageInterleaveMode->deflt);
  pOpen->pathParams.imagerowbytes = *(v4lImageRowBytes->deflt);
  pOpen->pathParams.imageskiprows = *(v4lImageSkipRows->deflt);
  pOpen->pathParams.imageskippixels = *(v4lImageSkipPixels->deflt);

  pOpen->pathParams.imageorientation = *(v4lImageOrientation->deflt);
  pOpen->pathParams.imagecolorspace = *(v4lImageColorspace->deflt);
  pOpen->pathParams.imagepacking = *(v4lImagePacking->deflt);
  pOpen->pathParams.imagesampling = *(v4lImageSampling->deflt);

  pOpen->pathParams.eventCount = 0;

  /* device is inactive
   */
  pOpen->pathParams.deviceState = ML_DEVICE_STATE_READY;

  pthread_mutex_init(&(pOpen->mutex), NULL);

  *retddPriv = (MLbyte*)pOpen;

  return ML_STATUS_NO_ERROR;
}


/* ---------------------------------------------------------------ddSetControls
 */
MLstatus ddSetControls(MLbyte* ddPriv,
		       MLopenid openObjectId,
		       MLpv *controls)
{
  v4lOpen* pOpen = (v4lOpen*)ddPriv;
  if( pOpen == NULL )
    return ML_STATUS_INTERNAL_ERROR;

  return processPathControls(pOpen, controls);
}


/* ---------------------------------------------------------------ddGetControls
 */
MLstatus ddGetControls(MLbyte* ddPriv,
		       MLopenid openObjectId,
		       MLpv *controls)
{
  v4lOpen* pOpen = (v4lOpen*)ddPriv;
  MLpv* pv = controls;
  int i;

  if( pOpen == NULL )
    return ML_STATUS_INTERNAL_ERROR;

  while( pv->param != ML_END )
    {
      if( ML_PARAM_GET_CLASS( pv->param) != ML_CLASS_USER )
	{
	  switch( pv->param )
	    {
	    case ML_DEVICE_EVENTS_INT32_ARRAY:
	      if( pv->maxLength==0)
		{
		  /* Querying just the length of the array
		   */
		  pv->maxLength = V4L_MAX_EVENTS;
		  pv->length = 0;
		  break;
		}
	      if( pv->maxLength < pOpen->pathParams.eventCount )
		{
		  pv->length = -1;
		  return ML_STATUS_INVALID_VALUE; /* insufficient space */
		}
	      for( i=0; i< pOpen->pathParams.eventCount; i++)
		pv->value.pInt32[i] = pOpen->pathParams.events[i];
	      pv->length = pOpen->pathParams.eventCount;
	      break;
	    
	    CASE_R(ML_DEVICE_STATE_INT32, pOpen->pathParams.deviceState);
	    CASE_R(ML_IMAGE_ORIENTATION_INT32,
		   pOpen->pathParams.imageorientation);
	    CASE_R(ML_IMAGE_COLORSPACE_INT32,
		   pOpen->pathParams.imagecolorspace);
	    CASE_R(ML_IMAGE_SAMPLING_INT32, pOpen->pathParams.imagesampling);
	    CASE_R(ML_IMAGE_PACKING_INT32, pOpen->pathParams.imagepacking);
	    CASE_R(ML_IMAGE_INTERLEAVE_MODE_INT32,
		   pOpen->pathParams.imageinterleave);
	    CASE_R(ML_IMAGE_WIDTH_INT32,pOpen->pathParams.imagewidth);
	    CASE_R(ML_IMAGE_HEIGHT_1_INT32,pOpen->pathParams.imageheight1);
	    CASE_R(ML_IMAGE_HEIGHT_2_INT32,pOpen->pathParams.imageheight2);
	    CASE_R(ML_IMAGE_COMPRESSION_INT32,
		   pOpen->pathParams.imagecompression);
	    CASE_R(ML_IMAGE_TEMPORAL_SAMPLING_INT32,
		   pOpen->pathParams.imagetemporal);
	    CASE_R(ML_IMAGE_ROW_BYTES_INT32,pOpen->pathParams.imagerowbytes);
	    CASE_R(ML_IMAGE_SKIP_PIXELS_INT32,
		   pOpen->pathParams.imageskippixels);
	    CASE_R(ML_IMAGE_SKIP_ROWS_INT32,pOpen->pathParams.imageskiprows);
	    CASE_R(ML_VIDEO_TIMING_INT32,pOpen->pathParams.videotiming);
	    CASE_R(ML_VIDEO_COLORSPACE_INT32,
		   pOpen->pathParams.videocolorspace);
	    CASE_R(ML_VIDEO_PRECISION_INT32,pOpen->pathParams.videoprecision);

	    case ML_QUEUE_SEND_COUNT_INT32:
	      pv->value.int32 = mlDIQueueGetSendCount(pOpen->pQueue);
	      pv->length=1;
	      break;
                 
	    case ML_QUEUE_RECEIVE_COUNT_INT32:
	      pv->value.int32 = mlDIQueueGetReceiveCount(pOpen->pQueue);
	      pv->length=1;
	      break;

	    case ML_QUEUE_SEND_WAITABLE_INT64:
	      pv->value.int64 =
		(MLint64)mlDIQueueGetSendWaitable(pOpen->pQueue);
	      pv->length=1;
	      break;

	    case ML_QUEUE_RECEIVE_WAITABLE_INT64:
	      pv->value.int64 =
		(MLint64)mlDIQueueGetReceiveWaitable(pOpen->pQueue);
	      pv->length=1;
	      break;

	    case ML_IMAGE_BUFFER_SIZE_INT32:
	      pv->value.int32 = 640*480*3; /* FIXME: will need to handle in
					    * more general way once more
					    * param options are possible */
	      pv->length=1;
	      break;
                 
	    default:
	      pv->length = -1;
	      return ML_STATUS_INVALID_PARAMETER;
	    }
	}
      pv++;
    }
  return ML_STATUS_NO_ERROR; 
}


/* --------------------------------------------------------------ddSendControls
 */
MLstatus ddSendControls(MLbyte* ddPriv,
			MLopenid openObjectId,
			MLpv *controls)
{
  v4lOpen* pOpen = (v4lOpen*)ddPriv;
  if( pOpen == NULL )
    return ML_STATUS_INTERNAL_ERROR;

  return mlDIQueuePushMessage(pOpen->pQueue,
			      ML_CONTROLS_IN_PROGRESS,
			      controls,
			      0, 0, 0);
}


/* -------------------------------------------------------------ddQueryControls
 */
MLstatus ddQueryControls(MLbyte* ddPriv,
			 MLopenid openObjectId,
			 MLpv *controls)
{
  v4lOpen* pOpen = (v4lOpen*)ddPriv;
  if( pOpen == NULL )
    return ML_STATUS_INTERNAL_ERROR;

  return ML_STATUS_INTERNAL_ERROR; /* not implemented yet */
}


/* ---------------------------------------------------------------ddSendBuffers
 */
MLstatus ddSendBuffers(MLbyte* ddPriv,
		       MLopenid openObjectId,
		       MLpv *buffers)
{
  v4lOpen* pOpen = (v4lOpen*)ddPriv;
  if( pOpen == NULL )
    return ML_STATUS_INTERNAL_ERROR;

  return mlDIQueuePushMessage(pOpen->pQueue,
			      ML_BUFFERS_IN_PROGRESS,
			      buffers,
			      0, 0, 0);
}


/* ------------------------------------------------------------ddReceiveMessage
 */
MLstatus ddReceiveMessage(MLbyte* ddPriv,
			  MLopenid openObjectId,
			  MLint32 *retMsgType, 
			  MLpv **retReply)
{
  v4lOpen* pOpen = (v4lOpen*)ddPriv;
  if( pOpen == NULL )
    return ML_STATUS_INTERNAL_ERROR;

    /* Just ask our queue to return the next message to the app.
     */
  return mlDIQueueReceiveMessage(pOpen->pQueue,
				 (enum mlMessageTypeEnum*)retMsgType,
				 retReply,
				 0, 0);
}


/* -----------------------------------------------------------------ddXcodeWork
 */
MLstatus ddXcodeWork(MLbyte* ddPriv,
		     MLopenid openObjectId)
{
  return ML_STATUS_INTERNAL_ERROR; /* not applicable for paths */
}


/* ---------------------------------------------------------------------ddClose
 */
MLstatus ddClose(MLbyte* ddPriv,
		 MLopenid openObjectId)
{
  v4lOpen* pOpen = (v4lOpen*)ddPriv;
  if( pOpen == NULL )
    return ML_STATUS_INTERNAL_ERROR;

  if(pOpen != NULL )
    {
      /* Force an end-transfer
       */
      MLpv msg[2];
      msg[0].param = ML_DEVICE_STATE_INT32;
      msg[0].value.int32 = ML_DEVICE_STATE_ABORTING;
      msg[1].param = ML_END;
      processPathControls(pOpen, msg);

      pthread_mutex_destroy(&(pOpen->mutex));
      
      mlDIQueueDestroy(pOpen->pQueue);
      free(pOpen);
    }

  return ML_STATUS_NO_ERROR;
}

MLphysicalDeviceOps ddOps =
{
  sizeof(MLphysicalDeviceOps),
  ML_DI_DD_ABI_VERSION,
  ddGetCapabilities,
  ddPvGetCapabilities,
  ddOpen,
  ddSetControls,
  ddGetControls,
  ddSendControls,
  ddQueryControls,
  ddSendBuffers,
  ddReceiveMessage,
  ddXcodeWork,
  ddClose
};
  

/* -------------------------------------------------------------------ddConnect
 *
 * This function must be provided by all device-dependent modules
 */
MLstatus ddConnect(MLbyte *physicalDeviceCookie,
		   MLint64 staticDeviceId,
		   MLphysicalDeviceOps *pOps,
		   MLbyte** ddDevicePriv)
{
#ifdef DEBUG
  char *e = getenv("MLVDEBUG");
  if (e) debugLevel = atoi(e);
  DEBG1(printf("[v4l mlmodule] debug level %d\n", debugLevel));
#endif

  DEBG3(printf("[v4l mlmodule] connecting to device %" FORMAT_LLD "\n", staticDeviceId));

  /* The physicalDeviceCookie will match that passed in the
   * call to NewPhysicalDevice.  
   * In this case, since we only support one type of physical device, 
   * there's no ambiguity, and we don't need the cookie.
   */
  *pOps = ddOps;

  /* Here we could further interrogate the device, but in this
   * case everything we need for the subsequent capabilities calls 
   * was stored at ddInterrogate time.  So just remember that here.
   */
  {
    v4lDeviceInfo* privateInfo = (v4lDeviceInfo*)malloc(sizeof(v4lDeviceInfo));
    if( privateInfo == NULL )
      return ML_STATUS_OUT_OF_MEMORY;

    *privateInfo = *((v4lDeviceInfo*)physicalDeviceCookie);
    *ddDevicePriv = (MLbyte*)privateInfo;
  }

  return ML_STATUS_NO_ERROR;
}


/* ---------------------------------------------------------------ddInterrogate
 *
 * This function must be provided by all device-dependent modules
 */
MLstatus ddInterrogate(MLsystemContext systemContext,
		       MLmoduleContext moduleContext)
{
  int status;
  int deviceNum;
  int channelNum;

#ifdef DEBUG
  char *e = getenv("MLVDEBUG");
  if (e) debugLevel = atoi(e);
  DEBG1(printf("[v4l mlmodule] debug level %d\n", debugLevel));
#endif

  for( deviceNum=0; deviceNum<4; deviceNum++)
    {	
      v4lDeviceInfo device;
      int vfd;
      struct video_capability vcap;
      struct video_picture vpicture;

      sprintf(device.location, "/dev/video%d", deviceNum);

      if ((vfd = open(device.location, O_RDWR)) < 0) 
	continue;
  
      if (ioctl(vfd, VIDIOCGCAP, &vcap) < 0) {
	perror("get capabilities");
	close(vfd);
	return -1;
      }

      if( (vcap.type & VID_TYPE_CAPTURE) != VID_TYPE_CAPTURE )
	{
	  fprintf(stderr, "[v4l mlmodule] sorry, the v4l interface does"
		  " not support captures to memory on this card.\n");
	  continue;
	}

      if (ioctl(vfd, VIDIOCGPICT, &vpicture) < 0) 
	{
	  perror("get picture");
	  fprintf(stderr, "[v4l mlmodule] unable to get picture params\n");
	  fprintf(stderr, "               skipping device.\n");
	  close(vfd);
	  continue;
	}
      vpicture.depth = 24;
      vpicture.palette = VIDEO_PALETTE_RGB24;
      if (ioctl(vfd, VIDIOCSPICT, &vpicture) < 0) 
	{
	  perror("set depth");
	  fprintf(stderr, "[v4l mlmodule] unable to use 24-bit mode\n");
	  fprintf(stderr, "               skipping device\n");
	  close(vfd);
	  continue;
	}

      device.size = sizeof(device);
      sprintf(device.name, "%s via v4l", vcap.name);
      device.index = deviceNum;
      device.numChannels = 0;
      for( channelNum=0; channelNum<vcap.channels; channelNum++)
	{	
	  struct video_channel vchannel;

	  vchannel.channel = channelNum;
  
	  if (ioctl(vfd, VIDIOCGCHAN, &vchannel) < 0) {
	    fprintf(stderr,"[v4l dmlodule] channel %d cap ioctl failed\n",
		    channelNum);
	    continue;
	  }

	  if( (vchannel.flags & VIDEO_TYPE_TV) == VIDEO_TYPE_TV)
	    {
	      /* this chanel is of type TV, we deal only with direct
	       * video input, so skip this one.
	       */
	      fprintf(stderr,"[v4l dmlodule] channel is of type TV, ignore\n");
	      continue;
	    }
	  
	  strcpy(device.channels[device.numChannels].name,vchannel.name);

	  if( (strstr(vchannel.name, "s") != NULL ||
	       strstr(vchannel.name, "S") != NULL) &&
	      (strstr(vchannel.name, "ideo") != NULL ||
	       strstr(vchannel.name, "IDEO") != NULL))
	    device.channels[device.numChannels].type = ML_JACK_TYPE_SVIDEO;
	  else
	    device.channels[device.numChannels].type = ML_JACK_TYPE_COMPOSITE;
	  device.numChannels++;
	}
      close(vfd);

      if( device.numChannels == 0)
	{
	  fprintf(stderr, "[v4l mlmodule] no suitable channels\n");
	  fprintf(stderr, "               skipping device\n");
	  continue;
	}

      status = mlDINewPhysicalDevice(systemContext,
				     moduleContext,
				     (MLbyte*)(&device),
				     sizeof(device));
      if( status != ML_STATUS_NO_ERROR )
	return status;

      DEBG3(printf("[v4l mlmodule] added new physical device\n"));
    } /* end foreach physical device */

  return ML_STATUS_NO_ERROR;
}

