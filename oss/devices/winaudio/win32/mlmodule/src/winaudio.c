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
#include "audioLoop.h"
#include "generic.h"
#include "devOps.h"

#include <math.h>

// Generic name of all WAVE devices, and version number of this device
// module (note version is 0x100 -- this translates to "1.0")
char* genericDevName = "WAVE audio device";
char* genericInName  = "input";
char* genericOutName = "output";
const int deviceVersion = 0x100;

int debugLevel = 0;

//-----------------------------------------------------------------------------
//
// Add a WAV device (input or output) to the list of known devices
MLstatus addPhysicalDevice( MLsystemContext systemContext,
			    MLmoduleContext moduleContext,
			    UINT_PTR devId,
			    WAVEINCAPS* inCaps,
			    WAVEOUTCAPS* outCaps
			    )
{
  WaveDeviceDetails devDetails;
  MLstatus status;
  int numJacks = 0;

  // Ensure there is at least an input OR an output
  if ( (inCaps == 0) && (outCaps == 0) ) {
    return ML_STATUS_INTERNAL_ERROR;
  }

  // Create a unique string identifying this input device. This is
  // used as the "location" string, to uniquely identify the devices.
  sprintf( devDetails.name, "Wave_%d", (int)devId );
  devDetails.devId = devId;

  DEBG4( printf( "[winaudio]: Adding WAVE device '%s'\n", devDetails.name ) );

  if ( inCaps != 0 ) {
    devDetails.jackDir[ numJacks ] = ML_DIRECTION_IN;
    devDetails.jackFormats[ numJacks ] = inCaps->dwFormats;
    DEBG4( printf( "\tDevice has input, formats = 0x%04lx\n"
		   "\tSystem name = '%s', with %d channels\n",
		   (unsigned long)devDetails.jackFormats[ numJacks ],
		   inCaps->szPname, (int)inCaps->wChannels ) );
    ++numJacks;
  }

  if ( outCaps != 0 ) {
    devDetails.jackDir[ numJacks ] = ML_DIRECTION_OUT;
    devDetails.jackFormats[ numJacks ] = outCaps->dwFormats;
    devDetails.outSupport = outCaps->dwSupport;
    DEBG4( printf( "\tDevice has output, formats = 0x%04lx, "
		   "support = 0x%04lx\n"
		   "\tSystem name = '%s', with %d channels\n",
		   (unsigned long)devDetails.jackFormats[ numJacks ],
		   (unsigned long)devDetails.outSupport,
		   outCaps->szPname, (int)outCaps->wChannels ) );
    ++numJacks;
  }

  // Sanity-checking
  assert( (numJacks == 1) || (numJacks == 2) );
  devDetails.numJacks = numJacks;

  status = mlDINewPhysicalDevice( systemContext, moduleContext,
				  (MLbyte*) &devDetails,
				  sizeof( devDetails )
				  );

  if ( status != ML_STATUS_NO_ERROR ) {
    DEBG1( printf( "[winaudio]: Error adding phys device: %d\n", status ) );
  }

  return status;
}

//-----------------------------------------------------------------------------
//
// Get/set the "eventsWanted" and "state" fields of an AudioPath
// structure. This takes care of locking, to ensure thread-safety.
// The "set" functions also return the previous setting (atomic
// get-and-set).
int getState( AudioPath* path )
{
  int ret = 0;
  assert( path != 0 );

  EnterCriticalSection( &path->lock );
  ret = path->state;
  LeaveCriticalSection( &path->lock );

  return ret;
}

//-----------------------------------------------------------------------------
//
// See above
int setState( AudioPath* path, int newState )
{
  int ret = 0;

  EnterCriticalSection( &path->lock );
  ret = path->state;
  path->state = newState;
  LeaveCriticalSection( &path->lock );

  return ret;
}

//-----------------------------------------------------------------------------
//
// See above
int getEventsWanted( AudioPath* path )
{
  int ret = 0;
  assert( path != 0 );

  EnterCriticalSection( &path->lock );
  ret = path->eventsWanted;
  LeaveCriticalSection( &path->lock );

  return ret;
}

//-----------------------------------------------------------------------------
//
// See above
int setEventsWanted( AudioPath* path, int newEventsWanted )
{
  int ret = 0;

  EnterCriticalSection( &path->lock );
  ret = path->eventsWanted;
  path->eventsWanted = newEventsWanted;
  LeaveCriticalSection( &path->lock );

  return ret;
}

//-----------------------------------------------------------------------------
//
// Get the "WAVEFORMATEX" field of an AudioPath structure. This takes
// care of locking, to ensure thread-safety.  There is no equivalent
// "set" function, as that is more complex (requires closing and
// re-opening the device), and is thus done under a larger-scoped
// lock.
void getWaveFormatEx( AudioPath* path, WAVEFORMATEX* pFormat )
{
  assert( pFormat != 0 );

  EnterCriticalSection( &path->lock );
  *pFormat = path->format;
  LeaveCriticalSection( &path->lock );
}

//-----------------------------------------------------------------------------
//
// Get the device handle (either HWAVEIN or HWAVEOUT, as appropriate)
// -- the caller will need to cast to the appropriate type according
// to the path->direction.
// No equivalent "set" for the same reason as getWaveFormatEx
void* getDevHandle( AudioPath* path )
{
  void* ret;

  EnterCriticalSection( &path->lock );
  ret = (path->direction == ML_DIRECTION_IN) ?
    (void*) path->inHandle : (void*) path->outHandle;
  LeaveCriticalSection( &path->lock );

  return ret;
}


//-----------------------------------------------------------------------------
//
// Perform a state change, by setting the new state in the Audio Path
// structure, and notifying the child thread. Furthermore, if the new
// state is ABORTING, this waits for the ABORT to be complete.
MLstatus doStateChange( AudioPath* path, int newState )
{
  MLstatus status = ML_STATUS_NO_ERROR;
  int prevState;

  DEBG3( printf( "\trequesting new state: %s\n",
		 mlDeviceStateName( newState ) ) );

  // Set the new state (returns the previous state)
  prevState = setState( path, newState );

  // Note that if the new state is "ABORT", the child thread will take
  // care of aborting the message queue (to ensure it is done in sync
  // with its other operations)

  // If the state actually changed, wake up the child (in some cases,
  // the child may already by "awake" -- for instance, if we aborted a
  // transfer-in-progress -- but it never hurts to send a wake-up
  // call)
  if ( prevState != newState ) {
    DEBG3( printf( "\tprevious state was %s, waking child\n",
		   mlDeviceStateName( prevState ) ) );
    wakeUpAudioLoop( path );

    // And if the new state is "ABORTING", we must wait for the abort
    // to complete before we can return to the caller (in particular,
    // this ensures that the app doesn't start enqueuing new messages
    // on the DI queue -- for a new transfer operation -- before we
    // are done aborting the queue from the previous transfer).
    //
    // Note that the child thread will always end up signalling this
    // event, even if it wasn't actually transferring when the abort
    // request came in -- so it is safe to always wait here.
    //
    // Note that we also recycle this event for "FINISH" complete, to
    // ensure an orderly shutdown
    if ( (newState == ML_DEVICE_STATE_ABORTING) ||
	 (newState == ML_DEVICE_STATE_FINISHING) ) {
      DEBG3( printf( "\twaiting for change to state %s...\n",
		     mlDeviceStateName( newState ) ) );
      if ( WaitForSingleObject( path->abortComplete, INFINITE ) ==
	   WAIT_FAILED ) {
	DEBG1( printf( "[winaudio]: Wait for abortComplete failed, "
		       "error code = %d\n", (int)GetLastError() ) );
	status = ML_STATUS_INTERNAL_ERROR;
      }
      DEBG3( printf( "\t...completed change to state %s\n",
		     mlDeviceStateName( newState ) ) );
    } // if newState == ML_DEVICE_STATE_ABORTING
  } // if prevState != newState

  return status;
}

//-----------------------------------------------------------------------------
//
// Functions returned by ddconnect(3dm) in the list of "device ops"
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
MLstatus ddGetCapabilities( MLbyte* ddDevicePriv,
			    MLint64 staticObjectId,
			    MLpv** capabilities )
{
  WaveDeviceDetails* waveDetails = (WaveDeviceDetails*)ddDevicePriv;
  MLstatus ret;

  DEBG4( printf( "[winaudio]: in ddGetCapabilities\n" ) );

  // Figure out what is being queried -- the device, a jack, or a
  // path?
  switch ( mlDIextractIdType( staticObjectId ) ) {

  case ML_REF_TYPE_DEVICE: {
    // Construct a description of this device and use utility function
    // in generic.c to construct param list
    MLDDdeviceDetails device;
    device.name = genericDevName;
    device.index = (MLint32) waveDetails->devId;
    device.version = deviceVersion;
    device.location = waveDetails->name;

    // Our devices have 1 or 2 channels, depending on whether there is
    // an input and/or an output jack.
    device.jackLength = waveDetails->numJacks;
    device.pathLength = device.jackLength;

    ret = ddIdentifyPhysicalDevice( &device, staticObjectId, capabilities );
  } break;

  case ML_REF_TYPE_JACK: {
    MLDDjackDetails jack;
    MLint32 jackIndex = mlDIextractJackIndex( staticObjectId );

    // Make sure the jack index is valid for this device (and
    // remember, jackIndex is zero-based)
    if ( jackIndex >= waveDetails->numJacks ) {
      DEBG1( printf( "[winaudio]: Invalid ID: jack ID out of range\n" ) );
      ret = ML_STATUS_INVALID_ID;
    } else {
      // For now, use generic jack names, based on the direction --
      // ie: input our output. Not the greatest -- it doesn't tell the
      // user anything about the physical jack. But that info doesn't
      // seem to be available through the WAVE API.
      jack.name = (waveDetails->jackDir[ jackIndex ] == ML_DIRECTION_IN) ?
	genericInName : genericOutName;
      jack.type = ML_JACK_TYPE_ANALOG_AUDIO;
      jack.direction = waveDetails->jackDir[ jackIndex ];
      jack.pathIndexes = &jackIndex;
      jack.pathLength = 1;

      // Not handled at this point: JACK_COMPONENT_SIZE_INT32,
      // PARAM_IDS_INT64_ARRAY, OPEN_OPTION_IDS_INT64_ARRAY,
      // JACK_FEATURES_BYTE_ARRAY

      ret = ddIdentifyJack( &jack, staticObjectId, capabilities );
    } // if jackIndex >= 1 ... else
  } break;

  case ML_REF_TYPE_PATH: {
    MLDDpathDetails path;
    MLint32 pathIndex = mlDIextractPathIndex( staticObjectId );

    // Make sure the path index is valid for this device (and
    // remember, pathIndex is zero-based)
    if ( pathIndex >= waveDetails->numJacks ) {
      DEBG1( printf( "[winaudio]: Invalid ID: path ID out of range\n" ) );
      ret = ML_STATUS_INVALID_ID;
    } else {
      // For now, use generic jack names, based on the direction --
      // ie: input our output. Not the greatest -- it doesn't tell the
      // user anything about the physical jack. But that info doesn't
      // seem to be available through the WAVE API.
      path.name = (waveDetails->jackDir[ pathIndex ] == ML_DIRECTION_IN) ?
	genericInName : genericOutName;

      if ( waveDetails->jackDir[ pathIndex ] == ML_DIRECTION_IN ) {
	path.type = ML_PATH_TYPE_DEV_TO_MEM;
	path.srcJackIndex = pathIndex;
	path.dstJackIndex = -1;
      } else {
	path.type = ML_PATH_TYPE_MEM_TO_DEV;
	path.srcJackIndex = -1;
	path.dstJackIndex = pathIndex;
      } // if isInput ... else
      path.pixelLineAlignment = -1; // not used
      path.bufferAlignment = 4; // not sure alignment matters

      // Construct param details specific to this device/path

      path.params = constructParamDetails( waveDetails, pathIndex,
					   &path.paramsLength );

      ret = ddIdentifyPath( &path, staticObjectId, capabilities );
    } // if pathIndex >= 1 ... else
  } break;

  default: {
    DEBG1( printf( "[winaudio]: Invalid ID: not a device, jack or path\n" ) );
    ret = ML_STATUS_INVALID_ID;
  } break;
  } // switch

  return ret;
}

//-----------------------------------------------------------------------------
//
MLstatus ddPvGetCapabilities( MLbyte* ddDevicePriv,
			      MLint64 staticObjectId,
			      MLint64 paramId,
			      MLpv** capabilities )
{
  WaveDeviceDetails* waveDetails = (WaveDeviceDetails*)ddDevicePriv;
  MLstatus ret = ML_STATUS_INVALID_ID;

  //  DEBG4( printf( "[winaudio]: in ddPvGetCapabilities\n" ) );

  // FIXME: could improve this by returning actual capabilities of the
  // device, rather than hard-coded defaults. For instance, could
  // verify whether the device supports Stereo, and whether it
  // supports both 8-bit and 16-bit formats.
  // For now, just return default values...

  if ( mlDIextractIdType( staticObjectId ) == ML_REF_TYPE_PATH ) {
    MLint32 pathIndex = mlDIextractPathIndex( staticObjectId );
    // Ensure that path index is valid for this device!
    if ( pathIndex < waveDetails->numJacks ) {
      MLDDint32paramDetails** paramDetails;
      int maxParams = 0;
      int i;

      paramDetails = constructParamDetails( waveDetails, pathIndex,
					    &maxParams );
      for ( i=0; i < maxParams; ++i ) {
	if ( paramDetails[i]->id == paramId ) {
	  ret = ddIdentifyParam( paramDetails[i], staticObjectId,
				 capabilities );
	  break;
	}
      }
    }
  }

  return ret;
}

//-----------------------------------------------------------------------------
//
MLstatus ddOpen( MLbyte *ddDevicePriv,
		 MLint64 staticObjectId,
		 MLopenid openObjectId,
		 MLpv *openOptions,
		 MLbyte **retddPriv )
{
  MLqueueOptions qOpt;
  MLopenOptions oOpt;
  MLint32 pathIndex;
  MLstatus status = ML_STATUS_NO_ERROR;
  WaveDeviceDetails *waveDetails = (WaveDeviceDetails*) ddDevicePriv;
  AudioPath *path = NULL;
  MLint64 staticDeviceId = mlDIparentIdOfLogDevId( staticObjectId );
  MLint64 sysId = mlDIparentIdOfDeviceId( staticDeviceId );

  DEBG3( printf( "[winaudio]: in ddOpen\n" ) );
  DEBG4( printf( "[winaudio]: staticObjectId = %" FORMAT_LLD
		 ", openObjectId = %" FORMAT_LLD
		 "\nstatic dev Id from staticObjectId = %" FORMAT_LLD
		 ", sys Id from static dev Id = %" FORMAT_LLD "\n",
		 staticObjectId, openObjectId, staticDeviceId, sysId ) );

  if ( mlDIextractIdType( staticObjectId ) != ML_REF_TYPE_PATH ) {
    // Modelled after ossaudio: this module only supports opening
    // paths.  But why? Shouldn't we allow opening jacks to set
    // controls?  Hmmm... maybe not, because that would be difficult
    // to make persistent (when we close the jack -- ie, the WAVE
    // device -- the settings would be lost.
    DEBG1( printf( "[winaudio]: Invalid ID: not a path ID\n" ) );
    return ML_STATUS_INVALID_ID;
  }

  pathIndex = mlDIextractPathIndex( staticObjectId );
  if ( pathIndex >= waveDetails->numJacks ) {
    DEBG1( printf( "[winaudio]: Invalid ID: path ID out of range\n" ) );
    return ML_STATUS_INVALID_ID;
  }

  DEBG3( printf( "[winaudio]: openning path %d: %s\n",
		 pathIndex,
		 (waveDetails->jackDir[pathIndex] == ML_DIRECTION_IN) ?
		 genericInName : genericOutName ) );

  // Parse open options
  if ( mlDIparseOpenOptions( openObjectId, openOptions, &oOpt ) !=
       ML_STATUS_NO_ERROR ) {
    DEBG1( printf( "[winaudio]: Invalid open options" ) );
    return ML_STATUS_INVALID_PARAMETER;
  }

  DEBG4( printf( "[winaudio]: ddOpen: got Open options\n" ) );

  path = malloc( sizeof( AudioPath ) );
  if ( path == NULL ) {
    DEBG1( printf( "[winaudio]: Out of memory allocating AudioPath" ) );
    return ML_STATUS_OUT_OF_MEMORY;
  }

  DEBG4( printf( "[winaudio]: ddOpen: allocated audio path 0x%lx\n",
		 (long) path ) );

  // Get the UST function to be used for this path. Which function is
  // used depends on the system on which this device is open -- that's
  // why we obtained the sysId, based on the static object Id
  status = mlDIGetUSTSource( sysId, &(path->USTSourceFunc) );
  if ( status != ML_STATUS_NO_ERROR ) {
    DEBG1( printf( "[winaudio]: Error obtaining UST source func" ) );
    free( path );
    return status;
  }

  // Setup a single queue between us and the DI layer.
  // Get queue options from open options.
  memset( &qOpt, 0, sizeof( qOpt ) );
  qOpt.sendSignalCount    = oOpt.sendSignalCount;
  qOpt.sendMaxCount       = oOpt.sendQueueCount;
  qOpt.receiveMaxCount    = oOpt.receiveQueueCount;
  qOpt.eventMaxCount      = oOpt.eventPayloadCount;
  qOpt.messagePayloadSize = oOpt.messagePayloadSize;
  qOpt.ddMessageSize      = 0;
  qOpt.ddEventSize        = sizeof( MLpv ) * 4;
  qOpt.ddAlignment        = 4;

  // Now we can make a queue.
  status = mlDIQueueCreate( 0, 0, &qOpt, &(path->pQueue) );
  if ( status != ML_STATUS_NO_ERROR ) {
    DEBG1( printf( "[winaudio]: Error creating DI queue" ) );
    free( path );
    return status;
  }

  DEBG4( printf( "[winaudio]: ddOpen: created DI queue 0x%lx\n",
		 (long) path->pQueue ) );

  path->openId = openObjectId;
  path->direction = waveDetails->jackDir[pathIndex];
  path->devId = (UINT) waveDetails->devId;
  path->capabilities = waveDetails->jackFormats[pathIndex];
  path->state = ML_DEVICE_STATE_READY;
  path->eventsWanted = WAVE_NO_EVENTS;
  path->nextMSC = 0;
  path->startOfIdleUST = 0;
  path->numOutstandingBuffers = 0;

  // Determine the path's support for volume control
  // For input paths: no volume control (not supported by WAVE API)
  // For outputs, check the outSupport field
  path->volumeControl = getVolumeFromDevDetails( waveDetails, pathIndex );

  // Init the lock used to synchronise access to the volatile parts of
  // the path structure
  InitializeCriticalSection( &path->lock );

  // Create the event used to signle new DI messages are available.
  path->newDIMessage = CreateEvent( 0     /* security attribs */,
				    FALSE /* NOT manual reset event */,
				    FALSE /* init state NOT signaled */,
				    0     /* event name */ );
  if ( path->newDIMessage == 0 ) {
    DEBG1( printf( "[winaudio]: Error creating DImsg event, error = %d\n",
		   (int)GetLastError() ) );
    DeleteCriticalSection( &path->lock );
    mlDIQueueDestroy( path->pQueue );
    free( path );
    // Assume the error is in some way related to insufficient
    // resources...
    return ML_STATUS_INSUFFICIENT_RESOURCES;
  }

  // Create the event used to signal the end of an ABORT operation.
  path->abortComplete = CreateEvent( 0     /* security attribs */,
				     FALSE /* NOT manual reset event */,
				     FALSE /* init state NOT signaled */,
				     0     /* event name */ );
  if ( path->abortComplete == 0 ) {
    DEBG1( printf( "[winaudio]: Error creating abort event, error = %d\n",
		   (int)GetLastError() ) );
    CloseHandle( path->newDIMessage );
    DeleteCriticalSection( &path->lock );
    mlDIQueueDestroy( path->pQueue );
    free( path );
    // Assume the error is in some way related to insufficient
    // resources...
    return ML_STATUS_INSUFFICIENT_RESOURCES;
  }

  // Create the child thread for this device. This must be done
  // *before* actually opening the device, since the open call will
  // need to pass the thread handle (as the "callback")
  path->childHandle =
    (HANDLE) _beginthreadex( NULL, // *security
			     0,    // stack_size
			     &audioLoop,
			     (void*) path, // arg passed to audioLoop
			     0,    // initflag
			     &path->childId
			     );
  if ( path->childHandle == 0 ) {
    DEBG1( printf( "[winaudio]: Error creating child thread\n" ) );
    CloseHandle( path->newDIMessage );
    CloseHandle( path->abortComplete );
    DeleteCriticalSection( &path->lock );
    mlDIQueueDestroy( path->pQueue );
    free( path );
    return ML_STATUS_INSUFFICIENT_RESOURCES;
  }

  DEBG4( printf( "[winaudio]: ddOpen: created child thread, id = %u\n",
		 path->childId ) );

  // Finally, open the actual WAVE device (final param is the format
  // -- set to NULL, to indicate we wish to use default format for
  // now)
  status = waveDeviceOpen( path, 0 );

  if ( status == ML_STATUS_NO_ERROR ) {
    DEBG4( printf( "[winaudio]: ddOpen successful\n" ) );
  }

  // Supply "path" as the private data structure for all subsequent
  // calls
  *retddPriv = (MLbyte*) path;
  return status;
}

//-----------------------------------------------------------------------------
//
MLstatus ddSetControls( MLbyte* ddPriv,	MLopenid openObjectId, MLpv *msg )
{
  MLstatus status = ML_STATUS_NO_ERROR;
  AudioPath* path = (AudioPath*) ddPriv;

  // Avoid compiler warnings
  (void) openObjectId;

  DEBG4( printf( "[winaudio]: in ddSetControls\n" ) );

  // Two types of setControls: setting the device state, and all
  // others.
  if ( msg[0].param == ML_DEVICE_STATE_INT32 ) {
    status = doStateChange( path, msg[0].value.int32 );

  } else {
    // Some non-state control -- first check if all parts of the
    // request message are valid at this time.
    MLint32 accessNeeded = ML_ACCESS_WRITE | ML_ACCESS_IMMEDIATE;

    if ( getState( path ) == ML_DEVICE_STATE_TRANSFERRING ) {
      accessNeeded |= ML_ACCESS_DURING_TRANSFER;
    }
    status = mlDIvalidateMsg( openObjectId, accessNeeded,
			      ML_FALSE /* not used in get-controls */, msg );

    if ( status == ML_STATUS_NO_ERROR ) {
      status = setAudioPathControls( path, msg );
    } else {
      DEBG2( printf( "[winaudio]: ddSetControls: "
		     "invalid request message\n" ) );
    }
  }

  return status;
}

//-----------------------------------------------------------------------------
//
MLstatus ddGetControls( MLbyte *ddPriv, MLopenid openObjectId, MLpv *msg )
{
  AudioPath* path = (AudioPath*) ddPriv;
  MLint32 accessNeeded = ML_ACCESS_RI;
  MLstatus status;

  DEBG4( printf( "[winaudio]: in ddGetControls\n" ) );

  if ( path == 0 ) {
    DEBG1( printf( "[winaudio]: ddGetControls: invalid AudioPath\n" ) );
    status = ML_STATUS_INTERNAL_ERROR;

  } else {

    // First, validate the message, ie: ensure that all control
    // requests can be handled at this time.
    //
    // Note: this could also be done in getAudioPathControls, as we
    // handle each part of the message. In particular, some queries
    // may not be allowed according to the access controls reported by
    // getCapabilities even though we are actually able to honour them
    // -- so we *could* just let them through. But then we wouldn't be
    // strictly compliant with the spec.

    // If we are currently in the midst of a transfer, make that part
    // of the required access flags
    if ( getState( path ) == ML_DEVICE_STATE_TRANSFERRING ) {
      accessNeeded |= ML_ACCESS_DURING_TRANSFER;
    }

    status = mlDIvalidateMsg( openObjectId, accessNeeded,
			      ML_TRUE /* used in get-controls */, msg );

    if ( status == ML_STATUS_NO_ERROR ) {
      status = getAudioPathControls( path, msg );
    } else {
      DEBG2( printf( "[winaudio]: ddGetControls: "
		     "invalid request message\n" ) );
    }
  } // if path == 0 ... else ...

  return status;
}

//-----------------------------------------------------------------------------
//
MLstatus ddSendControls( MLbyte *ddPriv,
			 MLopenid openObjectId,
			 MLpv *controls )
{
  AudioPath *path = (AudioPath *) ddPriv;
  MLstatus status = ML_STATUS_NO_ERROR;

  // Avoid compiler warnings
  (void) openObjectId;

  DEBG4( printf( "[winaudio]: in ddSendControls\n" ) );

  if ( path == 0 ) {
    DEBG1( printf( "[winaudio]: ddSendControls: invalid AudioPath\n" ) );
    status = ML_STATUS_INTERNAL_ERROR;

  } else {
    status = mlDIQueuePushMessage( path->pQueue,
				   ML_CONTROLS_IN_PROGRESS,
				   controls,
				   0, 0, 0 );

    // Let child thread know that there is a new message on the queue
    if ( SetEvent( path->newDIMessage ) == 0 ) {
      DEBG1( printf( "[winaudio]: ddSendControls: setEvent newDIMessage "
		     "failed, error code = %d\n", (int)GetLastError() ) );
      status = ML_STATUS_INTERNAL_ERROR;
    }
  }

  return status;
}

//-----------------------------------------------------------------------------
//
MLstatus ddQueryControls( MLbyte* ddPriv,
			  MLopenid openObjectId,
			  MLpv *controls )
{
  AudioPath *path = (AudioPath *) ddPriv;
  MLstatus status = ML_STATUS_NO_ERROR;

  // Avoid compiler warnings
  (void) openObjectId;

  DEBG4( printf( "[winaudio]: in ddQueryControls\n" ) );

  if ( path == 0 ) {
    DEBG1( printf( "[winaudio]: ddQueryControls: invalid AudioPath\n" ) );
    status = ML_STATUS_INTERNAL_ERROR;

  } else {
    status = mlDIQueuePushMessage( path->pQueue,
				   ML_QUERY_IN_PROGRESS,
				   controls,
				   0, 0, 0 );

    // Let child thread know that there is a new message on the queue
    if ( SetEvent( path->newDIMessage ) == 0 ) {
      DEBG1( printf( "[winaudio]: ddQueryControls: setEvent newDIMessage "
		     "failed, error code = %d\n", (int)GetLastError() ) );
      status = ML_STATUS_INTERNAL_ERROR;
    }
  }

  return status;
}

//-----------------------------------------------------------------------------
//
MLstatus ddSendBuffers( MLbyte* ddPriv,	MLopenid openObjectId, MLpv *buffers )
{
  AudioPath *path = (AudioPath *) ddPriv;
  MLstatus status = ML_STATUS_NO_ERROR;

  // Avoid compiler warnings
  (void) openObjectId;

  DEBG4( printf( "[winaudio]: in ddSendBuffers\n" ) );

  if ( path == 0 ) {
    DEBG1( printf( "[winaudio]: ddSendBuffers: invalid AudioPath\n" ) );
    status = ML_STATUS_INTERNAL_ERROR;

  } else {
    status = mlDIQueuePushMessage( path->pQueue,
				   ML_BUFFERS_IN_PROGRESS,
				   buffers,
				   0, 0, 0 );

    // Let child thread know that there is a new message on the queue
    if ( SetEvent( path->newDIMessage ) == 0 ) {
      DEBG1( printf( "[winaudio]: ddSendBuffers: setEvent newDIMessage "
		     "failed, error code = %d\n", (int)GetLastError() ) );
      status = ML_STATUS_INTERNAL_ERROR;
    }
  }

  return status;
}

//-----------------------------------------------------------------------------
//
MLstatus ddReceiveMessage( MLbyte *ddPriv,
			   MLopenid openObjectId,
			   MLint32 *retMsgType,
			   MLpv **retReply )
{
  AudioPath* path = (AudioPath*) ddPriv;
  MLstatus status = ML_STATUS_NO_ERROR;

  // Avoid compiler warnings
  (void) openObjectId;

  DEBG4( printf( "[winaudio]: in ddReceiveMessage\n" ) );

  if ( path == 0 ) {
    DEBG1( printf( "[winaudio]: ddReceiveMessage: invalid AudioPath\n" ) );
    status = ML_STATUS_INTERNAL_ERROR;

  } else {
    status = mlDIQueueReceiveMessage( path->pQueue,
				      (enum mlMessageTypeEnum*) retMsgType,
				      retReply, 0, 0 );
  }

  return status;
}

//-----------------------------------------------------------------------------
//
MLstatus ddXcodeWork( MLbyte *ddPriv, MLopenid openObjectId )
{
  DEBG4( printf( "[winaudio]: in ddXcodeWork\n" ) );

  // Avoid compiler warnings
  (void) ddPriv;
  (void) openObjectId;

  DEBG1( printf( "[winaudio]: ddXcodeWork: FUNCTION NOT APPLICABLE\n" ) );

  return ML_STATUS_INTERNAL_ERROR; // not applicable for paths
}

//-----------------------------------------------------------------------------
//
MLstatus ddClose( MLbyte* ddPriv, MLopenid openObjectId )
{
  AudioPath* path = (AudioPath*) ddPriv;

  DEBG4( printf( "[winaudio]: in ddClose\n" ) );

  if ( path == 0 ) {
    DEBG1( printf( "[winaudio]: ddClose: ddPriv = 0\n" ) );
    return ML_STATUS_INTERNAL_ERROR;
  }

  // Don't bother checking for errors -- what are we going to do?
  // Return early, and leave all the resources allocated?

  // Start by aborting the transfer in progress (if any)
  doStateChange( path, ML_DEVICE_STATE_ABORTING );

  // Now instruct the child thread to terminate
  doStateChange( path, ML_DEVICE_STATE_FINISHING );

  // Reset and close the WAVE device
  waveDeviceClose( path );

  // Destroy queue (it was aborted above, in doStateChange)
  DEBG4( printf( "\tDestroying DI queue...\n" ) );
  mlDIQueueDestroy( path->pQueue );

  // Close event handles
  DEBG4( printf( "\tClosing event handles...\n" ) );
  CloseHandle( path->newDIMessage );
  CloseHandle( path->abortComplete );

  // Terminate child thread and close thread handle
  //
  // Actually, "_endthreadex" is called automatically when the child
  // thread function returns, so all we need to do is close the
  // handle.
  DEBG4( printf( "\tClosing child thread handle...\n" ) );
  CloseHandle( path->childHandle );

  // Destroy critical section
  DeleteCriticalSection( &path->lock );

  // Finally, free path structure
  free( path );

  DEBG4( printf( "\t...ddClose done\n" ) );
  return ML_STATUS_NO_ERROR;
}

//-----------------------------------------------------------------------------
//
// Entry points called by the OpenML SDK
//
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
//
// This is the first device dependent entry point called by the mlSDK.
// This routine must call NewPhysicalDevice() for every physical
// device (i.e. board) that it can control.
MLstatus ddInterrogate(MLsystemContext systemContext,
		       MLmoduleContext moduleContext)
{
  MLstatus status = ML_STATUS_NO_ERROR;
  int numInDevs, numOutDevs, maxDevs, i;

#ifdef DEBUG
  char *e = getenv("ADEBG");
  if (e) debugLevel = atoi(e);
#endif

  DEBG4(printf("[winaudio]: in ddInterrogate\n"));

  // Add all WAVE devices found on the system. This will combine input
  // and output devices that share the same device ID, by creating a
  // device that has both an input and an output jack.

  numInDevs  = waveInGetNumDevs();
  numOutDevs = waveOutGetNumDevs();
  maxDevs = (numInDevs > numOutDevs) ? numInDevs : numOutDevs;

  DEBG3( printf( "[winaudio]: %d input & %d output wave device(s)\n",
		 numInDevs, numOutDevs ) );

  for ( i=0; i < maxDevs; ++i ) {
    WAVEINCAPS inCaps, *pInCaps = 0;
    WAVEOUTCAPS outCaps, *pOutCaps = 0;
    UINT_PTR devId = (UINT_PTR)i;
    MMRESULT mmStatus;

    if ( i < numInDevs ) {
      // This device has an input, get its capabilities
      DEBG4( printf( "[winaudio]: checking input for device %d\n", i ) );
      mmStatus = waveInGetDevCaps( devId, &inCaps, sizeof( WAVEINCAPS ) );
      if ( mmStatus == MMSYSERR_NOERROR ) {
	pInCaps = &inCaps;
      } else {
	DEBG1( printf( "[winaudio]: Error retrieving caps for input dev %d"
		       ", error code = %d\n", i, mmStatus ) );
      }
    } // if i < numInDevs

    if ( i < numOutDevs ) {
      // This device has an output, get its capabilities
      DEBG4( printf( "[winaudio]: checking output for device %d\n", i ) );
      mmStatus = waveOutGetDevCaps( devId, &outCaps, sizeof( WAVEOUTCAPS ) );
      if ( mmStatus == MMSYSERR_NOERROR ) {
	pOutCaps = &outCaps;
      } else {
	DEBG1( printf( "[winaudio]: Error retrieving caps for output dev %d"
		       ", error code = %d\n", i, mmStatus ) );
      }
    } // if i < numOutDevs
    status = addPhysicalDevice( systemContext, moduleContext, devId,
				pInCaps, pOutCaps );

    if ( status != ML_STATUS_NO_ERROR ) {
      // If there is an error, stop trying to add more devices
      break;
    }
  } // for i=0...maxDevs

  return status;
}

//-----------------------------------------------------------------------------
//
// ddConnect(3dm) will be called in every process that accesses a
// physical device exposed by your device-dependent module.  It
// establishes the connection between the process independent
// description of a physical device and process specific addresses for
// entry points and private memory structures.
MLstatus ddConnect(MLbyte *physicalDeviceCookie,
		   MLint64 staticDeviceId,
		   MLphysicalDeviceOps *pOps,
		   MLbyte **ddDevicePriv)
{
  WaveDeviceDetails* waveDetails = (WaveDeviceDetails*)physicalDeviceCookie;
  WaveDeviceDetails* waveDCopy = NULL;
  MLphysicalDeviceOps devOps = {
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

#ifdef DEBUG
  char *e = getenv("ADEBG");
  if (e) debugLevel = atoi(e);
#endif

  DEBG2( printf( "[winaudio]: in ddConnect\n" ) );
  DEBG4( printf( "[winaudio]: staticDeviceId = %" FORMAT_LLD ", sys Id = %"
		 FORMAT_LLD "\n", staticDeviceId,
		 mlDIparentIdOfDeviceId( staticDeviceId ) ) );

  waveDCopy = (WaveDeviceDetails*)malloc( sizeof( WaveDeviceDetails ) );
  if ( waveDCopy == NULL ) {
    *ddDevicePriv = NULL;
    return ML_STATUS_OUT_OF_MEMORY;
  }

  *waveDCopy = *waveDetails;
  *ddDevicePriv = (MLbyte*)waveDCopy;

  *pOps = devOps;

  return ML_STATUS_NO_ERROR;
}

//-----------------------------------------------------------------------------
//
// ddDisconnect(3dm) is called at exit time for each process which
// called ddConnect(3dm).  It is a convenient place to free any memory
// allocated during the connect call (thereby eliminating a reported
// memory leak which debugging tools may flag).
//
// Note that ddDisconnect(3dm) is not guaranteed to be called if an
// application terminates abnormally.
MLstatus ddDisconnect( MLbyte* ddDevPriv )
{
  WaveDeviceDetails* waveDetails = (WaveDeviceDetails*)ddDevPriv;

  DEBG2( printf( "[winaudio]: in ddDisconnect\n" ) );

  free( waveDetails );

  return ML_STATUS_NO_ERROR;
}

