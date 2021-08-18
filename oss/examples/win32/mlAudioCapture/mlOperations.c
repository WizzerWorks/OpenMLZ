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

#include "mlOperations.h"
#include "mlAudioCapture.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "audiofile.h"


// ============================================================================
//
// Local structure definitions
//
// ============================================================================

// Parameters for the transfer thread. This is filled-in by
// 'prepareTransfer()', partly with information obtained from the UI,
// partly with computed values. It also contains references to the
// open ML path, the allocated buffers, the file opened for writing.
//
// It is supplied to the child thread upon creation.
typedef struct {
  // libaudiofile file I/O information
  AFfilehandle outputFile;
  int frameSize;

  // ML transfer information
  MLint32 numBuffers; // straight from UI
  MLint32 numSamplesPerBuffer; // currently constant, but could be from UI
  MLint32 bufferSize; // computed: numSamples * sampleSize * numChannels
  void** buffers;
  MLopenid openPath;

  // Handle of UI windows -- used to send error messages as well as UI
  // update messages (for signal strength, etc.)
  HWND hUIWindow;
} TransferThreadParams;


// ============================================================================
//
// Local variables
//
// ============================================================================


// Error message, returned by getMLError().
// This is filled-in by functions that perform ML operations, when they
// encounter errors.
static char errorMsg[1024] = "No error";

// Events for communication between the child thread and the UI
// thread.
//  - threadEvent: signaled by the UI thread to indicate a change in state.
//    See the 'eventType' variable below.
//
//  - replyEvent: used by the child thread to acknowledge the last command.
//    Actually only used to acknowledge "FINISH_TRANSFER" -- the UI thread
//    expects no acknowledgement for the other events.
static HANDLE threadEvent = NULL;
static HANDLE replyEvent = NULL;

// State of child thread. Modified by UI thread (using functions
// below).
static enum { STATE_IDLE = 0,
	      STATE_FINISHING = 0x1,
	      STATE_TRANSFERING = 0x2,
	      STATE_STOP_TRANSFER = 0x4, // transitional state
	      STATE_MONITORING = 0x8,
	      STATE_STOP_MONITOR = 0x10,  // transitional state
} curState = STATE_IDLE;

// Handle of child thread
static HANDLE threadHandle = NULL;

// Indicates whether or not the signal strength should be computed by
// the child thread. This is simply set by the UI thread, without any
// form of synchronization or locking -- we assume that writing a
// boolean is atomic (and even if it isn't, at worst, the child thread
// will read an inconsistent value, which might make it compute -- or
// not -- when it should do the opposite. Not a big deal). Checked by
// the child thread each time a buffer is received from ML.
MLboolean computeStrength = ML_FALSE;

// Max value for a signed 16-bit int
#define MLINT16_MAX ((1<<15)-1)


// ============================================================================
//
// LOCAL FUNCTIONS
//
// ============================================================================


// ----------------------------------------------------------------------------
//
// Free the contents of a DevInfo structure, including any PathInfo
// structures it may point to.
//
// Does *not* free the DevInfo pointer itself.
static void freeDevPathInfo( const DevInfo* devInfo )
{
  int i;

  if ( devInfo->devName != NULL ) {
    free( (char*)devInfo->devName );
  }

  if ( devInfo->paths != NULL ) {
    for ( i=0; i < devInfo->numPaths; ++i ) {
      if ( devInfo->paths[i].pathName != NULL ) {
	free( (char*)devInfo->paths[i].pathName );
      }
    }
    free( devInfo->paths );
  }
  return;
}


// ----------------------------------------------------------------------------
//
// Add path info to a DevInfo structure
//
// This examines the ML path identified by the pathID, and determines
// if it is valid for our application (ie: if it can support
// device-to-memory transfer of audio buffers).
//
// If the path is valid, the info is added to the supplied DevInfo
// structure. This assume that the DevInfo structure contains at least
// one open PathInfo slot for us to use.
//
// Returns ML_TRUE if all is OK, ML_FALSE on error (in which case
// getMLerror() can be called to obtain the error message).
static MLboolean addPaths( DevInfo* devInfo, MLint64 pathID )
{
  MLpv* caps;
  MLpv* param;
  MLstatus status;
  PathInfo thisPath;

  // Fill in default (expected) values for this path
  thisPath.monoOK = ML_TRUE;
  thisPath.stereoOK = ML_TRUE;
  thisPath.pathID = pathID;
  thisPath.pathName = NULL;

  // Get ML info on this path. Remember to call mlFreeCapabilities()
  // when we are done!
  status = mlGetCapabilities( pathID, &caps );
  if ( status != ML_STATUS_NO_ERROR ) {
    sprintf( errorMsg, "Path-level mlGetCapabilities failed:\n%s",
	     mlStatusName( status ) );
    mlFreeCapabilities( caps );
    return ML_FALSE;
  }

  // Get path type, ie: dev-to-mem or not? Not an error if we can't
  // find, we can live... (although it *is* strange, this param should
  // be there)
  param = mlPvFind( caps, ML_PATH_TYPE_INT32 );
  if ( param != NULL ) {
    if ( param->value.int32 != ML_PATH_TYPE_DEV_TO_MEM ) {
      // Not an input path, can not use
      mlFreeCapabilities( caps );
      return ML_TRUE;
    }
  }

  // Get params that can be used on this path. We could check for all
  // the ones we need, but for now, just look for AUDIO_BUFFER_POINTER
  param = mlPvFind( caps, ML_PARAM_IDS_INT64_ARRAY );
  if ( param != NULL ) {
    int i;
    MLboolean pathOK = ML_FALSE;

    for ( i=0; i < param->length; ++i ) {
      if ( param->value.pInt64[i] == ML_AUDIO_BUFFER_POINTER ) {
	// Found it, path is usable
	pathOK = ML_TRUE;
	break;
      }
    }

    if ( ! pathOK ) {
      mlFreeCapabilities( caps );
      return ML_TRUE;
    }
  } // if param != NULL

  // Get device name and index
  param = mlPvFind( caps, ML_NAME_BYTE_ARRAY );
  if ( param == NULL ) {
    sprintf( errorMsg, "Could not obtain path name" );
    mlFreeCapabilities( caps );
    return ML_FALSE;
  }
  thisPath.pathName = strdup( param->value.pByte );

  // Copy the PathInfo into the next available slot in the DevInfo
  // structure. We assume the DevInfo structure allocated enough
  // slots. Then increment the number of valid paths in the structure.
  devInfo->paths[ devInfo->numPaths ] = thisPath;
  ++(devInfo->numPaths);

  mlFreeCapabilities( caps );
  return ML_TRUE;
}


// ----------------------------------------------------------------------------
//
// Add device info (including valid paths for the device) to the
// supplied CurSysInfo structure.
//
// The examines the ML device identified by devID, and finds all
// valid, usable paths (ie: paths appropriate for our device-to-memory
// audio transfer). If at least 1 valid path was found, the
// information for this device is added to the CurSysInfo structure
// (which is expected to have enough space pre-allocated).
//
// Returns ML_TRUE if all is OK, ML_FALSE on error (in which case
// getMLerror() can be called to obtain the error message).
static MLboolean addDevAndPaths( CurSysInfo* sysInfo, MLint64 devID )
{
  MLpv* caps;
  MLpv* param;
  MLstatus status;
  DevInfo thisDev;
  const char* devName;
  int fullNameSz;
  int numPaths;

  // Initial default settings for this dev
  thisDev.devID = devID;
  thisDev.devName = NULL;
  thisDev.numPaths = 0;
  thisDev.paths = NULL;

  // Get ML info for this device. Remember to call mlFreeCapabilities()
  // when done!
  status = mlGetCapabilities( devID, &caps );
  if ( status != ML_STATUS_NO_ERROR ) {
    sprintf( errorMsg, "device-level mlGetCapabilities failed:\n%s",
	     mlStatusName( status ) );
    mlFreeCapabilities( caps );
    return ML_FALSE;
  }

  // Get device name and index
  param = mlPvFind( caps, ML_NAME_BYTE_ARRAY );
  if ( param == NULL ) {
    sprintf( errorMsg, "Could not obtain device name" );
    mlFreeCapabilities( caps );
    return ML_FALSE;
  }
  devName = param->value.pByte;
  param = mlPvFind( caps, ML_DEVICE_INDEX_INT32 );
  if ( param == NULL ) {
    sprintf( errorMsg, "Could not obtain device index (device '%s')",
	     devName );
    mlFreeCapabilities( caps );
    return ML_FALSE;
  }

  // Convert the name and index into a single string, for use in the
  // UI to identify this device. The format is "name : id".
  //
  // Note that the pointer to the allocated string is written to the
  // DevInfo structure, and will be freed by the call to
  // freeDevPathInfo()
  fullNameSz = strlen( devName ) + 7; // allows room for " : 999" and
				      // NULL-terminator
  thisDev.devName = (char*) malloc( fullNameSz );
  sprintf( thisDev.devName, "%s : %3d", devName, param->value.int32 );

  // Get all paths on this device
  param = mlPvFind( caps, ML_DEVICE_PATH_IDS_INT64_ARRAY );
  if ( param == NULL ) {
    sprintf( errorMsg, "Could not get path list for device '%s'",
	     thisDev.devName );
    freeDevPathInfo( &thisDev );
    mlFreeCapabilities( caps );
    return ML_FALSE;
  }

  // Deal with the possibility that there are no paths.
  numPaths = param->length;
  if ( numPaths > 0 ) {
    int i;

    // Allocate room for max number of paths. Perhaps not all paths
    // will be usable, so when we are done, not all entries in the
    // array will be filled-in. Not a big deal.
    thisDev.paths = (PathInfo*) malloc( sizeof(PathInfo)*numPaths );

    // Iterate over all paths, and add those that seem appropriate.
    for ( i=0; i < numPaths; ++i ) {
      if ( ! addPaths( &thisDev, param->value.pInt64[i] ) ) {
	freeDevPathInfo( &thisDev );
	mlFreeCapabilities( caps );
	return ML_FALSE;
      }
    }

    // If we added at least 1 path, include the device in the system list
    if ( thisDev.numPaths > 0 ) {
      sysInfo->devs[ sysInfo->numDevs ] = thisDev;
      ++(sysInfo->numDevs);
    }
  } // if numPaths > 0

  mlFreeCapabilities( caps );
  return ML_TRUE;
}


// ----------------------------------------------------------------------------
//
// Send an error message to the specified window.
//
// The error message is composed of the supplied string followed by
// the string corresponding to the supplied MLstatus.
//
// Called from within the child thread, to (asynchronously) inform the
// UI thread of an error. Currently, all errors are fatal -- ie: the
// child thread is unable to continue working, and expects the UI
// thread to shut it down.
static void sendErrorMsg( HWND hwnd, const char* error, MLstatus status )
{
  char errStr[ 1024 ];
  sprintf( errStr, "%s:\n%s", error, mlStatusName( status ) );
  PostMessage( hwnd, WMA_MLAC_ERROR, 0, (LPARAM) strdup( errStr ) );
  return;
}


// ----------------------------------------------------------------------------
//
// Examine every sample in the supplied buffer and compute the maximum
// (absolute value) sample for each channel.
//
// If the computed maximums are greater than the values in 'lStrength'
// and 'rStrength', update those accordingly.
//
// The buffer is assumed to contain 16-bit integer values.
static void bufferSignalStrength( MLint16* buffer, MLint32 numSamples,
				  MLint32* lStrength, MLint32* rStrength )
{
  int i;
  MLint16 left = 0, right = 0;

  for ( i=0; i < numSamples; ++i ) {
    MLint16 val = *buffer;
    if ( val < 0 ) {
      val = -val;
    }
    if ( val > left ) {
      left = val;
    }
    ++buffer;
    val = *buffer;
    if ( val < 0 ) {
      val = -val;
    }
    if ( val > right ) {
      right = val;
    }
    ++buffer;
  }

  if ( right > *rStrength ) {
    *rStrength = right;
  }
  if ( left > *lStrength ) {
    *lStrength = left;
  }
  return;
}
				

// ----------------------------------------------------------------------------
//
// Free resources related to an active transfer
//
// This includes closing the output file, closing the ML path, and
// freeing the buffers.
//
// The TransferThreadParams structure itself is freed as well.
void freeTransferThreadParams( TransferThreadParams* params )
{
  if ( params != NULL ) {
    // Start by closing the ML path, before freeing the buffers that
    // are used by the path.
    if ( params->openPath != 0 ) {
      mlClose( params->openPath );
    }

    if ( params->outputFile != AF_NULL_FILEHANDLE ) {
      afCloseFile( params->outputFile );
    }

    if ( params->buffers != NULL ) {
      int i;
      for ( i=0; i < params->numBuffers; ++i ) {
	free( params->buffers[i] );
      }
      free( params->buffers );
    }

    // Finally, free the structure itself
    free( params );
  }

  return;
}


// ----------------------------------------------------------------------------
//
// Child thread entry point -- actual ML transfer code
//
// The child thread is responsible for the actual ML transfer. It is
// created by the UI thread through a call to 'prepareTransfer'.
//
// When the thread is created, the ML path is already open and ready
// for transfer, and the output file is likewise ready to be used.
//
// The child thread waits on the 'threadEvent' for the UI thread to
// signal a state change. At this point (depening on the new state,
// set by the UI thread), it either exits, or starts the transfer.
//
// During a transfer, the thread waits on both the 'threadEvent' (for
// new instructions from the UI thread) and the ML event (for messages
// such as 'BUFFER_COMPLETE'). If the UI sends a 'stop transfer'
// event, the ML transfer is stopped, but the path is kept open -- the
// thread simply loops back to waiting for a 'start transfer' (or
// exit) event.
DWORD WINAPI threadProc( LPVOID createParam )
{
  BOOL done = FALSE;
  BOOL inProgress = FALSE;
  MLstatus status;
  int numDropped = 0;
  MLint32 accumTime = 0; // Total transfer time, in milli-secs
  MLint64 accumMSC = 0; // Accumulated transferred MSCs
  TransferThreadParams* params = 0; // from createParam

  // Array of wait handles:
  //  item 0 is the event for the ML message queue
  //  itme 1 is the event from our UI thread
  HANDLE waitHandles[2];
  waitHandles[1] = threadEvent;

  params = (TransferThreadParams*) createParam;

  // Get the Handle to use to wait for ML messages
  status = mlGetReceiveWaitHandle( params->openPath, &(waitHandles[0]) );
  if ( status != ML_STATUS_NO_ERROR ) {
    sendErrorMsg( params->hUIWindow, "Can not obtain ML wait handle", status );
    done = ML_TRUE;
  }

  // Loop until it is time for the child thread to exit -- this
  // happens either when the UI thread sends the 'finish' event, or
  // when an error is encountered.
  while ( ! done ) {
    int i;
    int numCaptured = 0;
    MLint64 startUST = 0;
    MLint64 startMSC = 0;
    MLint32 diffTime; // Based on UST, but in milli-secs
    MLint64 diffMSC;
    MLint32 lStrength = 0, rStrength = 0;

    // Pre-roll the buffers
    for ( i = 0; i < params->numBuffers; i++ ) {
      MLpv msg[4];

#define PTR_PARAM_POS 0
#define UST_PARAM_POS 1
#define MSC_PARAM_POS 2

      msg[PTR_PARAM_POS].param = ML_AUDIO_BUFFER_POINTER;
      msg[PTR_PARAM_POS].value.pByte = (MLbyte *) params->buffers[i];
      msg[PTR_PARAM_POS].maxLength = params->bufferSize;

      // Request UST and MSC for every buffer, so we can monitor
      // progress (and, when all is done, compute the effective rate)
      msg[UST_PARAM_POS].param = ML_AUDIO_UST_INT64;
      msg[MSC_PARAM_POS].param = ML_AUDIO_MSC_INT64;
      msg[3].param = ML_END;

      status = mlSendBuffers( params->openPath, msg );
      if ( status != ML_STATUS_NO_ERROR ) {
	sendErrorMsg( params->hUIWindow, "Can not send ML buffers", status );
	done = ML_TRUE;
	break;
      }
    } // for i=0..numBuffers

    if ( done ) {
      break;
    }

    // Now wait for the order to get started!
    while ( curState == STATE_IDLE ) {
      WaitForSingleObject( threadEvent, INFINITE );
    }

    if ( (curState & STATE_FINISHING) != 0 ) {
#ifdef DEBUG
      fprintf( stdout, "State now FINISHING: breaking out of outer loop\n" );
      fflush( stdout );
#endif
      break;
    }

#ifdef DEBUG
    fprintf( stdout, "Starting transfer\n" );
    fflush( stdout );
#endif

    status = mlBeginTransfer( params->openPath );
    if ( status != ML_STATUS_NO_ERROR ) {
      sendErrorMsg( params->hUIWindow, "Can not begin ML transfer", status );
      break;
    }

    while ( (curState != STATE_IDLE) && ! done ) {

      if ( WaitForMultipleObjects( 2, waitHandles, FALSE, INFINITE ) ==
	   WAIT_OBJECT_0 ) {
	// ML message
	MLint32 messageType;
	MLpv *message;
	int cachedState;

	// Cache the current state -- this ensures that the state is
	// consistent throughout this iteration of the loop
	//
	// Note: we are not protecting the curState variable in any
	// way... let's hope that writing an 'int' is atomic on all
	// platforms.
	cachedState = curState;

	status = mlReceiveMessage( params->openPath, &messageType, &message );
	if ( status != ML_STATUS_NO_ERROR ) {
	  sendErrorMsg( params->hUIWindow,
			"Can not receive ML message", status );
	  done = ML_TRUE;
	  break;
	}

	switch ( messageType ) {
	case ML_BUFFERS_COMPLETE: {
	  MLint64 buffUST;
	  MLint64 buffMSC;

	  ++numCaptured;

	  // If actually capturing, write buffer to file
	  if ( (cachedState & STATE_TRANSFERING) != 0 ) {
	    afWriteFrames( params->outputFile, AF_DEFAULT_TRACK,
			   message[PTR_PARAM_POS].value.pByte,
			   message[PTR_PARAM_POS].length / params->frameSize );

	    // Get UST/MSC of this buffer, process to determine total
	    // transfer time, and effective rate
	    buffUST = message[UST_PARAM_POS].value.int64;
	    buffMSC = message[MSC_PARAM_POS].value.int64;
	    if ( startUST == 0 ) {
	      startUST = buffUST;
	      startMSC = buffMSC;

	    } else {
	      // Compute diff in milli-secs -- this should fit in a
	      // 32-bit value, which can be sent using
	      // PostMessage. Remember to add the accumulated capture
	      // time, which includes all previous start/stop times.
	      // Only send updates to the UI thread every 5 buffers --
	      // reduces unecessary messages
	      diffTime = (MLint32) ((buffUST - startUST)/1000000);
	      diffMSC = buffMSC - startMSC;
	      if ( (numCaptured % 5) == 0 ) {
		MLint32 diffPlusAccumTime = diffTime + accumTime;
		PostMessage( params->hUIWindow, WMA_MLAC_TOTAL_TIME,
			     (WPARAM) NULL, (LPARAM) diffPlusAccumTime );
		// Effective rate is the diff in MSC / diff in
		// Time. Remember to compensate for the fact that diff
		// in Time is in milli-secs, but we want a rate in Hz
		// (ie: samples / sec).
		if ( diffPlusAccumTime > 0 ) {
		  MLint32 rate = (MLint32) (((diffMSC + accumMSC) * 1000) /
					    diffPlusAccumTime);
		  PostMessage( params->hUIWindow, WMA_MLAC_EFFECTIVE_RATE,
			       (WPARAM) rate, (LPARAM) NULL );
		}
	      } // if numCaptured%5 == 0
	    } // if startUST == 0 ... else ...
	  }

	  // If monitoring is active, process buffer to determine the
	  // max signal strength in both channels. Process every
	  // buffer, but only send the max values every several
	  // buffers -- to avoid over-stressing the UI thread
	  if ( (cachedState & STATE_MONITORING) != 0 ) {
	    bufferSignalStrength( (MLint16*)message[PTR_PARAM_POS].value.pByte,
				  params->numSamplesPerBuffer,
				  &lStrength, &rStrength );

	    if ( (numCaptured % 5) == 0 ) {
	      PostMessage( params->hUIWindow, WMA_MLAC_RSIGNAL_STRENGTH,
			   rStrength, MLINT16_MAX );
	      PostMessage( params->hUIWindow, WMA_MLAC_LSIGNAL_STRENGTH,
			   lStrength, MLINT16_MAX );
	      rStrength = 0;
	      lStrength = 0;
	    }
	  } // if state & STATE_MONITORING

	  // Send the buffer back to the path so another set of
	  // samples can be captured into it.
	  status = mlSendBuffers( params->openPath, message );
	  if ( status != ML_STATUS_NO_ERROR ) {
	    sendErrorMsg( params->hUIWindow,
			  "Can not send ML buffer", status );
	    done = ML_TRUE;
	    break;
	  }
	} break; // case ML_BUFFERS_COMPLETE

	case ML_EVENT_AUDIO_SEQUENCE_LOST: {
	  MLpv* param;
	  MLint32 seqLostTime;

	  // Ignore this message if we aren't capturing (ie: if we are
	  // only monitoring, we don't worry about dropped buffers)
	  if ( (cachedState & STATE_TRANSFERING) != 0 ) {
	    param = mlPvFind( message, ML_AUDIO_UST_INT64 );
	    seqLostTime = (MLint32)((param->value.int64 - startUST)/1000000);
	    ++numDropped;
	    PostMessage( params->hUIWindow, WMA_MLAC_DROPPED_BUFF,
			 (WPARAM) numDropped,
			 (LPARAM) (seqLostTime+accumTime) );
	  }
	} break;

	default:
	  // Ignore message (could issue a warning, I guess)
	  break;

	} // switch messageType
    
      } else {
	// Message from our UI thread. Check for 'STOP' states --
	// these are transitional states: we perform any necessary
	// state-change actions, and reset the state variable to clear
	// both the 'TRANSFER' and 'STOP-TRANSFER' bits (or the
	// equivalent MONITOR bits)
	if ( (curState & STATE_STOP_TRANSFER) != 0 ) {
	  static const int transferMask =
	    ~(STATE_STOP_TRANSFER|STATE_TRANSFERING);

#ifdef DEBUG
	  fprintf( stdout, "State now STOP_TRANSFER\n" );
	  fflush( stdout );
#endif

	  // Update accumulated time etc. for computing capture stats.
	  accumTime += diffTime;
	  accumMSC += diffMSC;
	  diffTime = 0;
	  diffMSC = 0;
	  startUST = 0;
	  startMSC = 0;

	  // Make sure total time is up-to-date
	  PostMessage( params->hUIWindow, WMA_MLAC_TOTAL_TIME,
		       (WPARAM) NULL, (LPARAM) accumTime );

	  // Reset state bits
	  curState &= transferMask;
	}

	if ( (curState & STATE_STOP_MONITOR) != 0 ) {
	  static const int monitorMask =
	    ~(STATE_STOP_MONITOR|STATE_MONITORING);

#ifdef DEBUG
	  fprintf( stdout, "State now STOP_MONITOR\n" );
	  fflush( stdout );
#endif

	  // Send a final 'signal strength' message to set the meters
	  // to zero
	  PostMessage( params->hUIWindow, WMA_MLAC_RSIGNAL_STRENGTH,
		       0, MLINT16_MAX );
	  PostMessage( params->hUIWindow, WMA_MLAC_LSIGNAL_STRENGTH,
		       0, MLINT16_MAX );

	  // Clear the state bits
	  curState &= monitorMask;
	}

	if ( (curState & STATE_FINISHING) != 0  ) {
#ifdef DEBUG
	  fprintf( stdout, "State now FINISHING, setting done = TRUE\n" );
	  fflush( stdout );
#endif
	  done = TRUE;
	}
      } // if ( ML message ) else ( UI message )
    } // while curState != STATE_IDLE && ! done

#ifdef DEBUG
    fprintf( stdout, "Stopping ML transfer\n" );
    fflush( stdout );
#endif

    // No longer need to transfer (not capturing, not monitoring)
    mlEndTransfer( params->openPath );
  } // while ! done

  // Cleanup
  freeTransferThreadParams( params );

  // Finally, let the UI thread know we are exiting.
  SetEvent( replyEvent );
  return 0;
}


// ============================================================================
//
// EXTERNAL FUNCTIONS
//
// ============================================================================


// ----------------------------------------------------------------------------
//
// Get all device IDs for the current system, and build an info
// structure with all valid, usable devices (ie: those that can
// support our audio capture operation).
//
// Returns a pointer to a CurSysInfo  structure if all goes well -- in
// this case, the caller is responsible for freeing the structure when
// done.  Returns NULL  on error  (in which  case getMLerror()  can be
// called to obtain the error message).
const CurSysInfo* getSysInfo( void )
{
  MLpv* caps;
  MLpv* param;
  MLstatus status;
  int numDevs;
  int i;

  CurSysInfo* sysInfo = (CurSysInfo*) malloc( sizeof(CurSysInfo) );
  sysInfo->sysName = NULL;
  sysInfo->numDevs = 0;
  sysInfo->devs = NULL;

  // Get ML info on the current system. Remember to call
  // mlFreeCapabilities() when done!
  status = mlGetCapabilities( ML_SYSTEM_LOCALHOST, &caps );
  if ( status != ML_STATUS_NO_ERROR ) {
    sprintf( errorMsg, "system-level mlGetCapabilities failed:\n%s",
	     mlStatusName( status ) );
    free( sysInfo );
    return NULL;
  }

  // Get system name
  param = mlPvFind( caps, ML_NAME_BYTE_ARRAY );
  if ( param == NULL ) {
    sysInfo->sysName = strdup( "Unknown system" );
  } else {
    sysInfo->sysName = strdup( param->value.pByte );
  }

  // Get list of device IDs
  param = mlPvFind( caps, ML_SYSTEM_DEVICE_IDS_INT64_ARRAY );
  if ( (param == NULL) || (param->length == 0) ) {
    sprintf( errorMsg,
	     "Could not obtain device list, or no devices on this system" );
    mlFreeCapabilities( caps );
    freeSysInfo( sysInfo );
    return NULL;
  }
  numDevs = param->length;

  // Allocate enough space to fit info for all devices. It may be that
  // some devices are not appropriate for an audio capture, in which
  // case they won't be added to the structure -- so not all entries
  // in the array will necessarily be used.
  sysInfo->devs = (DevInfo*) malloc( sizeof(DevInfo)*numDevs );
  memset( sysInfo->devs, 0, sizeof(DevInfo)*numDevs );

  // Iterate over all device IDs, and add those that are relevant.
  for ( i=0; i < numDevs; ++i ) {
    if ( ! addDevAndPaths( sysInfo, param->value.pInt64[i] ) ) {
      mlFreeCapabilities( caps );
      freeSysInfo( sysInfo );
      return NULL;
    }
  }

  mlFreeCapabilities( caps );
  return sysInfo;
}


// ----------------------------------------------------------------------------
//
// Free the CurSysInfo structure. This also frees all the device and
// path structures contained in the sys info.
void freeSysInfo( const CurSysInfo* sysInfo )
{
  int i;

  if ( sysInfo != NULL ) {
    if ( sysInfo->sysName != NULL ) {
      free( (char*)sysInfo->sysName );
    }

    if ( sysInfo->devs != NULL ) {
      for ( i=0; i < sysInfo->numDevs; ++i ) {
	freeDevPathInfo( &(sysInfo->devs[i]) );
      }
      free( (char*)sysInfo->devs );
    }
    free( (char*)sysInfo );
  } // if sysInfo != NULL

  return;
}


// ----------------------------------------------------------------------------
//
// Prepare for an ML transfer. This involves preparing both the output
// file (through libaudiofile functionality) and the ML transfer
// itself.
//
// The output file is opened and prepared for writing.
//
// Then, the ML path is opened and appropriate controls are sent to
// it. The buffers are allocated, and finally, the child thread is
// created. The thread won't actually start transfering until
// startTransfer() is called.
MLboolean prepareTransfer( const TransferParams* params, HWND hwnd )
{
  TransferThreadParams* threadParams = NULL;

  // There should not already by a child thread -- only 1 transfer is
  // permitted at any given time. Call finishTransfer() before
  // starting a brand new one.
  assert( threadHandle == NULL );

  threadParams =
    (TransferThreadParams*) malloc( sizeof(TransferThreadParams) );
  memset( threadParams, 0, sizeof(TransferThreadParams) );
  threadParams->outputFile = AF_NULL_FILEHANDLE;
  threadParams->hUIWindow = hwnd;

  // Open file for writing
  {
    AFfilesetup outputFileSetup = afNewFileSetup();

    if ( outputFileSetup == AF_NULL_FILESETUP ) {
      sprintf( errorMsg, "Could not setup output file" );
      freeTransferThreadParams( threadParams );
      return ML_FALSE;
    }

    afInitChannels( outputFileSetup, AF_DEFAULT_TRACK, params->numChannels );
    afInitFileFormat( outputFileSetup, AF_FILE_WAVE );
    afInitSampleFormat( outputFileSetup, AF_DEFAULT_TRACK,
			AF_SAMPFMT_TWOSCOMP, 16 );
    afInitRate( outputFileSetup, AF_DEFAULT_TRACK, params->rate );
    threadParams->outputFile =
      afOpenFile( params->outFile, "w", outputFileSetup );
    afFreeFileSetup( outputFileSetup );

    if ( threadParams->outputFile == AF_NULL_FILEHANDLE ) {
      sprintf( errorMsg, "Could not open output file '%s'", params->outFile );
      freeTransferThreadParams( threadParams );
      return ML_FALSE;
    }

    threadParams->frameSize =
      (int) afGetFrameSize( threadParams->outputFile, AF_DEFAULT_TRACK, 0 );
  } // 'Open file for writing' scope

  // Setup ML transfer
  {
    MLstatus status;
    int i;
    MLpv controls[5];
    MLint32 events[] = { ML_EVENT_AUDIO_SEQUENCE_LOST };

    // Number of buffers, according to UI settings
    threadParams->numBuffers = params->numBuffers;

    // Set the number of samples per buffer -- this is currently a
    // constant, but could be configured in the UI.
    threadParams->numSamplesPerBuffer = 1024;

    // Calculate size of that buffer in bytes.  This will also be the
    // length passed in for the ML_AUDIO_BUFFER_POINTER param.
    //
    // NOTE: we currently hard-code the size of each sample, to a
    // 16-bit integer. This should eventually be configurable in the
    // UI (to determine whether we want 8-bit, signed or unsigned
    // 16-bit, or floating-point 32-bit samples).
    threadParams->bufferSize = threadParams->numSamplesPerBuffer *
      sizeof(MLint16) * params->numChannels;

    // Allocate the desired number of buffers of the appropriate size
    threadParams->buffers =
      (void**) malloc( sizeof(void*) * threadParams->numBuffers );
    if ( threadParams->buffers == NULL ) {
      sprintf( errorMsg, "Could not allocate buffer memory" );
      freeTransferThreadParams( threadParams );
      return ML_FALSE;
    }
    memset( threadParams->buffers, 0,
	    sizeof(void*) * threadParams->numBuffers );

    for ( i=0; i < threadParams->numBuffers; ++i ) {
      threadParams->buffers[i] = (void*) malloc( threadParams->bufferSize );
      if ( threadParams->buffers[i] == NULL ) {
	sprintf( errorMsg, "Could not allocate buffer memory" );
	freeTransferThreadParams( threadParams );
	return ML_FALSE;
      }
    }

    // Now open and set up the path.
    status = mlOpen( params->pathID, NULL, &(threadParams->openPath) );
    if ( status != ML_STATUS_NO_ERROR ) {
      sprintf( errorMsg, "Could not open ML path:\n%s",
	       mlStatusName( status ) );
      freeTransferThreadParams( threadParams );
      return ML_FALSE;
    }
    
    // Now set the capture attributes. We will expect 16-bit 2's
    // complement. Number of channels and rate as specified in the
    // TransferParams
    controls[0].param = ML_AUDIO_FORMAT_INT32;
    controls[0].value.int32 = ML_AUDIO_FORMAT_S16;
    controls[0].length = 1; /* makes debugging easier */
    controls[1].param = ML_AUDIO_CHANNELS_INT32;
    controls[1].value.int32 = params->numChannels;
    controls[1].length = 1;
    controls[2].param = ML_AUDIO_SAMPLE_RATE_REAL64;
    controls[2].value.real64 = params->rate;
    controls[2].length = 1;

    // Indicate that we are interested in sequence-lost event
    controls[3].param = ML_DEVICE_EVENTS_INT32_ARRAY;
    controls[3].value.pInt32 = events;
    controls[3].length = 1;
    controls[3].maxLength = controls[3].length;

    controls[4].param = ML_END;

    status = mlSetControls( threadParams->openPath, controls );
    if ( status != ML_STATUS_NO_ERROR ) {
      sprintf( errorMsg, "Can not set controls on path:\n%s",
	       mlStatusName( status ) );
      freeTransferThreadParams( threadParams );
      return ML_FALSE;
    }
  } // 'Setup ML transfer' scope
    
  // Create events and thread to handle ML transfer asynchronously
  // from UI thread. Make sure the state is set to 'IDLE' before the
  // thread starts.
  {
    threadEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
    replyEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
    if ( (threadEvent == NULL) || (replyEvent == NULL) ) {
      sprintf( errorMsg, "Can not create thread event" );
      return ML_FALSE;
    }

    curState = STATE_IDLE;

    threadHandle =
      CreateThread( NULL, 0, threadProc, (LPVOID) threadParams, 0, NULL );
    if ( threadHandle == NULL ) {
      sprintf( errorMsg, "Can not create thread" );
      freeTransferThreadParams( threadParams );
      return ML_FALSE;
    }
  } // 'Create events and thread' scope

  return ML_TRUE;
}


// ----------------------------------------------------------------------------
//
MLboolean startTransfer( void )
{
  curState |= STATE_TRANSFERING;
  SetEvent( threadEvent );
  return ML_TRUE;
}


// ----------------------------------------------------------------------------
//
MLboolean stopTransfer( void )
{
  curState |= STATE_STOP_TRANSFER;
  SetEvent( threadEvent );
  return ML_TRUE;
}


// ----------------------------------------------------------------------------
//
MLboolean finishTransfer( void )
{
  stopTransfer();

  // Instruct thread to terminate, and destroy it.
  if ( threadHandle != NULL ) {
    // If the thread was created, then both events were also
    // successfully created, so we can safely use them
    curState |= STATE_FINISHING;
    SetEvent( threadEvent );
    // Wait for thread to finish up -- give it at most 1 second
    WaitForSingleObject( replyEvent, 1000 );
    CloseHandle( threadHandle );
    threadHandle = NULL;
  }

  // Destroy events used to communicate with the thread.
  if ( threadEvent != NULL ) {
    CloseHandle( threadEvent );
    threadEvent = NULL;
  }
  if ( replyEvent != NULL ) {
    CloseHandle( replyEvent );
    replyEvent = NULL;
  }

  return ML_TRUE;
}


// ----------------------------------------------------------------------------
//
MLboolean startMonitoring( void )
{
  curState |= STATE_MONITORING;
  SetEvent( threadEvent );
  return ML_TRUE;
}


// ----------------------------------------------------------------------------
//
MLboolean stopMonitoring( void )
{
  curState |= STATE_STOP_MONITOR;
  SetEvent( threadEvent );
  return ML_TRUE;
}


// ----------------------------------------------------------------------------
//
const char* getMLError( void )
{
  return errorMsg;
}

