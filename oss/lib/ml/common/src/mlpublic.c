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

#include <stdarg.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include <ML/ml_private.h>

#ifndef	lengthof
#define	lengthof(a)	(sizeof(a)/sizeof(a[0]))
#endif

#define ML_SEND_QUEUE_COUNT_DEFAULT 64
#define ML_RECEIVE_QUEUE_COUNT_DEFAULT 256
#define ML_MESSAGE_PAYLOAD_SIZE_DEFAULT 8192
#define ML_EVENT_PAYLOAD_COUNT_DEFAULT 100
#define ML_SEND_SIGNAL_COUNT_DEFAULT 54

#include "mldebug.h"


/* System description structure. Private to this compilation unit.
 */
static MLsystemRec _thisSystem = { 0 /* init value for ID field */ };


/* Default open options. Private to this compilation unit.
 */
static MLopenOptions defaultOpenOptions =
{
  ML_MODE_RWS,
  ML_SEND_QUEUE_COUNT_DEFAULT,
  ML_RECEIVE_QUEUE_COUNT_DEFAULT,
  ML_MESSAGE_PAYLOAD_SIZE_DEFAULT,
  ML_EVENT_PAYLOAD_COUNT_DEFAULT,
  ML_SEND_SIGNAL_COUNT_DEFAULT,
  ML_XCODE_MODE_ASYNCHRONOUS,
  ML_XCODE_STREAM_SINGLE
};

static MLint64 openOptions[] = {
  ML_OPEN_MODE_INT32,
  ML_OPEN_XCODE_MODE_INT32,
  ML_OPEN_XCODE_STREAM_INT32,
  ML_OPEN_SEND_QUEUE_COUNT_INT32,
  ML_OPEN_RECEIVE_QUEUE_COUNT_INT32,
  ML_OPEN_MESSAGE_PAYLOAD_SIZE_INT32,
  ML_OPEN_EVENT_PAYLOAD_COUNT_INT32,
  ML_OPEN_SEND_SIGNAL_COUNT_INT32
};


/* Bit positions of various components of an ML ID
 */
static const int ML_REF_SHIFT_SYSTEM    = 0;
static const int ML_REF_SHIFT_DEVICE    = 12;
static const int ML_REF_SHIFT_JACK      = 0;
static const int ML_REF_SHIFT_PATH      = 0;
static const int ML_REF_SHIFT_XCODE     = 4;
static const int ML_REF_SHIFT_PIPE	= 0;
static const int ML_REF_SHIFT_UST_SOURCE = 0;


/* ============================================================================
 *
 * Local functions
 *
 * ==========================================================================*/


/* ----------------------------------------------------------_mlStaticParamName
 */
#define CASE_ENUM_TO_TEXT( value ) case value: return #value;
static const char* _mlStaticParamName( MLint64 param)
{
  /* Note - this must be kept in sync with the static params in
   * mlparam.h.
   */
  switch ( param ) {
    CASE_ENUM_TO_TEXT( ML_ID_INT64 );
    CASE_ENUM_TO_TEXT( ML_NAME_BYTE_ARRAY );
    CASE_ENUM_TO_TEXT( ML_PARENT_ID_INT64 );
    CASE_ENUM_TO_TEXT( ML_PARAM_IDS_INT64_ARRAY );
    CASE_ENUM_TO_TEXT( ML_OPEN_OPTION_IDS_INT64_ARRAY );

    CASE_ENUM_TO_TEXT( ML_SYSTEM_DEVICE_IDS_INT64_ARRAY );

    CASE_ENUM_TO_TEXT( ML_DEVICE_VERSION_INT32 );
    CASE_ENUM_TO_TEXT( ML_DEVICE_INDEX_INT32 );
    CASE_ENUM_TO_TEXT( ML_DEVICE_JACK_IDS_INT64_ARRAY );
    CASE_ENUM_TO_TEXT( ML_DEVICE_PATH_IDS_INT64_ARRAY );
    CASE_ENUM_TO_TEXT( ML_DEVICE_XCODE_IDS_INT64_ARRAY );
    CASE_ENUM_TO_TEXT( ML_DEVICE_LOCATION_BYTE_ARRAY );

    CASE_ENUM_TO_TEXT( ML_JACK_TYPE_INT32 );
    CASE_ENUM_TO_TEXT( ML_JACK_DIRECTION_INT32 );
    CASE_ENUM_TO_TEXT( ML_JACK_PATH_IDS_INT64_ARRAY );
    CASE_ENUM_TO_TEXT( ML_JACK_FEATURES_BYTE_ARRAY );

    CASE_ENUM_TO_TEXT( ML_PATH_TYPE_INT32 );
    CASE_ENUM_TO_TEXT( ML_PATH_SRC_JACK_ID_INT64 );
    CASE_ENUM_TO_TEXT( ML_PATH_DST_JACK_ID_INT64 );
    CASE_ENUM_TO_TEXT( ML_PATH_COMPONENT_ALIGNMENT_INT32 );
    CASE_ENUM_TO_TEXT( ML_PATH_BUFFER_ALIGNMENT_INT32 );
    CASE_ENUM_TO_TEXT( ML_PATH_FEATURES_BYTE_ARRAY );

    CASE_ENUM_TO_TEXT( ML_XCODE_ENGINE_TYPE_INT32 );
    CASE_ENUM_TO_TEXT( ML_XCODE_IMPLEMENTATION_TYPE_INT32 );
    CASE_ENUM_TO_TEXT( ML_XCODE_COMPONENT_ALIGNMENT_INT32 );
    CASE_ENUM_TO_TEXT( ML_XCODE_BUFFER_ALIGNMENT_INT32 );
    CASE_ENUM_TO_TEXT( ML_XCODE_FEATURES_BYTE_ARRAY );
    CASE_ENUM_TO_TEXT( ML_XCODE_SRC_PIPE_IDS_INT64_ARRAY );
    CASE_ENUM_TO_TEXT( ML_XCODE_DEST_PIPE_IDS_INT64_ARRAY );

    CASE_ENUM_TO_TEXT( ML_PARAM_TYPE_INT32 );
    CASE_ENUM_TO_TEXT( ML_PARAM_ACCESS_INT32 );

    CASE_ENUM_TO_TEXT( ML_PARAM_DEFAULT_INT32 );
    CASE_ENUM_TO_TEXT( ML_PARAM_DEFAULT_INT64 );
    CASE_ENUM_TO_TEXT( ML_PARAM_DEFAULT_REAL32 );
    CASE_ENUM_TO_TEXT( ML_PARAM_DEFAULT_REAL64 );

    CASE_ENUM_TO_TEXT( ML_PARAM_MINS_INT32_ARRAY );
    CASE_ENUM_TO_TEXT( ML_PARAM_MINS_INT64_ARRAY );
    CASE_ENUM_TO_TEXT( ML_PARAM_MINS_REAL32_ARRAY );
    CASE_ENUM_TO_TEXT( ML_PARAM_MINS_REAL64_ARRAY );

    CASE_ENUM_TO_TEXT( ML_PARAM_MAXS_INT32_ARRAY );
    CASE_ENUM_TO_TEXT( ML_PARAM_MAXS_INT64_ARRAY );
    CASE_ENUM_TO_TEXT( ML_PARAM_MAXS_REAL32_ARRAY );
    CASE_ENUM_TO_TEXT( ML_PARAM_MAXS_REAL64_ARRAY );

    CASE_ENUM_TO_TEXT( ML_PARAM_ENUM_VALUES_ARRAY );
    CASE_ENUM_TO_TEXT( ML_PARAM_ENUM_VALUES_INT32_ARRAY );
    CASE_ENUM_TO_TEXT( ML_PARAM_ENUM_VALUES_INT64_ARRAY );
    CASE_ENUM_TO_TEXT( ML_PARAM_ENUM_NAMES_BYTE_ARRAY );

    CASE_ENUM_TO_TEXT( ML_PARAM_INCREMENT );
    CASE_ENUM_TO_TEXT( ML_PARAM_INCREMENT_INT32 );
    CASE_ENUM_TO_TEXT( ML_PARAM_INCREMENT_INT64 );
    CASE_ENUM_TO_TEXT( ML_PARAM_INCREMENT_REAL32 );
    CASE_ENUM_TO_TEXT( ML_PARAM_INCREMENT_REAL64 );

    CASE_ENUM_TO_TEXT( ML_PARAM_INCREMENT_ARRAY );
    CASE_ENUM_TO_TEXT( ML_PARAM_INCREMENT_INT32_ARRAY );
    CASE_ENUM_TO_TEXT( ML_PARAM_INCREMENT_INT64_ARRAY );
    CASE_ENUM_TO_TEXT( ML_PARAM_INCREMENT_REAL32_ARRAY );
    CASE_ENUM_TO_TEXT( ML_PARAM_INCREMENT_REAL64_ARRAY );

    CASE_ENUM_TO_TEXT( ML_UST_SOURCE_IDS_INT64_ARRAY );
    CASE_ENUM_TO_TEXT( ML_UST_SELECTED_SOURCE_ID_INT64 );
    CASE_ENUM_TO_TEXT( ML_UST_SOURCE_UPDATE_PERIOD_INT32 );
    CASE_ENUM_TO_TEXT( ML_UST_SOURCE_LATENCY_VARIATION_INT32 );

    CASE_ENUM_TO_TEXT( ML_OPEN_MODE_INT32 );
    CASE_ENUM_TO_TEXT( ML_OPEN_XCODE_MODE_INT32 );
    CASE_ENUM_TO_TEXT( ML_OPEN_XCODE_STREAM_INT32 );
    CASE_ENUM_TO_TEXT( ML_OPEN_SEND_QUEUE_COUNT_INT32 );
    CASE_ENUM_TO_TEXT( ML_OPEN_RECEIVE_QUEUE_COUNT_INT32 );
    CASE_ENUM_TO_TEXT( ML_OPEN_MESSAGE_PAYLOAD_SIZE_INT32 );
    CASE_ENUM_TO_TEXT( ML_OPEN_EVENT_PAYLOAD_COUNT_INT32 );
    CASE_ENUM_TO_TEXT( ML_OPEN_SEND_SIGNAL_COUNT_INT32 );

  default: {
    static char msg[48];
    sprintf( msg, "(internal or undefined param 0x%" FORMAT_LLX ")", param );
    return msg;
  }
  } /* switch param */
}


/* ----------------------------------------------------------------getOptOffset
 */
static void *getOptOffset( MLopenOptions* pOpenOpts, MLint64 param )
{
    switch ( param ) {

    case ML_OPEN_MODE_INT32:
      return &pOpenOpts->mode;

    case ML_OPEN_XCODE_MODE_INT32:
      return &pOpenOpts->softwareXcodeMode;

    case ML_OPEN_XCODE_STREAM_INT32:
      return &pOpenOpts->xcodeStream;

    case ML_OPEN_SEND_QUEUE_COUNT_INT32:
      return &pOpenOpts->sendQueueCount;

    case ML_OPEN_RECEIVE_QUEUE_COUNT_INT32:
      return &pOpenOpts->receiveQueueCount;

    case ML_OPEN_MESSAGE_PAYLOAD_SIZE_INT32:
      return &pOpenOpts->messagePayloadSize;

    case ML_OPEN_EVENT_PAYLOAD_COUNT_INT32:
      return &pOpenOpts->eventPayloadCount;

    case ML_OPEN_SEND_SIGNAL_COUNT_INT32:
      return &pOpenOpts->sendSignalCount;
    }

    fprintf( stderr, "[getOptOffset] Internal Error!\n" );
    return NULL;
}


/* ----------------------------------------------------------mliLookupDeviceRec
 */
static MLphysicalDeviceRec* mliLookupDeviceRec( MLint64 objectId, 
						MLint64* staticObjectId )
{
  MLint32 devIndex;
  MLint64 staticDeviceId;
  MLint32 objectType;

  if ( objectId == 0 ) {
    return NULL;
  }

  objectType = mlDIextractIdType( objectId );

  if ( objectType ==  ML_REF_TYPE_SYSTEM ) {
    return NULL;
  }

  if ( objectType == ML_REF_TYPE_DEVICE ) {
    staticDeviceId = objectId;
    *staticObjectId = objectId;

  } else {
    if ( mlDIisOpenId( objectId ) ) {
	*staticObjectId = mlDIconvertOpenIdToStaticId( objectId );
    } else {
      *staticObjectId = objectId;
    }
    if ( (staticDeviceId = mlDIparentIdOfLogDevId( *staticObjectId))==0 ) {
      return NULL;
    }
  }

  for ( devIndex=0; devIndex < _thisSystem.physicalDeviceCount; devIndex++ ) {
    if ( _thisSystem.physicalDevices[devIndex].id == staticDeviceId ) {
      return _thisSystem.physicalDevices+devIndex;
    }
  }
  return NULL;
}


/* ------------------------------------------------------mliLookupOpenDeviceRec
 */
static MLopenDeviceRec* mliLookupOpenDeviceRec( MLint64 objectId )
{
  MLint32 index;

  if ( mlDIextractIdType( objectId ) == ML_REF_TYPE_XCODE ) {
    /* Need to lookup the xcode id and not the pipe id
     */
    objectId = mlDIextractXcodeIdFromPipeId( objectId );
  }
  for ( index = 0; index < ML_MAX_OPEN_DEVICES; index++ ) {
    if ( _thisSystem.openDevices[ index ].id == objectId ) {
      return _thisSystem.openDevices+index;
    }
  }
  return NULL;
}


/* ----------------------------------------------------------mliMakeUSTSourceId
 *
 * Note that this is an internal function, not a DI function -- nobody
 * else needs this functionality
 */
static MLint64 mliMakeUSTSourceId( MLint64 systemId,
				   MLint32 ustSourceIndex )
{
  systemId = systemId; /* Not currently used */
  if ( ustSourceIndex == -1 ) {
    /* Return a special ID indicating this is the "builtin" source
     */
    return ML_BUILTIN_UST_SOURCE;
  }
  return (MLint64) (ML_REF_TYPE_UST_SOURCE | 
		    (ustSourceIndex<<ML_REF_SHIFT_UST_SOURCE)) << 32;
}


/* ----------------------------------------------------_mlSystemGetCapabilities
 */
static MLstatus _mlSystemGetCapabilities( const MLint64 systemId, 
					  MLpv**capabilities )
{
  if ( ML_REF_GET_TYPE( systemId ) != ML_REF_TYPE_SYSTEM ) {
    return ML_STATUS_INVALID_ID;
  }
	
  if ( capabilities == NULL ) {
    return ML_STATUS_INVALID_ARGUMENT;
  }

  if( systemId != ML_SYSTEM_LOCALHOST &&
      systemId != _thisSystem.id ) {
    return ML_STATUS_INVALID_ID;
  }

  {
    MLpv cap[6];
    MLint64 devices[ML_MAX_PHYSICAL_DEVICES];
    MLint32 devCount;
    MLint64 ustSources[ML_MAX_MODULES];
    MLint32 moduleCount;
    MLint32 ustSrcCount;

    for ( devCount=0; devCount < _thisSystem.physicalDeviceCount; devCount++ ){
      devices[devCount] = _thisSystem.physicalDevices[devCount].id;
    }

    ustSrcCount = 0;
    for ( moduleCount=0; moduleCount < _thisSystem.moduleCount;
	  ++moduleCount ) {
      if ( _thisSystem.modules[moduleCount].ustUpdatePeriod > 0 ) {
	ustSources[ustSrcCount] =
	  mliMakeUSTSourceId( _thisSystem.id, moduleCount );
	++ustSrcCount;
      }
    }

    cap[0].param = ML_ID_INT64;
    cap[0].value.int64 = _thisSystem.id;

    cap[1].param = ML_NAME_BYTE_ARRAY;
	assert(_thisSystem.name != NULL);
    cap[1].value.pByte = _thisSystem.name;
    cap[1].length = (MLint32) (strlen( (const char *)_thisSystem.name)+1 );

    cap[2].param = ML_SYSTEM_DEVICE_IDS_INT64_ARRAY;
    cap[2].value.pInt64 = devices;
    cap[2].length = devCount;

    cap[3].param = ML_UST_SOURCE_IDS_INT64_ARRAY;
    cap[3].value.pInt64 = ustSources;
    cap[3].length = ustSrcCount;

    cap[4].param = ML_UST_SELECTED_SOURCE_ID_INT64;
    cap[4].value.int64 = mliMakeUSTSourceId( _thisSystem.id,
					     _thisSystem.ustSrcModuleIndex );

    cap[5].param = ML_END;

    return mlDIcapDup( cap, capabilities );
  }
}


/* ----------------------------------------------------mliExtractUSTSourceIndex
 *
 * Note that this is an internal function, not a DI function -- nobody
 * else needs this functionality
 */
static MLint32 mliExtractUSTSourceIndex( MLint64 id )
{
  mlAssert( ML_REF_GET_TYPE( id ) == ML_REF_TYPE_UST_SOURCE );
  return ((MLint32) (id >> 32) & ML_REF_MASK_UST_SOURCE) >>
    ML_REF_SHIFT_UST_SOURCE;
}


/* -------------------------------------------------------_mlUSTGetCapabilities
 */
static MLstatus _mlUSTGetCapabilities( MLint64 objectId,
				       MLpv** capabilities )
{
  MLpv cap[7];
  MLint32 moduleIndex = mliExtractUSTSourceIndex( objectId );
  MLmoduleRec* pModule;
  int i;

  /* For now, hard-coded to use _thisSystem
   */
  pModule = &(_thisSystem.modules[moduleIndex]);
  if ( pModule->ustUpdatePeriod <= 0 ) {
    /* This means this module does *not* provide a UST source. The ID
     * is invalid.
     */
    return ML_STATUS_INVALID_ID;
  }

  i = 0;
  cap[i].param = ML_ID_INT64;
  cap[i].value.int64 = objectId;

  ++i;
  cap[i].param = ML_PARENT_ID_INT64;
  cap[i].value.int64 = mlDIparentIdOfDeviceId( objectId );

  ++i;
  cap[i].param = ML_NAME_BYTE_ARRAY;
  cap[i].value.pByte = (MLbyte*) pModule->ustName;
  cap[i].length = pModule->ustNameSize;
  cap[i].maxLength = cap[i].length;

  ++i;
  cap[i].param = ML_DESCRIPTION_BYTE_ARRAY;
  cap[i].value.pByte = (MLbyte*) pModule->ustDescription;
  cap[i].length = pModule->ustDescriptionSize;
  cap[i].maxLength = cap[i].length;

  ++i;
  cap[i].param = ML_UST_SOURCE_UPDATE_PERIOD_INT32;
  cap[i].value.int32 = pModule->ustUpdatePeriod;

  ++i;
  cap[i].param = ML_UST_SOURCE_LATENCY_VARIATION_INT32;
  cap[i].value.int32 = pModule->ustLatencyVar;

  ++i;
  cap[i].param = ML_END;

  return mlDIcapDup( cap, capabilities );
}


/* -------------------------------------------------mlOpenOptionPvGetCapability
 */
static MLstatus
mlOpenOptionPvGetCapability( const MLint64 staticObjectId,
			     const MLint64 paramId,
			     MLpv **capabilities )
{
  MLint32 i;

  for ( i = 0; i < lengthof( openOptions ); i++ ) {
    if ( paramId == openOptions[ i ] ) {

      MLint32 *pDefault;
      MLpv* caps = (MLpv*) mlMalloc( sizeof( MLpv )*7 ), *pv = caps;

      if ( caps == NULL ) {
	return ML_STATUS_OUT_OF_MEMORY;
      }

      pv->param = ML_ID_INT64;
      pv->value.int64 = paramId;
      pv->length = 1;
      pv++;

      pv->param = ML_NAME_BYTE_ARRAY;
      pv->value.pByte = (MLbyte*) _mlStaticParamName( paramId );
      pv->maxLength = pv->length =
	(MLint32) strlen( (char*)pv->value.pByte ) + 1 ;
      pv++;

      pv->param = ML_PARENT_ID_INT64;
      pv->value.int64 = staticObjectId;
      pv->length = 1;
      pv++;

      pv->param = ML_PARAM_TYPE_INT32;
      pv->value.int32 = ML_PARAM_GET_TYPE( paramId );
      pv->length = 1;
      pv++;

      pDefault = getOptOffset( &defaultOpenOptions, paramId );
      pv->param = ML_PARAM_DEFAULT_INT32;
      pv->value.int32 = *pDefault;
      pv->length = 1;
      pv++;

      pv->param = ML_PARAM_ACCESS_INT32;
      pv->value.int32 = ML_ACCESS_OPEN_OPTION | ML_ACCESS_RW;
      pv->length = 1;
      pv++;

      pv->param = ML_END;

      *capabilities = caps;
      return ML_STATUS_NO_ERROR;
    }
  }
  return ML_STATUS_INVALID_ID;
}


/* ------------------------------------------------------------------_flatticep
 *
 * Is value on the (floating point) lattice minValue + k * incr?
 * incr must be non-zero.
 */
static MLint32 _flatticep( MLreal64 value, MLreal64 minValue, MLreal64 incr )
{
  /* Reduce modulo incr
   */
  MLreal64 t = fmod( fabs( value - minValue ), fabs( incr ) );
  MLreal64 u = fabs( t - incr ); /* Once more may get us just beneath zero */

  mlAssert( incr > 0 );

  if ( u < t ) { /* Choose the reduction nearest to zero */
    t = u;
  }

  if ( t <= DI_LATTICE_VALIDATE_DELTA * fabs(incr) ||
       t <= DBL_EPSILON * fabs(value) ) {
    return 0;
  } else {
    return -1;
  }
}


/* -------------------------------------------------------------------sprintval
 */
static void sprintval( char *errorString, char *p1, char *p2,
		       MLint64 param, MLvalue value )
{
  char *s = errorString;

  s += sprintf( s, p1 );

  switch ( ML_PARAM_GET_TYPE_ELEM( param ) ) {
  case ML_TYPE_ELEM_BYTE:
    s += sprintf( s, " %d (0x%x) ", value.byte, value.byte );
    break;

  case ML_TYPE_ELEM_INT32:
    s += sprintf( s, " %d (0x%x) ", value.int32, value.int32 );
    break;

  case ML_TYPE_ELEM_INT64:
    s += sprintf( s, " %" FORMAT_LLX " ", value.int64 );
    break;

  case ML_TYPE_ELEM_REAL32:
    s += sprintf( s, " %f ", value.real32 );
    break;

  case ML_TYPE_ELEM_REAL64:
#if defined(ML_OS_LINUX) || defined(COMPILER_GCC)
    s += sprintf( s, " %f ", value.real64 );
#else
    s += sprintf( s, " %llf ", value.real64 );
#endif
    break;

  case ML_TYPE_ELEM_PV:
  case ML_TYPE_ELEM_MSG:
    break;
  }

  sprintf( s, p2 );
}


/* ---------------------------------------------------------mlDIvalidatePvValue
 */
static MLstatus mlDIvalidatePvValue( char *errorString, MLint64 param,
				     MLvalue value, MLpv* paramCapVec )
{
  MLstatus status = ML_STATUS_NO_ERROR;
  MLint32 match = 0;

  MLint32 type = ML_PARAM_GET_TYPE_ELEM( param );
  MLint64 minsId = ML_PARAM_MINS_ARRAY | type;
  MLint64 maxsId = ML_PARAM_MAXS_ARRAY | type;
  MLint64 incId  = ML_PARAM_INCREMENT  | type;
  MLint64 incAId = ML_PARAM_INCREMENT_ARRAY  | type;
  MLint64 enumValueId = ML_PARAM_ENUM_VALUES_ARRAY | type;

  MLpv* paramEnums = mlPvFind( paramCapVec, enumValueId);
  MLpv* paramMins  = mlPvFind( paramCapVec, minsId );
  MLpv* paramMaxs  = mlPvFind( paramCapVec, maxsId );
  MLpv* paramInc   = mlPvFind( paramCapVec, incId );
  MLpv* paramIncA  = mlPvFind( paramCapVec, incAId );

  if ( paramMins != NULL || paramMaxs != NULL ) {
    if ( paramMins == NULL ) {
      sprintf( errorString, "param capabiltiies has maxs but not mins" );
      status = ML_STATUS_INTERNAL_ERROR;
      goto PARAM_ERROR;
    }
    if ( paramMaxs == NULL ) {
      sprintf( errorString, "param capabiltiies has mins but not maxs" );
      status = ML_STATUS_INTERNAL_ERROR;
      goto PARAM_ERROR;
    }
    if ( paramMins->length < 0 ) {
      sprintf( errorString, "param capabiltiies has mins length < 0" );
      status = ML_STATUS_INTERNAL_ERROR;
      goto PARAM_ERROR;
    }
    if ( paramMaxs->length != paramMins->length ) {
      sprintf( errorString,
	       "param capabiltiies has mins and maxs of unequal length" );
      status = ML_STATUS_INTERNAL_ERROR;
      goto PARAM_ERROR;
    }

    if ( paramIncA != NULL ) {
      if ( paramIncA->length != paramMins->length ) {
	sprintf( errorString, "param capabiltiies has min/maxs and increments "
		 "of unequal length" );
	status = ML_STATUS_INTERNAL_ERROR;
	goto PARAM_ERROR;
      }
    }

  } else {
    if ( paramInc != NULL || paramIncA != NULL ) {
      sprintf( errorString,
	       "param capabiltiies has an increment but no mins or maxs " );
      status = ML_STATUS_INTERNAL_ERROR;
      goto PARAM_ERROR;
    }
  }

  if ( paramEnums != NULL && paramEnums->length > 0 ) { 
    MLint32 i, match = 0;
    for ( i=0; i < paramEnums->length; i++ ) {
      switch ( type ) {
      case ML_TYPE_ELEM_BYTE:
	match = ( value.byte == paramEnums->value.pByte[i] );
	break;
      case ML_TYPE_ELEM_INT32:
	match = ( value.int32 == paramEnums->value.pInt32[i] );
	break;
      case ML_TYPE_ELEM_INT64:
	match = ( value.int64 == paramEnums->value.pInt64[i] );
	break;
      case ML_TYPE_ELEM_REAL32:
	match = ( value.real32 == paramEnums->value.pReal32[i] );
	break;
      case ML_TYPE_ELEM_REAL64:
	match = ( value.real64 == paramEnums->value.pReal64[i] );
	break;
      }
      if ( match ) {
	break;
      }
    }
    if ( ! match ) {
      goto invalid_value;
    }
  }

  if ( paramMins != NULL && paramMins->length > 0 ) {
    MLint32 i;
    for ( i=0; i < paramMins->length; i++ ) {
      switch ( type ) {
      case ML_TYPE_ELEM_BYTE:
	match = ( value.byte >= paramMins->value.pByte[i] &&
		  value.byte <= paramMaxs->value.pByte[i] );
	break;
      case ML_TYPE_ELEM_INT32:
	match = ( value.int32 >= paramMins->value.pInt32[i] &&
		  value.int32 <= paramMaxs->value.pInt32[i] );
	break;
      case ML_TYPE_ELEM_INT64:
	match = ( value.int64 >= paramMins->value.pInt64[i] &&
		  value.int64 <= paramMaxs->value.pInt64[i] );
	break;
      case ML_TYPE_ELEM_REAL32:
	match = ( value.real32 >= paramMins->value.pReal32[i] &&
		  value.real32 <= paramMaxs->value.pReal32[i] );
	break;
      case ML_TYPE_ELEM_REAL64:
	match = ( value.real64 >= paramMins->value.pReal64[i] &&
			      value.real64 <= paramMaxs->value.pReal64[i] );
	break;
      }
      if ( match ) {
	break;
      }
    }
    if ( ! match ) {
      goto invalid_value;
    }
  }

  if ( paramInc ) {
    MLint32 i;
    MLvalue incr = paramInc->value;
    assert( paramMins->length > 0 );

    for ( i = 0; i < paramMins->length; i++ ) {
      switch ( type ) {
      case ML_TYPE_ELEM_BYTE:
	match = ( incr.byte == 0 ) || ( incr.byte == 1 ) ||
	  ( (value.byte - paramMins->value.pByte[i]) % incr.byte == 0 );
	break;
      case ML_TYPE_ELEM_INT32:
	match = ( incr.int32 == 0 ) || ( incr.int32 == 1 ) ||
	  ( (value.int32 - paramMins->value.pInt32[i]) % incr.int32 == 0 );
	break;
      case ML_TYPE_ELEM_INT64:
	match = ( incr.int64 == 0 ) || ( incr.int64 == 1 ) ||
	  ( (value.int64 - paramMins->value.pInt64[i]) % incr.int64 == 0 );
	break;
      case ML_TYPE_ELEM_REAL32:
	match = ( incr.real32 == 0.0 ) || ( incr.real32 == 1.0 ) ||
	  ! _flatticep( value.real32, paramMins->value.pReal32[i],
			incr.real32 );
	break;
      case ML_TYPE_ELEM_REAL64:
	match = ( incr.real64 == 0.0 ) || ( incr.real64 == 1.0 ) ||
	  ! _flatticep( value.real64, paramMins->value.pReal64[i],
			incr.real64 );
	break;
      }
      if ( match ) {
	break;
      }
    }
    if ( ! match ) {
      sprintval( errorString,
		 "a value of", "fails increment validation", param, value );
      status = ML_STATUS_INVALID_VALUE;
      goto PARAM_ERROR;
    }

  } else if ( paramIncA && paramIncA->length > 0 ) {
    MLint32 i;
    assert( paramMins->length == paramIncA->length );

    for ( i = 0; i < paramMins->length; i++ ) {
      switch ( type ) {
      case ML_TYPE_ELEM_BYTE: {
	MLbyte incr = paramIncA->value.pByte[i];
	match = ( incr == 0 ) || ( incr == 1 ) ||
	  ( (value.byte - paramMins->value.pByte[i]) % incr == 0 );
	break;
      }
      case ML_TYPE_ELEM_INT32: {
	MLint32 incr = paramIncA->value.pInt32[i];
	match = ( incr == 0 ) || ( incr == 1 ) ||
	  ( (value.int32 - paramMins->value.pInt32[i]) % incr == 0 );
	break;
      }
      case ML_TYPE_ELEM_INT64: {
	MLint64 incr = paramIncA->value.pInt64[i];
	match = ( incr == 0 ) || ( incr == 1 ) ||
	  ( (value.int64 - paramMins->value.pInt64[i]) % incr == 0 );
	break;
      }
      case ML_TYPE_ELEM_REAL32: {
	MLreal32 incr = paramIncA->value.pReal32[i];
	match = ( incr == 0.0 ) || ( incr == 1.0 ) ||
	  ! _flatticep( value.real32, paramMins->value.pReal32[i], incr );
	break;
      }
      case ML_TYPE_ELEM_REAL64: {
	MLreal64 incr = paramIncA->value.pReal64[i];
	match = ( incr == 0.0 ) || ( incr == 1.0 ) ||
	  ! _flatticep( value.real64, paramMins->value.pReal64[i], incr );
	break;
      }
      }
      if ( match ) {
	break;
      }
    }
    if ( ! match ) {
      sprintval( errorString,
		 "a value of", "fails increment validation", param, value );
      status = ML_STATUS_INVALID_VALUE;
      goto PARAM_ERROR;
    }
  }
  return status;

 invalid_value:
  sprintval( errorString,
	     "a value of", "is not legal on this device", param, value );
  status = ML_STATUS_INVALID_VALUE;
  goto PARAM_ERROR;

 PARAM_ERROR:
  return status;
}


/* -----------------------------------------------_mliAcquireOpenDeviceInstance
 */
static
MLstatus _mliAcquireOpenDeviceInstance( MLint64 objectId,
					MLopenDeviceRec* *pOpenDevReturn )
{
  MLint64 staticObjectId;
  MLphysicalDeviceRec* pDevice =
    mliLookupDeviceRec( objectId, &staticObjectId );
  MLint32 index;

  /* First find a free slot in the openDevice record. We don't use the
   * first slot - the first open device is 1.
   */
  for ( index=1; index < ML_MAX_OPEN_DEVICES; index++ ) {
    if ( _thisSystem.openDevices[index].id < 4 ) {
      /* Found a free open device slot, so use that index to generate
       * a unique openid for this device.
       */
      MLopenDeviceRec* pOpenDev = _thisSystem.openDevices+index;
      pOpenDev->id = staticObjectId | index;
      pOpenDev->device = pDevice;
      pOpenDev->objectid = staticObjectId;
      pOpenDev->ddPriv = NULL;

      *pOpenDevReturn = pOpenDev;
      return ML_STATUS_NO_ERROR;
    }
  }
  return ML_STATUS_INSUFFICIENT_RESOURCES;
}


/* --------------------------------------------_mliRelinquishOpenDeviceInstance
 */
static MLstatus _mliRelinquishOpenDeviceInstance( MLopenDeviceRec* pOpenDev )
{
  if ( NULL == pOpenDev ) { /* Undecipherable reference! */
    return ML_STATUS_INVALID_ID;
  }

  /* Use very small id value to indicate when no longer in use
   */
  pOpenDev->id = 1; 
	
  return ML_STATUS_NO_ERROR;
}


/* ---------------------------------------------------------_valueInt32ToString
 *
 * Convert a mlPv.value.int32 to an ASCII string.
 *
 * For enumerated parameters, output the name of the enumeration,
 * otherwise just output an int.
 *
 * size is the number of bytes in the buffer, and is modified in the
 * call to return the number of bytes written (excluding the
 * terminating \0)
 */
static MLstatus _valueInt32ToString( MLpv* paramCap, MLint32 value, 
				     char* buffer, MLint32* size )
{
  int i;
  char* desc;

  MLpv *enumValues, *enumNames;

  enumValues = mlPvFind( paramCap, ML_PARAM_ENUM_VALUES_INT32_ARRAY );
  enumNames = mlPvFind( paramCap, ML_PARAM_ENUM_NAMES_BYTE_ARRAY );

  if ( enumNames==NULL || enumValues==NULL || 
       enumValues->length==0 || enumNames->length == 0 ) {
    /* Its not an enumerated parameter, so, provided there is
     * sufficient space, we can simply print the value.
     */
    if ( *size < 12 ) { /* quick conservative check */
      *size=0;
      return ML_STATUS_INVALID_ARGUMENT;
    }
    *size = sprintf( buffer, "%d", value );
    return ML_STATUS_NO_ERROR;
  }

  desc = (char*) (enumNames->value.pByte);
  for ( i=0; i < enumValues->length; i++ ) {
    int descSize = (int) strlen( desc )+1;
    if ( enumValues->value.pInt32[i] == value ) {
      /* Enum value found
       */
      if ( descSize > *size ) {
	*size = 0;
	return ML_STATUS_INVALID_ARGUMENT;
      }
      *size = sprintf( buffer, "%s", desc );
      return ML_STATUS_NO_ERROR;
    }
    /* The i'th value doesn't match so advance to the next one
     */
    desc += descSize;
  }
  
#ifndef NDEBUG
  {
    MLpv* paramName = mlPvFind( paramCap, ML_NAME_BYTE_ARRAY );
    if ( paramName != NULL && paramName->value.pByte != NULL ) {
      mlDebug("[mlPvValueToString] Error converting param %s\n",
	      paramName->value.pByte );
    }
    mlDebug( "[mlPvValueToString] found enumerated param with an "
	     "illegal value (0x%x).\n", value );
  }
#endif
  *size = 0;
  return ML_STATUS_INVALID_VALUE;
}

/* ---------------------------------------------------------_valueInt64ToString
 *
 * Convert a mlPv.value.int64 to an ASCII string.
 *
 * For enumerated parameters, output the name of the enumeration,
 * otherwise just output an int.
 *
 * size is the number of bytes in the buffer, and is modified in the
 * call to return the number of bytes written (excluding the
 * terminating \0)
 */
static MLstatus _valueInt64ToString( MLpv* paramCap, MLint64 value, 
				     char* buffer, MLint32* size )
{
  int i;
  char* desc;

  MLpv *enumValues, *enumNames;

  enumValues = mlPvFind( paramCap, ML_PARAM_ENUM_VALUES_INT64_ARRAY );
  enumNames = mlPvFind( paramCap, ML_PARAM_ENUM_NAMES_BYTE_ARRAY );
	    
  if ( enumNames==NULL || enumValues==NULL || 
       enumValues->length==0 || enumNames->length == 0 ) {
    /* Its not an enumerated parameter, so, provided there is
     * sufficient space, we can simply print the value.  Since it's a
     * 64 bit number, usually printing it in hex makes more sense.
     * Most likely it's an ID.
     */
    if ( *size < 18 ) { /* quick conservative check */
      *size=0;
      return ML_STATUS_INVALID_ARGUMENT;
    }
    *size = sprintf( buffer, "0x%" FORMAT_LLX, value );
    return ML_STATUS_NO_ERROR;
  }

  desc = (char*) (enumNames->value.pByte);
  for ( i=0; i < enumValues->length; i++ ) {
    int descSize = (int) strlen( desc )+1;
    if ( enumValues->value.pInt64[i] == value ) {
      /* Enum value found
       */
      if ( descSize > *size ) {
	*size = 0;
	return ML_STATUS_INVALID_ARGUMENT;
      }
      *size = sprintf( buffer, "%s", desc );
      return ML_STATUS_NO_ERROR;
    }
    /* The i'th value doesn't match so advance to the next one
     */
    desc += descSize;
  }

#ifndef NDEBUG
  {
    MLpv* paramName = mlPvFind( paramCap, ML_NAME_BYTE_ARRAY );
    if ( paramName != NULL && paramName->value.pByte != NULL ) {
      mlDebug( "[mlPvValueToString] Error converting param %s\n",
	       paramName->value.pByte );
    }
    mlDebug( "[mlPvValueToString] found enumerated param with an "
	     "illegal value (0x%" FORMAT_LLX ").\n", value );
  }
#endif
  *size = 0;
  return ML_STATUS_INVALID_VALUE;
}


/* -----------------------------------------------------------------mlstringcmp
 *
 * compare string 'a' and string 'b' for equality
 *
 * string a is a normal null terminated string
 * string b has possible tokens that can terminate it
 */
static int mlstringcmp( const char *sa, const char *sb )
{
  static char *suffices[] = {
    "_BYTE", "_INT32", "_INT64", "_REAL32", "_REAL64",
    "_POINTER", "_ARRAY", NULL
  };
  static char *string_tokens = "= \t\n";
  MLint32 la = 0, lb = 0;

  if ( strncasecmp( sa, "ML_", 3 ) == 0 ) { sa += 3; la += 3; }
  if ( strncasecmp( sb, "ML_", 3 ) == 0 ) { sb += 3; lb += 3; }

  for ( ;; ) {
    register char ca, cb;
    if ( (ca = *sa) ) { sa++; }
    if ( (cb = *sb) ) { sb++; }
    if ( toupper( ca ) == toupper( cb ) ) {
      if ( ca == '\0' ) {
	return lb;
      }
      la++; 
      lb++;
      continue;
    }
    if ( ca == '\0' ) {
      char **p;
      int found = 0;
      if ( strchr( string_tokens, cb ) != NULL ) {
	return lb;
      }
      for ( p = suffices; *p; p++ ) {
	MLint32 l = (MLint32) strlen( *p );
	if ( strncasecmp( sb-1, *p, l ) == 0 ) {
	  lb += l;
	  sb += l-1;
	  found = 1;
	  break;
	}
      }
      if ( found ) {
	continue;
      }
      return 0;
    }
    if ( cb == '\0' || strchr( string_tokens, cb ) != NULL ) {
      char **p;
      int found = 0;
      for ( p = suffices; *p; p++ ) {
	MLint32 l = (MLint32) strlen( *p );
	if ( strncasecmp( sa-1, *p, l ) == 0 ) {
	  la += l;
	  sa += l-1;
	  found = 1;
	  break;
	}
      }
      if ( found ) {
	if ( cb != '\0' ) {
	  sb--;
	}
	continue;
      }
      return 0;
    }
    if ( ca == '_' && lb > 0 ) {
      sb--;
      lb--;
      continue;
    }
    if ( cb == '_' && la > 0 ) {
      sa--;
      la--;
      continue;
    }
    return 0;
  }
  /* NOT REACHED */
}


/* -------------------------------------------------------_valueInt32FromString
 *
 * Given a string, interpret it as a mlPv.value.int32
 *
 * For enumerated parameters, we expect the string to be the name of
 * an enumerated value, otherwise its just an int.
 *
 * The string must previously have been generated by
 * mlPvValueInt32ToString
 *
 * returned size is the number of bytes read
 */
static MLstatus _valueInt32FromString( MLpv* paramCap, MLint32* value, 
				       const char* buffer, MLint32* size )
{
  int i;
  char* desc;
  MLpv *enumValues, *enumNames;

  enumValues = mlPvFind( paramCap, ML_PARAM_ENUM_VALUES_INT32_ARRAY );
  enumNames = mlPvFind( paramCap, ML_PARAM_ENUM_NAMES_BYTE_ARRAY );
	    
  if ( enumNames==NULL || enumValues==NULL ||
       enumNames->length==0 || enumValues->length==0 ||
       isdigit( buffer[0] ) ) {
    /* Its not an enumerated parameter, so, we no longer need any
     * param capabilites and can simply read the value.
     */
    int retSize=0;
    if ( sscanf( buffer, "%d%n", value, &retSize ) == 1 ) {
      if ( size != NULL ) {
	*size = retSize;
      }
      return ML_STATUS_NO_ERROR;
    } else {
      if ( size != NULL) {
	*size=0;
      }
      return ML_STATUS_INVALID_VALUE;
    }
  }

  desc = (char*) (enumNames->value.pByte);
  for ( i=0; i < enumValues->length; i++ ) {
    MLint32 matchLength = mlstringcmp( desc, buffer );
    if ( matchLength ) {
      /* Enum value found
       */
      if ( size != NULL ) {
	*size = matchLength;
      }
      *value = enumValues->value.pInt32[i];
      return ML_STATUS_NO_ERROR;
    }
    /* The i'th value doesn't match so advance to the next one
     */
    desc += (int) strlen( desc )+1;
  }
  if ( size != NULL ) {
    *size=0;
  }

  return ML_STATUS_INVALID_VALUE;
}


/* -------------------------------------------------------_valueInt64FromString
 *
 * Given a string, interpret it as a mlPv.value.int64
 *
 * For enumerated parameters, we expect the string to be the name of
 * an enumerated value, otherwise its just an int.
 *
 * The string must previously have been generated by
 * mlPvValueInt64ToString
 *
 * returned size is the number of bytes read
 */
static MLstatus _valueInt64FromString( MLpv* paramCap, MLint64* value, 
				       const char* buffer, MLint32* size )
{
  int i;
  char* desc;
  MLpv *enumValues, *enumNames;

  enumValues = mlPvFind( paramCap, ML_PARAM_ENUM_VALUES_INT64_ARRAY );
  enumNames = mlPvFind( paramCap, ML_PARAM_ENUM_NAMES_BYTE_ARRAY );
	    
  if( enumNames==NULL || enumValues==NULL ||
      enumNames->length==0 || enumValues->length==0 ||
      isdigit( buffer[0] ) ) {
    /* Its not an enumerated parameter, so, we no longer need any
     * param capabilites and can simply read the value.
     */
    int retSize=0;
    if ( sscanf( buffer, "%lld%n", value, &retSize ) == 1 ) {
      if ( size != NULL ) {
	*size = retSize;
      }
      return ML_STATUS_NO_ERROR;

    } else {
      if ( size != NULL ) {
	*size=0;
      }
      return ML_STATUS_INVALID_VALUE;
    }
  }

  desc = (char*) (enumNames->value.pByte);
  for ( i=0; i < enumValues->length; i++ ) {
    MLint32 matchLength = mlstringcmp( desc, buffer );
    if ( matchLength ) {
      /* Enum value found
       */
      if ( size != NULL ) {
	*size = matchLength;
      }
      *value = enumValues->value.pInt64[i];
      return ML_STATUS_NO_ERROR;
    }
    /* The i'th value doesn't match so advance to the next one
     */
    desc += (int) strlen( desc )+1;
  }
  if ( size != NULL ) {
    *size=0;
  }

  return ML_STATUS_INVALID_VALUE;
}


/* ------------------------------------------------------------_valueFromString
 *
 * Given a string, interpret it as a mlPv.value
 *
 * The string must previously have been generated by
 * mlPvValueInt32ToString
 *
 * returned size is the number of bytes read
 */ 
static MLstatus _valueFromString( MLpv* paramCap, const char* buffer, 
				  MLint32* size, MLpv* pv, 
				  MLbyte* arrayData, MLint32 arrayLength )
{
  MLint32 bytes_read = 0;

  if ( ML_PARAM_IS_POINTER( pv->param ) ) {
    /* can't read/write pointers */
    pv->value.pByte=NULL;
    pv->length=0;
    pv->maxLength=0;
    bytes_read=0;

  } else if ( ML_PARAM_IS_ARRAY( pv->param ) ) {
    int i;

    if ( ML_PARAM_GET_TYPE_ELEM( pv->param ) == ML_TYPE_ELEM_BYTE ) {
      if ( *buffer != '\"' ) {
	return ML_STATUS_INVALID_VALUE;
      } else {
	bytes_read++;
      }
    } else {
      if ( *buffer != '[' ) {
	return ML_STATUS_INVALID_VALUE;
      } else {
	bytes_read++;
      }
    }

    pv->value.pByte = arrayData;
    pv->length = 0;
    pv->maxLength = arrayLength/_mlDIPvSizeofElem( pv->param );

    for ( i=0; i < arrayLength/_mlDIPvSizeofElem( pv->param ); i++ ) {
      MLint32 space_for_punctuation = 1; /* comma or closing bracket */
      MLint32 this_value_size = 0;

      if ( (ML_PARAM_GET_TYPE_ELEM( pv->param ) == ML_TYPE_ELEM_BYTE ) &&
	   ((*(buffer+bytes_read)=='\"') ||
	    (*(buffer+bytes_read)== ']')) ) {
	    break;
      }

      if ( (*(buffer+bytes_read)=='\0') ||
	   (*(buffer+bytes_read)=='\n') ) {
	return ML_STATUS_INVALID_VALUE;
      }

      if ( *(buffer+bytes_read)==',' ) {
	bytes_read++;
      }

      switch ( ML_PARAM_GET_TYPE_ELEM( pv->param ) ) {
      case ML_TYPE_ELEM_BYTE:
	if ( *(buffer+bytes_read) != '\\' ) {
	  *(arrayData + i) = *(buffer+bytes_read);
	  this_value_size = 1;
	} else {
	  int value;
	  if ( sscanf( buffer+bytes_read, "\\%d%n", &value, &this_value_size )
	       < 1 ) {
	    return ML_STATUS_INVALID_VALUE;
	  }
	  *(arrayData + i) = value;
	}
	break;

      case ML_TYPE_ELEM_INT32: {
	MLstatus status;
	if ( (status = _valueInt32FromString( paramCap, 
					      ((MLint32*) arrayData)+i,
					      buffer+bytes_read, 
					      &this_value_size ) )
	     != ML_STATUS_NO_ERROR ) {
	  return status;
	}
	break;
      }

      case ML_TYPE_ELEM_INT64:
	if ( sscanf( buffer+bytes_read, "%" FORMAT_LLX "%n",
		     ((MLint64*) arrayData + i), &this_value_size) < 1 ) {
	  return ML_STATUS_INVALID_VALUE;
	}
	break;

      case ML_TYPE_ELEM_REAL32: 
	if ( sscanf( buffer+bytes_read, "%f%n",
		     ((MLreal32*) arrayData + i), &this_value_size) < 1 ) {
	  return ML_STATUS_INVALID_VALUE;
	}
	break;

      case ML_TYPE_ELEM_REAL64: 
	if ( sscanf( buffer+bytes_read, "%lf%n",
		     ((MLreal64*) arrayData + i), &this_value_size) < 1 ) {
	  return ML_STATUS_INVALID_VALUE;
	}
	break;

      default:
	return ML_STATUS_INVALID_VALUE;
      } /* switch ML_PARAM_GET_TYPE_ELEM */
      bytes_read += (this_value_size+space_for_punctuation);
    } /* for i=0... */
    pv->length = i;
    arrayLength = i*_mlDIPvSizeofElem(pv->param);

  } else {
    MLint32 this_value_size = *size-bytes_read;

    switch ( ML_PARAM_GET_TYPE( pv->param ) ) {
    case ML_TYPE_INT32: {
      MLstatus status;
      if ( (status = _valueInt32FromString( paramCap, &(pv->value.int32),
					    buffer+bytes_read, 
					    &this_value_size )) ) {
	return status;
      }
      break;
    }

    case ML_TYPE_INT64: {
      MLstatus status;
      if ( (status = _valueInt64FromString( paramCap, &(pv->value.int64),
					    buffer+bytes_read, 
					    &this_value_size )) ) {
	return status;
      }
      break;
    }

    case ML_TYPE_REAL32: 
      if ( sscanf( buffer+bytes_read, "%f%n",
		   &(pv->value.real32), &this_value_size ) < 1 ) {
	return ML_STATUS_INVALID_VALUE;
      }
      break;

    case ML_TYPE_REAL64: 
      if ( sscanf( buffer+bytes_read, "%lf%n",
		   &(pv->value.real64), &this_value_size ) < 1 ) {
	return ML_STATUS_INVALID_VALUE;
      }
      break;

    default:
      return ML_STATUS_INVALID_VALUE;
    } /* switch ML_PARAM_GET_TYPE... */
    bytes_read = this_value_size;
    pv->length = 1;  /* by convention, scalars have length 1 */
    arrayLength = 0; /* no array space necessary for scalar */
  }
  *size = bytes_read;
  return ML_STATUS_NO_ERROR;
}


/* ============================================================================
 *
 * Public functions
 *
 * ==========================================================================*/


/* ---------------------------------------------------------------------_mlInit
 *
 * OS-independent initialisation -- to be called at system startup
 * (see mlgenesis.c)
 */
MLstatus _mlInit( void )
{
  /* Extra safety -- the 'id' field is set to 0 statically, so all
   * this is not strictly necessary.
   */
  _thisSystem.id = 0;
  _thisSystem.physicalDeviceCount = 0;
  _thisSystem.moduleCount = 0;

  return ML_STATUS_NO_ERROR;
}


/* ------------------------------------------------------------------_mlExiting
 *
 * OS-independent finalisation -- to be called at system tear-down
 * (see mlgenesis.c)
 */
MLstatus _mlExiting( void )
{
  /* Is this absolutely necessary? This function is called 'atexit'
   * (on Unix), or at the very end of dllmain() (on Windows) -- so
   * what code could possible execute after this, that might try to
   * make use of ML functionality?
   */
  return _mlInit();
}


/* -----------------------------------------------------------------mlDebugStop
 *
 * Purely here for mlmodule debugging
 */
void mlDebugStop( void )
{}



/* ----------------------------------------------------------------mlStatusName
 */
const char* MLAPI mlStatusName( MLstatus status )
{
  /* Note - this must be kept in sync with the status definitions in
   * mldefs.h.  To enforce this, we use a switch statement.
   */
  switch ( (enum mlStatusReturnEnum) status ) {
    CASE_ENUM_TO_TEXT( ML_STATUS_NO_ERROR );
    CASE_ENUM_TO_TEXT( ML_STATUS_NO_OPERATION );
    CASE_ENUM_TO_TEXT( ML_STATUS_OUT_OF_MEMORY );
    CASE_ENUM_TO_TEXT( ML_STATUS_INVALID_ID );
    CASE_ENUM_TO_TEXT( ML_STATUS_INVALID_ARGUMENT );
    CASE_ENUM_TO_TEXT( ML_STATUS_INVALID_VALUE );
    CASE_ENUM_TO_TEXT( ML_STATUS_INVALID_PARAMETER );
    CASE_ENUM_TO_TEXT( ML_STATUS_RECEIVE_QUEUE_EMPTY );
    CASE_ENUM_TO_TEXT( ML_STATUS_SEND_QUEUE_OVERFLOW );
    CASE_ENUM_TO_TEXT( ML_STATUS_RECEIVE_QUEUE_OVERFLOW );
    CASE_ENUM_TO_TEXT( ML_STATUS_INSUFFICIENT_RESOURCES );
    CASE_ENUM_TO_TEXT( ML_STATUS_DEVICE_UNAVAILABLE );
    CASE_ENUM_TO_TEXT( ML_STATUS_ACCESS_DENIED );
    CASE_ENUM_TO_TEXT( ML_STATUS_DEVICE_ERROR );
    CASE_ENUM_TO_TEXT( ML_STATUS_DEVICE_BUSY );
    CASE_ENUM_TO_TEXT( ML_STATUS_INVALID_CONFIGURATION );
    CASE_ENUM_TO_TEXT( ML_STATUS_GENLOCK_NO_SIGNAL );
    CASE_ENUM_TO_TEXT( ML_STATUS_GENLOCK_UNKNOWN_SIGNAL );
    CASE_ENUM_TO_TEXT( ML_STATUS_GENLOCK_ILLEGAL_COMBINATION );
    CASE_ENUM_TO_TEXT( ML_STATUS_GENLOCK_TIMING_MISMATCH );
    CASE_ENUM_TO_TEXT( ML_STATUS_INTERNAL_ERROR );

    /* NO default clause here - we want the compiler to tell us if we
     * miss one.
     */
  }
  {
    static char msg[80];
    sprintf( msg, "Unknown status %d", status );
    return msg;
  }
}


/* ---------------------------------------------------------------mlMessageName
 */
const char* MLAPI mlMessageName( MLint32 messageType )
{
  /* Note - this must be kept in sync with the message type
   * definitions in mldefs.h.  To enforce this, we use a switch
   * statement.
   */
  switch ( (enum mlMessageTypeEnum) messageType ) {
    CASE_ENUM_TO_TEXT( ML_MESSAGE_INVALID );
    CASE_ENUM_TO_TEXT( ML_BUFFERS_COMPLETE );
    CASE_ENUM_TO_TEXT( ML_BUFFERS_ABORTED );
    CASE_ENUM_TO_TEXT( ML_BUFFERS_FAILED );
    CASE_ENUM_TO_TEXT( ML_CONTROLS_COMPLETE );
    CASE_ENUM_TO_TEXT( ML_CONTROLS_ABORTED );
    CASE_ENUM_TO_TEXT( ML_CONTROLS_FAILED );
    CASE_ENUM_TO_TEXT( ML_QUERY_CONTROLS_COMPLETE );
    CASE_ENUM_TO_TEXT( ML_QUERY_CONTROLS_FAILED );
    CASE_ENUM_TO_TEXT( ML_QUERY_CONTROLS_ABORTED );
    CASE_ENUM_TO_TEXT( ML_EVENT_QUEUE_OVERFLOW );
    CASE_ENUM_TO_TEXT( ML_EVENT_DEVICE_INFO );
    CASE_ENUM_TO_TEXT( ML_EVENT_DEVICE_ERROR );
    CASE_ENUM_TO_TEXT( ML_EVENT_DEVICE_UNAVAILABLE );
    CASE_ENUM_TO_TEXT( ML_EVENT_VIDEO_SEQUENCE_LOST );
    CASE_ENUM_TO_TEXT( ML_EVENT_VIDEO_SYNC_GAINED );
    CASE_ENUM_TO_TEXT( ML_EVENT_VIDEO_SYNC_LOST );
    CASE_ENUM_TO_TEXT( ML_EVENT_VIDEO_VERTICAL_RETRACE );
    CASE_ENUM_TO_TEXT( ML_EVENT_VIDEO_SIGNAL_GAINED );
    CASE_ENUM_TO_TEXT( ML_EVENT_VIDEO_SIGNAL_LOST );
    CASE_ENUM_TO_TEXT( ML_EVENT_XCODE_FAILED );
    CASE_ENUM_TO_TEXT( ML_REDUNDANT_MESSAGE );
    CASE_ENUM_TO_TEXT( ML_BUFFERS_IN_PROGRESS );
    CASE_ENUM_TO_TEXT( ML_CONTROLS_IN_PROGRESS );
    CASE_ENUM_TO_TEXT( ML_QUERY_IN_PROGRESS );
    CASE_ENUM_TO_TEXT( ML_EVENT_AUDIO_SEQUENCE_LOST );
    CASE_ENUM_TO_TEXT( ML_EVENT_AUDIO_SAMPLE_RATE_CHANGED );

    /* NO default clause here - we want the compiler to tell us if we
     * miss one.
     */
  }
  return NULL;
}


/* -----------------------------------------------------------mlDeviceStateName
 */
const char* MLAPI mlDeviceStateName( MLint32 deviceState )
{
  /* Note - this must be kept in sync with the message type definitions
   * in mldefs.h.  To enforce this, we use a switch statement.
   */
  switch ( deviceState ) {
    CASE_ENUM_TO_TEXT( ML_DEVICE_STATE_READY );
    CASE_ENUM_TO_TEXT( ML_DEVICE_STATE_TRANSFERRING );
    CASE_ENUM_TO_TEXT( ML_DEVICE_STATE_WAITING );
    CASE_ENUM_TO_TEXT( ML_DEVICE_STATE_ABORTING );
    CASE_ENUM_TO_TEXT( ML_DEVICE_STATE_FINISHING );

    /* NO default clause here - we want the compiler to tell us if we
     * miss one.
     */
  }
  return NULL;
}


/* ----------------------------------------------------------------mlAccessName
 */
const char* MLAPI mlAccessName( MLint32 access )
{
#define	BIT_TO_STRING(x)	if ( access & x ) return #x;

  BIT_TO_STRING( ML_ACCESS_READ );
  BIT_TO_STRING( ML_ACCESS_WRITE );
  BIT_TO_STRING( ML_ACCESS_PERSISTENT );
  BIT_TO_STRING( ML_ACCESS_BUFFER_PARAM );
  BIT_TO_STRING( ML_ACCESS_IMMEDIATE );
  BIT_TO_STRING( ML_ACCESS_QUEUED );
  BIT_TO_STRING( ML_ACCESS_SEND_BUFFER );
  BIT_TO_STRING( ML_ACCESS_DURING_TRANSFER );
  BIT_TO_STRING( ML_ACCESS_PASS_THROUGH );
  BIT_TO_STRING( ML_ACCESS_BIT_ARRAY );
  BIT_TO_STRING( ML_ACCESS_OPEN_OPTION );

  return NULL;
}


/* ----------------------------------------------------mlDIprocessSelectIdParam
 *
 * Deal with the ML_SELECT_ID_INT64 control.  For non-xcodes, a value
 * not equal to our device means ignore all controls until the next
 * ML_SELECT_ID_INT64 or ML_END.  A value equal to zero or our device
 * means that controls once again apply to us.  Xcodes can
 * additionally take a value which selects a child pipe, otherwise the
 * non-xcode rules apply.
 *
 * "rval" is a returned arg modified to indicate which device has been
 * selected.  A return value of 0 means to ignore controls.
 */
MLstatus MLAPI mlDIprocessSelectIdParam( MLint64 val,
					 MLopenid device,
					 MLopenid* rval )
{
  *rval = 0;
  if ( val == 0 || val == device ) {
    /* You can always go home...
     */
    *rval = device;

  } else if ( ML_REF_GET_TYPE( device ) == ML_REF_TYPE_XCODE ) {
    /* xcodes can handle child pipes
     */
    if ( val == ML_XCODE_SRC_PIPE || val == ML_XCODE_DST_PIPE ) {
      /* Pipe specified as just src or dst
       */
      *rval = device | val;
    } else {
      /* Full id specified, make sure that it's one of our pipes
       */
      MLint32 myId = (MLint32) (device >> 32) & ~ML_REF_MASK_PIPE;
      MLint32 newId = (MLint32) (val >> 32) & ~ML_REF_MASK_PIPE;
      if ( myId == newId ) {
	*rval = val;
      }
    }
  }
  return ML_STATUS_NO_ERROR;
}


/* -------------------------------------------------------mlDIgetPrivateDataPtr
 */
MLstatus MLAPI mlDIgetPrivateDataPtr( MLint64 objectId, void **privPtr )
{
  MLint64 staticId;
  MLint32 objectType = mlDIextractIdType( objectId);
  MLopenDeviceRec* pOpenDev = mliLookupOpenDeviceRec( objectId );
  MLphysicalDeviceRec* pDevice = mliLookupDeviceRec( objectId, &staticId );

  assert( privPtr );

  if ( objectType ==  ML_REF_TYPE_SYSTEM ) {
    *privPtr = NULL;		/* No system private pointer yet */

  } else if ( objectType == ML_REF_TYPE_DEVICE ) {
    *privPtr = (void*)pDevice->ddDevicePriv; /* Device has own private ptr */

  } else if ( pOpenDev == NULL ) {
    return ML_STATUS_INVALID_ID; /* Below device the thing must be open */

  } else {
    *privPtr = (void*)pOpenDev->ddPriv;
  }

  return ML_STATUS_NO_ERROR;
}


/* ----------------------------------------------------------------mlGetVersion
 */
MLstatus MLAPI mlGetVersion( MLint32 *major, MLint32 *minor )
{
  if ( major == NULL || minor == NULL ) {
    return ML_STATUS_INVALID_ARGUMENT;
  }

  *major = ML_VERSION_MAJOR;
  *minor = ML_VERSION_MINOR;

  return ML_STATUS_NO_ERROR;
}


/* ------------------------------------------------------------------mlDIcapDup
 */
MLstatus MLAPI mlDIcapDup( MLpv *msg, MLpv **retCopy )
{
  MLint32 nParams, nBytes;
  MLpv *clone;
  MLstatus status;

  mlPvSizes( msg, &nParams, &nBytes );

  clone = (MLpv *) mlMalloc( nBytes );
  if ( NULL == clone ) {
    return ML_STATUS_OUT_OF_MEMORY;
  }

  status = mlPvCopy( msg, clone, nBytes );
  if ( status != ML_STATUS_NO_ERROR ) {
    mlFree( clone );
    return status;
  }

  *retCopy = clone;
  return ML_STATUS_NO_ERROR;		
}


/* -----------------------------------------------------------mlGetCapabilities
 */
MLstatus MLAPI mlGetCapabilities( MLint64 objectId, 
				  MLpv**capabilities )
{
  MLint64 staticObjectId;
  MLphysicalDeviceRec* pDevice;
  MLint32 objectType = mlDIextractIdType( objectId );

  if( objectType ==  ML_REF_TYPE_SYSTEM ) {
    if ( _thisSystem.id == 0 ) {
      /* First-time init required -- bootstrap the system
       */
      MLstatus status = _mlOSBootstrapSystem( &_thisSystem );
      if ( status != ML_STATUS_NO_ERROR ) {
	return ML_STATUS_INTERNAL_ERROR;
      }
      mlAssert( _thisSystem.id != 0 );
    }

    return _mlSystemGetCapabilities( objectId, capabilities );
  }

  /* Make sure the system was properly bootstrapped (achieved by
   * obtaining the system's capabilities) before attempting to obtain
   * caps for devices, etc.
   *
   * If not, consider the supplied ID invalid.
   */
  if ( _thisSystem.id == 0 ) {
    return ML_STATUS_INVALID_ID;
  }

  if ( objectType == ML_REF_TYPE_UST_SOURCE ) {
    return _mlUSTGetCapabilities( objectId, capabilities );
  }

  pDevice = mliLookupDeviceRec( objectId, &staticObjectId );
  if ( pDevice == NULL ) {
    return ML_STATUS_INVALID_ID;
  }

  if ( _mlOSBootstrapDevice( &_thisSystem, pDevice ) ) {
    return ML_STATUS_INTERNAL_ERROR;
  }

  return (pDevice->ops.ddGetCapabilities)( pDevice->ddDevicePriv,
					   staticObjectId, capabilities );
}


/* ---------------------------------------------------------mlPvGetCapabilities
 */
MLstatus MLAPI mlPvGetCapabilities( const MLint64 objectId,
				    const MLint64 paramId,
				    MLpv**capabilities )
{
  if ( ML_PARAM_GET_CLASS(paramId) == ML_CLASS_STATIC ||
       ML_PARAM_GET_CLASS(paramId) == ML_CLASS_INTERNAL ) {
    /* Its a static parameter, so we the can generate the param
     * capabilites right here without needing any objectId.
     */
    MLpv* caps = (MLpv*) mlMalloc( sizeof( MLpv )*6 );
    if ( caps == NULL ) {
      return ML_STATUS_OUT_OF_MEMORY;
    }

    caps[0].param = ML_ID_INT64;
    caps[0].value.int64 = paramId;
    caps[1].param = ML_NAME_BYTE_ARRAY;
    caps[1].value.pByte = (MLbyte*) _mlStaticParamName( paramId );
    caps[1].length = (MLint32) strlen( _mlStaticParamName( paramId ) )+1;
    caps[1].maxLength = caps[1].length;
    caps[2].param = ML_PARENT_ID_INT64;
    caps[2].value.int64 = (MLint64) 0;
    caps[3].param = ML_PARAM_TYPE_INT32;
    caps[3].value.int32 = ML_PARAM_GET_TYPE( paramId );
    caps[4].param = ML_PARAM_ACCESS_INT32;
    caps[4].value.int32 = ML_ACCESS_R;
    caps[5].param = ML_END;

    *capabilities = caps;
    return ML_STATUS_NO_ERROR;
  }

  if ( objectId == 0 ) {
    return ML_STATUS_INVALID_ID;
  }

  {
    MLint64 staticObjectId;
    MLphysicalDeviceRec* pDevice;
    pDevice = mliLookupDeviceRec( objectId, &staticObjectId );
    if ( pDevice == NULL ) {
      return ML_STATUS_INVALID_ID;
    }

    if ( _mlOSBootstrapDevice(&_thisSystem, pDevice) ) {
      return ML_STATUS_INTERNAL_ERROR;
    }

    {
      MLstatus status =
	(pDevice->ops.ddPvGetCapabilities)( pDevice->ddDevicePriv,
					    staticObjectId, paramId,
					    capabilities );

      /* If the parameter was not found, then see if it's an open
       * option and ML can return the capabilities
       */
      if ( status == ML_STATUS_INVALID_ID ) {
	if ( ML_STATUS_NO_ERROR ==
	     mlOpenOptionPvGetCapability( staticObjectId, paramId,
					  capabilities ) ) {
	  return ML_STATUS_NO_ERROR;
	}
	/* If not "no error" then return original status
	 */
      }
      return status;
    }
  }
}


/* ----------------------------------------------------------mlFreeCapabilities
 */
MLstatus MLAPI mlFreeCapabilities( MLpv *capabilities )
{
  if ( capabilities == NULL ) {
    return ML_STATUS_INVALID_ARGUMENT;
  }

  mlFree( capabilities );

  return ML_STATUS_NO_ERROR;
}


/* -------------------------------------------------------------mlDIvalidateMsg
 *
 * Validate message.  "beforeGet" indicates checking *before* values
 * are placed in the return message.
 */
MLstatus MLAPI mlDIvalidateMsg( MLopenid openDeviceId, MLint32 accessNeeded,
				MLint32 beforeGet, MLpv* msg )
{
  MLpv* pv = msg;
  MLint64 selectedId = openDeviceId;
  MLstatus retStatus = ML_STATUS_NO_ERROR;

  while ( pv->param != ML_END ) {
    MLstatus status = ML_STATUS_NO_ERROR;

    if ( selectedId != 0 ) {
      status = mlDIvalidatePv( selectedId, accessNeeded, beforeGet, pv );
    }

    if ( status != ML_STATUS_NO_ERROR ) {
      void _mlOSDebugDrivers( char * );
      _mlOSDebugDrivers( "mlDIvalidateMsg" );
    }
    /* Account for the possibilitiy of a selectId parameter
     */
    if ( pv->param == ML_SELECT_ID_INT64 ) {
      mlDIprocessSelectIdParam( pv->value.int64, openDeviceId, &selectedId );
    }

    /* Continue to check all params, even if one fails, but remember
     * the first failure and return it later
     */
    if ( (status!=ML_STATUS_NO_ERROR) && (retStatus == ML_STATUS_NO_ERROR) ) {
      retStatus = status;
    }

    pv++;
  }
  return retStatus;
}


/* --------------------------------------------------------------mlDIvalidatePv
 */
MLstatus MLAPI mlDIvalidatePv( MLopenid openDeviceId, MLint32 accessNeeded,
			       MLint32 beforeGet, MLpv* pv )
{
  MLint64 param;
  MLpv *devCapVec  = NULL;
  MLpv *paramCapVec= NULL;
  MLpv *devName    = NULL;
  MLpv *paramName  = NULL;
  MLpv *paramAccess= NULL;

  MLstatus status  = ML_STATUS_NO_ERROR;
  char errorString[256] = "";

  param = pv->param;
	
  if ( (param & ML_PARAM_MASK_CLASS) == ML_CLASS_USER ) {
    return ML_STATUS_NO_ERROR;
  }

  if ( (status = mlGetCapabilities(openDeviceId, &devCapVec)) ) {
    return status;
  }

  if ( (devName = mlPvFind(devCapVec, ML_NAME_BYTE_ARRAY)) == NULL ) {
    return ML_STATUS_INTERNAL_ERROR;
  }

  if ( (status = mlPvGetCapabilities(openDeviceId, param, &paramCapVec)) ) {
    if ( accessNeeded == ML_ACCESS_OPEN_OPTION ) {
      /* For open options, the device is not required to recognize the
       * parameter as ml provides default open option checking
       */
      return ML_STATUS_NO_ERROR;
    }
    sprintf( errorString, "parameter was not recognized by device" );
    status = ML_STATUS_INVALID_PARAMETER;
    goto PARAM_ERROR;
  }

  if ( (paramName = mlPvFind(paramCapVec, ML_NAME_BYTE_ARRAY)) == NULL ) {
    sprintf( errorString, "parameter name not found in param capabilities" );
    status = ML_STATUS_INTERNAL_ERROR;
    goto PARAM_ERROR;
  }

  if ( (paramAccess = mlPvFind(paramCapVec, ML_PARAM_ACCESS_INT32)) == NULL ) {
    sprintf( errorString, "parameter access not found in param capabilities" );
    status = ML_STATUS_INTERNAL_ERROR;
    goto PARAM_ERROR;
  }

  /* Check access
   */
  {
    MLint32 accessPermitted = paramAccess->value.int32;
    MLint32 allowed = accessPermitted & accessNeeded;

    if ( allowed != accessNeeded ) {
      MLint32 denied = allowed ^ accessNeeded;
      if ( denied != ML_ACCESS_W ) {
	/* No write access is allowed if value is not changed
	 */
	sprintf( errorString, "access %s not available for this parameter",
		 mlAccessName( denied ) );
	status = ML_STATUS_INVALID_PARAMETER;
	goto PARAM_ERROR;
      }
    }
  }

  /* Check value
   */
  if ( ML_PARAM_IS_POINTER( param ) ) {
    if ( pv->value.pByte == NULL ) {
      sprintf(errorString,"warning - passing NULL pointer");
    }

    if ( (accessNeeded & ML_ACCESS_SEND_BUFFER) != ML_ACCESS_SEND_BUFFER ) {
      if ( beforeGet && pv->maxLength <= 0 ) {
	sprintf( errorString, "unexpected maxLength <=0 for pointer" );
	status = ML_STATUS_INVALID_VALUE;
	goto PARAM_ERROR;

      } else if ( ! beforeGet && pv->length <=0 ) {
	sprintf( errorString, "unexpected length <=0 for pointer" );
	status = ML_STATUS_INVALID_VALUE;
	goto PARAM_ERROR;
      }
    }

  } else if ( ML_PARAM_IS_ARRAY( param ) ) {
    if ( beforeGet ) {
      if ( pv->maxLength < 0 ) {
	sprintf( errorString, "maxLength (%d) cannot be < 0 for an array",
		 pv->maxLength );
	status = ML_STATUS_INVALID_VALUE;
	goto PARAM_ERROR;

      } else if ( pv->maxLength > 0 && pv->value.pByte == NULL ) {
	sprintf( errorString, "passing NULL array pointer" );
	status = ML_STATUS_INVALID_VALUE;
	goto PARAM_ERROR;
      }

    } else {
      if ( pv->length < 0 ) {
	sprintf( errorString, "length (%d) cannot be < 0 for an array",
		 pv->length );
	status = ML_STATUS_INVALID_VALUE;
	goto PARAM_ERROR;

      } else if ( pv->length > 0 && pv->value.pByte == NULL ) {
	sprintf( errorString, "passing NULL array pointer" );
	status = ML_STATUS_INVALID_VALUE;
	goto PARAM_ERROR;

      } else {
	MLint32 i;
	for ( i = 0; i < pv->length; i++ ) {
	  MLvalue val;
	  val.int64 = 0;
	  switch ( ML_PARAM_GET_TYPE_ELEM( pv->param ) ) {
	  case ML_TYPE_ELEM_BYTE:   val.byte   = pv->value.pByte[i];   break;
	  case ML_TYPE_ELEM_INT32:  val.int32  = pv->value.pInt32[i];  break;
	  case ML_TYPE_ELEM_REAL32: val.real32 = pv->value.pReal32[i]; break;
	  case ML_TYPE_ELEM_INT64:  val.int64  = pv->value.pInt64[i];  break;
	  case ML_TYPE_ELEM_REAL64: val.real64 = pv->value.pReal64[i]; break;
	  default: goto PARAM_ERROR; /* errorString = 0 so no error */
	  }
	  if ( (status = mlDIvalidatePvValue( errorString, pv->param,
					      val, paramCapVec )) ) {
	    goto PARAM_ERROR;
	  }
	}
      }
    }

  } else if ( ML_PARAM_IS_SCALAR( param ) && ! beforeGet ) {
    status = mlDIvalidatePvValue( errorString, pv->param, pv->value,
				  paramCapVec );
    if ( status ) {
      goto PARAM_ERROR;
    }
  }

 PARAM_ERROR:

  if ( *errorString != '\0' ) {
    /* Mark parameter as invalid
     */
    pv->length = -1;
    if ( paramName != NULL ) {
      mlDebug( "\n[Validate param] checking parameter: %s ", 
	       (const char*) paramName->value.pByte );
    } else {
      mlDebug( "\n[Validate param] checking parameter: %" FORMAT_LLX " (%s)",
	       param, _mlStaticParamName( param ) );
    }

    if ( devName != NULL ) {
      MLint64 staticId = mlDIconvertOpenIdToStaticId( openDeviceId );
      switch ( mlDIextractIdType(staticId) ) {
      case ML_REF_TYPE_PATH:
	mlDebug( "on path" );
	break;
      case ML_REF_TYPE_XCODE:
	mlDebug( "on xcode" );
	break;
      default:
	mlDebug( "on" );
	break;
      }
      mlDebug( ": %s\n", (const char*) devName->value.pByte );

    } else {
      mlDebug("\n");
    }

    mlDebug( "                 " );
    mlDebug( errorString );
    mlDebug( "\n" );
  }

  if ( paramCapVec != NULL ) {
    mlFreeCapabilities(paramCapVec);
  }
  if ( devCapVec != NULL ) {
    mlFreeCapabilities(devCapVec);
  }

  return status;
}


/* ---------------------------------------------------------mlDIgetParamDefault
 */
MLstatus MLAPI mlDIgetParamDefault( MLint64 openid, MLint64 param, void *def )
{
  MLpv *paramCap, *pv;
  MLstatus status;
  MLint32 type;

  if ( (status = mlPvGetCapabilities( openid, param, &paramCap )) !=
       ML_STATUS_NO_ERROR ) {
    return status;
  }

  if ( (pv = mlPvFind( paramCap, ML_PARAM_TYPE_INT32 )) != NULL ) {
    type = pv->value.int32;

    if ( (pv = mlPvFind( paramCap, ML_PARAM_DEFAULT | type )) != NULL &&
	 pv->length > 0 ) {
      switch ( type ) {
      case ML_TYPE_INT32:
	*(MLint32*)def = pv->value.int32;
	break;

      case ML_TYPE_INT64:
	*(MLint64*)def = pv->value.int64;
	break;

      case ML_TYPE_REAL32:
	*(MLreal32*)def = pv->value.real32;
	break;

      case ML_TYPE_REAL64:
	*(MLreal64*)def = pv->value.real64;
	break;

      default:
	return ML_STATUS_INVALID_PARAMETER;
      }
      mlFreeCapabilities( paramCap );
      return ML_STATUS_NO_ERROR;
    }
  }
  mlFreeCapabilities( paramCap );
  return ML_STATUS_INVALID_ID;
}


/* --------------------------------------------------------mlDIparseOpenOptions
 */
/* ARGSUSED */
MLstatus MLAPI mlDIparseOpenOptions( MLint64 openid,
				     MLpv* options, 
				     MLopenOptions* pOpenOpts )
{
  MLstatus vstatus = ML_STATUS_NO_ERROR;
  MLpv *pv;
  MLpv emptyMsg[1] = {{ML_END, {0}, 0, 0}};
  MLint32 i;

  if ( pOpenOpts == NULL ) {
    return ML_STATUS_INVALID_ARGUMENT;
  }

  /* Set default open options
   */
  *pOpenOpts = defaultOpenOptions;

  if ( options == NULL ) {
    options = emptyMsg; /* Simplifies logic below */
  }

  /* Validate the open options and their values
   * Mar 2005: not sure why this has been commented-out. It should be
   * re-instated, otherwise there is no checking that the user-supplied
   * options are valid.
   */
  /* vstatus = mlDIvalidateMsg( openid, ML_ACCESS_OPEN_OPTION, 0, options ); */

  /* Now setup defaults
   */
  for ( i = 0; i < lengthof( openOptions ); i++ ) {
    MLint64 param = openOptions[ i ];
    MLint32 *pDefault = getOptOffset( pOpenOpts, param );

    /* Setup devices requested default if there is one
     */
    (void) mlDIgetParamDefault( openid, param, (void *)pDefault );

    /* (if user options were valid, look for option there as well) we
     * only allow the user to increase a default, not decrease it
     *
     * Mar 2005: preventing users from setting a value smaller than a
     * default may make no sense at all -- for enum's for instance,
     * relative ordering of the various values is generally
     * irrelevant. This constraint artificially restricts users from
     * using certain values (which *should* be legal). So the
     * constraint is removed.
     *
     */
    if ( ! vstatus && ( pv = mlPvFind( options, param ) ) ) {
      /* if ( *pDefault < pv->value.int32 ) { */
      *pDefault = pv->value.int32; /* All open options are int32's */
      /* } */
    }
  }

  /* If the incoming message was invalid, allow the device driver to
   * return (or continue with it's own defaults in place).
   */
  if ( vstatus ) {
    return vstatus;
  }

  /* Check dependency - the receive queue count must have space for
   * every event and at least a reasonable fraction of the send queue.
   * Arbitrarily choose that reasonable fraction to be 25%.
   */
  if ( pOpenOpts->receiveQueueCount < 
       pOpenOpts->sendQueueCount*0.25 + pOpenOpts->eventPayloadCount ) {

    mlDebug( "[mliValidateOpenOptions] Error - the recieve queue\n"
	     " count %d is too small relative to the event count %d\n"
	     " and the send queue count %d.  Either make it larger,\n"
	     " or make one of them smaller.\n",
	     pOpenOpts->receiveQueueCount, pOpenOpts->eventPayloadCount,
	     pOpenOpts->sendQueueCount);

    if ( (pv = mlPvFind( options, ML_OPEN_SEND_QUEUE_COUNT_INT32 )) != NULL ) {
      pv->length = -1;
    }
    if ( (pv = mlPvFind( options, ML_OPEN_RECEIVE_QUEUE_COUNT_INT32 ))
	 != NULL ) {
      pv->length = -1;
    }
    if ( (pv = mlPvFind( options, ML_OPEN_EVENT_PAYLOAD_COUNT_INT32 ))
	 != NULL ) {
      pv->length = -1;
    }

    return ML_STATUS_INVALID_VALUE;
  }

  if ( pOpenOpts->sendSignalCount >= pOpenOpts->sendQueueCount ) {
    mlDebug( "[mliValidateOpenOptions] Error - the send signal count %d\n"
	     "must be less than the send queue count %d.\n",
	     pOpenOpts->sendSignalCount, pOpenOpts->sendQueueCount);

    if ( (pv = mlPvFind( options, ML_OPEN_SEND_QUEUE_COUNT_INT32 )) != NULL ) {
      pv->length = -1;
    }
    if ( (pv = mlPvFind( options, ML_OPEN_SEND_SIGNAL_COUNT_INT32 ))
	 != NULL ) {
      pv->length = -1;
    }

    return ML_STATUS_INVALID_VALUE;
  }

  return ML_STATUS_NO_ERROR;
}


/* ----------------------------------------------------------------------mlOpen
 */
MLstatus MLAPI mlOpen( MLint64 objectid, MLpv* options, MLopenid* openid )
{
  MLstatus status;
  MLint64 staticObjectId;
  MLopenDeviceRec* pOpenDev;
  MLphysicalDeviceRec* pDevice =
    mliLookupDeviceRec( objectid, &staticObjectId );

  if ( pDevice == NULL ) {
    return ML_STATUS_INVALID_ID;
  }

  status = _mliAcquireOpenDeviceInstance( objectid, &pOpenDev );
  if ( status ) {
    mlDebug( "mlOpen: error in _mliAcquireOpenDeviceInstance ('%s')\n",
	     mlStatusName( status ) );
    return status;
  }

  status = (pDevice->ops.ddOpen)( pDevice->ddDevicePriv, 
				  staticObjectId, 
				  pOpenDev->id, 
				  options, 
				  &(pOpenDev->ddPriv) );

  if ( status != ML_STATUS_NO_ERROR ) {
    mlDebug( "mlOpen: error in ddOpen ('%s')\n", mlStatusName( status ) );
    _mliRelinquishOpenDeviceInstance( pOpenDev );
  }

  *openid = pOpenDev->id;

  return status;
}


/* ---------------------------------------------------------------mlGetControls
 */
MLstatus MLAPI mlGetControls( MLopenid openid, MLpv* controls )
{
  MLint64 staticObjectId;
  MLopenDeviceRec* pOpenDev = mliLookupOpenDeviceRec( openid );
  MLphysicalDeviceRec* pDevice = mliLookupDeviceRec( openid, &staticObjectId );

  if ( pOpenDev == NULL || pDevice == NULL ) {
    return ML_STATUS_INVALID_ID;
  }

  return (pDevice->ops.ddGetControls)( pOpenDev->ddPriv, openid, controls );
}


/* ---------------------------------------------------------------mlSetControls
 */
MLstatus MLAPI mlSetControls( MLopenid openid, MLpv* controls )
{
  MLint64 staticObjectId;
  MLopenDeviceRec* pOpenDev = mliLookupOpenDeviceRec( openid );
  MLphysicalDeviceRec* pDevice = mliLookupDeviceRec( openid, &staticObjectId );

  if ( pOpenDev == NULL || pDevice == NULL ) {
    return ML_STATUS_INVALID_ID;
  }

  return (pDevice->ops.ddSetControls)( pOpenDev->ddPriv, openid, controls );
}


/* --------------------------------------------------------------mlSendControls
 */
MLstatus MLAPI mlSendControls( MLopenid openid, MLpv* controls )
{
  MLint64 staticObjectId;
  MLopenDeviceRec* pOpenDev = mliLookupOpenDeviceRec( openid );
  MLphysicalDeviceRec* pDevice = mliLookupDeviceRec( openid, &staticObjectId );

  if ( pOpenDev == NULL || pDevice == NULL ) {
    return ML_STATUS_INVALID_ID;
  }

  return (pDevice->ops.ddSendControls)( pOpenDev->ddPriv, openid, controls );
}


/* -------------------------------------------------------------mlQueryControls
 */
MLstatus MLAPI mlQueryControls( MLopenid openid, MLpv* controls )
{
  MLint64 staticObjectId;
  MLopenDeviceRec* pOpenDev = mliLookupOpenDeviceRec( openid );
  MLphysicalDeviceRec* pDevice = mliLookupDeviceRec( openid, &staticObjectId );

  if ( pOpenDev == NULL || pDevice == NULL ) {
    return ML_STATUS_INVALID_ID;
  }

  return (pDevice->ops.ddQueryControls)( pOpenDev->ddPriv, openid, controls );
}


/* ---------------------------------------------------------------mlSendBuffers
 */
MLstatus MLAPI mlSendBuffers( MLopenid openid, MLpv* controls )
{
  MLint64 staticObjectId;
  MLopenDeviceRec* pOpenDev = mliLookupOpenDeviceRec( openid );
  MLphysicalDeviceRec* pDevice = mliLookupDeviceRec( openid, &staticObjectId );

  if ( pOpenDev == NULL || pDevice == NULL ) {
    return ML_STATUS_INVALID_ID;
  }

  return (pDevice->ops.ddSendBuffers)( pOpenDev->ddPriv, openid, controls );
}


/* ------------------------------------------------------------mlReceiveMessage
 */
MLstatus MLAPI mlReceiveMessage( MLopenid openid, MLint32* msgType, 
				 MLpv** message )
{
  MLint64 staticObjectId;
  MLopenDeviceRec* pOpenDev = mliLookupOpenDeviceRec( openid );
  MLphysicalDeviceRec* pDevice = mliLookupDeviceRec( openid, &staticObjectId );

  if ( pOpenDev == NULL || pDevice == NULL ) {
    return ML_STATUS_INVALID_ID;
  }

  return (pDevice->ops.ddReceiveMessage)( pOpenDev->ddPriv, openid, msgType,
					  message );
}


/* -------------------------------------------------------------mlBeginTransfer
 */
MLstatus MLAPI mlBeginTransfer( MLopenid openid )
{
  MLpv msg[2];
  msg[0].param = ML_DEVICE_STATE_INT32;
  msg[0].value.int32 = ML_DEVICE_STATE_TRANSFERRING;
  msg[0].length = 1;
  msg[1].param = ML_END;

  return mlSetControls( openid, msg );
}


/* ---------------------------------------------------------------mlEndTransfer
 */
MLstatus MLAPI mlEndTransfer( MLopenid openid )
{
  MLpv msg[2];
  msg[0].param = ML_DEVICE_STATE_INT32;
  msg[0].value.int32 = ML_DEVICE_STATE_ABORTING;
  msg[0].length = 1;
  msg[1].param = ML_END;

  return mlSetControls( openid, msg );
}


/* -------------------------------------------------------mlGetSendMessageCount
 */
MLstatus MLAPI mlGetSendMessageCount( MLopenid openid, MLint32* count )
{
  MLstatus status;
  MLpv msg[2];
  msg[0].param = ML_QUEUE_SEND_COUNT_INT32;
  msg[1].param = ML_END;
  
  status = mlGetControls( openid, msg );

  if ( status == ML_STATUS_NO_ERROR ) {
    *count = msg[0].value.int32;
  }
  return status;
}


/* ----------------------------------------------------mlGetReceiveMessageCount
 */
MLstatus MLAPI mlGetReceiveMessageCount( MLopenid openid, MLint32* count )
{
  MLstatus status;
  MLpv msg[2];
  msg[0].param = ML_QUEUE_RECEIVE_COUNT_INT32;
  msg[1].param = ML_END;

  status = mlGetControls( openid, msg );

  if ( status == ML_STATUS_NO_ERROR ) {
    *count = msg[0].value.int32;
  }

  return status;
}


/* ---------------------------------------------------------mlGetSendWaitHandle
 */
MLstatus MLAPI mlGetSendWaitHandle( MLopenid openid, MLwaitable* w )
{
  MLstatus status;
  MLpv msg[2];
  msg[0].param = ML_QUEUE_SEND_WAITABLE_INT64;
  msg[1].param = ML_END;

  status = mlGetControls( openid, msg );

  if ( status == ML_STATUS_NO_ERROR ) {
    *w = (MLwaitable) (msg[0].value.int64);

  } else if ( status == ML_STATUS_INVALID_PARAMETER ) {
    /* We may have an older MLmodule that has a 32 bit waitable
     */
    MLstatus status2;
    msg[0].param = ML_QUEUE_SEND_WAITABLE_INT32;
    status2 = mlGetControls( openid, msg );
    if ( status2 == ML_STATUS_NO_ERROR ) {
      *w = (MLwaitable) (msg[0].value.int32);
      status = status2;
    }
  }

  return status;
}


/* ------------------------------------------------------mlGetReceiveWaitHandle
 */
MLstatus MLAPI mlGetReceiveWaitHandle( MLopenid openid, MLwaitable* w )
{
  MLstatus status;
  MLpv msg[2];
  msg[0].param = ML_QUEUE_RECEIVE_WAITABLE_INT64;
  msg[1].param = ML_END;

  status = mlGetControls( openid, msg );

  if ( status == ML_STATUS_NO_ERROR ) {
    *w = (MLwaitable) (msg[0].value.int64);

  } else if ( status == ML_STATUS_INVALID_PARAMETER ) {
    /* We may have an older MLmodule that has a 32 bit waitable
     */
    MLstatus status2;
    msg[0].param = ML_QUEUE_RECEIVE_WAITABLE_INT32;
    status2 = mlGetControls( openid, msg );
    if ( status2 == ML_STATUS_NO_ERROR ) {
      *w = (MLwaitable) (msg[0].value.int32);
      status = status2;
    }
  }

  return status;
}


/* ----------------------------------------------------------mlXcodeGetOpenPipe
 */
MLstatus MLAPI mlXcodeGetOpenPipe( MLopenid openid, MLint64 pipeid, 
				   MLopenid* handle )
{
  /* FIXME: NOT implemented yet
   */
  MLopenDeviceRec* pOpenDev = mliLookupOpenDeviceRec( openid );
  if ( pipeid == 0 ) {
    return ML_STATUS_INVALID_ARGUMENT;
  }
  if ( handle == NULL ) {
    return ML_STATUS_INVALID_ARGUMENT;
  }

  if ( pOpenDev == NULL ) {
    return ML_STATUS_INVALID_ID;
  }

  fprintf( stderr, "[mlXcodeGetOpenPipe] Sorry, this function is not"
	   "yet implemented\n" );

  return ML_STATUS_INTERNAL_ERROR;
}


/* -----------------------------------------------------------------mlXcodeWork
 *
 * Synchronous mode operation for software codecs
 */
MLstatus MLAPI mlXcodeWork( MLopenid openid )
{
  MLint64 staticObjectId;
  MLopenDeviceRec* pOpenDev = mliLookupOpenDeviceRec( openid );
  MLphysicalDeviceRec* pDevice = mliLookupDeviceRec( openid, &staticObjectId );

  if ( pOpenDev == NULL || pDevice == NULL ) {
    return ML_STATUS_INVALID_ID;
  }

  return (pDevice->ops.ddXcodeWork)( pOpenDev->ddPriv, openid );
}


/* ---------------------------------------------------------------------mlClose
 */
MLstatus MLAPI mlClose( MLopenid openid )
{
  MLstatus status;
  MLint64 staticObjectId;
  MLopenDeviceRec* pOpenDev = mliLookupOpenDeviceRec( openid );
  MLphysicalDeviceRec* pDevice = mliLookupDeviceRec( openid, &staticObjectId );

  if ( pOpenDev == NULL || pDevice == NULL ) {
    return ML_STATUS_INVALID_ID;
  }

  status = (pDevice->ops.ddClose)( pOpenDev->ddPriv, openid );

  _mliRelinquishOpenDeviceInstance( pOpenDev );

  return status;
}


/* --------------------------------------------------------------mlSystemGetUST
 */
MLstatus MLAPI mlGetSystemUST( MLint64 sysId, MLint64* ust )
{
  if ( sysId != ML_SYSTEM_LOCALHOST && sysId != _thisSystem.id ) {
    mlDebug( "[mlGetSystemUST] Invalid system id - is "
	     "not ML_SYSTEM_LOCALHOST or this system's id (%d.%d.%d.%d)\n",
	     _thisSystem.id >> 24 & 0xff, _thisSystem.id >> 16 & 0xff,
	     _thisSystem.id >>  8 & 0xff, _thisSystem.id & 0xff );
    return ML_STATUS_INVALID_ID;
  }

  /* Call the function whose address is stored in the system rec. --
   * it was placed there at the end of the _mlOSBootstrapSystem call
   */
  if ( (ust != NULL) && ( _thisSystem.getUST(ust) == 0 ) ) {
    return ML_STATUS_NO_ERROR;
  } else {
    return ML_STATUS_INVALID_ARGUMENT;
  }
}


/* ------------------------------------------------------------mlDIGetUSTSource
 */
MLstatus MLAPI mlDIGetUSTSource( MLint64 sysId,
				 MLstatus (**USTSource)( MLint64* ) )
{
  /* In the current implementation, there can be only 1 system
   * Id. Make sure the one supplied by the device is reasonable.
   */
  if ( sysId != _thisSystem.id ) {
    mlDebug( "[mlDIGetUSTSource] Invalid system id - is "
	     "this system's id (%d.%d.%d.%d)\n",
	     _thisSystem.id >> 24 & 0xff, _thisSystem.id >> 16 & 0xff,
	     _thisSystem.id >>  8 & 0xff, _thisSystem.id & 0xff );
    return ML_STATUS_INVALID_ID;
  }

  if ( USTSource == NULL ) {
    return ML_STATUS_INVALID_ARGUMENT;
  }


  /* For now, since there is only 1 system (see comment above),
   * hard-code the system rec "_thisSystem" -- but eventually, we need
   * to find the appropriate system rec based on the supplied sysId.
   */
  *USTSource = _thisSystem.getUST;

  return ML_STATUS_NO_ERROR;
}


/* -----------------------------------------------------------mlPvValueToString
 *
 * Convert a mlPv.value to a character string.
 *
 * Returned size is the length of the string (excluding \0).
 */
MLstatus MLAPI mlPvValueToString( MLint64 objectId, MLpv* pv, 
				  char* buffer, MLint32* size )
{
  MLpv* paramCap;
  MLint32 used_space=0;
  MLstatus status = ML_STATUS_NO_ERROR;

  /* Look up the meaning of this param on this particular device
   */
  if ( mlPvGetCapabilities(objectId, pv->param, &paramCap) ) {
    *size = 0;
    return ML_STATUS_INVALID_PARAMETER;
  }
      
  if ( ML_PARAM_IS_POINTER( pv->param ) ) {
    used_space += sprintf(buffer,"0x%p", pv->value.pByte);
    status = ML_STATUS_NO_ERROR;

  } else if ( ML_PARAM_IS_ARRAY( pv->param ) ) {
    int i;

    if ( ML_PARAM_GET_TYPE_ELEM( pv->param ) == ML_TYPE_ELEM_BYTE ) {
      used_space += sprintf(buffer, "\"");
    } else {
      used_space += sprintf(buffer, "[");
    }

    for ( i=0; i < pv->length && status == ML_STATUS_NO_ERROR; i++ ) {
      MLint32 space_for_punctuation = 1; /* comma or closing bracket */
      MLint32 space_for_value = 12;      /* conservative guess */

      if ( *size -used_space -space_for_punctuation - space_for_value < 0 ) {
	status = ML_STATUS_INVALID_ARGUMENT;
	break;
      }

      /* Add punctuation between array entries, if necessary
       */
      if ( i > 0 && ML_PARAM_GET_TYPE_ELEM( pv->param ) != ML_TYPE_ELEM_BYTE ){
	used_space += sprintf(buffer+used_space,", ");
      }

      switch ( ML_PARAM_GET_TYPE_ELEM(pv->param) ) {
      case ML_TYPE_ELEM_BYTE:
	if ( pv->value.pByte[i] >= ' ' && pv->value.pByte[i] <= '~'
	     /*(pv->value.pByte[i] >= 'A' && pv->value.pByte[i] <= 'Z') ||
	       (pv->value.pByte[i] >= 'a' && pv->value.pByte[i] <= 'z') ||
	       (pv->value.pByte[i] >= '0' && pv->value.pByte[i] <= '9') ||
	       (pv->value.pByte[i] >= ' ' && pv->value.pByte[i] <= '/') ||
	       pv->value.pByte[i] == '_'*/) {
	  used_space += sprintf( buffer+used_space,"%c",
				 (char) (pv->value.pByte[i]) );
	} else if ( pv->value.pByte[i] == 0 ) {
	  used_space += sprintf( buffer+used_space, "\\0" );
	} else {
	  used_space += sprintf( buffer+used_space, "\\0x%x",
				 (int) (pv->value.pByte[i]) );
	}
	break;

      case ML_TYPE_ELEM_INT32: {
	MLint32 this_value_size = *size - used_space;
	status = _valueInt32ToString( paramCap, pv->value.pInt32[i], 
				      buffer+used_space, 
				      &this_value_size );
	used_space += this_value_size;
	break;
      }

      case ML_TYPE_ELEM_INT64:  
	used_space += sprintf( buffer+used_space, "%" FORMAT_LLX, 
			       pv->value.pInt64[i] );
	break;

      case ML_TYPE_ELEM_REAL32: 
	used_space += sprintf( buffer+used_space, "%f", 
			       (double) (pv->value.pReal32[i]) );
	break;

      case ML_TYPE_ELEM_REAL64: 
	used_space += sprintf( buffer+used_space, "%f", 
			       (double) (pv->value.pReal64[i]) );
	break;

      default:
	status = ML_STATUS_INVALID_VALUE;
	break;
      }
    }
    if ( ML_PARAM_GET_TYPE_ELEM( pv->param ) == ML_TYPE_ELEM_BYTE ) {
      used_space += sprintf( buffer+used_space, "\"" );
    } else {
      used_space += sprintf( buffer+used_space, "]" );
    }

  } else {
    switch ( ML_PARAM_GET_TYPE( pv->param ) ) {
    case ML_TYPE_INT32: {
      MLint32 space_for_this = *size - used_space;
      status = _valueInt32ToString( paramCap, pv->value.int32, 
				    buffer, &space_for_this );
      used_space += space_for_this;
      break;
    }

    case ML_TYPE_INT64: {
      MLint32 space_for_this = *size - used_space;
      status = _valueInt64ToString( paramCap, pv->value.int64, 
				    buffer, &space_for_this );
      used_space += space_for_this;
      break;
    }

    case ML_TYPE_REAL32: 
      used_space += sprintf( buffer+used_space, "%f", 
			     (double) (pv->value.real32) );
      break;

    case ML_TYPE_REAL64: 
      used_space += sprintf( buffer+used_space, "%f", 
			     (double) (pv->value.real64) );
      break;

    default:
      status = ML_STATUS_INVALID_VALUE;
    }
  }

  if ( status == ML_STATUS_NO_ERROR ) {
    *size = used_space;
  } else {
    *size = 0;
  }
  mlFreeCapabilities( paramCap );
  return status;
}


/* -----------------------------------------------------------mlPvParamToString
 *
 * Convert MLpv.param to a character string
 *
 * size is the number of bytes in the buffer, and is modified in the
 * call to return the number of bytes written (excluding the
 * terminating \0)
 */
MLstatus MLAPI mlPvParamToString( MLint64 objectId, MLpv* pv, 
				  char* buffer, MLint32* size )
{
  MLpv* paramCap, *paramName;

  /* Look up the meaning of this param on this particular device
   */
  if ( mlPvGetCapabilities( objectId, pv->param, &paramCap ) ) {
    *size = 0;
    return ML_STATUS_INVALID_PARAMETER;
  }
      
  if ( (paramName = mlPvFind(paramCap, ML_NAME_BYTE_ARRAY)) == NULL ) {
    mlFreeCapabilities( paramCap );
    *size = 0;
    return ML_STATUS_INTERNAL_ERROR;
  }

  if ( paramName->length + 1 > *size ) {
    mlFreeCapabilities( paramCap );
    *size = 0;
    return ML_STATUS_INVALID_ARGUMENT;
  }

  *size = sprintf( buffer, "%s", (char*)paramName->value.pByte );
  mlFreeCapabilities( paramCap );
  return ML_STATUS_NO_ERROR;
}


/* ----------------------------------------------------------------mlPvToString
 * Convert a mlPv to a character string of the form:
 * "param = value"
 *
 * size is the number of bytes in the buffer, and is modified in the
 * call to return the number of bytes written (excluding the
 * terminating \0)
 */
MLstatus MLAPI mlPvToString( MLint64 objectId, MLpv* pv, 
			     char* buffer, MLint32* size )
{
  MLint32 used_space=0;
  MLint32 space_for_this = *size;
  MLstatus status;
  
  if ( (status = mlPvParamToString( objectId, pv, buffer, &space_for_this )) ){
    *size = 0;
    return status;
  }

  if ( *size - space_for_this < 10 ) {
    /* Quick cursory check - we need additional space for the ' = '
     * and for the actual value.
     */
    *size =0;
    return ML_STATUS_INVALID_ARGUMENT;
  }

  used_space+=space_for_this;
  used_space+= sprintf( buffer+space_for_this, " = " );
  space_for_this = *size - used_space;

  if ( (status = mlPvValueToString( objectId, pv, buffer+used_space, 
				    &space_for_this) ) ) {
    *size = 0;
    return status;
  }
  *size = used_space + space_for_this;
  return ML_STATUS_NO_ERROR;
}


/* -------------------------------------------------------------------mlPvSizes
 *
 * Compute the number of elements in a params list and the number of
 * bytes required to store it (and any arrays to which it refers).
 */
void MLAPI mlPvSizes( MLpv* params, MLint32* pnParams, MLint32* pnBytes )
{
  MLint32 nParams = 1; /* Always have at least one parameter: ML_END */
  MLint32 nBytes = 0;

  /* Now add space for any arrays in those parameters
   */
  while ( params->param != ML_END ) {
    if ( ML_PARAM_IS_ARRAY( params->param ) ) {
      /* Force multiple of 8
       */
      nBytes +=
	(params->length*_mlDIPvSizeofElem( params->param ) + 7) & ~(0x7);
    }
    nParams++;
    params++;
  }

  /* Now add space for the parameter array itself
   */
  nBytes += nParams * sizeof( MLpv );
	
  if ( pnParams != NULL ) {
    *pnParams = nParams;
  }
  if ( pnBytes != NULL ) {
    *pnBytes = nBytes;
  }
}


/* --------------------------------------------------------------------mlPvCopy
 *
 * Copy an array of params (and any arrays to which it refers) into a
 * buffer.
 */
MLstatus MLAPI mlPvCopy( MLpv* params, void* toBuffer, 
			 MLint32 sizeofBuffer )
{
  MLint32 nBytes;
  MLint32 nParams;
  MLpv* copy;
  char* arrayData;
  int i;
	
  /* Begin by precomputing and checking sizes
   */
  mlPvSizes( params, &nParams, &nBytes );
  /* We should never encounter this error in 1.0. 
   */
  if ( nBytes > sizeofBuffer ) {
    return ML_STATUS_INTERNAL_ERROR;
  }
	
  /* Then copy the parameter list into the buffer
   */
  memcpy( toBuffer, params, nParams*sizeof( MLpv ) );
  copy = (MLpv*) toBuffer;
  arrayData = (char*) (copy + nParams);
	
  /* Now go through that list and copy any arrays into the buffer
   * (there is no need to look at the last parameter - it must be
   * ML_END - so just loop to nParams-1)
   */
  for ( i=0; i < nParams-1; i++ ) {
    if ( ML_PARAM_IS_ARRAY( copy[i].param ) ) {
      if ( params[i].value.pByte != NULL ) {
	memcpy( arrayData, params[i].value.pByte, 
		params[i].length*_mlDIPvSizeofElem( copy[i].param ) );
      }

#if defined(DEBUG) || defined(_DEBUG)
      {
	/* Explicitly zero the alignment pad bytes to quiet Purify.
	 */
	MLint32 lenToEnd = params[i].length*_mlDIPvSizeofElem( copy[i].param );
	MLint32 lenToAlignedEnd = 
	  (params[i].length*_mlDIPvSizeofElem( params[i].param ) + 7) & ~(0x7);

	if ( lenToAlignedEnd > lenToEnd ) {
	  memset( arrayData + lenToEnd, 0, lenToAlignedEnd - lenToEnd );
	}
      }
#endif

      copy[i].value.pByte = (MLbyte *) arrayData; 
      /* Force multiple of 8
       */
      arrayData +=
	(params[i].length*_mlDIPvSizeofElem( params[i].param ) + 7) & ~(0x7);
    }
  }
  return ML_STATUS_NO_ERROR;
}


/* ---------------------------------------------------------mlPvValueFromString
 *
 * Convert a text string to a mlPv.value
 *
 * The string must previously have been generated by mlPvValueToString
 *
 * returned size is the number of bytes read
 */ 
MLstatus MLAPI mlPvValueFromString( MLint64 objectId, const char* buffer, 
				    MLint32* size, MLpv* pv, 
				    MLbyte* arrayData, MLint32 arrayLength )
{
  MLstatus status;
  MLpv *paramCap;

  /* Look up the meaning of this param on this particular device
   */
  if ( mlPvGetCapabilities( objectId, pv->param, &paramCap ) ) {
    *size = 0;
    mlDebug( "Get capabilities failed\n" );
    return ML_STATUS_INVALID_PARAMETER;
  }

  status =
    _valueFromString( paramCap, buffer, size, pv, arrayData,arrayLength );

  mlFreeCapabilities( paramCap );
  return status;
}


/* --------------------------------------------------------mluPvParamFromString
 *
 * Convert a text string to a mlPv.param
 *
 * The string must previously have been generated by mlPvParamToString
 *
 * returned size is the number of bytes read
 */
MLstatus MLAPI mlPvParamFromString( MLint64 objectId, const char* buffer, 
				    MLint32* size, MLpv* pv )
{
  int i;
  MLstatus status;
  MLpv* devCap;
  MLpv* devParams;
  MLint64 paramTypes[2] =
    { ML_PARAM_IDS_INT64_ARRAY,  ML_OPEN_OPTION_IDS_INT64_ARRAY };
  int pt;

  /* Search through all the params on the device to find a match
   */
  if ( (status = mlGetCapabilities( objectId, &devCap ))
       != ML_STATUS_NO_ERROR ) {
    return status;
  }

  for ( pt = 0; pt < 2; pt++ ) {
    if ( (devParams = mlPvFind( devCap, paramTypes[pt] )) == NULL ) {
      mlFreeCapabilities( devCap );
      if ( pt == 0 ) {
	return ML_STATUS_INTERNAL_ERROR;
      }
      return ML_STATUS_INVALID_PARAMETER;
    }

    for ( i=0; i < devParams->length; i++ ) {
      MLpv* paramCap, *paramId, *paramName;
      MLint32 matchLength;

      mlPvGetCapabilities( objectId, devParams->value.pInt64[i], &paramCap );
      paramId = mlPvFind( paramCap, ML_ID_INT64 );
      paramName = mlPvFind( paramCap, ML_NAME_BYTE_ARRAY );

      if ( paramName == NULL || paramId == NULL ) {
	mlFreeCapabilities( paramCap );
	mlFreeCapabilities( devCap );
	return ML_STATUS_INTERNAL_ERROR;
      }

      matchLength =
	mlstringcmp( (const char*) paramName->value.pByte, buffer );
      if ( matchLength ) {
	pv->param = paramId->value.int64;
	*size = matchLength;
	mlFreeCapabilities( paramCap );
	mlFreeCapabilities( devCap );
	return ML_STATUS_NO_ERROR;
      }
      mlFreeCapabilities( paramCap );
    }	  
  }

  mlFreeCapabilities( devCap );
  return ML_STATUS_INVALID_PARAMETER;
}


/* --------------------------------------------------------------mlPvFromString
 *
 * Convert a text string to a mlPV.
 *
 * The string must previously have been generated by mluPvToString
 */
MLstatus MLAPI mlPvFromString( MLint64 objectId, const char* buffer, 
			       MLint32* size, MLpv* pv, 
			       MLbyte* arrayData, MLint32 arrayLength )
{
  MLint32 paramSize = *size;
  MLint32 punctSize = 0;
  MLint32 valueSize = 0;
  MLstatus status = ML_STATUS_NO_ERROR;

  /* Skip leading whitespace
   */
  if ( buffer ) {
    while ( isspace( *buffer ) ) {
      buffer++;
    }
  }

  if ( (status = mlPvParamFromString( objectId, buffer, &paramSize, pv )) ) {
    *size = paramSize;
    mlDebug( "Unable to parse param name from %s\n", buffer );
    return status;
  }

  /* Skip over punctuation - expect an "=" surrounded by optional spaces
   */
  {
    const char* thisChar = strpbrk( buffer, " \t\n=" );
    if ( thisChar ) {
      paramSize = (MLint32) (thisChar - buffer);

      while ( isspace( *thisChar ) ) {
	thisChar++;
	punctSize++;
      }

      if ( *thisChar != '=' ) {
	*size = paramSize;
	mlDebug( "Unable to parse punctuation from %s\n", buffer+paramSize );
	return ML_STATUS_INVALID_ARGUMENT;
      }

      do {
	thisChar++;
	punctSize++;
      } while( isspace( *thisChar ) );
    }
    valueSize = *size - paramSize - punctSize;

    status = mlPvValueFromString( objectId, buffer+paramSize+punctSize, 
				  &valueSize, pv, arrayData, arrayLength );
  }

  if ( status ) {
    mlDebug( "Unable to parse value from %s\n", buffer+paramSize+punctSize );
  }
  *size = paramSize + punctSize + valueSize;
  return status;
}


/* --------------------------------------------------------------------mlPvFind
 *
 * Find the address of the first parameter to match a particular param
 * (or NULL if not present).
 */
MLpv* MLAPI mlPvFind( MLpv* params, MLint64 searchParam )
{
  if ( params ) {
    while ( params->param != ML_END ) {
      if ( params->param == searchParam ) {
	return params;
      }
      params++;
    }

    if ( searchParam == ML_END ) {
      return params;
    }
  }
  return NULL;
}


/* -----------------------------------------------------------_mlDIPvSizeofElem
 */         
MLint32  MLAPI _mlDIPvSizeofElem( MLint64 param )
{
  switch( ML_PARAM_GET_TYPE_ELEM( param ) ) {
  case ML_TYPE_ELEM_BYTE:   return sizeof( MLbyte );
  case ML_TYPE_ELEM_INT32:  return sizeof( MLint32 );
  case ML_TYPE_ELEM_INT64:  return sizeof( MLint64 );
  case ML_TYPE_ELEM_REAL32: return sizeof( MLreal32 );
  case ML_TYPE_ELEM_REAL64: return sizeof( MLreal64 );
  case ML_TYPE_ELEM_MSG:    return sizeof( MLpv* );
  case ML_TYPE_ELEM_PV:     return sizeof( MLpv );
  }
  return 0;
}


/* ----------------------------------------------------mlDI extraction routines
 */
MLint32 MLAPI mlDIextractIdType( MLint64 id )
{
  return ML_REF_GET_TYPE( id );
}

/* System index is really the system id (aka hostid)
 */
MLint32 MLAPI mlDIextractSystemIndex( MLint64 id )
{
  mlAssert( ML_REF_GET_TYPE( id ) == ML_REF_TYPE_SYSTEM );
  return ((MLint32) (id) & 0xffffffff);
}

MLint32 MLAPI mlDIextractDeviceIndex( MLint64 id )
{
  if ( ML_REF_GET_TYPE( id ) != ML_REF_TYPE_DEVICE &&
       ML_REF_GET_TYPE( id ) != ML_REF_TYPE_JACK &&
       ML_REF_GET_TYPE( id ) != ML_REF_TYPE_PATH &&
       ML_REF_GET_TYPE( id ) != ML_REF_TYPE_XCODE ) {
    return -1;
  }
      
  return ((MLint32) (id >> 32) & ML_REF_MASK_DEVICE) >> ML_REF_SHIFT_DEVICE;
}

MLint32 MLAPI mlDIextractJackIndex( MLint64 id )
{
  mlAssert( ML_REF_GET_TYPE( id ) == ML_REF_TYPE_JACK );
  return ((MLint32) (id >> 32) & ML_REF_MASK_JACK) >> ML_REF_SHIFT_JACK;
}

MLint32 MLAPI mlDIextractPathIndex( MLint64 id )
{
  mlAssert( ML_REF_GET_TYPE( id ) == ML_REF_TYPE_PATH );
  return ((MLint32) (id >> 32) & ML_REF_MASK_PATH) >> ML_REF_SHIFT_PATH;
}

MLint32 MLAPI mlDIextractXcodeEngineIndex( MLint64 id )
{
  mlAssert( ML_REF_GET_TYPE( id ) == ML_REF_TYPE_XCODE);
  return ((MLint32) (id >> 32) & ML_REF_MASK_XCODE) >> ML_REF_SHIFT_XCODE;
}

MLint32 MLAPI mlDIextractXcodePipeIndex( MLint64 id )
{
  mlAssert( ML_REF_GET_TYPE( id ) == ML_REF_TYPE_XCODE );
  return ((MLint32) (id >> 32) & ML_REF_MASK_PIPE) >> ML_REF_SHIFT_PIPE;
}

MLint64 MLAPI mlDIextractXcodeIdFromPipeId( MLint64 id )
{
  mlAssert( ML_REF_GET_TYPE( id ) == ML_REF_TYPE_XCODE );
  return id & ~((MLint64) ML_REF_MASK_PIPE << (ML_REF_SHIFT_PIPE + 32));
}


/* --------------------------------------------------mlDI construct ID routines
 */
MLint64 MLAPI mlDImakeSystemId( MLint32 systemIndex )
{
  return ((MLint64) (ML_REF_TYPE_SYSTEM) << 32) | (MLuint32) systemIndex;
}

MLint64 MLAPI mlDImakeDeviceId( MLint64 systemId, MLint32 deviceIndex )
{
  systemId = systemId; /* Not currently used */
  return (MLint64) (ML_REF_TYPE_DEVICE | 
		    (deviceIndex<<ML_REF_SHIFT_DEVICE)) << 32;
}

MLint64 MLAPI mlDImakeJackId( MLint64 deviceId, MLint32 jackIndex )
{
  MLint32 devIndex = mlDIextractDeviceIndex( deviceId );
  if ( devIndex == -1 ) {
    return 0;
  }

  return (MLint64) (ML_REF_TYPE_JACK | (devIndex << ML_REF_SHIFT_DEVICE) |
		    (jackIndex << ML_REF_SHIFT_JACK)) << 32;
}

MLint64 MLAPI mlDImakePathId( MLint64 deviceId, MLint32 pathIndex )
{
  MLint32 devIndex = mlDIextractDeviceIndex( deviceId );
  if ( devIndex == -1 ) {
    return 0;
  }
 
  return (MLint64) (ML_REF_TYPE_PATH | (devIndex << ML_REF_SHIFT_DEVICE) |
		    (pathIndex << ML_REF_SHIFT_PATH)) << 32;
}

MLint64 MLAPI mlDImakeXcodeEngineId( MLint64 deviceId, MLint32 xcodeIndex )
{
  MLint32 devIndex = mlDIextractDeviceIndex( deviceId );
  if ( devIndex == -1 ) {
    return 0;
  }

  return (MLint64) (ML_REF_TYPE_XCODE | (devIndex << ML_REF_SHIFT_DEVICE) |
		    (xcodeIndex << ML_REF_SHIFT_XCODE)) << 32;
}

MLint64 MLAPI mlDImakeXcodePipeId( MLint64 xcodeEngineId, MLint32 pipeIndex )
{
  return xcodeEngineId | (((MLint64) pipeIndex << ML_REF_SHIFT_PIPE) << 32);
}


/* ------------------------------------------------------mlDIparentIdOfDeviceId
 */
/* ARGSUSED */
MLint64 MLAPI mlDIparentIdOfDeviceId( MLint64 deviceId )
{
  /* In this preliminary implementation, all physical devices are on
   * the local system
   */
  return _thisSystem.id;
}


/* ------------------------------------------------------mlDIparentIdOfLogDevId
 */
MLint64 MLAPI mlDIparentIdOfLogDevId( MLint64 logDevId )
{
  MLint32 devIndex = mlDIextractDeviceIndex( logDevId );
  if ( devIndex == -1 ) {
    return 0;
  }

  return mlDImakeDeviceId( _thisSystem.id, devIndex );
}


/* ----------------------------------------------------------------mlDIisOpenId
 */
MLint32 MLAPI mlDIisOpenId( MLint64 candidateId )
{
  if ( candidateId & 0x7fffffff ) {
    return 1;
  } else {
    return 0;
  }
}


/* -------------------------------------------------mlDIconvertOpenIdToStaticId
 */
MLint64 MLAPI mlDIconvertOpenIdToStaticId( MLopenid openid )
{
  return openid & (MLopenid) 0xffffffff00000000;
}
