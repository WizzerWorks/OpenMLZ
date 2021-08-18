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

#ifndef _ML_PARAM_H_
#define _ML_PARAM_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <ML/mltypes.h>


/* ------------------------------------------------------------------------MLpv
 *
 * Parameter-value structure definition.
 */
typedef union MLvalue_u {
  MLbyte    byte;     
  MLint32   int32;
  MLint64   int64;     
  MLreal32  real32;
  MLreal64  real64;
  MLbyte*   pByte;
  MLint32*  pInt32;
  MLint64*  pInt64;
  MLreal32* pReal32;
  MLreal64* pReal64;
  struct MLpv_t* pPv;
  struct MLpv_t** ppPv;
} MLvalue;


typedef struct MLpv_t
{
  MLint64     param;
  MLvalue     value;
  MLint32     maxLength;
  MLint32     length;
} MLpv;


/* -------------------------------------------------------------------Constants
 *
 * Constants for extracting information from the parameter name
 */
enum MLpvMaskEnum
{
  ML_PARAM_MASK_ID        = 0x7FFFF000, /* unique parameter id */
  ML_PARAM_MASK_CLASS     = 0x7FF00000, /* unique parameter class */
  ML_PARAM_MASK_INDEX     = 0x000FF000, /* unique parameter index
					 * within class */
  ML_PARAM_MASK_RESERVED  = 0x00000F00, /* future use */
  ML_PARAM_MASK_TYPE      = 0x000000FF, /* typeof param value */

  ML_PARAM_MASK_TYPE_ELEM = 0x0000000F, /* byte, int... */
  ML_PARAM_MASK_TYPE_VAL  = 0x000000F0  /* scalar, array, pointer... */
};

enum MLpvTypeElemEnum
{
  ML_TYPE_ELEM_BYTE   = 0x01,
  ML_TYPE_ELEM_INT32  = 0x02,
  ML_TYPE_ELEM_INT64  = 0x03,
  ML_TYPE_ELEM_REAL32 = 0x05,
  ML_TYPE_ELEM_REAL64 = 0x06,
  ML_TYPE_ELEM_PV     = 0x07,
  ML_TYPE_ELEM_MSG    = 0x08 /* a message is a list of MLpv's */
};

enum MLpvTypeValEnum
{
  ML_TYPE_VAL_SCALAR  = 0x10,
  ML_TYPE_VAL_POINTER = 0x20,
  ML_TYPE_VAL_ARRAY   = 0x40
};

enum MLpvTypeEnum
{
  ML_TYPE_BYTE           = ML_TYPE_ELEM_BYTE  | ML_TYPE_VAL_SCALAR,
  ML_TYPE_INT32          = ML_TYPE_ELEM_INT32 | ML_TYPE_VAL_SCALAR,
  ML_TYPE_INT64          = ML_TYPE_ELEM_INT64 | ML_TYPE_VAL_SCALAR,
  ML_TYPE_REAL32         = ML_TYPE_ELEM_REAL32| ML_TYPE_VAL_SCALAR,
  ML_TYPE_REAL64         = ML_TYPE_ELEM_REAL64| ML_TYPE_VAL_SCALAR,

  ML_TYPE_BYTE_POINTER   = ML_TYPE_ELEM_BYTE  | ML_TYPE_VAL_POINTER,
  ML_TYPE_INT32_POINTER  = ML_TYPE_ELEM_INT32 | ML_TYPE_VAL_POINTER,
  ML_TYPE_INT64_POINTER  = ML_TYPE_ELEM_INT64 | ML_TYPE_VAL_POINTER,
  ML_TYPE_REAL32_POINTER = ML_TYPE_ELEM_REAL32| ML_TYPE_VAL_POINTER,
  ML_TYPE_REAL64_POINTER = ML_TYPE_ELEM_REAL64| ML_TYPE_VAL_POINTER,
  ML_TYPE_PV_POINTER     = ML_TYPE_ELEM_PV    | ML_TYPE_VAL_POINTER,

  ML_TYPE_BYTE_ARRAY     = ML_TYPE_ELEM_BYTE  | ML_TYPE_VAL_ARRAY,
  ML_TYPE_INT32_ARRAY    = ML_TYPE_ELEM_INT32 | ML_TYPE_VAL_ARRAY,
  ML_TYPE_INT64_ARRAY    = ML_TYPE_ELEM_INT64 | ML_TYPE_VAL_ARRAY,
  ML_TYPE_REAL32_ARRAY   = ML_TYPE_ELEM_REAL32| ML_TYPE_VAL_ARRAY,
  ML_TYPE_REAL64_ARRAY   = ML_TYPE_ELEM_REAL64| ML_TYPE_VAL_ARRAY,
  ML_TYPE_PV_ARRAY       = ML_TYPE_ELEM_PV    | ML_TYPE_VAL_ARRAY,
  ML_TYPE_MSG_ARRAY      = ML_TYPE_ELEM_MSG   | ML_TYPE_VAL_ARRAY
};


/* ----------------------------------------------------------------------Macros
 *
 * Macros for extracting information from the parameter name
 */
#define ML_PARAM_GET_ID(name)        ((MLint32)(name)&ML_PARAM_MASK_ID)
#define ML_PARAM_GET_CLASS(name)     ((MLint32)(name)&ML_PARAM_MASK_CLASS)
#define ML_PARAM_GET_TYPE(name) \
     (enum MLpvTypeEnum)((MLint32)(name)&ML_PARAM_MASK_TYPE)
#define ML_PARAM_GET_TYPE_ELEM(name) \
     (enum MLpvTypeElemEnum)((MLint32)(name)&ML_PARAM_MASK_TYPE_ELEM)
#define ML_PARAM_GET_TYPE_VAL(name) \
     (enum MLpvTypeValEnum)((MLint32)(name)&ML_PARAM_MASK_TYPE_VAL)
#define ML_PARAM_IS_ARRAY(name) \
     (ML_PARAM_GET_TYPE_VAL(name) == ML_TYPE_VAL_ARRAY)
#define ML_PARAM_IS_SCALAR(name) \
     (ML_PARAM_GET_TYPE_VAL(name) == ML_TYPE_VAL_SCALAR)
#define ML_PARAM_IS_POINTER(name) \
     (ML_PARAM_GET_TYPE_VAL(name) == ML_TYPE_VAL_POINTER)


/* Create a new parameter name
 */
#define ML_PARAM_NAME( class, type, index ) ( (class)|(type)|((index)<<12) )


/* Create a new resource id, given a reference (see below) and name
 * (see above)
 */
#define ML_PARAM_NAME_TO_ID( targetRef, name) ( (MLint64)(targetRef) | (name) )


/* ------------------------------------------------------------------References
 *
 * A reference has the high 32 bits from this mask. 
 */
enum mlRefMaskEnum
{
  ML_REF_MASK_TYPE        = 0xF0000000, /* type of reference */
  ML_REF_MASK_SYSTEM      = 0x0FFFFFFF, /* system count */
  ML_REF_MASK_DEVICE      = 0x0FFFF000, /* device count */
  ML_REF_MASK_JACK        = 0x00000FFF, /* jack count */
  ML_REF_MASK_PATH        = 0x00000FFF, /* path count */
  ML_REF_MASK_XCODE       = 0x00000FF0, /* xcode count */
  ML_REF_MASK_PIPE	  = 0x0000000F, /* xcode pipe */
  ML_REF_MASK_UST_SOURCE  = 0x0FFFFFFF  /* UST source count */
};

enum mlRefType
{
  ML_REF_TYPE_SYSTEM      = 0x10000000,
  ML_REF_TYPE_DEVICE      = 0x20000000,
  ML_REF_TYPE_JACK        = 0x30000000,
  ML_REF_TYPE_PATH        = 0x40000000,
  ML_REF_TYPE_XCODE       = 0x50000000,
  ML_REF_TYPE_UST_SOURCE  = 0x60000000
};

#define ML_REF_GET_TYPE(id)  ((MLint32)(id>>32) & ML_REF_MASK_TYPE)
#define ML_REF_GET_XCODE_PIPE(id) ((MLint32)(id>>32) & ML_REF_MASK_PIPE)


/* -----------------------------------------------------------Parameter Classes
 */
enum MLpvClassEnum
{
  ML_CLASS_STATIC	= 0,
  ML_CLASS_USER		= 0x00100000,
  ML_CLASS_JACK		= 0x00200000,
  ML_CLASS_VIDEO	= 0x00300000,
  ML_CLASS_IMAGE	= 0x00400000,
  ML_CLASS_XCODE	= 0x00500000,
  ML_CLASS_DEVICE	= 0x00600000,
  ML_CLASS_INTERNAL	= 0x00700000,
  ML_CLASS_OPEN_DEVICE	= 0x00800000,
  ML_CLASS_WAIT_FOR	= 0x00900000,
  ML_CLASS_CONDITIONAL	= 0x00a00000,
  ML_CLASS_BUFFER	= 0x00b00000,
  ML_CLASS_COLOR	= 0x00c00000,
  ML_CLASS_MPEG		= 0x00d00000,
  ML_CLASS_AUDIO	= 0x00e00000,
  ML_CLASS_CUSTOM_DD	= 0x00f00000,
  ML_CLASS_ANC_DATA	= 0x01000000
};


/* Parameter ID bases for mlSDK subsystems
 */
#define ML_PV_STATIC_CLASS_BASE_SYSTEM 	0x10
#define ML_PV_STATIC_CLASS_BASE_DEVICE	0x20
#define ML_PV_STATIC_CLASS_BASE_JACK	0x30
#define ML_PV_STATIC_CLASS_BASE_PATH	0x40
#define ML_PV_STATIC_CLASS_BASE_XCODE	0x50
#define ML_PV_STATIC_CLASS_BASE_PARAM	0x60
#define ML_PV_STATIC_CLASS_BASE_UST     0x70


/* ---------------------------------------------------------Capabilities params
 *
 * Parameters returned in capabilities arrays
 */
enum MLpvStaticClassEnum
{
  ML_END                          = 0,
  ML_ID_INT64                     = ML_PARAM_NAME( ML_CLASS_STATIC,
						   ML_TYPE_INT64,       0x1 ),
  ML_NAME_BYTE_ARRAY              = ML_PARAM_NAME( ML_CLASS_STATIC,
						   ML_TYPE_BYTE_ARRAY,  0x2 ),
  ML_PARENT_ID_INT64              = ML_PARAM_NAME( ML_CLASS_STATIC,
						   ML_TYPE_INT64,       0x3 ),
  ML_PARAM_IDS_INT64_ARRAY        = ML_PARAM_NAME( ML_CLASS_STATIC,
						   ML_TYPE_INT64_ARRAY, 0x4 ),
  ML_PRESET_MSG_ARRAY             = ML_PARAM_NAME( ML_CLASS_STATIC,
						   ML_TYPE_MSG_ARRAY,   0x5 ),
  ML_OPEN_OPTION_IDS_INT64_ARRAY  = ML_PARAM_NAME( ML_CLASS_STATIC,
						   ML_TYPE_INT64_ARRAY, 0x6 ),

  ML_SYSTEM_DEVICE_IDS_INT64_ARRAY= ML_PARAM_NAME( ML_CLASS_STATIC,
						   ML_TYPE_INT64_ARRAY, 0x10 ),
  ML_DESCRIPTION_BYTE_ARRAY       = ML_PARAM_NAME( ML_CLASS_STATIC,
						   ML_TYPE_BYTE_ARRAY,  0x11 ),

  ML_DEVICE_VERSION_INT32         = ML_PARAM_NAME( ML_CLASS_STATIC,
						   ML_TYPE_INT32,       0x20 ),
  ML_DEVICE_INDEX_INT32           = ML_PARAM_NAME( ML_CLASS_STATIC,
						   ML_TYPE_INT32,       0x21 ),
  ML_DEVICE_JACK_IDS_INT64_ARRAY  = ML_PARAM_NAME( ML_CLASS_STATIC,
						   ML_TYPE_INT64_ARRAY, 0x22 ),
  ML_DEVICE_PATH_IDS_INT64_ARRAY  = ML_PARAM_NAME( ML_CLASS_STATIC,
						   ML_TYPE_INT64_ARRAY, 0x23 ),
  ML_DEVICE_XCODE_IDS_INT64_ARRAY = ML_PARAM_NAME( ML_CLASS_STATIC,
						   ML_TYPE_INT64_ARRAY, 0x24 ),
  ML_DEVICE_LOCATION_BYTE_ARRAY   = ML_PARAM_NAME( ML_CLASS_STATIC,
						   ML_TYPE_BYTE_ARRAY,  0x25 ),
  
  ML_PARAM_TYPE_INT32             = ML_PARAM_NAME( ML_CLASS_STATIC,
						   ML_TYPE_INT32,       0x62 ),
  ML_PARAM_ACCESS_INT32           = ML_PARAM_NAME( ML_CLASS_STATIC,
						   ML_TYPE_INT32,       0x63 ),

  ML_PARAM_DEFAULT                = ML_PARAM_NAME( ML_CLASS_STATIC,
						   0 /* polymorphic */, 0x64 ),
  ML_PARAM_DEFAULT_INT32          = ML_PARAM_NAME( ML_CLASS_STATIC,
						   ML_TYPE_INT32,       0x64 ),
  ML_PARAM_DEFAULT_INT64          = ML_PARAM_NAME( ML_CLASS_STATIC,
						   ML_TYPE_INT64,       0x64 ),
  ML_PARAM_DEFAULT_REAL32         = ML_PARAM_NAME( ML_CLASS_STATIC,
						   ML_TYPE_REAL32,      0x64 ),
  ML_PARAM_DEFAULT_REAL64         = ML_PARAM_NAME( ML_CLASS_STATIC,
						   ML_TYPE_REAL64,      0x64 ),

  ML_PARAM_MINS_ARRAY             = ML_PARAM_NAME( ML_CLASS_STATIC,
						   ML_TYPE_VAL_ARRAY,   0x65 ),
  ML_PARAM_MINS_INT32_ARRAY       = ML_PARAM_NAME( ML_CLASS_STATIC,
						   ML_TYPE_INT32_ARRAY, 0x65 ),
  ML_PARAM_MINS_INT64_ARRAY       = ML_PARAM_NAME( ML_CLASS_STATIC,
						   ML_TYPE_INT64_ARRAY, 0x65 ),
  ML_PARAM_MINS_REAL32_ARRAY      = ML_PARAM_NAME( ML_CLASS_STATIC,
						   ML_TYPE_REAL32_ARRAY,0x65 ),
  ML_PARAM_MINS_REAL64_ARRAY      = ML_PARAM_NAME( ML_CLASS_STATIC,
						   ML_TYPE_REAL64_ARRAY,0x65 ),

  ML_PARAM_MAXS_ARRAY             = ML_PARAM_NAME( ML_CLASS_STATIC,
						   ML_TYPE_VAL_ARRAY,   0x66 ),
  ML_PARAM_MAXS_INT32_ARRAY       = ML_PARAM_NAME( ML_CLASS_STATIC,
						   ML_TYPE_INT32_ARRAY, 0x66 ),
  ML_PARAM_MAXS_INT64_ARRAY       = ML_PARAM_NAME( ML_CLASS_STATIC,
						   ML_TYPE_INT64_ARRAY, 0x66 ),
  ML_PARAM_MAXS_REAL32_ARRAY      = ML_PARAM_NAME( ML_CLASS_STATIC,
						   ML_TYPE_REAL32_ARRAY,0x66 ),
  ML_PARAM_MAXS_REAL64_ARRAY      = ML_PARAM_NAME( ML_CLASS_STATIC,
						   ML_TYPE_REAL64_ARRAY,0x66 ),

  ML_PARAM_ENUM_VALUES_ARRAY      = ML_PARAM_NAME( ML_CLASS_STATIC,
						   ML_TYPE_VAL_ARRAY,   0x67 ),
  ML_PARAM_ENUM_VALUES_INT32_ARRAY= ML_PARAM_NAME( ML_CLASS_STATIC,
						   ML_TYPE_INT32_ARRAY, 0x67 ),
  ML_PARAM_ENUM_VALUES_INT64_ARRAY= ML_PARAM_NAME( ML_CLASS_STATIC,
						   ML_TYPE_INT64_ARRAY, 0x67 ),
  ML_PARAM_ENUM_NAMES_BYTE_ARRAY  = ML_PARAM_NAME( ML_CLASS_STATIC,
						   ML_TYPE_BYTE_ARRAY,  0x68 ),

  ML_PARAM_INCREMENT              = ML_PARAM_NAME( ML_CLASS_STATIC,
						   ML_TYPE_VAL_SCALAR,  0x69 ),
  ML_PARAM_INCREMENT_INT32        = ML_PARAM_NAME( ML_CLASS_STATIC,
						   ML_TYPE_INT32,       0x69 ),
  ML_PARAM_INCREMENT_INT64        = ML_PARAM_NAME( ML_CLASS_STATIC,
						   ML_TYPE_INT64,       0x69 ),
  ML_PARAM_INCREMENT_REAL32       = ML_PARAM_NAME( ML_CLASS_STATIC,
						   ML_TYPE_REAL32,      0x69 ),
  ML_PARAM_INCREMENT_REAL64       = ML_PARAM_NAME( ML_CLASS_STATIC,
						   ML_TYPE_REAL64,      0x69 ),

  ML_PARAM_INCREMENT_ARRAY        = ML_PARAM_NAME( ML_CLASS_STATIC,
						   ML_TYPE_VAL_ARRAY,   0x69 ),
  ML_PARAM_INCREMENT_INT32_ARRAY  = ML_PARAM_NAME( ML_CLASS_STATIC,
						   ML_TYPE_INT32_ARRAY, 0x69 ),
  ML_PARAM_INCREMENT_INT64_ARRAY  = ML_PARAM_NAME( ML_CLASS_STATIC,
						   ML_TYPE_INT64_ARRAY, 0x69 ),
  ML_PARAM_INCREMENT_REAL32_ARRAY = ML_PARAM_NAME( ML_CLASS_STATIC,
						   ML_TYPE_REAL32_ARRAY,0x69 ),
  ML_PARAM_INCREMENT_REAL64_ARRAY = ML_PARAM_NAME( ML_CLASS_STATIC,
						   ML_TYPE_REAL64_ARRAY,0x69 ),

  ML_UST_SOURCE_IDS_INT64_ARRAY =
        ML_PARAM_NAME( ML_CLASS_STATIC, ML_TYPE_INT64_ARRAY,
		       ML_PV_STATIC_CLASS_BASE_UST + 0 ),
  ML_UST_SELECTED_SOURCE_ID_INT64 =
        ML_PARAM_NAME( ML_CLASS_STATIC, ML_TYPE_INT64,
		       ML_PV_STATIC_CLASS_BASE_UST + 1 ),
  ML_UST_SOURCE_UPDATE_PERIOD_INT32 =
        ML_PARAM_NAME( ML_CLASS_STATIC, ML_TYPE_INT32,
		       ML_PV_STATIC_CLASS_BASE_UST + 2 ),
  ML_UST_SOURCE_LATENCY_VARIATION_INT32 =
        ML_PARAM_NAME( ML_CLASS_STATIC, ML_TYPE_INT32,
		       ML_PV_STATIC_CLASS_BASE_UST + 3 ),

  /* Note the following are in ML_CLASS_BUFFER, not ML_CLASS_STATIC */
  ML_DATA_BYTE_ARRAY              = ML_PARAM_NAME( ML_CLASS_BUFFER,
						   ML_TYPE_BYTE_ARRAY,  0x70 ),
  ML_DATA_INT32_ARRAY             = ML_PARAM_NAME( ML_CLASS_BUFFER,
						   ML_TYPE_INT32_ARRAY, 0x70 ),
  ML_DATA_INT64_ARRAY             = ML_PARAM_NAME( ML_CLASS_BUFFER,
						   ML_TYPE_INT64_ARRAY, 0x70 ),

  ML_PARAM_INVALID = ~1
};


enum mlParamAccessFlags
{
  /* These access flags are public and guaranteed not to change */
  ML_ACCESS_READ  = 0x01,             /* may be read */
  ML_ACCESS_WRITE = 0x02,             /* may be written */

  ML_ACCESS_PERSISTENT      = 0x0100, /* 1 if control has persistent default */
  ML_ACCESS_BUFFER_PARAM    = 0x0200, /* belongs to buffer grp not param grp.*/
  ML_ACCESS_IMMEDIATE       = 0x0400, /* may be used in set/get */
  ML_ACCESS_QUEUED          = 0x0800, /* may be used in send/query */
  ML_ACCESS_SEND_BUFFER     = 0x1000, /* may be used in sendBuffers */
  ML_ACCESS_DURING_TRANSFER = 0x2000, /* may by used during a transfer */
  ML_ACCESS_PASS_THROUGH    = 0x4000, /* is passed through untouched */
  ML_ACCESS_BIT_ARRAY       = 0x8000, /* convert to internal bit array */
  ML_ACCESS_OPEN_OPTION     = 0x00010000, /* usable as an open option */


  /* And some convenient abbreviations
   */

  ML_ACCESS_R    = ML_ACCESS_READ,
  ML_ACCESS_RI   = ML_ACCESS_R   | ML_ACCESS_IMMEDIATE,
  ML_ACCESS_RQ   = ML_ACCESS_R   | ML_ACCESS_QUEUED,
  ML_ACCESS_RQT  = ML_ACCESS_RQ  | ML_ACCESS_DURING_TRANSFER,
  ML_ACCESS_RBT  = ML_ACCESS_R   | ML_ACCESS_SEND_BUFFER |
                   ML_ACCESS_DURING_TRANSFER,

  ML_ACCESS_W    = ML_ACCESS_WRITE,
  ML_ACCESS_RW   = ML_ACCESS_R   | ML_ACCESS_W,
  ML_ACCESS_RWI  = ML_ACCESS_RW  | ML_ACCESS_IMMEDIATE,
  ML_ACCESS_RWQ  = ML_ACCESS_RW  | ML_ACCESS_QUEUED,
  ML_ACCESS_RWQT = ML_ACCESS_RWQ | ML_ACCESS_DURING_TRANSFER,
  ML_ACCESS_RWBT = ML_ACCESS_RW  | ML_ACCESS_SEND_BUFFER |
                   ML_ACCESS_DURING_TRANSFER
};


#define ML_SYSTEM_SELFTEST \
   (((MLint64)ML_REF_TYPE_SYSTEM <<32) | ((MLint64)(0x0FFFFFFF) << 32))
#define ML_SYSTEM_DAEMON_CALLING \
   (((MLint64)ML_REF_TYPE_SYSTEM <<32) | ((MLint64)(0) << 32))
#define ML_SYSTEM_LOCALHOST \
   (((MLint64)ML_REF_TYPE_SYSTEM <<32) | ((MLint64)(1) << 32))
#define ML_JACK_GENLOCK_INTERNAL \
   ((MLint64)(ML_REF_TYPE_JACK | ML_REF_MASK_JACK) <<32)
#define ML_JACK_NULL ((MLint64)0)


/* ID of the "default built-in" UST source. This source's capabilities
 * can not be queried.
 */
#define ML_BUILTIN_UST_SOURCE \
   ((MLint64)(ML_REF_TYPE_UST_SOURCE | ML_REF_MASK_UST_SOURCE) << 32)


/* -----------------------------------------------------------------During open
 *
 * Params for use during open call to jack, path or xcode
 */
enum MLpvOpenDeviceClassEnum
{
  ML_OPEN_MODE_INT32                 = ML_PARAM_NAME( ML_CLASS_OPEN_DEVICE,
						      ML_TYPE_INT32, 0x1 ),
  ML_OPEN_SEND_QUEUE_COUNT_INT32     = ML_PARAM_NAME( ML_CLASS_OPEN_DEVICE,
						      ML_TYPE_INT32, 0x2 ),
  ML_OPEN_RECEIVE_QUEUE_COUNT_INT32  = ML_PARAM_NAME( ML_CLASS_OPEN_DEVICE,
						      ML_TYPE_INT32, 0x3 ),
  ML_OPEN_MESSAGE_PAYLOAD_SIZE_INT32 = ML_PARAM_NAME( ML_CLASS_OPEN_DEVICE,
						      ML_TYPE_INT32, 0x4 ),
  ML_OPEN_EVENT_PAYLOAD_COUNT_INT32  = ML_PARAM_NAME( ML_CLASS_OPEN_DEVICE,
						      ML_TYPE_INT32, 0x5 ),
  ML_OPEN_SEND_SIGNAL_COUNT_INT32    = ML_PARAM_NAME( ML_CLASS_OPEN_DEVICE,
						      ML_TYPE_INT32, 0x6 ),
  ML_OPEN_XCODE_MODE_INT32           = ML_PARAM_NAME( ML_CLASS_OPEN_DEVICE,
						      ML_TYPE_INT32, 0x7 ),
  ML_OPEN_XCODE_STREAM_INT32         = ML_PARAM_NAME( ML_CLASS_OPEN_DEVICE,
						      ML_TYPE_INT32, 0x8 )
};


enum mlOpenModes 
{
  ML_MODE_RO  = 0,
  ML_MODE_RWS = 1,
  ML_MODE_RWE = 2
};


/* ------------------------------------------------------------------After open
 *
 * Params which may be sent/recieved to/from a device
 * (jack,path,xcode) after it has been opened.
 */
enum MLpvDeviceClassEnum
{
  ML_DEVICE_EVENTS_INT32_ARRAY    = ML_PARAM_NAME( ML_CLASS_DEVICE,
						   ML_TYPE_INT32_ARRAY, 0x1 ),
  ML_DEVICE_STATE_INT32           = ML_PARAM_NAME( ML_CLASS_DEVICE,
						   ML_TYPE_INT32, 0x2 ),
  ML_SELECT_ID_INT64              = ML_PARAM_NAME( ML_CLASS_DEVICE,
						   ML_TYPE_INT64, 0x3 ),
  ML_QUEUE_SEND_COUNT_INT32       = ML_PARAM_NAME( ML_CLASS_DEVICE,
						   ML_TYPE_INT32, 0x4 ),
  ML_QUEUE_RECEIVE_COUNT_INT32    = ML_PARAM_NAME( ML_CLASS_DEVICE,
						   ML_TYPE_INT32, 0x5 ),
  
  /* For current MLmodules the INT64 versions of the following should
   * be used the INT32 versions are being kept for backwards
   * compability with older MLmodules
   */
  ML_QUEUE_SEND_WAITABLE_INT64    = ML_PARAM_NAME( ML_CLASS_DEVICE,
						   ML_TYPE_INT64, 0x6 ),
  ML_QUEUE_SEND_WAITABLE_INT32    = ML_PARAM_NAME( ML_CLASS_DEVICE,
						   ML_TYPE_INT32, 0x6 ),
  ML_QUEUE_RECEIVE_WAITABLE_INT64 = ML_PARAM_NAME( ML_CLASS_DEVICE,
						   ML_TYPE_INT64, 0x7 ),
  ML_QUEUE_RECEIVE_WAITABLE_INT32 = ML_PARAM_NAME( ML_CLASS_DEVICE,
						   ML_TYPE_INT32, 0x7 )
};


enum mlDeviceStateEnum
{
  ML_DEVICE_STATE_READY,
  ML_DEVICE_STATE_TRANSFERRING,
  ML_DEVICE_STATE_WAITING,
  ML_DEVICE_STATE_ABORTING,
  ML_DEVICE_STATE_FINISHING
};


enum MLpvWaitForClassEnum
{
  ML_WAIT_FOR_UST_INT64 = ML_PARAM_NAME( ML_CLASS_WAIT_FOR,
					 ML_TYPE_INT64, 0x1 ),
  ML_WAIT_FOR_GPI_INT64 = ML_PARAM_NAME( ML_CLASS_WAIT_FOR,
					 ML_TYPE_INT64, 0x2 )
};


enum MLpvConditionalClassEnum
{
  ML_IF_VIDEO_UST_LT_INT64 = ML_PARAM_NAME( ML_CLASS_CONDITIONAL,
					    ML_TYPE_INT64, 0x1 ),
  ML_IF_VIDEO_UST_GT_INT64 = ML_PARAM_NAME( ML_CLASS_CONDITIONAL,
					    ML_TYPE_INT64, 0x2 ),
  ML_IF_AUDIO_UST_LT_INT64 = ML_PARAM_NAME( ML_CLASS_CONDITIONAL,
					    ML_TYPE_INT64, 0x3 ),
  ML_IF_AUDIO_UST_GT_INT64 = ML_PARAM_NAME( ML_CLASS_CONDITIONAL,
					    ML_TYPE_INT64, 0x4 )
};


enum MLpvInternalClassEnum
{
  ML_DI_COOKIE_BYTE_ARRAY = ML_PARAM_NAME( ML_CLASS_INTERNAL,
					   ML_TYPE_BYTE_ARRAY, 0x1 ),
  ML_DD_COOKIE_BYTE_ARRAY = ML_PARAM_NAME( ML_CLASS_INTERNAL,
					   ML_TYPE_BYTE_ARRAY, 0x2 ),
  ML_DD_DEVICE_INIT       = ML_PARAM_NAME( ML_CLASS_INTERNAL,
					   ML_TYPE_INT32,      0x3 ),
  ML_DD_SEND_ENGINE_TASK  = ML_PARAM_NAME( ML_CLASS_INTERNAL,
					   ML_TYPE_INT32,      0x4 )
};


/* Utilities for swapping bytes:
 */
#define	ML_BYTE_SWAP_INT16(a)	( (((a) << 8) & 0xff00) |\
				  (((a) >> 8) & 0x00ff) )

#define	ML_BYTE_SWAP_INT32(a)	( (((a) << 24) & 0xff000000) |\
				  (((a) <<  8) & 0x00ff0000) |\
				  (((a) >>  8) & 0x0000ff00) |\
				  (((a) >> 24) & 0x000000ff)  )
#ifdef __cplusplus 
}
#endif

#endif /* _ML_PARAM_H_ */
