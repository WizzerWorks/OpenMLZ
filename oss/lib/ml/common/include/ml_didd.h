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

#ifndef _ML_DIDD_H
#define _ML_DIDD_H
#ifdef __cplusplus
extern "C" {
#endif

#include <ML/ml.h>
#include <ML/mlqueue.h>

#define ML_DI_DD_ABI_VERSION 2


/* These are opaque, device-independent structures maintained
 * in the DI layer.
 */
typedef void* MLsystemContext;
typedef void* MLopenDeviceContext;
typedef MLint32 MLmoduleContext;


/* This is a public structure, passed in the ddConnect call 
 */
typedef struct _MLphysicalDeviceOps 
{
  MLint32 thisSize; /* DD should set to sizeof(MLphysicalDeviceOps) */
  MLint32 version;  /* DD should set to ML_DI_DD_ABI_VERSION */

  MLstatus (*ddGetCapabilities)(MLbyte* ddDevPriv,
				MLint64 staticObjectId,
				MLpv** capabilities);

  MLstatus (*ddPvGetCapabilities)(MLbyte* ddDevPriv,
				  MLint64 staticObjectId,
				  MLint64 paramId,
				  MLpv** capabilities);

  MLstatus (*ddOpen)(MLbyte*  ddDevPriv,
		     MLint64  staticObjectId,
		     MLopenid openObjectId,
		     MLpv*    openOptions,
		     MLbyte** retddOpenPriv);

  MLstatus (*ddSetControls)(MLbyte* ddOpenPriv,
			    MLopenid openObjectId,
			    MLpv *controls);

  MLstatus (*ddGetControls)(MLbyte* ddOpenPriv,
			    MLopenid openObjectId,
			    MLpv *controls);

  MLstatus (*ddSendControls)(MLbyte* ddOpenPriv,
			     MLopenid openObjectId,
			     MLpv *controls);

  MLstatus (*ddQueryControls)(MLbyte* ddOpenPriv,
			     MLopenid openObjectId,
			     MLpv *controls);

  MLstatus (*ddSendBuffers)(MLbyte* ddOpenPriv,
			    MLopenid openObjectId,
			    MLpv *buffers);

  MLstatus (*ddReceiveMessage)(MLbyte* ddOpenPriv,
			       MLopenid openObjectId,
			       MLint32 *retMsgType, 
			       MLpv **retReply);

  MLstatus (*ddXcodeWork)(MLbyte* ddOpenPriv,
			  MLopenid openObjectId);

  MLstatus (*ddClose)(MLbyte* ddOpenPriv,
		      MLopenid openObjectId);

} MLphysicalDeviceOps;


/* These two functions must be provided by all device-dependent
 * modules
 */
MLstatus ddInterrogate( MLsystemContext systemContext,
			MLmoduleContext moduleContext );

MLstatus ddConnect( MLbyte *physicalDeviceCookie,
		    MLint64 staticDeviceId,
		    MLphysicalDeviceOps *pOps,
		    MLbyte** retddDevPriv );


/* This function is defined by device-dependent modules that wish to
 * provide a system-wide UST source.
 */
MLint32 ddGetUST( MLint64* UST );


/* This function is provided by the di layer, and must be called once 
 * for each physical device the module wishes to expose in its ddInterrogate
 * procedure.  The moduleContext must match that passed to your module in
 * the call to ddInterrogate.
 */
#define ML_MAX_COOKIE_SIZE 2048

MLstatus MLAPI mlDINewPhysicalDevice( MLsystemContext systemContext,
				      MLmoduleContext moduleContext,
				      const MLbyte* ddCookie,
				      MLint32 ddCookieSize );


/* Function provided by the DI layer to allow a device-dependent
 * module to register a system-wide UST source, during its
 * ddInterrogate procedure. The metrics should be as accurate as
 * possible in measuring the source's accuracy, as they are used by
 * the DI layer to select the best UST source:
 *   updatePeriod: reciprocal of UST source's update frequency.
 *                 Expressed in nano-seconds
 *   latencyVariation: difference between the maximum and minimum latencies
 *                     expected when sampling the source.
 *                     Expressed in nano-seconds.
 *   name: name of source, to a max of ML_MAX_USTNAME chars. This should
 *         be a unique identifier, not a full description of the source
 *         (see "description", below)
 *         e.g.: "ACME EarSplitter"
 *   description: plain-language description of source, to a max of
 *                ML_MAX_USTDESCRIPTION chars.
 *                e.g.: "ACME EarSplitter Audio on-board high-precision UST"
 *                Used only in queries, to inform end-users.
 *
 * Note: there is no need for a "ddCookie" to identify the UST source,
 * as each device-dependent module is limited to providing a single
 * source.
 */
#define ML_MAX_USTNAME 32
#define ML_MAX_USTDESCRIPTION 128

MLstatus MLAPI mlDINewUSTSource( MLsystemContext systemContext,
				 MLmoduleContext moduleContext,
				 MLint32 updatePeriod,
				 MLint32 latencyVariation,
				 const char* name,
				 MLint32 nameSize,
				 const char* description,
				 MLint32 descriptionSize );


/* Function provided by the DI layer so that device-dependent modules
 * may retrieve the address of the function retained as the system's
 * UST source. This function must not be called during the
 * ddInterrogate procedure (the UST source has not yet been selected
 * at that point).
 *
 * Note that all device-dependent modules should call this to obtain a
 * UST source, even if they have their own potential UST source
 * function.
 *
 * The 'sysId' identifies the system from which the device was
 * opened. This can be obtained by calling mlDIparentIdOfDeviceId() on
 * the static device Id supplied in the ddConnect() call.
 */
MLstatus MLAPI mlDIGetUSTSource( MLint64 sysId,
				 MLint32 (**USTSource)( MLint64* ) );


/* The following are convenience functions, provided for optional
 * use by a device-dependent module.
 */
typedef struct _MLopenOptions {
  MLint32 mode;
  MLint32 sendQueueCount;		
  MLint32 receiveQueueCount;
  MLint32 messagePayloadSize; /* bytes */
  MLint32 eventPayloadCount;
  MLint32 sendSignalCount;
  MLint32 softwareXcodeMode; /* ML_XCODE_{A}SYNCHRONOUS */
  MLint32 xcodeStream;       /* ML_XCODE_STREAM_{SINGLE,MULTI} */
} MLopenOptions;

MLstatus MLAPI mlDIparseOpenOptions( MLopenid openDeviceId,
				     MLpv* rawOptions, 
				     MLopenOptions* parsedOptions );

MLstatus MLAPI mlDIvalidateMsg( MLopenid openDeviceId,
				MLint32 accessNeeded,
				MLint32 beforeGet,
				MLpv* msg );

MLstatus MLAPI mlDIvalidatePv( MLopenid openDeviceId,
			       MLint32 accessNeeded,
			       MLint32 beforeGet,
			       MLpv* pv );

MLstatus MLAPI mlDIgetParamDefault( MLint64 openid,
				    MLint64 param,
				    void *defaultPtr );

MLstatus MLAPI mlDIprocessSelectIdParam( MLint64 val,
					 MLopenid device,
					 MLopenid* rval );


/* Return connected device or open device private data pointer
 */
MLstatus MLAPI mlDIgetPrivateDataPtr( MLint64 objectId,
				      void **privPtr );


/* Create a duplicate copy of a capabilities message: Computes the
 * message size, malloc's memory and then copies the message into that
 * memory.
 *
 * Returned pointer must be freed by mlFreeCapabilities
 */
MLstatus MLAPI mlDIcapDup( MLpv* cpabilities, MLpv** retDup );


/* Functions to extract information from static object IDs.
 * Object IDs may refer to systems, devices, jacks, paths, or xcodes
 */
MLint32 MLAPI mlDIextractIdType( MLint64 id );
MLint32 MLAPI mlDIextractSystemIndex( MLint64 id );
MLint32 MLAPI mlDIextractDeviceIndex( MLint64 id );
MLint32 MLAPI mlDIextractJackIndex( MLint64 id );
MLint32 MLAPI mlDIextractPathIndex( MLint64 id );
MLint32 MLAPI mlDIextractXcodeEngineIndex( MLint64 id );
MLint32 MLAPI mlDIextractXcodePipeIndex( MLint64 id );
MLint64 MLAPI mlDIextractXcodeIdFromPipeId( MLint64 id );

MLint64 MLAPI mlDImakeSystemId( MLint32 systemIndex );
MLint64 MLAPI mlDImakeDeviceId( MLint64 systemId, MLint32 deviceIndex );
MLint64 MLAPI mlDImakeJackId( MLint64 deviceId, MLint32 jackIndex );
MLint64 MLAPI mlDImakePathId( MLint64 deviceId, MLint32 pathIndex );
MLint64 MLAPI mlDImakeXcodeEngineId( MLint64 deviceId,
				     MLint32 xcodeEngineIndx );
MLint64 MLAPI mlDImakeXcodePipeId( MLint64 xcodeEngineId, MLint32 pipeIndex );


/* Function to return the parent (system id) of a physical device
 */
MLint64 MLAPI mlDIparentIdOfDeviceId( MLint64 deviceId );


/* Function to return the parent (device id) of a logical device
 * (jack,path,xcode)
 */
MLint64 MLAPI mlDIparentIdOfLogDevId( MLint64 logDevId );


/* Function to extract information from openid's
 */
MLint32 MLAPI mlDIisOpenId( MLint64 candidateId );
MLint64 MLAPI mlDIconvertOpenIdToStaticId( MLopenid openid );


/* Macro to construct a device-dependent event message type (for use
 * in the mlDIQueueSendDeviceEvent)
 */
#define MLDI_DD_EVENT_TYPE(index) (enum mlMessageTypeEnum)(ML_DD_EVENT+(index))

#ifdef __cplusplus 
}
#endif

#endif /* ML_DIDD_H */


