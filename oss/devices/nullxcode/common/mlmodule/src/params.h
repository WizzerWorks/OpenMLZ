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


/* This header contains a description of the controls that we support, as
 * well as the various data structures that we use to keep track of them.
 */

static MLint32 zero[] = { 0 };

/* ---------------------- Open Options on the engine ------------------------*/

#define	lengthof(a)	sizeof(a)/sizeof(a[0])

static MLint32 openModeEnums[] = {
    ML_MODE_RO,
    ML_MODE_RWS,
    ML_MODE_RWE
};

static char openModeNames[] = 
    "ML_MODE_RO\0"
    "ML_MODE_RWS\0"
    "ML_MODE_RWE\0"
;

static MLDDparamDetails openMode[] = {
    { ML_OPEN_MODE_INT32,     
      "ML_OPEN_MODE_INT32", 
      ML_TYPE_INT32, 
      ML_ACCESS_OPEN_OPTION,
      0,
      0, 0,
      0, 0,
      openModeEnums, lengthof(openModeEnums),
      openModeNames, sizeof(openModeNames)
    }
};

/* default: 64 send msgs, 100 event msgs, 256 recv msgs */

static MLint32 send_queue_default = 64;
static MLint32 send_queue_min = 2;
static MLint32 send_queue_max = 1024;
static MLDDparamDetails openSendQueueCount[] = {
    { ML_OPEN_SEND_QUEUE_COUNT_INT32,
      "ML_OPEN_SEND_QUEUE_COUNT_INT32",
      ML_TYPE_INT32,
      ML_ACCESS_OPEN_OPTION,
      &send_queue_default,
      &send_queue_min, 1,
      &send_queue_max, 1,
      0, 0,
      0, 0
    }
};

static MLint32 recv_queue_default = 256;
static MLint32 recv_queue_min = 4;
static MLint32 recv_queue_max = 1024;
static MLDDparamDetails openRecvQueueCount[] = {
    { ML_OPEN_RECEIVE_QUEUE_COUNT_INT32,
      "ML_OPEN_RECEIVE_QUEUE_COUNT_INT32",
      ML_TYPE_INT32,
      ML_ACCESS_OPEN_OPTION,
      &recv_queue_default,
      &recv_queue_min, 1,
      &recv_queue_max, 1,
      0, 0,
      0, 0
    }
};

static MLint32 event_queue_default = 100;
static MLint32 event_queue_min = 2;
static MLint32 event_queue_max = 1024;
static MLDDparamDetails openEventPayloadCount[] = {
    { ML_OPEN_EVENT_PAYLOAD_COUNT_INT32,
      "ML_OPEN_EVENT_PAYLOAD_COUNT_INT32",
      ML_TYPE_INT32,
      ML_ACCESS_OPEN_OPTION,
      &event_queue_default,
      &event_queue_min, 1,
      &event_queue_max, 1,
      0, 0,
      0, 0
    }
};

static MLint32 msg_payload_default = 8192;
static MLint32 msg_payload_min = 1024;
static MLint32 msg_payload_max = 128 * 1024 * 1024;
static MLDDparamDetails openMsgPayloadSize[] = {
    { ML_OPEN_MESSAGE_PAYLOAD_SIZE_INT32,
      "ML_OPEN_MESSAGE_PAYLOAD_SIZE_INT32",
      ML_TYPE_INT32,
      ML_ACCESS_OPEN_OPTION,
      &msg_payload_default,
      &msg_payload_min, 1,
      &msg_payload_max, 1,
      0, 0,
      0, 0
    }
};

/* default signal count is 10 less than default send queue size */
static MLint32 send_signal_count = 54;
static MLint32 send_signal_min = 1;
static MLint32 send_signal_max = 1024;
static MLDDparamDetails openSendSignalCount[] = {
    { ML_OPEN_SEND_SIGNAL_COUNT_INT32,
      "ML_OPEN_SEND_SIGNAL_COUNT_INT32",
      ML_TYPE_INT32,
      ML_ACCESS_OPEN_OPTION,
      &send_signal_count,
      &send_signal_min, 1,
      &send_signal_max, 1,
      0, 0,
      0, 0
    }
};

static MLint32 openXcodeModeEnums[] = {
    ML_XCODE_MODE_SYNCHRONOUS,
    ML_XCODE_MODE_ASYNCHRONOUS
};

static char openXcodeModeNames[] = 
    "ML_XCODE_MODE_SYNCHRONOUS\0"
    "ML_XCODE_MODE_ASYNCHRONOUS\0"
;

static MLDDparamDetails openXcodeMode[] = {
    { ML_OPEN_XCODE_MODE_INT32,     
      "ML_OPEN_XCODE_MODE_INT32", 
      ML_TYPE_INT32, 
      ML_ACCESS_OPEN_OPTION,
      0,
      0, 0,
      0, 0,
      openXcodeModeEnums, lengthof(openXcodeModeEnums),
      openXcodeModeNames, sizeof(openXcodeModeNames)
    }
};

static MLint32 openStreamModeEnums[] = {
    ML_XCODE_STREAM_SINGLE,
    ML_XCODE_STREAM_MULTI
};

static char openStreamModeNames[] = 
    "ML_XCODE_STREAM_SINGLE\0"
    "ML_XCODE_STREAM_MULTI\0"
;

static MLDDparamDetails openStreamMode[] = {
    { ML_OPEN_XCODE_STREAM_INT32,     
      "ML_OPEN_XCODE_STREAM_INT32", 
      ML_TYPE_INT32, 
      ML_ACCESS_OPEN_OPTION,
      0,
      0, 0,
      0, 0,
      openStreamModeEnums, lengthof(openStreamModeEnums),
      openStreamModeNames, sizeof(openStreamModeNames)
    }
};

/* ------------------------- Controls on the engine -------------------------*/

/* These controls are read/write.
 */

/*------------- ML_SELECT_ID_INT64
 */
static MLDDparamDetails selectId[] = {
    { ML_SELECT_ID_INT64,
      "ML_SELECT_ID_INT64",
      ML_TYPE_INT64,
      ML_ACCESS_RWI | ML_ACCESS_RWBT,
      0,
      0, 0,
      0, 0,
      0, 0,
      0, 0
    }
};
     

/*------------- ML_DEVICE_EVENTS_INT32_ARRAY
 */
static MLint32 eventEnums[] = {
    ML_EVENT_XCODE_FAILED,
    ML_EVENT_DEVICE_UNAVAILABLE
};

static char eventNames[] = 
    "ML_EVENT_XCODE_FAILED\0"
    "ML_EVENT_DEVICE_UNAVAILABLE\0"
;

static MLDDparamDetails deviceEvent[] = {
    { ML_DEVICE_EVENTS_INT32_ARRAY,     
      "ML_DEVICE_EVENTS_INT32_ARRAY", 
      ML_TYPE_INT32_ARRAY, 
      ML_ACCESS_RWI,
      0,
      0, 0,
      0, 0,
      eventEnums, sizeof(eventEnums) / sizeof(MLint32),
      eventNames, sizeof(eventNames)
    }
};

/*------------- ML_DEVICE_STATE_INT32
 */
static MLint32 stateEnums[] = {
    ML_DEVICE_STATE_READY,
    ML_DEVICE_STATE_TRANSFERRING,
    ML_DEVICE_STATE_ABORTING,
    ML_DEVICE_STATE_FINISHING
};

static char stateNames[] = 
    "ML_DEVICE_STATE_READY\0"
    "ML_DEVICE_STATE_TRANSFERRING\0"
    "ML_DEVICE_STATE_ABORTING\0"
    "ML_DEVICE_STATE_FINISHING\0"
;

static MLDDparamDetails deviceState[] = {
    { ML_DEVICE_STATE_INT32,	
      "ML_DEVICE_STATE_INT32",     
      ML_TYPE_INT32,		
      ML_ACCESS_RWI | ML_ACCESS_RWQT,
      &stateEnums[0],
      0, 0,		
      0, 0,		
      stateEnums, sizeof(stateEnums)/sizeof(MLint32),
      stateNames, sizeof(stateNames) 
    }
};

/* These controls are read-only.  They have a computed value which
 * cannot be changed.
 */

/*------------- ML_QUEUE_SEND_COUNT_INT32
 */
static MLDDparamDetails sendCount[] = {
    { ML_QUEUE_SEND_COUNT_INT32,
      "ML_QUEUE_SEND_COUNT_INT32",
      ML_TYPE_INT32,
      ML_ACCESS_RI | ML_ACCESS_RQT,
      0,
      0, 0,
      0, 0,
      0, 0,
      0, 0
    }
};
     
/*------------- ML_QUEUE_RECEIVE_COUNT_INT32
 */
static MLDDparamDetails receiveCount[] = {
    { ML_QUEUE_RECEIVE_COUNT_INT32,
      "ML_QUEUE_RECEIVE_COUNT_INT32",
      ML_TYPE_INT32,
      ML_ACCESS_RI | ML_ACCESS_RQT,
      0,
      0, 0,
      0, 0,
      0, 0,
      0, 0
    }
};
     
/*------------- ML_QUEUE_SEND_WAITABLE_INT64
 */
static MLDDparamDetails sendWaitable[] = {
    { ML_QUEUE_SEND_WAITABLE_INT64,
      "ML_QUEUE_SEND_WAITABLE_INT64",
      ML_TYPE_INT64,
      ML_ACCESS_RI | ML_ACCESS_RQT,
      0,
      0, 0,
      0, 0,
      0, 0,
      0, 0
    }
};
     
/*------------- ML_QUEUE_RECEIVE_WAITABLE_INT64
 */
static MLDDparamDetails receiveWaitable[] = {
    { ML_QUEUE_RECEIVE_WAITABLE_INT64,
      "ML_QUEUE_RECEIVE_WAITABLE_INT64",
      ML_TYPE_INT64,
      ML_ACCESS_RI | ML_ACCESS_RQT,
      0,
      0, 0,
      0, 0,
      0, 0,
      0, 0
    }
};


/*---------------------- Controls on transcoder pipes -----------------------*/

/* These controls are read-only.  They have a single default value
 * which cannot be changed.
 */

/*------------- ML_IMAGE_TEMPORAL_SAMPLING_INT32
 */
static MLint32 temporalSamplingEnums[] = {
    ML_TEMPORAL_SAMPLING_PROGRESSIVE
};

static char temporalSamplingNames[] = 
    "ML_TEMPORAL_SAMPLING_PROGRESSIVE\0"
;

static MLDDparamDetails temporalSampling[] = {
    { ML_IMAGE_TEMPORAL_SAMPLING_INT32,
      "ML_IMAGE_TEMPORAL_SAMPLING_INT32",
      ML_TYPE_INT32,
      ML_ACCESS_RI | ML_ACCESS_RQT,
      &temporalSamplingEnums[0],
      0, 0,
      0, 0,
      temporalSamplingEnums, sizeof(temporalSamplingEnums) / sizeof(MLint32),
      temporalSamplingNames, sizeof(temporalSamplingNames)
    }
};
      
/*------------- ML_IMAGE_VIDEO_UST_INT64
 */
MLDDparamDetails videoUST[] = {
    { ML_VIDEO_UST_INT64,     
      "ML_VIDEO_UST_INT64", 
      ML_TYPE_INT64, 
      ML_ACCESS_RWBT | ML_ACCESS_BUFFER_PARAM,
      0,
      0, 0,
      0, 0,
      0, 0,
      0, 0
    }
};

/*------------- ML_IMAGE_VIDEO_MSC_INT64
 */
MLDDparamDetails videoMSC[] = {
    { ML_VIDEO_MSC_INT64,     
      "ML_VIDEO_MSC_INT64", 
      ML_TYPE_INT64,
      ML_ACCESS_RWBT | ML_ACCESS_BUFFER_PARAM,
      0,
      0, 0,
      0, 0,
      0, 0,
      0, 0
    }
};


/*------------- ML_IMAGE_HEIGHT_2_INT32
 */
static MLDDparamDetails imageHeight2[] = {
    { ML_IMAGE_HEIGHT_2_INT32,
      "ML_IMAGE_HEIGHT_2_INT32",
      ML_TYPE_INT32,
      ML_ACCESS_RI | ML_ACCESS_RQT,
      zero,
      zero, 1,
      zero, 1,
      0, 0,
      0, 0,
    }
};

         
/*------------- ML_IMAGE_ROW_BYTES_INT32
 */
static MLDDparamDetails imageRowBytes[] = {
    { ML_IMAGE_ROW_BYTES_INT32,
      "ML_IMAGE_ROW_BYTES_INT32",
      ML_TYPE_INT32,
      ML_ACCESS_RI | ML_ACCESS_RQT,
      zero,
      zero, 1,  
      zero, 1,  
      0, 0,
      0, 0
    }
};


/*------------- ML_IMAGE_SKIP_PIXELS_INT32
 */
static MLDDparamDetails imageSkipPixels[] = {
    { ML_IMAGE_SKIP_PIXELS_INT32,
      "ML_IMAGE_SKIP_PIXELS_INT32",
      ML_TYPE_INT32,
      ML_ACCESS_RI | ML_ACCESS_RQT,
      zero,
      zero, 1,  
      zero, 1,  
      0, 0,
      0, 0
    }
};


/*------------- ML_IMAGE_SKIP_ROWS_INT32
 */
static MLDDparamDetails imageSkipRows[] = {
    { ML_IMAGE_SKIP_ROWS_INT32,
      "ML_IMAGE_SKIP_ROWS_INT32",
      ML_TYPE_INT32,
      ML_ACCESS_RI | ML_ACCESS_RQT,
      zero,
      zero, 1,  
      zero, 1,  
      0, 0,
      0, 0
    }
};


/*------------- ML_IMAGE_INTERLEAVE_MODE_INT32
 */
static MLint32 interleaveEnums[] = {
    ML_INTERLEAVE_MODE_SINGLE_FIELD
};

static char interleaveNames[] = {
    "ML_INTERLEAVE_MODE_SINGLE_FIELD\0"
};

static MLDDparamDetails interleave[] = {
    { ML_IMAGE_INTERLEAVE_MODE_INT32,     
      "ML_IMAGE_INTERLEAVE_MODE_INT32", 
      ML_TYPE_INT32, 
      ML_ACCESS_RI | ML_ACCESS_RQT,
      &interleaveEnums[0],
      0, 0,
      0, 0,
      interleaveEnums, sizeof(interleaveEnums)/sizeof(MLint32),
      interleaveNames, sizeof(interleaveNames)
    }
};


/* These controls are read-only.  They have a computed value which
 * cannot be changed.
 */

/*------------- ML_IMAGE_BUFFER_SIZE_INT32
 */
static MLDDparamDetails imageBufferSize[] = {
    { ML_IMAGE_BUFFER_SIZE_INT32,
      "ML_IMAGE_BUFFER_SIZE_INT32",
      ML_TYPE_INT32,
      ML_ACCESS_RI | ML_ACCESS_RQT,
      zero,
      0, 0,
      0, 0,
      0, 0,
      0, 0,
    }
};

         
/* These controls are read/write.
 */

/*------------- ML_IMAGE_COMPRESSION_INT32
 */
static MLint32 imageCompressionEnums[] = {
    ML_COMPRESSION_UNCOMPRESSED,
    ML_COMPRESSION_BASELINE_JPEG,
    ML_COMPRESSION_LOSSLESS_JPEG,
    ML_COMPRESSION_DV_625,
    ML_COMPRESSION_DV_525,
    ML_COMPRESSION_DVCPRO_625,
    ML_COMPRESSION_DVCPRO_525,
    ML_COMPRESSION_DVCPRO50_625,
    ML_COMPRESSION_DVCPRO50_525,
    ML_COMPRESSION_MPEG2I
};

static char imageCompressionNames[] = 
    "ML_COMPRESSION_UNCOMPRESSED\0"
    "ML_COMPRESSION_BASELINE_JPEG\0"
    "ML_COMPRESSION_LOSSLESS_JPEG\0"
    "ML_COMPRESSION_DV_625\0"
    "ML_COMPRESSION_DV_525\0"
    "ML_COMPRESSION_DVCPRO_625\0"
    "ML_COMPRESSION_DVCPRO_525\0"
    "ML_COMPRESSION_DVCPRO50_625\0"
    "ML_COMPRESSION_DVCPRO50_525\0"
    "ML_COMPRESSION_MPEG2\0"
;

static MLDDparamDetails imageCompression[] = {
    { ML_IMAGE_COMPRESSION_INT32,
      "ML_IMAGE_COMPRESSION_INT32",
      ML_TYPE_INT32,
      ML_ACCESS_RWI | ML_ACCESS_RWQT,
      &imageCompressionEnums[0],
      0, 0,
      0, 0,
      imageCompressionEnums, sizeof(imageCompressionEnums) / sizeof(MLint32),
      imageCompressionNames, sizeof(imageCompressionNames)
    }
};


/*------------- ML_IMAGE_COLORSPACE_INT32
 */
static MLint32 imageColorspaceEnums[] = {
    ML_COLORSPACE_RGB_601_FULL,
    ML_COLORSPACE_RGB_601_HEAD,
    ML_COLORSPACE_CbYCr_601_FULL,
    ML_COLORSPACE_CbYCr_601_HEAD,
    ML_COLORSPACE_RGB_240M_FULL,
    ML_COLORSPACE_RGB_240M_HEAD,
    ML_COLORSPACE_CbYCr_240M_FULL,
    ML_COLORSPACE_CbYCr_240M_HEAD,
    ML_COLORSPACE_RGB_709_FULL,
    ML_COLORSPACE_RGB_709_HEAD,
    ML_COLORSPACE_CbYCr_709_FULL,
    ML_COLORSPACE_CbYCr_709_HEAD
};

static char imageColorspaceNames[] = 
    "ML_COLORSPACE_RGB_601_FULL\0"
    "ML_COLORSPACE_RGB_601_HEAD\0"
    "ML_COLORSPACE_CbYCr_601_FULL\0"
    "ML_COLORSPACE_CbYCr_601_HEAD\0"
    "ML_COLORSPACE_RGB_240M_FULL\0"
    "ML_COLORSPACE_RGB_240M_HEAD\0"
    "ML_COLORSPACE_CbYCr_240M_FULL\0"
    "ML_COLORSPACE_CbYCr_240M_HEAD\0"
    "ML_COLORSPACE_RGB_709_FULL\0"
    "ML_COLORSPACE_RGB_709_HEAD\0"
    "ML_COLORSPACE_CbYCr_709_FULL\0"
    "ML_COLORSPACE_CbYCr_709_HEAD\0"
;

static MLDDparamDetails imageColorspace[] = {
    { ML_IMAGE_COLORSPACE_INT32,
      "ML_IMAGE_COLORSPACE_INT32",
      ML_TYPE_INT32,
      ML_ACCESS_RWI | ML_ACCESS_RWQT,
      &imageColorspaceEnums[0],
      0, 0,
      0, 0,
      imageColorspaceEnums, sizeof(imageColorspaceEnums) / sizeof(MLint32),
      imageColorspaceNames, sizeof(imageColorspaceNames)
    }
};


/*------------- ML_IMAGE_PACKING_INT32
 */
static MLint32 imagePackingEnums[] = {
    ML_PACKING_8,
    ML_PACKING_8_R,
    ML_PACKING_8_4123,
    ML_PACKING_8_3214,
    ML_PACKING_10,
    ML_PACKING_10_R,
    ML_PACKING_S12,
    ML_PACKING_10in16L,
    ML_PACKING_10in16L_R,
    ML_PACKING_10in16R,
    ML_PACKING_10in16R_R,
    ML_PACKING_S12in16L,
    ML_PACKING_S12in16R,
    ML_PACKING_S13in16L,
    ML_PACKING_S13in16R,
    ML_PACKING_10_10_10_2,
    ML_PACKING_10_10_10_2_R
};

static char imagePackingNames[] = 
    "ML_PACKING_8\0"
    "ML_PACKING_8_R\0"
    "ML_PACKING_8_4123\0"
    "ML_PACKING_8_3214\0"
    "ML_PACKING_10\0"
    "ML_PACKING_10_R\0"
    "ML_PACKING_S12\0"
    "ML_PACKING_10in16L\0"
    "ML_PACKING_10in16L_R\0"
    "ML_PACKING_10in16R\0"
    "ML_PACKING_10in16R_R\0"
    "ML_PACKING_S12in16L\0"
    "ML_PACKING_S12in16R\0"
    "ML_PACKING_S13in16L\0"
    "ML_PACKING_S13in16R\0"
    "ML_PACKING_10_10_10_2\0"
    "ML_PACKING_10_10_10_2_R\0"
;

static MLDDparamDetails imagePacking[] = {
    { ML_IMAGE_PACKING_INT32,
      "ML_IMAGE_PACKING_INT32",
      ML_TYPE_INT32,
      ML_ACCESS_RWI | ML_ACCESS_RWQT,
      &imagePackingEnums[0],
      0, 0,
      0, 0,
      imagePackingEnums, sizeof(imagePackingEnums) / sizeof(MLint32),
      imagePackingNames, sizeof(imagePackingNames)
    }
};
     

/*------------- ML_IMAGE_SAMPLING_INT32
 */
static MLint32 imageSamplingEnums[] = {
    ML_SAMPLING_4444,
    ML_SAMPLING_4224,
    ML_SAMPLING_444,
    ML_SAMPLING_422,
    ML_SAMPLING_420_MPEG1,
    ML_SAMPLING_420_MPEG2,
    ML_SAMPLING_420_DVC625,
    ML_SAMPLING_411_DVC,
    ML_SAMPLING_4004,
    ML_SAMPLING_400
};

static char imageSamplingNames[] = 
    "ML_SAMPLING_4444\0"
    "ML_SAMPLING_4224\0"
    "ML_SAMPLING_444\0"
    "ML_SAMPLING_422\0"
    "ML_SAMPLING_420_MPEG1\0"
    "ML_SAMPLING_420_MPEG2\0"
    "ML_SAMPLING_420_DVC625\0"
    "ML_SAMPLING_411_DVC\0"
    "ML_SAMPLING_4004\0"
    "ML_SAMPLING_400\0"
;

static MLDDparamDetails imageSampling[] = {
    { ML_IMAGE_SAMPLING_INT32,
      "ML_IMAGE_SAMPLING_INT32",
      ML_TYPE_INT32,
      ML_ACCESS_RWI | ML_ACCESS_RWQT,
      &imageSamplingEnums[0],
      0, 0,
      0, 0,
      imageSamplingEnums, sizeof(imageSamplingEnums) / sizeof(MLint32),
      imageSamplingNames, sizeof(imageSamplingNames)
    }
};
      

/*------------- ML_IMAGE_WIDTH_INT32
 */
static MLint32 imageWidthVals[] = {
    720
};

static MLDDparamDetails imageWidth[] = {
    { ML_IMAGE_WIDTH_INT32,
      "ML_IMAGE_WIDTH_INT32",
      ML_TYPE_INT32,
      ML_ACCESS_RWI | ML_ACCESS_RWQT,
      &imageWidthVals[0],
      0, 0,
      0, 0,
      0, 0,
      0, 0,
    }
};
         

/*------------- ML_IMAGE_HEIGHT_1_INT32
 */
static MLint32 imageHeight1Vals[] = {
    486
};

static MLDDparamDetails imageHeight1[] = {
    { ML_IMAGE_HEIGHT_1_INT32,
      "ML_IMAGE_HEIGHT_1_INT32",
      ML_TYPE_INT32,
      ML_ACCESS_RWI | ML_ACCESS_RWQT,
      &imageHeight1Vals[0],
      0, 0,
      0, 0,
      0, 0,
      0, 0,
    }
};

         
/*------------- ML_IMAGE_ORIENTATION_INT32
 */
static MLint32 imageOrientationEnums[] = {
    ML_ORIENTATION_TOP_TO_BOTTOM,
    ML_ORIENTATION_BOTTOM_TO_TOP
};

static char imageOrientationNames[] = {
    "ML_ORIENTATION_TOP_TO_BOTTOM\0"
    "ML_ORIENTATION_BOTTOM_TO_TOP\0"
};

static MLDDparamDetails imageOrientation[] = {
    { ML_IMAGE_ORIENTATION_INT32,     
      "ML_IMAGE_ORIENTATION_INT32", 
      ML_TYPE_INT32, 
      ML_ACCESS_RWI | ML_ACCESS_RWQT,
      &imageOrientationEnums[0],
      0, 0,
      0, 0,
      imageOrientationEnums, sizeof(imageOrientationEnums)/sizeof(MLint32),
      imageOrientationNames, sizeof(imageOrientationNames)
    }
};


/*------------- ML_IMAGE_BUFFER_POINTER
 */
static MLDDparamDetails imageBuffer[] = {
    { ML_IMAGE_BUFFER_POINTER,     
      "ML_IMAGE_BUFFER_POINTER", 
      ML_TYPE_BYTE_POINTER, 
      ML_ACCESS_RWBT,
      0,
      0, 0,
      0, 0,
      0, 0,
      0, 0,
    }
};


/* This example shows two complete engines, one which copies memory 
 * and one which clears memory.  Each engine has source and destination
 * params.  In our trivial example case, the capabilities are identical 
 * across the engines.  In a real transcoder, the engines might correspond
 * to encode and decode, and the engine/pipe params might differ.
 */

/* Engine and pipe capabilities.  Allocated and initialized
 * by ddConnect(), returned by ddGetCapabilities().
 */
static MLpv* devCaps = 0;
static MLpv* copyEngineCaps = 0;
static MLpv* clearEngineCaps = 0;
static MLpv* copySrcCaps = 0;
static MLpv* clearSrcCaps = 0;
static MLpv* copyDstCaps = 0;
static MLpv* clearDstCaps = 0;

/* The params supported by the engine.  Put into engineCaps 
 * by ddConnect(), returned as ML_PARAM_IDS_INT64_ARRAY by ddGetCapabilities()
 */
static MLint64 engineOptionsList[] = {
    ML_OPEN_MODE_INT32,
    ML_OPEN_SEND_QUEUE_COUNT_INT32,
    ML_OPEN_RECEIVE_QUEUE_COUNT_INT32,
    ML_OPEN_MESSAGE_PAYLOAD_SIZE_INT32,
    ML_OPEN_EVENT_PAYLOAD_COUNT_INT32,
    ML_OPEN_SEND_SIGNAL_COUNT_INT32,
    ML_OPEN_XCODE_MODE_INT32,
    ML_OPEN_XCODE_STREAM_INT32        
};

static MLint64 engineParamsList[] = {
    ML_SELECT_ID_INT64,
    ML_DEVICE_EVENTS_INT32_ARRAY,
    ML_DEVICE_STATE_INT32,
    ML_QUEUE_SEND_COUNT_INT32,
    ML_QUEUE_RECEIVE_COUNT_INT32,
    ML_QUEUE_SEND_WAITABLE_INT64,
    ML_QUEUE_RECEIVE_WAITABLE_INT64
};

/* The params supported by the pipes.  Put into srcCaps and dstCaps
 * by ddConnect(), returned as ML_PARAM_IDS_INT64_ARRAY by ddGetCapabilities()
 */
static MLint64 pipeParamsList[] = {
    ML_SELECT_ID_INT64,
    ML_IMAGE_TEMPORAL_SAMPLING_INT32,
    ML_VIDEO_UST_INT64,
    ML_VIDEO_MSC_INT64,
    ML_IMAGE_ROW_BYTES_INT32,
    ML_IMAGE_SKIP_PIXELS_INT32,
    ML_IMAGE_SKIP_ROWS_INT32,
    ML_IMAGE_INTERLEAVE_MODE_INT32,
    ML_IMAGE_BUFFER_SIZE_INT32,
    ML_IMAGE_COMPRESSION_INT32,
    ML_IMAGE_COLORSPACE_INT32,
    ML_IMAGE_PACKING_INT32,
    ML_IMAGE_SAMPLING_INT32,
    ML_IMAGE_WIDTH_INT32,
    ML_IMAGE_HEIGHT_1_INT32,
    ML_IMAGE_HEIGHT_2_INT32,
    ML_IMAGE_ORIENTATION_INT32,
    ML_IMAGE_BUFFER_POINTER
};

/* Capabilities for individual parameters.  These arrays are needed by both
 * ddPvGetCapabilities() and processControls().  For example, a call
 * to ddPvGetCapabilites(engineDeviceId, ...) will search thru
 * engineParams to find the correct parameter capability.  
 * Similarly, processControls() will lookup a parameter's capabilities
 * and verify the requested message against them.
 */

static MLDDparamDetails* copyEngineOptions[] = {
    openMode,
    openSendQueueCount,
    openRecvQueueCount,
    openMsgPayloadSize,
    openEventPayloadCount,
    openSendSignalCount,
    openXcodeMode,
    openStreamMode,
    0
};
static MLDDparamDetails* copyEngineParams[] = {
    selectId,
    deviceEvent,
    deviceState,
    sendCount,
    receiveCount,
    sendWaitable,
    receiveWaitable,
    0
};
static MLDDparamDetails* copySrcParams[] = {
    selectId,
    imageCompression,
    imageColorspace,
    imagePacking,
    imageSampling,
    temporalSampling,
    imageWidth,
    imageHeight1,
    imageHeight2,
    imageRowBytes,
    imageSkipPixels,
    imageSkipRows,
    imageOrientation,
    interleave,
    imageBufferSize,
    imageBuffer,
    videoUST,
    videoMSC,
    0
};
static MLDDparamDetails* copyDstParams[] = {
    selectId,
    imageCompression,
    imageColorspace,
    imagePacking,
    imageSampling,
    temporalSampling,
    imageWidth,
    imageHeight1,
    imageHeight2,
    imageRowBytes,
    imageSkipPixels,
    imageSkipRows,
    imageOrientation,
    interleave,
    imageBufferSize,
    imageBuffer,
    videoUST,
    videoMSC,
    0
};

static MLDDparamDetails* clearEngineOptions[] = {
    openMode,
    openSendQueueCount,
    openRecvQueueCount,
    openMsgPayloadSize,
    openEventPayloadCount,
    openSendSignalCount,
    openXcodeMode,
    openStreamMode,
    0
};

static MLDDparamDetails* clearEngineParams[] = {
    selectId,
    deviceEvent,
    deviceState,
    sendCount,
    receiveCount,
    sendWaitable,
    receiveWaitable,
    0
};
static MLDDparamDetails* clearSrcParams[] = {
    selectId,
    imageCompression,
    imageColorspace,
    imagePacking,
    imageSampling,
    temporalSampling,
    imageWidth,
    imageHeight1,
    imageHeight2,
    imageRowBytes,
    imageSkipPixels,
    imageSkipRows,
    imageOrientation,
    interleave,
    imageBufferSize,
    imageBuffer,
    videoUST,
    videoMSC,
    0
};
static MLDDparamDetails* clearDstParams[] = {
    selectId,
    imageCompression,
    imageColorspace,
    imagePacking,
    imageSampling,
    temporalSampling,
    imageWidth,
    imageHeight1,
    imageHeight2,
    imageRowBytes,
    imageSkipPixels,
    imageSkipRows,
    imageOrientation,
    interleave,
    imageBufferSize,
    imageBuffer,
    videoUST,
    videoMSC,
    0
};


