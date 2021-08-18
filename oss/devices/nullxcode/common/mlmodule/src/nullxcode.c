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
 * Additional Notice Provisions:
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

/* Example single-stream transcoder.  Does no particularly useful work, but
 * does illustrate how to connect a device to mlSDK's device independent layer.
 *
 * Illustrates the following:
 *   - how to connect a single-stream transcoder to the mlSDK DI layer.
 *   - providing multiple trancoder engines
 *   - synchronous and asynchronous operation
 *   - parallelization
 *
 * Two transcoder engines are supplied, one which simply copies the input
 * to the output, and another which clears the output.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <errno.h>

#include "ML/ml.h"
#include "ML/ml_didd.h"
#include "ML/ml_oswrap.h"
#include "ML/ml_private.h"

#include <assert.h>

#ifndef ML_OS_NT
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/dir.h>
#include <sys/time.h>
#endif
#include <sys/types.h>

#include "nullxcode.h"
#include "params.h"

/* A useful function for debug printfs...
 */
static void
debug( char* fmt, ... )
{
  if ( getenv( "MLMODULE_DEBUG" ) ) {
    va_list args;
    va_start( args, fmt );
    vfprintf( stderr, fmt, args );
    va_end( args );
  }
}


/* Some useful macros used in the switch statements in ddGetControls()
 * and processControls()
 */
#define PIPE_CHECK \
    if ( ! pInOut ) { \
        fprintf( stderr, "[mlSDK nullxcode param verification] error, "\
                "no pipe selected\n" ); \
        pv->length = -1;\
        return ML_STATUS_INVALID_VALUE; \
    } 

#define CASE_RO(PARAM,VALUE) \
case PARAM: \
    PIPE_CHECK; \
    if ( (VALUE) != pv->value.int32 ) { \
        fprintf( stderr, "[mlSDK nullxcode param verification] error, param " \
                 #PARAM " has value %d, but should have value %d\n", \
                 pv->value.int32, (VALUE) ); \
        pv->length = -1;\
        return ML_STATUS_INVALID_VALUE; \
    } \
    break

#define CASE_R(PARAM,VALUE) \
case PARAM: \
    PIPE_CHECK; \
    pv->value.int32 = (VALUE); \
    break

#define CASE_W(PARAM,VALUE) \
case PARAM: \
    PIPE_CHECK; \
    (VALUE) = pv->value.int32; \
    break


/* Strings that we return to the DI layer.
 * These change for a different transcoder.
 */
#define MY_COOKIE "nullXcode"
#define MY_NAME "nullXcode"
#define MY_LOCATION "Software Null Xcode Device"
#define MY_SRC_PIPE_NAME "nullXcodeInputPipe"
#define MY_DST_PIPE_NAME "nullXcodeOutputPipe"

static char* engineNames[2] = {
    "nullXcodeMemoryToMemoryCopy",
    "nullXcodeMemoryClear"
};



/* Forward declarations of all functions.
 *
 * Note that only ddInterrogate() and ddConnect() are visible to the
 * outside.  They are the entry points required by mlSDK.  The DI layer
 * accesses all other functions via the dispatch table returned to
 * the DI layer by ddConnect().
 */

/* Functions exported to the DI layer.  All transcoders must provide 
 * these entry-points.  These should require only minimal changes
 * for a different transcoder.
 */
MLstatus ddInterrogate( MLsystemContext systemContext,
			MLmoduleContext moduleContext );

MLstatus ddConnect( MLbyte* physicalDeviceCookie,
		    MLint64 staticDeviceId,
		    MLphysicalDeviceOps* pOps,
		    MLbyte** retDevicePriv );

/* Callbacks that ddConnect() registers with the DI layer.  All
 * transcoders must register a complete set of callbacks 
 * in ddConnect().
 */
static MLstatus ddGetCapabilities( MLbyte* devicePriv,
				   MLopenid staticObjectId,
				   MLpv** cap );

static MLstatus ddPvGetCapabilities( MLbyte* devicePriv,
				     MLopenid staticObjectId,
				     MLint64 paramId,
				     MLpv** cap );

static MLstatus ddOpen( MLbyte* devicePriv,
			MLopenid staticObjectId,
			MLopenid openObjectId,
			MLpv* openOptions,
			MLbyte** retddPriv );

static MLstatus ddSetControls( MLbyte* ddPriv,
			       MLopenid openObjectId,
			       MLpv* controls );

static MLstatus ddGetControls( MLbyte* ddPriv,
			       MLopenid openObjectId,
			       MLpv* controls );

static MLstatus ddSendControls( MLbyte* ddPriv,
				MLopenid openObjectId,
				MLpv* controls );

static MLstatus ddQueryControls( MLbyte* ddPriv,
				 MLopenid openObjectId,
				 MLpv* controls );

static MLstatus ddSendBuffers( MLbyte* ddPriv,
			       MLopenid openObjectId,
			       MLpv* buffers );

static MLstatus ddReceiveMessage( MLbyte* ddPriv,
				  MLopenid openObjectId,
				  MLint32* retMsgType,
				  MLpv** retReply );

static MLstatus ddXcodeWork( MLbyte* ddPriv,
			     MLopenid openObjectId );

static MLstatus ddClose( MLbyte* ddPriv,
			 MLopenid openObjectId );

/* Internal helper functions.  They may require some
 * changes for a different transcoder.
 */
static MLint32 getState( MLDDopen* pOpen );
static int advanceState( MLDDopen* pOpen,
			 int oldState,
			 int newState,
			 int noaction );
static _mlOSThreadRetValue workThread( void* data );

static int processBuffer( MLDDopen* pOpen );
static MLDDxcodeParams* handleSelectId( MLDDopen* pOpen,
					MLint64 val );
static MLstatus lookupCapability( MLDDparamDetails* paramList[],
				  MLint64 paramId,
				  MLpv** capability );

/* These are the functions that performs the real work of 
 * the transcoder.  They will certainly change for a
 * different transcoder.
 */
static bool verifyControls( MLDDxcodeParams* in,
			    MLDDxcodeParams* out );
static MLstatus processControls( MLDDopen* pOpen,
				 MLpv* msg,
				 bool validateMsg );
static bool transformBuffer( MLDDopen* pOpen,
			     MLbyte* inBuf,
			     MLint32 inSize,
			     MLDDxcodeParams* inParams,
			     MLbyte* outBuf,
			     MLint32 outSize,
			     MLDDxcodeParams* outParams );
static MLstatus computeImageBufferSize( MLDDxcodeParams* pInOut,
					MLint32* rval );

static int getNumProcessors( void );

/*
 *---------------- Entry points visible to the DI layer ---------------------
 */


/* ---------------------------------------------------------------ddInterrogate
 *
 * ddInterrogate() is the first entry-point in every device-dependent module.
 * Its job is to register the capabilities of each physical device using the 
 * DI callback mlDINewPhysicalDevice(3ml).
 *
 * ddInterrogate() is passwd two arguments, both of which are opaque references
 * used by the DI layer.  These opaque reference should be passed in the call
 * to mlDINewPhysicalDevice() without change.
 *
 * ddInterrogate() must only register the device.  It should not reserve or
 * allocate resources.  Those activities occur in ddConnect().
 */
MLstatus
ddInterrogate( MLsystemContext systemContext,
	       MLmoduleContext moduleContext )
{
  char* my_cookie = MY_COOKIE;
  
  return mlDINewPhysicalDevice( systemContext, moduleContext,
				(MLbyte*) my_cookie, 
				(MLint32) strlen( my_cookie )+1 );
}


/* -------------------------------------------------------------------ddConnect
 *
 * ddConnect() will be called in every process that accesses a physical device.
 * It establishes the connection between the process independent description
 * of the device and process specific addresses for entry points and private
 * memory structures.  The particular device is selected by the device cookie
 * array registered by ddInterrogate().
 *
 * ddConnect() should construct and return a table of function pointers to 
 * the device-dependent routines for this device.  ddConnect() also allocates
 * and initializes all of the device-dependent structures that we will need.
 */
MLstatus
ddConnect( MLbyte* physicalDeviceCookie,
	   MLint64 staticDeviceId,
	   MLphysicalDeviceOps* pOps,
	   MLbyte** retDevicePriv )
{
  MLint64 sysId;
  MLint64 engineId[2];
  MLint64 srcId[2];
  MLint64 dstId[2];
  MLstatus s = ML_STATUS_NO_ERROR;
  MLpv tmpCaps[25];
  int i = 0;
  int engineNum = 0;
  int nparm1, nparm2;
    
  /* Here's our dispatch table.  We copy it back to
   * the DI layer before we return.
   */
  MLphysicalDeviceOps nullOps = { 
    sizeof( MLphysicalDeviceOps ),
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

  /* Sanity check that we were called correctly.
   */
  assert( physicalDeviceCookie );
  assert( pOps );
  if ( strcmp( (char*) physicalDeviceCookie, MY_COOKIE ) ) {
    debug( "[mlSDK nullxcode ddConnect] bad device cookie %s\n",
	   physicalDeviceCookie );
    return ML_STATUS_DEVICE_UNAVAILABLE;
  }

  *retDevicePriv = NULL;

  /* Sanity check that we haven't forgotten a parameter in params.h
   */
  nparm1 = sizeof( engineParamsList ) / sizeof( MLint64 );
  nparm2 = sizeof( copyEngineParams ) / sizeof( MLDDparamDetails* ) - 1;
  if ( nparm1 != nparm2 ) {
    debug( "[mlSDK nullxcode ddConnect] "
	   "engineParamsList and engineParams don't match\n" );
    return ML_STATUS_DEVICE_UNAVAILABLE;
  }
  nparm1 = sizeof( pipeParamsList ) / sizeof( MLint64 );
  nparm2 = sizeof( copySrcParams ) / sizeof( MLDDparamDetails* ) - 1;
  if ( nparm1 != nparm2 ) {
    debug( "[mlSDK nullxcode ddConnect] "
	   "pipeParamsList and srcParams don't match\n" );
    return ML_STATUS_DEVICE_UNAVAILABLE;
  }
  nparm1 = sizeof( pipeParamsList ) / sizeof( MLint64 );
  nparm2 = sizeof( copyDstParams ) / sizeof( MLDDparamDetails* ) - 1;
  if ( nparm1 != nparm2 ) {
    debug( "[mlSDK nullxcode ddConnect] "
	   "pipeParamsList and dstParams don't match\n" );
    return ML_STATUS_DEVICE_UNAVAILABLE;
  }

  /* First we figure out the ids of all of our parts.
   */
  sysId = mlDIparentIdOfDeviceId( staticDeviceId );

  engineId[0] = mlDImakeXcodeEngineId( staticDeviceId, 0 );
  srcId[0] = mlDImakeXcodePipeId( engineId[0], ML_XCODE_SRC_PIPE >> 32 );
  dstId[0] = mlDImakeXcodePipeId( engineId[0], ML_XCODE_DST_PIPE >> 32 );

  engineId[1] = mlDImakeXcodeEngineId( staticDeviceId, 1 );
  srcId[1] = mlDImakeXcodePipeId( engineId[1], ML_XCODE_SRC_PIPE >> 32 );
  dstId[1] = mlDImakeXcodePipeId( engineId[1], ML_XCODE_DST_PIPE >> 32 );

  /* Setup lots of capabilities which can be retrieved
   * via ddGetCapabilities().  Will be set in tmpCaps and
   * then copied to correct capability list via mlDIcapDup().
   *
   * All of the setup is done here so that we can catch memory allocation
   * failures.  The idea is that if ddConnect() suceeds, the device
   * has allocated all of its resources.
   */

  /* device capabilities
   */
  i = 0;
  tmpCaps[i].param = ML_ID_INT64;
  tmpCaps[i].value.int64 = staticDeviceId;
  tmpCaps[i].length = 1;
  i++;
  tmpCaps[i].param = ML_NAME_BYTE_ARRAY;
  tmpCaps[i].value.pByte = (MLbyte*) MY_NAME;
  tmpCaps[i].length = (int) strlen( (char*) tmpCaps[i].value.pByte ) + 1;
  tmpCaps[i].maxLength = tmpCaps[i].length;
  i++;
  tmpCaps[i].param = ML_PARENT_ID_INT64;
  tmpCaps[i].value.int64 = sysId;
  tmpCaps[i].length = 1;
  i++;
  tmpCaps[i].param = ML_DEVICE_VERSION_INT32;
  tmpCaps[i].value.int32 = 0x100;
  tmpCaps[i].length = 1;
  i++;
  tmpCaps[i].param = ML_DEVICE_INDEX_INT32;
  tmpCaps[i].value.int32 = 0;
  tmpCaps[i].length = 1;
  i++;
  tmpCaps[i].param = ML_DEVICE_LOCATION_BYTE_ARRAY;
  tmpCaps[i].value.pByte = (MLbyte*) MY_LOCATION;
  tmpCaps[i].length = (int) strlen( (char*) tmpCaps[i].value.pByte ) + 1;
  tmpCaps[i].maxLength = tmpCaps[i].length;
  i++;
  tmpCaps[i].param = ML_DEVICE_JACK_IDS_INT64_ARRAY;
  tmpCaps[i].value.pInt64 = 0;
  tmpCaps[i].length = 0;
  i++;
  tmpCaps[i].param = ML_DEVICE_PATH_IDS_INT64_ARRAY;
  tmpCaps[i].value.pInt64 = 0;
  tmpCaps[i].length = 0;
  i++;
  tmpCaps[i].param = ML_DEVICE_XCODE_IDS_INT64_ARRAY;
  tmpCaps[i].value.pInt64 = &engineId[0];
  tmpCaps[i].length = 2;
  i++;
  tmpCaps[i].param = ML_END;

  if ( mlDIcapDup( tmpCaps, &devCaps ) != ML_STATUS_NO_ERROR ) {
    goto NOMEM;
  }

  /* We have two engines, a memory copier and a memory clearer.  For
   * many codecs, these would really be a compression engine and a
   * decompression engine.
   */
  for ( engineNum = 0; engineNum < 2; engineNum++ ) {
    /* engine capabilities
     */
    i = 0;
    tmpCaps[i].param = ML_ID_INT64;
    tmpCaps[i].value.int64 = engineId[engineNum];
    tmpCaps[i].length = 1;
    i++;
    tmpCaps[i].param = ML_NAME_BYTE_ARRAY;
    tmpCaps[i].value.pByte = (MLbyte*) engineNames[engineNum];
    tmpCaps[i].length = (int) strlen( (char*) tmpCaps[i].value.pByte ) + 1;
    tmpCaps[i].maxLength = tmpCaps[i].length;
    i++;
    tmpCaps[i].param = ML_PARENT_ID_INT64;
    tmpCaps[i].value.int64 = staticDeviceId;
    tmpCaps[i].length = 1;
    i++;
    tmpCaps[i].param = ML_PARAM_IDS_INT64_ARRAY;
    tmpCaps[i].value.pInt64 = engineParamsList;
    tmpCaps[i].length = sizeof( engineParamsList ) / sizeof( MLint64 );
    tmpCaps[i].maxLength = tmpCaps[i].length;
    i++;
    tmpCaps[i].param = ML_OPEN_OPTION_IDS_INT64_ARRAY;
    tmpCaps[i].value.pInt64 = engineOptionsList;
    tmpCaps[i].length = sizeof( engineOptionsList ) / sizeof( MLint64 );
    tmpCaps[i].maxLength = tmpCaps[i].length;
    i++;
    /* Presets are not supported yet */
    tmpCaps[i].param = ML_PRESET_MSG_ARRAY;
    tmpCaps[i].value.ppPv = 0;
    tmpCaps[i].length = 0;
    i++;
    tmpCaps[i].param = ML_XCODE_ENGINE_TYPE_INT32;
    tmpCaps[i].value.int32 = ML_XCODE_ENGINE_TYPE_NULL;
    tmpCaps[i].length = 1;
    i++;
    tmpCaps[i].param = ML_XCODE_IMPLEMENTATION_TYPE_INT32;
    tmpCaps[i].value.int32 = ML_XCODE_IMPLEMENTATION_TYPE_SW;
    tmpCaps[i].length = 1;
    i++;
    tmpCaps[i].param = ML_XCODE_COMPONENT_ALIGNMENT_INT32;
    tmpCaps[i].value.int32 = 4;
    tmpCaps[i].length = 1;
    i++;
    tmpCaps[i].param = ML_XCODE_BUFFER_ALIGNMENT_INT32;
    tmpCaps[i].value.int32 = 4;
    tmpCaps[i].length = 1;
    i++;
#ifdef NOTDEF
    /* Features are not supported yet */
    tmpCaps[i].param = ML_XCODE_FEATURES_BYTE_ARRAY;
    tmpCaps[i].value.pByte = 0;
    tmpCaps[i].length = 0;
    i++;
#endif
    tmpCaps[i].param = ML_XCODE_SRC_PIPE_IDS_INT64_ARRAY;
    tmpCaps[i].value.pInt64 = (MLint64*) &srcId[engineNum];
    tmpCaps[i].length = 1;
    tmpCaps[i].maxLength = tmpCaps[i].length;
    i++;
    tmpCaps[i].param = ML_XCODE_DEST_PIPE_IDS_INT64_ARRAY;
    tmpCaps[i].value.pInt64 = (MLint64*) &dstId[engineNum];
    tmpCaps[i].length = 1;
    tmpCaps[i].maxLength = tmpCaps[i].length;
    i++;
    tmpCaps[i].param = ML_END;

    if ( mlDIcapDup( tmpCaps, engineNum ? &clearEngineCaps
		     : &copyEngineCaps ) != ML_STATUS_NO_ERROR ) {
      goto NOMEM;
    }

    /* source pipe capabilities
     */
    i = 0;
    tmpCaps[i].param = ML_ID_INT64;
    tmpCaps[i].value.int64 = srcId[engineNum];
    tmpCaps[i].length = 1;
    i++;
    tmpCaps[i].param = ML_NAME_BYTE_ARRAY;
    tmpCaps[i].value.pByte = (MLbyte*) MY_SRC_PIPE_NAME;
    tmpCaps[i].length = (int) strlen( (char*) tmpCaps[i].value.pByte ) + 1;
    tmpCaps[i].maxLength = tmpCaps[i].length;
    i++;
    tmpCaps[i].param = ML_PARENT_ID_INT64;
    tmpCaps[i].value.int64 = engineId[engineNum];
    tmpCaps[i].length = 1;
    i++;
    tmpCaps[i].param = ML_PARAM_IDS_INT64_ARRAY;
    tmpCaps[i].value.pInt64 = pipeParamsList;
    tmpCaps[i].length = sizeof( pipeParamsList ) / sizeof( MLint64 );
    tmpCaps[i].maxLength = tmpCaps[i].length;
    i++;
    tmpCaps[i].param = ML_PIPE_TYPE_INT32;
    tmpCaps[i].value.int32 = ML_PIPE_TYPE_MEM_TO_ENGINE;
    tmpCaps[i].length = 1;
    i++;
    tmpCaps[i].param = ML_END;

    if ( mlDIcapDup( tmpCaps, engineNum ? &copySrcCaps
		     : &clearSrcCaps ) != ML_STATUS_NO_ERROR ) {
      goto NOMEM;
    }

    /* destination pipe capabilities
     */
    i = 0;
    tmpCaps[i].param = ML_ID_INT64;
    tmpCaps[i].value.int64 = dstId[engineNum];
    tmpCaps[i].length = 1;
    i++;
    tmpCaps[i].param = ML_NAME_BYTE_ARRAY;
    tmpCaps[i].value.pByte = (MLbyte*) MY_DST_PIPE_NAME;
    tmpCaps[i].length = (int) strlen( (char*) tmpCaps[i].value.pByte ) + 1;
    tmpCaps[i].maxLength = tmpCaps[i].length;
    i++;
    tmpCaps[i].param = ML_PARENT_ID_INT64;
    tmpCaps[i].value.int64 = engineId[engineNum];
    tmpCaps[i].length = 1;
    i++;
    tmpCaps[i].param = ML_PARAM_IDS_INT64_ARRAY;
    tmpCaps[i].value.pInt64 = pipeParamsList;
    tmpCaps[i].length = sizeof( pipeParamsList ) / sizeof( MLint64 );
    tmpCaps[i].maxLength = tmpCaps[i].length;
    i++;
    tmpCaps[i].param = ML_PIPE_TYPE_INT32;
    tmpCaps[i].value.int32 = ML_PIPE_TYPE_ENGINE_TO_MEM;
    tmpCaps[i].length = 1;
    i++;
    tmpCaps[i].param = ML_END;

    if ( mlDIcapDup( tmpCaps, engineNum ? &clearDstCaps
		     : &copyDstCaps ) != ML_STATUS_NO_ERROR ) {
      goto NOMEM;
    }
  }

  /* Finally, return the dispatch table to the DI layer.
   */
  *pOps = nullOps;

  return s;

 NOMEM:
  /* A malloc failed, time to leave without leaking memory.
   */
  if ( devCaps ) {
    mlFreeCapabilities( devCaps );
    devCaps = 0;
  }
  if ( copyEngineCaps ) {
    mlFreeCapabilities( copyEngineCaps );
    copyEngineCaps = 0;
  }
  if ( copySrcCaps ) {
    mlFreeCapabilities( copySrcCaps );
    copySrcCaps = 0;
  }
  if ( copyDstCaps ) {
    mlFreeCapabilities( copyDstCaps );
    copyDstCaps = 0;
  }
  if ( clearEngineCaps ) {
    mlFreeCapabilities( clearEngineCaps );
    clearEngineCaps = 0;
  }
  if ( clearSrcCaps ) {
    mlFreeCapabilities( clearSrcCaps );
    clearSrcCaps = 0;
  }
  if ( clearDstCaps ) {
    mlFreeCapabilities( clearDstCaps );
    clearDstCaps = 0;
  }
  return ML_STATUS_OUT_OF_MEMORY;
}


/*
 *---------------- Callbacks returned to the DI layer ---------------------
 */


/* -----------------------------------------------------------ddGetCapabilities
 *
 * Return the capabilities for the specified object.  Capabibilities arrays
 * were setup by ddConnect()
 */
/* ARGSUSED */
static MLstatus
ddGetCapabilities( MLbyte* devicePriv,
		   MLopenid staticObjectId,
		   MLpv** cap )
{
  MLint32 id = 0;
  MLpv* toCopy = 0;
  MLint32 engineIndex = 0;

  /* Use staticObjectId to determine what capabilities to return.
   * If id refers to a device, its the device capabilites.
   * Otherwise, it should refer to the engine or a pipe.
   * Note that we also make use of the xcodeEngineIndex embedded 
   * in the staticObjectId in order to determine which
   * transcoder to use.
   */
  if ( ML_REF_GET_TYPE( staticObjectId ) == ML_REF_TYPE_DEVICE ) {
    toCopy = devCaps;
  } else {
    if ( ML_REF_GET_TYPE( staticObjectId ) != ML_REF_TYPE_XCODE ) {
      debug( "[mlSDK nullxcode ddGetCapabilities] "
	     "bad objectId 0x%" FORMAT_LLX "\n", staticObjectId );
      *cap = 0;
      return ML_STATUS_INVALID_ID;
    }
    id = ML_REF_GET_XCODE_PIPE( staticObjectId );
    engineIndex = mlDIextractXcodeEngineIndex( staticObjectId );
    if ( id == 0 ) {
      toCopy = engineIndex ? clearEngineCaps : copyEngineCaps;
    } else if ( id == ML_REF_GET_XCODE_PIPE( ML_XCODE_SRC_PIPE ) ) {
      toCopy = engineIndex ? clearSrcCaps : copySrcCaps;
    } else if ( id == ML_REF_GET_XCODE_PIPE( ML_XCODE_DST_PIPE ) ) {
      toCopy = engineIndex ? clearDstCaps : copyDstCaps;
    }
  }

  /* Semantics of mlGetCapabilities() say to return a copy of the
   * capability.  The caller will free the copy via mlFreeCapabilities().
   */
  return mlDIcapDup( toCopy, cap );
}


/* ---------------------------------------------------------ddPvGetCapabilities
 *
 * Return the capabilities for the specified param on the specified object.
 * Capabibilities arrays were setup by ddConnect()
 */
/* ARGSUSED */
static MLstatus
ddPvGetCapabilities( MLbyte* devicePriv,
		     MLopenid staticObjectId,
		     MLint64 paramId,
		     MLpv** capabilities )
{
  MLstatus status;
  MLint32 pid = 0;
  MLDDparamDetails** capList = 0;
  MLDDparamDetails** optList = 0;
  MLint32 engineIndex = mlDIextractXcodeEngineIndex( staticObjectId );

  if ( ! capabilities ) {
    return ML_STATUS_INVALID_ARGUMENT;
  }

  /* Is the param on one of our engines or one of our pipes?
   * Note that we also make use of the xcodeEngineIndex embedded 
   * in the staticObjectId in order to determine which
   * transcoder to use.
   */
  pid = ML_REF_GET_XCODE_PIPE( staticObjectId );
  if ( pid == ML_REF_GET_XCODE_PIPE( ML_XCODE_SRC_PIPE ) ) {
    capList = engineIndex ? clearSrcParams : copySrcParams;
    optList = 0;
  } else if ( pid == ML_REF_GET_XCODE_PIPE( ML_XCODE_DST_PIPE ) ) {
    capList = engineIndex ? clearDstParams : copyDstParams;
    optList = 0;
  } else {
    capList = engineIndex ? clearEngineParams : copyEngineParams;
    optList = engineIndex ? clearEngineOptions : copyEngineOptions;
  }

  /* Find the capability and return a copy to the user.
   */
  status = lookupCapability( capList, paramId, capabilities );
  if ( status && optList ) {
    status = lookupCapability( optList, paramId, capabilities );
  }
  return status;
}


/* ----------------------------------------------------------------------ddOpen
 *
 * Open the copying transform.  Main job is to allocate and initialize
 * instance-specific data.  In our case, this means a message queue
 * and a MLDDopen struct.  The MLDDopen* that we return to the DI 
 * layer will be passed back to us as the "ddPriv" argument to 
 * our callback functions.
 */
static MLstatus
ddOpen( MLbyte* ddDevicePriv,
	MLopenid staticObjectId,
	MLopenid openObjectId,
	MLpv* openOptions,
	MLbyte** retddPriv )
{
  MLqueueOptions qOpt;
  MLopenOptions oOpt;
  MLDDopen* pOpen = 0;

  /* Parse open options
   */
  if ( mlDIparseOpenOptions( openObjectId, openOptions, &oOpt )
       != ML_STATUS_NO_ERROR) {
    return ML_STATUS_INVALID_PARAMETER;
  }

  /* We are a single-stream transcoder...
   */
  if ( oOpt.xcodeStream != ML_XCODE_STREAM_SINGLE ) {
    return ML_STATUS_INVALID_PARAMETER;
  }

  /* Get our instance-specific private structure
   */
  pOpen = (MLDDopen*) malloc( sizeof( MLDDopen ) );
  if ( pOpen == 0 ) {
    return ML_STATUS_OUT_OF_MEMORY;
  }
  memset( pOpen, 0, sizeof( MLDDopen ) );

  /* Setup a single queue between us and the DI layer.  Get queue
   * options from open options.
   */
  memset( &qOpt, 0, sizeof( qOpt ) );
  qOpt.sendSignalCount = oOpt.sendSignalCount;
  qOpt.sendMaxCount = oOpt.sendQueueCount;
  qOpt.receiveMaxCount = oOpt.receiveQueueCount;
  qOpt.eventMaxCount = oOpt.eventPayloadCount;
  qOpt.messagePayloadSize = oOpt.messagePayloadSize;
  qOpt.ddMessageSize = sizeof( MLDDxcodeParams ) * 2;
  qOpt.ddEventSize = sizeof( MLpv ) * 3;
  qOpt.ddAlignment = 4;

  /* Now we can make a queue.
   */
  if ( mlDIQueueCreate( 0, 0, &qOpt, &pOpen->q ) != ML_STATUS_NO_ERROR ) {
    free( pOpen );
    *retddPriv = 0;
    return ML_STATUS_OUT_OF_MEMORY;
  }

  /* Setup instance-specific info from open options and defaults
   *
   * This will change depending upon what instance-specific 
   * data a transcoder needs.  Also, note that we use 2 threads
   * per processor.  Other codecs may want to use different
   * degrees of parallelization.
   */
  pOpen->diContext = ddDevicePriv;
  pOpen->buffersSent = false;
  pOpen->isAsync = (oOpt.softwareXcodeMode == ML_XCODE_MODE_ASYNCHRONOUS);
  if ( pOpen->isAsync ) {
    pOpen->numThreads = getNumProcessors() * 2;
  } else {
    pOpen->numThreads = 0;
  }
  pOpen->useCopyEngine = (mlDIextractXcodeEngineIndex( staticObjectId ) == 0);
  _mlOSNewLockLite( &pOpen->mutex );
  pOpen->myId = openObjectId;
  pOpen->deviceState = ML_DEVICE_STATE_READY;
  pOpen->ust = 0;
  pOpen->msc = 0;
  pOpen->inParams.compression = *(imageCompression->deflt);
  pOpen->inParams.colorspace = *(imageColorspace->deflt);
  pOpen->inParams.packing = *(imagePacking->deflt);
  pOpen->inParams.sampling = *(imageSampling->deflt);
  pOpen->inParams.width = *(imageWidth->deflt);
  pOpen->inParams.height = *(imageHeight1->deflt);
  pOpen->inParams.orientation = *(imageOrientation->deflt);
  /* Output params are identical to input params
   */
  pOpen->outParams = pOpen->inParams;

  /* If async, allocate all of our thread data structures.
   */
  if ( pOpen->isAsync ) {
    int n = pOpen->numThreads;
    pOpen->threadData = malloc( n * sizeof( ThreadData ) );
    if ( ! pOpen->threadData ) {
      free( pOpen );
      *retddPriv = 0;
      return ML_STATUS_OUT_OF_MEMORY;
    }
    memset( pOpen->threadData, 0, n * sizeof( ThreadData ) );
  }

  *retddPriv = (MLbyte*) pOpen;
  return ML_STATUS_NO_ERROR; 
}


/* ---------------------------------------------------------------ddSetControls
 *
 * Immediately set controls on the open device. Block until the
 * device has processed the message.
 */
static MLstatus
ddSetControls( MLbyte* ddPriv,
	       MLopenid openObjectId,
	       MLpv* controls )
{ 
  MLDDopen* pOpen = (MLDDopen*) ddPriv;
  /* Validate entire message against param capabilities.  This call
   * can be expensive.  Codecs will probably want to individually
   * validate params against whatever restrictions they need to
   * impose.
   */
  MLstatus s = mlDIvalidateMsg( openObjectId, ML_ACCESS_RWI, false, controls );
  if ( s != ML_STATUS_NO_ERROR ) {
    return s;
  }
  return processControls( pOpen, controls, false );
}


/* ---------------------------------------------------------------ddGetControls
 *
 * Immediately get controls on the open device.  Block until the
 * device has processed the message.
 */
static MLstatus
ddGetControls( MLbyte* ddPriv,
	       MLopenid openObjectId,
	       MLpv* controls )
{
  MLDDopen* pOpen = (MLDDopen*) ddPriv;
  MLpv* pv = 0;
  MLDDxcodeParams* pInOut = 0;

  MLstatus s = mlDIvalidateMsg( openObjectId, ML_ACCESS_RI, true, controls );
  if ( s != ML_STATUS_NO_ERROR ) {
    return s;
  }
  _mlOSLockLite( &pOpen->mutex );

  /* Actually get the requested controls.
   *
   * This will change depending upon what controls a transcoder supports.
   */
  for ( pv = controls; pv->param != ML_END; pv++ ) {
    if ( ML_PARAM_GET_CLASS( pv->param ) != ML_CLASS_USER ) {
      /* Are we skipping until a new id select or ML_END?
       */
      if ( (pInOut == (MLDDxcodeParams*) -1L)
	   && (pv->param != ML_SELECT_ID_INT64) ) {
	continue;
      }

      switch ( pv->param ) {

	/* Control determines what the following controls effect.  A
	 * value of 0 means "back to engine".  An unrecognized values
	 * means ignore controls until next ML_SELECT_ID_INT64 or
	 * ML_END.
	 */
      case ML_SELECT_ID_INT64:
	pInOut = handleSelectId( pOpen, pv->value.int64 );
	break;

	/* Controls on the engine.
	 */
      case ML_DEVICE_EVENTS_INT32_ARRAY: {
	int i;
	if ( pv->maxLength == 0 ) {
	  /* Querying just the length of the array */
	  pv->maxLength = MAX_EVENTS;
	  pv->length = 0;
	  break;
	}
	if ( pv->maxLength < pOpen->eventCount ) {
	  pv->length = -1;
	  _mlOSUnlockLite( &pOpen->mutex );
	  return ML_STATUS_INVALID_VALUE; /* insufficient space */
	}
	for ( i = 0; i < pOpen->eventCount; i++ ) {
	  pv->value.pInt32[i] = pOpen->events[i];
	}
	pv->length = pOpen->eventCount;
	break;
      }

      case ML_DEVICE_STATE_INT32:
	pv->value.int32 = pOpen->deviceState;
	break;

      case ML_QUEUE_SEND_COUNT_INT32:
	pv->value.int32 = mlDIQueueGetSendCount( pOpen->q );
	break;

      case ML_QUEUE_RECEIVE_COUNT_INT32:
	pv->value.int32 = mlDIQueueGetReceiveCount( pOpen->q );
	break;

      case ML_QUEUE_SEND_WAITABLE_INT64:
	pv->value.int64 = (MLint64) mlDIQueueGetSendWaitable( pOpen->q );
	break;

      case ML_QUEUE_RECEIVE_WAITABLE_INT64:
	pv->value.int64 = (MLint64) mlDIQueueGetReceiveWaitable( pOpen->q );
	break;

	/* Controls on a pipe which have non-modifiable values.
	 */
      case ML_VIDEO_UST_INT64:
	pv->value.int64 = pOpen->ust;
	break;

      case ML_VIDEO_MSC_INT64:
	pv->value.int64 = pOpen->msc;
	break;

	CASE_R(ML_IMAGE_HEIGHT_2_INT32, *(imageHeight2->deflt));
	CASE_R(ML_IMAGE_ROW_BYTES_INT32, *(imageRowBytes->deflt));
	CASE_R(ML_IMAGE_SKIP_PIXELS_INT32, *(imageSkipPixels->deflt));
	CASE_R(ML_IMAGE_SKIP_ROWS_INT32, *(imageSkipRows->deflt));
	CASE_R(ML_IMAGE_INTERLEAVE_MODE_INT32, *(interleave->deflt));
	CASE_R(ML_IMAGE_TEMPORAL_SAMPLING_INT32,
	       *(temporalSampling->deflt));

	/* Controls on a pipe which have modifiable values.
	 */
	CASE_R(ML_IMAGE_COMPRESSION_INT32, pInOut->compression);
	CASE_R(ML_IMAGE_COLORSPACE_INT32, pInOut->colorspace);
	CASE_R(ML_IMAGE_PACKING_INT32, pInOut->packing);
	CASE_R(ML_IMAGE_SAMPLING_INT32, pInOut->sampling);
	CASE_R(ML_IMAGE_WIDTH_INT32, pInOut->width);
	CASE_R(ML_IMAGE_HEIGHT_1_INT32, pInOut->height);
	CASE_R(ML_IMAGE_ORIENTATION_INT32, pInOut->orientation);

	/* Controls on a pipe which have computed values.
	 */
      case ML_IMAGE_BUFFER_SIZE_INT32: {
	MLint32 rval = 0;
	PIPE_CHECK;
	rval = computeImageBufferSize( pInOut, &pv->value.int32 );
	if ( rval != ML_STATUS_NO_ERROR ) {
	  pv->length = -1;
	  _mlOSUnlockLite( &pOpen->mutex );
	  return rval;
	}
	break;
      }

	/* Unrecognized control
	 */
      default:
	pv->length = -1;
	_mlOSUnlockLite( &pOpen->mutex );
	return ML_STATUS_INVALID_PARAMETER;
      }
    }
  }
  _mlOSUnlockLite( &pOpen->mutex );

  return ML_STATUS_NO_ERROR; 
}


/* -------------------------------------------------------------ddQueryControls
 *
 * Query controls from an open device, non-blocking.
 */
/* ARGSUSED */
static MLstatus
ddQueryControls( MLbyte* ddPriv,
		 MLopenid openObjectId,
		 MLpv* controls )
{
  /* This is currently unsupported.  Need to add
   * message types for querys to really support it.
   */
  return ML_STATUS_INTERNAL_ERROR;
}


/* --------------------------------------------------------------ddSendControls
 *
 * Send controls to an open device, non-blocking.
 */
/* ARGSUSED */
static MLstatus
ddSendControls( MLbyte* ddPriv,
		MLopenid openObjectId,
		MLpv* controls )
{
  /* Just push message onto the queue.  It will get read and
   * processed by processBuffer().  Note that we set the
   * messageType to ML_CONTROLS_IN_PROGRESS.  This will
   * be used by processControls() and processBuffers() to
   * distinguish control messages from buffer messages.
   */
  MLDDopen* pOpen = (MLDDopen*) ddPriv;
  MLstatus s;
  _mlOSLockLite( &pOpen->mutex );
  s = mlDIQueuePushMessage( pOpen->q, ML_CONTROLS_IN_PROGRESS, controls,
			    0, 0, 0 );
  _mlOSUnlockLite( &pOpen->mutex );
  return s;
}


/* ---------------------------------------------------------------ddSendBuffers
 *
 * Send buffers to an open device, non-blocking
 */
/* ARGSUSED */
static MLstatus
ddSendBuffers( MLbyte* ddPriv,
	       MLopenid openObjectId,
	       MLpv* buffers )
{ 
  /* Push message onto the queue.  It will get read and processed by
   * processBuffer().  Note that we set the messageType to
   * ML_BUFFERS_IN_PROGRESS.  This will be used by processControls()
   * and processBuffers() to distinguish control messages from buffer
   * messages.
   *
   * Note that we also add a copy of the current inParams and
   * outParams private data.  Adding the state ensures that parallel
   * threads won't have state race conditions (e.g. a buffer being
   * processed by thread A won't be affected by a subsequent
   * sendControls processed by thread B.
   */
  MLDDopen* pOpen = (MLDDopen*) ddPriv;
  MLstatus s;
  _mlOSLockLite( &pOpen->mutex );
  pOpen->buffersSent = true;
  s = mlDIQueuePushMessage( pOpen->q, ML_BUFFERS_IN_PROGRESS, buffers,
			    (MLbyte*) &pOpen->inParams,
			    sizeof( MLDDxcodeParams ) * 2, 1 );
  _mlOSUnlockLite( &pOpen->mutex );
  return s;
}


/* ------------------------------------------------------------ddReceiveMessage
 *
 * Receive a message from an open device, blocking.
 */
/* ARGSUSED */
static MLstatus
ddReceiveMessage( MLbyte* ddPriv,
		  MLopenid openObjectId,
		  MLint32* retMsgType,
		  MLpv** retReply )
{
  /* Just ask our queue to return the next message to the app.
   */
  MLDDopen* pOpen = (MLDDopen*) ddPriv;
  MLstatus s;
  _mlOSLockLite( &pOpen->mutex );
  s = mlDIQueueReceiveMessage( pOpen->q, (enum mlMessageTypeEnum*) retMsgType,
			       retReply, 0, 0 );
  _mlOSUnlockLite( &pOpen->mutex );
  return s;
}


/* -----------------------------------------------------------------ddXcodeWork
 *
 * For software-only transcoders opened with the ML_XCODE_MODE_INT32 open
 * option explicitly set to ML_XCODE_MODE_SYNCHRONOUS, this function allows
 * an application to control exactly when (and in which thread) the
 * processing for that codec takes place.
 *
 * This function performs one unit of processing for the specified codec.
 * The processing is done in the thread of the calling process and the call
 * does not return until the processing is complete. 
 *
 * For most codecs a "unit of work" is the processing of a single buffer
 * from the source queue and the writing of a single resulting buffer on
 * the destination queue. 
 */
/* ARGSUSED */
static MLstatus
ddXcodeWork( MLbyte* ddPriv,
	     MLopenid openObjectId )
{
  MLstatus rval = ML_STATUS_INVALID_ID;
  MLDDopen* pOpen = (MLDDopen*) ddPriv;

  switch ( processBuffer( pOpen ) ) {
  case 0:
    rval = ML_STATUS_NO_ERROR;
    break;
  case 1:
    rval = ML_STATUS_NO_OPERATION;
    break;
  default:
    rval = ML_STATUS_INVALID_ID;
    break;
  }
  return rval;
}


/* ---------------------------------------------------------------------ddClose
 *
 * Close a previously opened device.
 * Any pending un-processed messages shall be discarded.
 * This is a blocking call.  It does not return until the device 
 * has been closed and any resources freed.
 */
/* ARGSUSED */
static MLstatus
ddClose( MLbyte* ddPriv,
	 MLopenid openObjectId )
{
  MLDDopen* pOpen = (MLDDopen*) ddPriv;
  if ( pOpen ) {
    /* Force an end-of-transfer.
     */
    MLpv msg[2];
    msg[0].param = ML_DEVICE_STATE_INT32;
    msg[0].value.int32 = ML_DEVICE_STATE_ABORTING;
    msg[1].param = ML_END;
    processControls( pOpen, msg, false );
    _mlOSFreeLockLite( &pOpen->mutex );
    if ( pOpen->threadData ) {
      free( pOpen->threadData );
    }
    free( pOpen );
  }
  return ML_STATUS_NO_ERROR; 
}


/*
 *---------------- Internal helpers -----------------------------------------
 */


/* ------------------------------------------------------------lookupCapability
 *
 * Return a copy of the requested parameter capabilities.
 */
static MLstatus
lookupCapability( MLDDparamDetails* paramList[],
		  MLint64 paramId,
		  MLpv** capability )
{
  int i = 0;

  *capability = 0;
  for ( i = 0; paramList[i] != 0; i++ ) {
    MLDDparamDetails* p = paramList[i];
    if ( p->id == paramId ) {
      MLpv pv[10];
      pv[0].param = ML_ID_INT64;
      pv[0].value.int64 = p->id;
      pv[0].length = 1;
      pv[1].param = ML_NAME_BYTE_ARRAY;
      pv[1].value.pByte = (MLbyte*) p->name;
      pv[1].length = (int) strlen( (char*) pv[1].value.pByte ) + 1;
      pv[1].maxLength = pv[1].length;
      pv[2].param = ML_PARAM_TYPE_INT32;
      pv[2].value.int32 = p->type;
      pv[2].length = 1;
      pv[3].param = ML_PARAM_ACCESS_INT32;
      pv[3].value.int32 = p->access;
      pv[3].length = 1;
      pv[4].param = ML_PARAM_DEFAULT | p->type;
      if ( p->deflt && p->type == ML_TYPE_INT32 ) {
	pv[4].value.int32 = *(p->deflt); 
	pv[4].length = 1;
	pv[4].maxLength = 0;
      } else {
	pv[4].value.int64 = 0; 
	pv[4].length = 0;
	pv[4].maxLength = 0;
      }
      pv[5].param = ML_PARAM_MINS_ARRAY | ML_PARAM_GET_TYPE_ELEM( p->id );
      pv[5].value.pInt32 = p->mins;
      pv[5].length = p->minsLength;
      pv[5].maxLength = pv[5].length;
      pv[6].param = ML_PARAM_MAXS_ARRAY | ML_PARAM_GET_TYPE_ELEM( p->id );
      pv[6].value.pInt32 = p->maxs;
      pv[6].length = p->maxsLength;
      pv[6].maxLength = pv[6].length;
      pv[7].param = ML_PARAM_ENUM_VALUES_INT32_ARRAY;
      pv[7].value.pInt32 = p->enumValues;
      pv[7].length = p->enumValuesLength;
      pv[7].maxLength = pv[7].length;
      pv[8].param = ML_PARAM_ENUM_NAMES_BYTE_ARRAY;
      pv[8].value.pByte = (MLbyte*) p->enumNames;
      pv[8].length = p->enumNamesLength;
      pv[8].maxLength = pv[8].length;
      pv[9].param = ML_END;

      return mlDIcapDup( pv, capability );
    }
  }
  return ML_STATUS_INVALID_ID;
}


/* --------------------------------------------------------------------getState
 *
 * Just a convenience routine to get state, taking care of locking.
 */
static MLint32
getState( MLDDopen* pOpen )
{
  MLint32 state = 0;
  _mlOSLockLite( &pOpen->mutex );
  state = pOpen->deviceState;
  _mlOSUnlockLite( &pOpen->mutex );
  return state;
}

    
/* ----------------------------------------------------------------advanceState
 *
 * Check and/or make state transitions.
 */
static int
advanceState( MLDDopen* pOpen,
	      int oldState,
	      int newState,
	      int noaction )
{
  enum actions { 
    FINISH=1, 
    ABORT=2, 
    STARTTRANSFER=3, 
    FLUSH = 4,
  };
  MLint32 requiredAction = 0;

  /* Make sure that any device state transitions are valid, and figure out
   * how to make the transition.
   */
  switch ( oldState ) {
  case ML_DEVICE_STATE_TRANSFERRING:
    if ( newState == ML_DEVICE_STATE_ABORTING ) {
      requiredAction = ABORT; 
    } else if ( newState == ML_DEVICE_STATE_FINISHING ) {
      requiredAction = FINISH;
    } else {
      debug( "[mlSDK nullxcode advanceState] "
	     "can't switch from %d to %d\n", oldState, newState );
      return -1;
    }
    break;

  case ML_DEVICE_STATE_ABORTING:
  case ML_DEVICE_STATE_FINISHING:
    debug( "[mlSDK nullxcode advanceState] "
	   "can't switch from %d to %d\n", oldState, newState );
    return -1;

  case ML_DEVICE_STATE_READY:
    if ( newState == ML_DEVICE_STATE_TRANSFERRING ) {
      requiredAction = STARTTRANSFER;
    } else if ( newState == ML_DEVICE_STATE_ABORTING ) {
      requiredAction = FLUSH;
    } else if ( newState == ML_DEVICE_STATE_FINISHING ) {
      return 0;
    } else {
      debug( "[mlSDK nullxcode advanceState] "
	     "can't switch from %d to %d\n", oldState, newState );
      return -1;
    }
    break;

  default: 
    fprintf( stderr, "[mlSDK nullxcode advanceState] unknown state %d\n",
	     oldState );
    return -1;
  }

  /* If only verifying, we're done.
   */
  if ( noaction ) {
    return 0;
  }

  /* Now make any state transitions.
   */
  switch ( requiredAction ) {
  case STARTTRANSFER:
    /* Device state has been set already.  If async, we must start
     * some threads to handle transforming.  Otherwise, the user must
     * call mlXcodeWork() to get anything done.  Note that our
     * thread's stateData is silly, but it does illustrate how to
     * handle thread-specific state.
     */
    if ( pOpen->isAsync ) {
      int i;
      if ( pOpen->isAsync ) {
	pOpen->numThreads = getNumProcessors() * 2;
      }

      for ( i = 0; i < pOpen->numThreads; i++ ) {
	pOpen->threadData[i].pOpen = pOpen;
	sprintf( pOpen->threadData[i].stateData, "I am thread %d\n", i );
	if ( _mlOSThreadCreate(&pOpen->threadData[i].threadId,
			       workThread, &pOpen->threadData[i])
	     != ML_STATUS_NO_ERROR ) {
	  debug( "[mlSDK nullxcode advanceState] thread create failed\n" );
	  pOpen->numThreads = i-1;
	  return ML_STATUS_INTERNAL_ERROR;
	}
      }
    }
    break;

  case FINISH:
  case ABORT: 
    /* Abort processing is a tad tricky if we have multiple threads.
     *   - Abort any pending messages.
     *   - Send a device-dependent "abort" message for each thread
     *     This ensures that any threads blocked in select() will
     *     wakeup.
     *   - The threads will see the device state, which has already
     *     been set, and know that they must exit.
     *   - We then wait for each dying thread.
     */
    if ( pOpen->isAsync ) {
      int i;
      _mlOSLockLite( &pOpen->mutex );
      mlDIQueueAbortMessages( pOpen->q );
      _mlOSUnlockLite( &pOpen->mutex );
      for ( i = 0; i < pOpen->numThreads; i++ ) {
	_mlOSLockLite( &pOpen->mutex );
	(void) mlDIQueueSendDeviceEvent( pOpen->q,
					 MLDI_DD_EVENT_TYPE( 1 ) );
	_mlOSUnlockLite( &pOpen->mutex );
      }
      for ( i = 0; i < pOpen->numThreads; i++ ) {
	int ret;
	_mlOSThreadJoin( &pOpen->threadData[i].threadId, &ret );
      }
      pOpen->numThreads=0;
    }
    /* No break here - fall through */

  case FLUSH:
    /* A flush is pretty simple, just abort pending messages.
     */
    _mlOSLockLite( &pOpen->mutex );
    mlDIQueueAbortMessages( pOpen->q );
    pOpen->deviceState = ML_DEVICE_STATE_READY;
    _mlOSUnlockLite( &pOpen->mutex );
    break;

  default:
    break;
  }
  return 0; 
}


/* ------------------------------------------------------------------workThread
 *
 * An asynchronous transform thread.  Just hangs about processing
 * buffers as long as the state is transferring or finishing.
 */
static _mlOSThreadRetValue
workThread( void* data )
{
  ThreadData* startData = (ThreadData*) data;
  MLDDopen* pOpen = (MLDDopen*) startData->pOpen;
  MLint32 state;
  MLwaitable waitHandle;
#ifdef ML_OS_NT
  DWORD dwRet = 0;
#else
  fd_set fdset;
#endif

  assert( pOpen->isAsync );

  /* Here's the only place that we use our stateData.  However,
   * it does illustrate one way to handle thread-specific data.
   */
  debug( "DEBUG: Thread says: %s", startData->stateData );

  /* Keep processing messages until we hit an error or our
   * state tells us to quit.
   *
   * Note the use of the receiveWaitable on our queue.  It is
   * just a file descriptor telling us when a message is ready
   * to be received.
   */
  waitHandle = mlDIQueueGetDeviceWaitable( pOpen->q );
  state = getState( pOpen );
  while ( (state == ML_DEVICE_STATE_TRANSFERRING
	   || state == ML_DEVICE_STATE_FINISHING
	   || state == ML_DEVICE_STATE_ABORTING ) ) {
    /* Select on our device queue until a message is ready.
     */
#ifndef ML_OS_NT
    FD_ZERO( &fdset );
    FD_SET( waitHandle, &fdset );
    if ( select( waitHandle+1, &fdset, 0, 0, 0 ) == -1 ) {
      if ( errno != EINTR ) {
	fprintf( stderr, "[mlSDK nullxcode workThread] select error %d\n",
		 errno);
	break;
      }
    }
    if ( ! FD_ISSET( waitHandle, &fdset ) ) {
      continue;
    }
#else
    dwRet = WaitForSingleObject( waitHandle, 10000 );
    if ( dwRet == WAIT_ABANDONED ) {
      break;
    } else if ( dwRet == WAIT_TIMEOUT ) {
      continue;
    }
#endif

    /* We have a message, let's handle it.
     */
    if ( processBuffer( pOpen ) == -1 ) {
      /* All is not well */
      break;
    }

    state = getState( pOpen );
  }

  _mlOSThreadExit( &startData->threadId, 0 );
  return 0;  /* silence compiler warning */
}


/* ---------------------------------------------------------------processBuffer
 *
 * Transform a single buffer.
 * Returns 0 on success, 1 for no work, -1 for error
 */
static int
processBuffer( MLDDopen* pOpen )
{
  MLint32 status;
  MLqueueEntry* entry;
  MLpv* msg;
  MLpv* pv;
  MLpv* outPv = 0;
  void* inPtr = 0;
  void* outPtr = 0;
  MLint32 inSize = 0;
  MLint32 outSize = 0;
  int whichBuf = -1;
  int rval = 0;
  bool xcodeWorked = false;

  MLint32 msgType;
  MLbyte* privData = 0;
  MLint32 privDataSize = 0;
  MLDDxcodeParams* inParams = 0;
  MLDDxcodeParams* outParams = 0;

  /* Process any control messages which may precede the next
   * buffers message.  Control messages are identified
   * by a message type of ML_CONTROLS_IN_PROGRESS.
   * At end of loop, msg is either null or points to a 
   * sendBuffers msg.
   */
  _mlOSLockLite( &pOpen->mutex );
  status = mlDIQueueNextMessage( pOpen->q, &entry,
				 (enum mlMessageTypeEnum*) &msgType,
				 &msg, &privData, &privDataSize );
  _mlOSUnlockLite( &pOpen->mutex );

  if ( msgType == MLDI_DD_EVENT_TYPE( 1 ) ) {
    rval = -1;
    goto UPDATE;
  }

  while ( status == ML_STATUS_NO_ERROR &&
	  (msgType == ML_CONTROLS_IN_PROGRESS
	   || msgType == ML_CONTROLS_FAILED) ) {
    enum mlMessageTypeEnum reply;
    if ( msgType == ML_CONTROLS_FAILED ) {
      /* nothing we can do except skip the message
       */
      reply = ML_CONTROLS_FAILED;
    } else {
      if ( msg == 0 ) {
	rval = -1;
	goto DONE;
      }
      if ( processControls( pOpen, msg, true ) ) {
	reply = ML_CONTROLS_FAILED;
      } else {
	reply = ML_CONTROLS_COMPLETE;
      }
    }
    /* Put reply into message.  Message will be returned to app 
     * by mlDIQueueAdvanceMessages().
     */
    _mlOSLockLite( &pOpen->mutex );
    if ( mlDIQueueUpdateMessage( entry, reply ) != ML_STATUS_NO_ERROR ) {
      rval = -1;
      _mlOSUnlockLite( &pOpen->mutex );
      goto DONE;
    }

    /* Take a look at the next one...
     */
    status = mlDIQueueNextMessage( pOpen->q, &entry,
				   (enum mlMessageTypeEnum*) &msgType,
				   &msg, &privData, &privDataSize );
    _mlOSUnlockLite( &pOpen->mutex );
    if ( msgType == MLDI_DD_EVENT_TYPE( 1 ) ) {
      rval = -1;
      goto UPDATE;
    }
  }

  /* If there's no message, we out of here...
   */
  if ( status == ML_STATUS_RECEIVE_QUEUE_EMPTY ) {
    rval = 1;
    goto DONE;
  }

  /* If there's an error, we return with failure.
   */
  if ( status != ML_STATUS_NO_ERROR ) {
    debug( "[mlSDK nullxcode processBuffer] internal error - "
	   "mlDIQueueNextMessage returned status %d\n", status );
    rval = -1;
    goto DONE;
  }
        
  /* We should have a buffers message...
   */
  if ( msgType != ML_BUFFERS_IN_PROGRESS ) {
    debug( "[mlSDK nullxcode processBuffer] "
	   "internal error - unexpected messageType 0x%x\n", msgType );
    rval = -1;
    goto DONE;
  }

  /* Get the input and output buffers and their sizes.  Verify them all.
   */
  for ( pv = msg; pv->param != ML_END; pv++ ) {
    if ( ML_PARAM_GET_CLASS( pv->param ) == ML_CLASS_USER ) {
      continue;
    }
    switch ( pv->param ) {
    case ML_SELECT_ID_INT64:
      if ( pv->value.int64 == ML_XCODE_SRC_PIPE ) {
	whichBuf = 0;
      } else if ( pv->value.int64 == ML_XCODE_DST_PIPE ) {
	whichBuf = 1;
      } else {
	debug( "[mlSDK nullxcode processBuffer] "
	       "Invalid ML_SELECT_ID_INT64 value\n" );
	pv->length = -1;
	rval = -1;
	goto DONE;
      }
      break;

    case ML_IMAGE_BUFFER_POINTER:
      switch ( whichBuf ) {
      case 0:
	inPtr = pv->value.pByte;
	inSize = pv->length;
	break;
      case 1:
	outPv = pv;
	outPtr = pv->value.pByte;
	outSize = pv->maxLength;
	break;
      default:
	debug( "[mlSDK nullxcode processBuffer] Must set ML_SELECT_ID_INT64 "
	       "before ML_IMAGE_BUFFER_POINTER\n");
	pv->length = -1;
	rval = -1;
	goto DONE;
      }
      break;
                 
      /* ust/msc can be given to us on input, and will be passed to output
       */
    case ML_VIDEO_UST_INT64:
      if ( whichBuf == 0 ) {
	pOpen->ust = pv->value.int64;
      } else {
	pv->value.int64 = pOpen->ust;
      }
      break;

    case ML_VIDEO_MSC_INT64:
      if ( whichBuf == 0 ) {
	pOpen->msc = pv->value.int64;
      } else {
	pv->value.int64 = pOpen->msc;
      }
      break;

    default:
      pv->length = -1;
      rval = -1;
      goto DONE;
    }
  }

  if ( ! inPtr || ! outPtr ) {
    debug( "[mlSDK nullxcode processBuffer] Missing buffer pointer\n" );
    rval = -1;
    goto DONE;
  }
  if ( inSize > outSize ) {
    debug("[mlSDK nullxcode processBuffer] "
	  "Input buffer larger than output buffer\n");
    rval = -1;
    goto DONE;
  }

  /* Our last call to mlDIQueueNextMessage() set privData pointing to a
   * copy of the input and output params in effect at the time these
   * buffers were queued.  Since our tranform is so trivial, we don't
   * actually need these params, we'll just make sure that they are equal.
   * However, this illustrates how to send private data with each buffer to
   * ensure correct parallel operation.
   */
  if ( privDataSize != sizeof( MLDDxcodeParams ) * 2 ) {
    fprintf( stderr, "[mlSDK nullxcode processBuffer] "
	     "internal error - unexpected privDataSize\n" );
    rval = -1;
    goto DONE;
  }
  inParams = (MLDDxcodeParams*) privData;
  outParams = (MLDDxcodeParams*) (privData + sizeof( MLDDxcodeParams ));

  /* Here's where the actual transform occurs.
   *
   * In our case, it's trivial.  However, this does show how to send
   * a failure event to an interested client.  Note that we advance
   * the queue prior to pushing the event.  This ensures that any
   * preceeding controls are replied to before the event.
   */
  xcodeWorked = transformBuffer( pOpen, inPtr, inSize, inParams,
				 outPtr, outSize, outParams );

  if ( xcodeWorked ) {
    outPv->length = inSize;
  } else {
    int i;
    _mlOSLockLite( &pOpen->mutex );
    for ( i = 0; i < pOpen->eventCount; i++ ) {
      if ( pOpen->events[i] == ML_EVENT_XCODE_FAILED ) {
	MLpv eventRecord[3];
	eventRecord[0].param = ML_VIDEO_UST_INT64;
	eventRecord[0].value.int64 = pOpen->ust;
	eventRecord[1].param = ML_VIDEO_MSC_INT64;
	eventRecord[1].value.int64 = pOpen->msc;
	eventRecord[2].param = ML_END;
	mlDIQueueAdvanceMessages( pOpen->q );
	mlDIQueueReturnEvent( pOpen->q, ML_EVENT_XCODE_FAILED, eventRecord );
	break;
      }
    }
    _mlOSUnlockLite( &pOpen->mutex );
    outPv->length = -1;
    rval = 0;
  }

  /* Update this message to indicate if the transform worked.
   */
 UPDATE:
  _mlOSLockLite( &pOpen->mutex );
  if ( mlDIQueueUpdateMessage( entry, xcodeWorked ? ML_BUFFERS_COMPLETE
                               : ML_BUFFERS_FAILED) != ML_STATUS_NO_ERROR ) {
    rval = -1;
    _mlOSUnlockLite( &pOpen->mutex );
    goto DONE;
  }
  _mlOSUnlockLite( &pOpen->mutex );

 DONE:
  /* All done, move any processed messages onto the reply queue.
   * Note that mlDIQueueAdvanceMessages() stops when an unprocessed
   * message is encountered.  This ensures correct ordering of our
   * replies to the app if we have multiple threads.  For example,
   * suppose we have a request queue like:
   *       1  2  3  4  5  6
   * Other threads have processed 1 and 4, we have just finished 2,
   * but the thread handling 3 isn't done yet.  Our advance messages
   * call will cause 1 and 2 to be returned to the app, but not 4.
   * When 3 finally finishes, its advance messages will cause 3 and 4
   * to be returned.
   */
  _mlOSLockLite( &pOpen->mutex );
  mlDIQueueAdvanceMessages( pOpen->q );
  _mlOSUnlockLite( &pOpen->mutex );
  return rval;
}


/* --------------------------------------------------------------handleSelectId
 *
 * Deal with the ML_SELECT_ID_INT64 control.  A value not equal to our device
 * or our pipes means ignore all controls until the next ML_SELECT_ID_INT64
 * or ML_END.  A value equal to zero or our device means that controls once
 * again apply to us.  Return pointer to MLDDxcodeParams which controls 
 * should now effect.  Return of 0 means engine, -1 means ignore.
 */
static MLDDxcodeParams*
handleSelectId( MLDDopen* pOpen,
		MLint64 val )
{
  MLDDxcodeParams* rval = 0;
  MLint64 dev = 0;
  MLint32 pid = 0;

  mlDIprocessSelectIdParam( val, pOpen->myId, &dev );
  pid = ML_REF_GET_XCODE_PIPE( dev );
  if ( pid == ML_REF_GET_XCODE_PIPE( ML_XCODE_SRC_PIPE ) ) {
    rval = &(pOpen->inParams);
  } else if ( pid == ML_REF_GET_XCODE_PIPE( ML_XCODE_DST_PIPE ) ) {
    rval = &(pOpen->outParams);
  } else if ( dev == pOpen->myId ) {
    rval = 0;
  } else {
    rval = (MLDDxcodeParams*) -1L;
  }
  return rval;
}


/*
 *---------------- The real work of the transcoder ----------------------------
 */


/* --------------------------------------------------------------verifyControls
 *
 * Controls verification.
 *
 * For a real transcoder, this function needs to determine
 * if "in" and "out" describe a legal combination of input 
 * and output controls.  Return true if so, else false.
 *
 * In our trivial null transcoder, input and output params
 * must be equal.
 */
static bool
verifyControls( MLDDxcodeParams* in,
		MLDDxcodeParams* out )
{
  if ( memcmp( in, out, sizeof( MLDDxcodeParams ) ) ) {
    debug( "[mlSDK nullxcode verifyControls] input params != output params\n");
    return false;
  }
  return true;
}


/* -------------------------------------------------------------processControls
 *
 * Process a single control message.  Much of this function can 
 * be reused without change.  A different transcoder will probably
 * need to modify the switch() statement which examines the controls.
 *
 * N.B. it is OK to (re)set a RO parameter to its *current* value
 */
static MLstatus
processControls( MLDDopen* pOpen,
		 MLpv* msg,
		 bool validateMsg )
{ 
  MLpv* pv;
  MLDDopen newParams;
  MLint32 newState;
  MLint32 oldState;
  MLDDxcodeParams* pInOut = 0;
  /* MLDDparamDetails** capList = 0; */

  /* Make a copy of our current state and controls.  We will change
   * and verify the copy before changing the real controls.
   */
  _mlOSLockLite( &pOpen->mutex );
  memset( &newParams, 0, sizeof( newParams ) );
  newParams.deviceState = pOpen->deviceState;
  newParams.myId = pOpen->myId; /* needed for handleSelectId() */
  newParams.inParams = pOpen->inParams;
  newParams.outParams = pOpen->outParams;
  _mlOSUnlockLite( &pOpen->mutex );

  /* Validate entire message against param capabilities.
   * This call can be expensive.  Codecs will probably
   * want to individually validate params against whatever
   * restrictions they need to impose.
   */
  if ( validateMsg ) {
    if ( mlDIvalidateMsg( pOpen->myId, ML_ACCESS_RWQ, true, msg )
	 != ML_STATUS_NO_ERROR) {
      debug( "[mlSDK nullxcode processControls] invalid parameter value\n" );
      return ML_STATUS_INVALID_VALUE;
    }
  }

  for ( pv = msg; pv->param != ML_END; pv++ ) {
    if ( ML_PARAM_GET_CLASS( pv->param ) != ML_CLASS_USER ) {
      /* Are we skipping until a new id select or ML_END?
       */
      if ( (pInOut == (MLDDxcodeParams*)-1L)
	   && (pv->param != ML_SELECT_ID_INT64) ) {
	continue;
      }

      /* Here is where a different transcoder probably needs to make
       * changes, depending upon what controls it supports.
       */
      switch ( pv->param ) {

      case ML_SELECT_ID_INT64:
	/* Determines what the following controls effect.  A value
	 * of 0 means "back to engine".  An unrecognized values means
	 * ignore controls until next ML_SELECT_ID_INT64 or ML_END.
	 */
	pInOut = handleSelectId( &newParams, pv->value.int64 );
	break;

	/* Controls on the engine.
	 */
      case ML_DEVICE_EVENTS_INT32_ARRAY: {
	int i;
	newParams.eventCount=0;
	for ( i = 0; i < pv->length; i++ ) {
	  MLint32 newEvent = pv->value.pInt32[i];
	  switch ( newEvent ) {
	  case ML_EVENT_XCODE_FAILED:
	    newParams.events[newParams.eventCount++] = newEvent;
	    break;
	  default:
	    debug( "[mlSDK nullxcode processControls] unsupported event\n" );
	    return ML_STATUS_INVALID_VALUE;
	  }
	}
	break;
      }

      case ML_DEVICE_STATE_INT32:
	newParams.deviceState = pv->value.int32;
	break;

      case ML_VIDEO_UST_INT64:
	pOpen->ust = pv->value.int64;
	break;

      case ML_VIDEO_MSC_INT64:
	pOpen->msc = pv->value.int64;
	break;

	/* Controls on a pipe which have non-modifiable values.
	 * It is ok to "set" one of these to its current value.
	 */
	CASE_RO(ML_IMAGE_HEIGHT_2_INT32, *(imageHeight2->deflt));
	CASE_RO(ML_IMAGE_ROW_BYTES_INT32, *(imageRowBytes->deflt));
	CASE_RO(ML_IMAGE_SKIP_PIXELS_INT32, *(imageSkipPixels->deflt));
	CASE_RO(ML_IMAGE_SKIP_ROWS_INT32, *(imageSkipRows->deflt));
	CASE_RO(ML_IMAGE_INTERLEAVE_MODE_INT32, *(interleave->deflt));
	CASE_RO(ML_IMAGE_TEMPORAL_SAMPLING_INT32,
		*(temporalSampling->deflt));

	/* Controls on a pipe which have modifiable values.
	 */
	CASE_W(ML_IMAGE_COMPRESSION_INT32, pInOut->compression);
	CASE_W(ML_IMAGE_COLORSPACE_INT32, pInOut->colorspace);
	CASE_W(ML_IMAGE_PACKING_INT32, pInOut->packing);
	CASE_W(ML_IMAGE_SAMPLING_INT32, pInOut->sampling);
	CASE_W(ML_IMAGE_WIDTH_INT32, pInOut->width);
	CASE_W(ML_IMAGE_HEIGHT_1_INT32, pInOut->height);
	CASE_W(ML_IMAGE_ORIENTATION_INT32, pInOut->orientation);

	/* Unrecognized control
	 */
      default:
	pv->length = -1;
	return ML_STATUS_INVALID_PARAMETER;
      }
    }
  }

  /* Now check the new device state.
   */
  newState = newParams.deviceState;
  oldState = getState( pOpen );
  if ( newState != oldState ) {
    if ( advanceState( pOpen, oldState, newState, 1 ) ) {
      pv = mlPvFind( msg, ML_DEVICE_STATE_INT32 );
      if ( pv ) {
	pv->length = -1;
      }
      fprintf( stderr, "[mlSDK nullxcode processControls] "
	       "failed error check.\n");
      return ML_STATUS_INVALID_VALUE;
    }
  }

  /* Verify any param changes.
   */
  if ( ! verifyControls( &newParams.inParams, &newParams.outParams ) ) {
    return ML_STATUS_INVALID_VALUE;
  }
  
  /* All is well, so commit the param/state changes
   */
  _mlOSLockLite( &pOpen->mutex );
  pOpen->deviceState = newParams.deviceState;
  memcpy( pOpen->events, newParams.events, sizeof( newParams.events ) );
  pOpen->eventCount = newParams.eventCount;
  pOpen->inParams = newParams.inParams;
  pOpen->outParams = newParams.outParams;
  _mlOSUnlockLite( &pOpen->mutex );

  /* Advance state if necessary
   */
  if ( newState != oldState ) {
    if ( advanceState( pOpen, oldState, newState, 0 ) ) {
      fprintf( stderr, "[mlSDK nullxcode processControls] "
	       "internal error.\n" );
      return ML_STATUS_INTERNAL_ERROR;
    }
  }

  return ML_STATUS_NO_ERROR;
}


/* -------------------------------------------------------------transformBuffer
 *
 * Actually transcode an input buffer into an output buffer.
 * This is where the real work of a transcoder will occur.
 *
 * In our case, it's pretty trivial.
 */
static bool
transformBuffer( MLDDopen* pOpen,
		 MLbyte* inBuf,
		 MLint32 inSize,
		 MLDDxcodeParams* inParams,
		 MLbyte* outBuf,
		 MLint32 outSize,
		 MLDDxcodeParams* outParams )
{
  bool rval = false;
    
  /* Must verify controls in order to catch incompatible settings
   * made via mlSendControls()
   */
  if ( ! verifyControls( inParams, outParams ) ) {
    return false;
  }

  /* Transform away...
   */
  if ( pOpen->useCopyEngine ) {
    rval = (memcpy( outBuf, inBuf, inSize ) == outBuf);
  } else {
    memset( outBuf, 0, outSize );
    rval = true;
  }
  return rval;
}


/* ------------------------------------------------------computeImageBufferSize
 *
 * Compute the size of an image buffer on a particular pipe.
 */
static MLstatus
computeImageBufferSize( MLDDxcodeParams* pInOut,
			MLint32* rval)
{
  MLint32 bytesPerPixelDenomRet, bytesPerPixelNumRet;
  MLint32 bytesPerLine;
  MLpv params[3];

  if ( ! pInOut ) {
    return ML_STATUS_INVALID_PARAMETER;
  }

  /* Get pixel size
   */
  params[0].param = ML_IMAGE_PACKING_INT32;
  params[0].value.int32 = pInOut->packing ;
  params[1].param = ML_IMAGE_SAMPLING_INT32;
  params[1].value.int32 = pInOut->sampling ;
  params[2].param = ML_END;
  if ( mluComputeImagePixelSize( params, &bytesPerPixelNumRet,
                                 &bytesPerPixelDenomRet )
       != ML_STATUS_NO_ERROR ) {
    debug( "[mlSDK nullxcode computeImageBufferSize] "
	   "cannot determine pixel size\n" );
    return ML_STATUS_INVALID_VALUE;
  }

  /* Do the math
   */
  bytesPerLine = (MLint32) ceil( pInOut->width *
				 (double) bytesPerPixelNumRet
				 / (double) bytesPerPixelDenomRet );
  *rval = bytesPerLine * pInOut->height;

  return ML_STATUS_NO_ERROR;
}


/* ------------------------------------------------------------getNumProcessors
 *
 * Determine how many processors we have by examining
 * /proc/cpuinfo.  
 */
static int
getNumProcessors()
{
#ifndef ML_OS_NT
  int rval = 0;
  struct utsname uts;

  uname( &uts );
  if ( ! strncasecmp( uts.sysname, "Linux", strlen( "Linux" ) ) ) {
    char buf[200];
    FILE* fp = fopen( "/proc/cpuinfo", "r" );
    if ( ! fp ) {
      return 1;
    }
    while ( fgets( buf, 200, fp ) ) {
      if ( ! strncmp( buf, "processor\t:", strlen( "processor\t:" ) ) ) {
	rval++;
      }
    }
    return rval;
  } else if ( ! strncasecmp( uts.sysname, "IRIX", strlen( "IRIX" ) ) ) {
    struct direct* dp;
    DIR* dirp = opendir( "/hw/cpunum" );
    if ( ! dirp ) {
      return 1;
    }
    while ( (dp = readdir( dirp )) != NULL ) {
      /* If the name is not "." or "..", it refers to a processor. */
      if ( strcmp( dp->d_name, "." ) != 0 && strcmp( dp->d_name, ".." ) != 0) {
	rval++;
      }
    }
    closedir( dirp );
    return rval;
  }
  return 1;
#else
  SYSTEM_INFO si;
  GetSystemInfo( &si );
  return si.dwNumberOfProcessors;
#endif
}
