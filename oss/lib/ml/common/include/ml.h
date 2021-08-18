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

#ifndef _ML_H
#define _ML_H
#ifdef __cplusplus
extern "C" {
#endif

#include <ML/mltypes.h>
#include <ML/mldefs.h>
#include <ML/mlparam.h>

#include <ML/mlancdata.h>
#include <ML/mlaudio.h>
#include <ML/mlcompression.h>
#include <ML/mlimage.h>
#include <ML/mlpath.h>
#include <ML/mljack.h>
#include <ML/mluser.h>
#include <ML/mlvideo.h>
#include <ML/mlxcode.h>

MLstatus MLAPI mlGetVersion( MLint32 *major, MLint32 *minor );

MLstatus MLAPI mlGetCapabilities( MLint64 id, MLpv** capabilities );
MLstatus MLAPI mlPvGetCapabilities( const MLint64 deviceid,
				    const MLint64 paramid,
				    MLpv** capabilities );
MLstatus MLAPI mlFreeCapabilities( MLpv* capabilities );

MLstatus MLAPI mlOpen( MLint64 id, MLpv* options, MLopenid* openid );
MLstatus MLAPI mlClose( MLopenid openid );

MLstatus MLAPI mlSetControls( MLopenid openid, MLpv* controls );
MLstatus MLAPI mlGetControls( MLopenid openid, MLpv* controls );
MLstatus MLAPI mlQueryControls( MLopenid openid, MLpv* controls );

MLstatus MLAPI mlSendControls( MLopenid openid, MLpv* controls );
MLstatus MLAPI mlSendBuffers( MLopenid openid, MLpv* buffers );
MLstatus MLAPI mlReceiveMessage( MLopenid openid, MLint32 *status,
				 MLpv** reply );

MLstatus MLAPI mlGetReceiveWaitHandle( MLopenid openid, MLwaitable* handle );
MLstatus MLAPI mlGetSendWaitHandle( MLopenid openid, MLwaitable* handle );

MLstatus MLAPI mlGetSendMessageCount( MLopenid openid, MLint32* count );
MLstatus MLAPI mlGetReceiveMessageCount( MLopenid openid, MLint32* count );

MLstatus MLAPI mlBeginTransfer( MLopenid openid );
MLstatus MLAPI mlEndTransfer( MLopenid openid );

MLstatus MLAPI mlXcodeGetOpenPipe( MLopenid openid, MLint64 pipeid,
				   MLopenid* openpipeid );
MLstatus MLAPI mlXcodeWork( MLopenid codec );

MLstatus MLAPI mlGetSystemUST( MLint64 sysId, MLint64* ust );

MLstatus MLAPI mlSavePersistentControls( MLopenid openId );
MLstatus MLAPI mlRestorePersistentControls( MLopenid openId );

MLstatus MLAPI mlPvValueToString( MLint64 objectId, MLpv* pv, 
				  char* buffer, MLint32* bufferSize );
MLstatus MLAPI mlPvParamToString( MLint64 objectId, MLpv* pv,
				  char* buffer, MLint32* bufferSize );
MLstatus MLAPI mlPvToString( MLint64 objectId, MLpv* pv, 
			       char* buffer, MLint32* bufferSize );
MLstatus MLAPI mlPvValueFromString( MLint64 objectId, const char* buffer, 
				    MLint32* bufferSize, MLpv* pv, 
				    MLbyte* arrayData, MLint32 arraySize );
MLstatus MLAPI mlPvParamFromString( MLint64 objectId, const char* buffer, 
				    MLint32* size, MLpv* pv );
MLstatus MLAPI mlPvFromString( MLint64 objectId, const char* buffer,
			       MLint32* bufferSize, MLpv* pv, 
			       MLbyte* arrayData, MLint32 arraySize );

void 	 MLAPI mlPvSizes( MLpv* params, MLint32* pnParams, MLint32* pnBytes );
MLpv* 	 MLAPI mlPvFind( MLpv* params, MLint64 searchParam );
MLstatus MLAPI mlPvCopy( MLpv* params, void* toBuffer, MLint32 sizeofBuffer );

const char* MLAPI mlStatusName( MLstatus status );
const char* MLAPI mlMessageName( MLint32 messageType );
const char* MLAPI mlDeviceStateName( MLint32 deviceState );

#ifdef __cplusplus 
}
#endif

#endif /* ML_H */


