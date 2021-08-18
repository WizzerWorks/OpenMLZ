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

// If this macro is not defined on Metrowerks, Windows.h includes imm.h
// which defines LPUINT, which is then re-defined in mmsystem.h
#define NOIME

#include <ML/ml_private.h>
#include <ML/ml_didd.h>
#include <ML/ml_oswrap.h>

#include "winaudio.h"
#include "audioParams.h"
#include "generic.h"

//-----------------------------------------------------------------------------
//
// Device params
// Hard-coded list of capabilities we support for paths, etc.
//
//-----------------------------------------------------------------------------

MLint32 stateEnums[] =
  {
    ML_DEVICE_STATE_TRANSFERRING,
    ML_DEVICE_STATE_WAITING,      // FIXME: Does not seem to be used
    ML_DEVICE_STATE_ABORTING,
    ML_DEVICE_STATE_FINISHING,
    ML_DEVICE_STATE_READY
  };

char stateNames[] =
"ML_DEVICE_STATE_TRANSFERRING\0"
"ML_DEVICE_STATE_WAITING\0"
"ML_DEVICE_STATE_ABORTING\0"
"ML_DEVICE_STATE_FINISHING\0"
"ML_DEVICE_STATE_READY\0";

MLDDint32paramDetails deviceState[] =
  {{
    ML_DEVICE_STATE_INT32,
    "ML_DEVICE_STATE_INT32",
    ML_TYPE_INT32,
    ML_ACCESS_RWI | ML_ACCESS_DURING_TRANSFER,
    NULL,
    NULL, 0,
    NULL, 0,
    stateEnums, sizeof (stateEnums) / sizeof (MLint32),
    stateNames, sizeof (stateNames)
  }};

MLDDint32paramDetails sendCount[] =
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

MLDDint32paramDetails receiveCount[] =
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

MLDDint32paramDetails sendWaitable[] =
  {{
    ML_QUEUE_SEND_WAITABLE_INT64,
    "ML_QUEUE_SEND_WAITABLE_INT64",
    ML_TYPE_INT32,
    ML_ACCESS_RI,
    NULL,
    NULL, 0,
    NULL, 0,
    NULL, 0,
    NULL, 0
  }};

MLDDint32paramDetails receiveWaitable[] =
  {{
    ML_QUEUE_RECEIVE_WAITABLE_INT64,
    "ML_QUEUE_RECEIVE_WAITABLE_INT64",
    ML_TYPE_INT32,
    ML_ACCESS_RI,
    NULL,
    NULL, 0,
    NULL, 0,
    NULL, 0,
    NULL, 0
  }};

MLint32 compressionEnums[] =
  {
    ML_COMPRESSION_UNCOMPRESSED
  };

char compressionNames[] =
"ML_COMPRESSION_UNCOMPRESSED\0";

MLDDint32paramDetails audioCompression[] =
  {{
    ML_AUDIO_COMPRESSION_INT32,
    "ML_AUDIO_COMPRESSION_INT32",
    ML_TYPE_INT32,
    ML_ACCESS_RWI | ML_ACCESS_QUEUED,
    compressionEnums,
    NULL, 0,
    NULL, 0,
    compressionEnums, sizeof (compressionEnums) / sizeof (MLint32),
    compressionNames, sizeof (compressionNames)
  }};

MLDDint32paramDetails audioFrameSize[] =
  {{
    ML_AUDIO_FRAME_SIZE_INT32,
    "ML_AUDIO_FRAME_SIZE_INT32",
    ML_TYPE_INT32,
    ML_ACCESS_RI | ML_ACCESS_QUEUED | ML_ACCESS_DURING_TRANSFER,
    NULL,
    NULL, 0,
    NULL, 0,
    NULL, 0,
    NULL, 0,
  }};

MLint32 channelEnums[] =
  {
    ML_CHANNELS_MONO,
    ML_CHANNELS_STEREO
  };

char channelNames[] =
"ML_CHANNELS_MONO\0"
"ML_CHANNELS_STEREO\0";

MLDDint32paramDetails audioChannels[] =
  {{
    ML_AUDIO_CHANNELS_INT32,
    "ML_AUDIO_CHANNELS_INT32",
    ML_TYPE_INT32,
    ML_ACCESS_RWI | ML_ACCESS_QUEUED,
    channelEnums,
    NULL, 0,
    NULL, 0,
    channelEnums, sizeof (channelEnums) / sizeof (MLint32),
    channelNames, sizeof (channelNames)
  }};

MLDDint32paramDetails audioSampleRate[] =
  {{
    ML_AUDIO_SAMPLE_RATE_REAL64,
    "ML_AUDIO_SAMPLE_RATE_REAL64",
    ML_TYPE_REAL64,
    ML_ACCESS_RWI | ML_ACCESS_QUEUED,
    NULL,
    NULL, 0,
    NULL, 0,
    NULL, 0,
    NULL, 0,
  }};

MLint32 formatEnums[] =
  {
    ML_AUDIO_FORMAT_S16,
    ML_AUDIO_FORMAT_U8
  };

char formatNames[] =
"ML_FORMAT_S16\0"
"ML_FORMAT_U8\0";

MLDDint32paramDetails audioFormat[] =
  {{
    ML_AUDIO_FORMAT_INT32,
    "ML_AUDIO_FORMAT_INT32",
    ML_TYPE_INT32,
    ML_ACCESS_RWI | ML_ACCESS_QUEUED,
    formatEnums,
    NULL, 0,
    NULL, 0,
    formatEnums, sizeof (formatEnums) / sizeof (MLint32),
    formatNames, sizeof (formatNames)
  }};

MLDDint32paramDetails audioBuffer[] =
  {{
    ML_AUDIO_BUFFER_POINTER,
    "ML_AUDIO_BUFFER_POINTER",
    ML_TYPE_BYTE_POINTER,
    ML_ACCESS_RWBT,
    NULL,
    NULL, 0,
    NULL, 0,
    NULL, 0,
    NULL, 0
  }};

MLDDint32paramDetails audioUST[] =
  {{
    ML_AUDIO_UST_INT64,
    "ML_AUDIO_UST_INT64",
    ML_TYPE_INT64,
    ML_ACCESS_RBT,
    NULL,
    NULL, 0,
    NULL, 0,
    NULL, 0,
    NULL, 0
  }};

MLDDint32paramDetails audioMSC[] =
  {{
    ML_AUDIO_MSC_INT64,
    "ML_AUDIO_MSC_INT64",
    ML_TYPE_INT64,
    ML_ACCESS_RBT,
    NULL,
    NULL, 0,
    NULL, 0,
    NULL, 0,
    NULL, 0
  }};

MLDDint32paramDetails waitForAudioUST[] =
  {{
    ML_WAIT_FOR_AUDIO_UST_INT64,
    "ML_WAIT_FOR_AUDIO_UST_INT64",
    ML_TYPE_INT64,
    ML_ACCESS_RWBT,
    NULL,
    NULL, 0,
    NULL, 0,
    NULL, 0,
    NULL, 0
  }};

MLDDint32paramDetails waitForAudioMSC[] =
  {{
    ML_WAIT_FOR_AUDIO_MSC_INT64,
    "ML_WAIT_FOR_AUDIO_MSC_INT64",
    ML_TYPE_INT64,
    ML_ACCESS_RWBT,
    NULL,
    NULL, 0,
    NULL, 0,
    NULL, 0,
    NULL, 0
  }};


MLint32 eventEnums[] = 
  {
    ML_EVENT_AUDIO_SEQUENCE_LOST,
    ML_EVENT_AUDIO_SAMPLE_RATE_CHANGED
  };

// Note: AUDIO_SAMPLE_RATE_CHANGED: see comment for RATE_CHANGED_EVENT
// in winaudio.h
char eventNames[] =
"ML_EVENT_AUDIO_SEQUENCE_LOST\0"
"ML_EVENT_AUDIO_SAMPLE_RATE_CHANGED\0";

MLDDint32paramDetails deviceEvent[] =
  {{
    ML_DEVICE_EVENTS_INT32_ARRAY,
    "ML_DEVICE_EVENTS_INT32_ARRAY",
    ML_TYPE_INT32_ARRAY,
    ML_ACCESS_RWI | ML_ACCESS_QUEUED | ML_ACCESS_DURING_TRANSFER,
    NULL,
    NULL, 0,
    NULL, 0,
    eventEnums, sizeof (eventEnums) / sizeof (MLint32),
    eventNames, sizeof (eventNames)
  }};

MLDDint32paramDetails audioGains[] =
  {{
    ML_AUDIO_GAINS_REAL64_ARRAY,
    "ML_AUDIO_GAINS_REAL64_ARRAY",
    ML_TYPE_REAL64_ARRAY,
    ML_ACCESS_RWI | ML_ACCESS_QUEUED | ML_ACCESS_DURING_TRANSFER,
    NULL,
    NULL, 0,
    NULL, 0,
    NULL, 0,
    NULL, 0,
  }};

MLDDint32paramDetails* pathParamDetails[] =
  {
    deviceState,
    sendCount,
    receiveCount,
    sendWaitable,
    receiveWaitable,
    audioCompression,
    audioFrameSize,
    audioChannels,
    audioSampleRate,
    audioFormat,
    audioBuffer,
    audioUST,
    audioMSC,
    waitForAudioUST,
    waitForAudioMSC,
    deviceEvent
    // audioGains is conditionally added in constructParamDetails
  };


//-----------------------------------------------------------------------------
//
// Determine if the specified path of the device supports volume
// controls.
enum WaveVolumeControl getVolumeFromDevDetails( WaveDeviceDetails* devDetails,
						int pathIndex )
{
  enum WaveVolumeControl ret = WAVE_VOLUME_NO_CONTROL;

  // Input paths have no volume control -- only output paths can have
  // them.
  if ( devDetails->jackDir[ pathIndex ] != ML_DIRECTION_IN ) {
    if ( (devDetails->outSupport & WAVECAPS_LRVOLUME) != 0 ) {
      ret = WAVE_VOLUME_L_R_CONTROL;
    } else if ( (devDetails->outSupport & WAVECAPS_VOLUME) != 0 ) {
      ret = WAVE_VOLUME_SINGLE_CONTROL;
    }
  }
  return ret;
}

//-----------------------------------------------------------------------------
//
// Construct a list of parameters applicable to the specified
// path. This starts with the base (common) parameters (applicable to
// all WAVE paths), and adds params that are specific to the current
// path.
//
// The list is constructed in a static memory area, which should not
// be deleted, and which is invalidated by the next call (and thus
// this function is NOT thread-safe).
//
// The size of the returned array is written to the paramSize argument.
MLDDint32paramDetails** constructParamDetails( WaveDeviceDetails* devDetails,
					       int pathIndex,
					       int* paramSize )
{
  // Currently, there is only 1 optional parameter: Audio Gains. So
  // our internal array can be sized 1 larger than the base array.
  static MLDDint32paramDetails* localArray[ (sizeof( pathParamDetails ) /
					     sizeof( void* )) + 1 ];
  int i;

  assert( paramSize != 0 );

  // Copy base params over
  for ( i=0; i < (sizeof( pathParamDetails ) / sizeof( void* )); ++i ) {
    localArray[i] = pathParamDetails[i];
  }

  // Now determine if we should add the AUDIO_GAINS control
  if ( getVolumeFromDevDetails( devDetails, pathIndex ) !=
       WAVE_VOLUME_NO_CONTROL ) {
    // Yes -- add that control to the local array, and increment size
    localArray[i] = audioGains;
    ++i;
  }

  // FIXME: would also like to customise the AUDIO_SAMPLE_RATE
  // control, to indicate the min and max rates supported by this
  // device (we know this by looking at the jackFormats field of the
  // device details). The problem is that the returned structure only
  // support MLin32 min and max arrays, and SAMPLE_RATE is an
  // MLreal64.

  *paramSize = i;
  return localArray;
}

//-----------------------------------------------------------------------------
//
// Check that the specified value is present in the value array.
// Called by validateParamValue (see below)
int validateValueInArray( MLint32 value, MLint32* array, int size )
{
  int valid = 0;
  int i;

  for ( i=0; i < size; ++i ) {
    if ( array[i] == value ) {
      valid = 1;
      break;
    }
  }

  return valid;
}

//-----------------------------------------------------------------------------
//
// Validate that the param/value pair in the given message is valid,
// using the global enum arrays defined at the top of this file.  If
// the parameter does not have a list of valid values, then this
// simply return 1, ie: OK.
int validateParamValue( MLpv* msg )
{
  int valid = 0; // Assume NOT OK
  MLint32* validArray = 0;
  int validArraySize = 0;
  int valueArray = 0; // Assume this is not an INT32_ARRAY value

  switch ( msg->param ) {

  case ML_AUDIO_COMPRESSION_INT32:
    validArray = compressionEnums;
    validArraySize = sizeof( compressionEnums ) / sizeof( MLint32 );
    break;

  case ML_AUDIO_CHANNELS_INT32:
    validArray = channelEnums;
    validArraySize = sizeof( channelEnums ) / sizeof( MLint32 );
    break;

  case ML_AUDIO_FORMAT_INT32:
    validArray = formatEnums;
    validArraySize = sizeof( formatEnums ) / sizeof( MLint32 );
    break;

  case ML_DEVICE_EVENTS_INT32_ARRAY:
    validArray = eventEnums;
    validArraySize = sizeof( eventEnums ) / sizeof( MLint32 );
    valueArray = 1; // this is in fact an INT32_ARRAY
    break;

  default:
    // No array defined for this, assume value is valid
    valid = 1;
    break;

  } // switch

  // If we found an array, go through it looking for the value
  if ( validArray != 0 ) {
    if ( valueArray == 0 ) {
      // Single INT32 value, easy...
      valid = validateValueInArray( msg->value.int32,
				    validArray, validArraySize );
      if ( valid != 1 ) {
	DEBG1( printf( "\tsetControls: invalid value %d for param %"
		       FORMAT_LLX "\n", msg->value.int32, msg->param ) );
      }
    } else {
      // INT32_ARRAY -- loop through every entry in the array
      int i;
      for ( i=0; i < msg->length; ++i ) {
	valid = validateValueInArray( msg->value.pInt32[i],
				      validArray, validArraySize );
	if ( valid != 1 ) {
	  DEBG1( printf( "\tsetControls: invalid value %d for param %"
			 FORMAT_LLX "\n", msg->value.pInt32[i], msg->param ) );
	  break;
      }
      } // for i=0...msg->length
    }
  } // if validArray != 0

  return valid;
}

