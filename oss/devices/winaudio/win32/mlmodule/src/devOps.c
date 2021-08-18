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
#include "devOps.h"
#include "audioParams.h"

#include <math.h>

//-----------------------------------------------------------------------------
//
// Returns 1 if the number of channels is supported by the format
int isNumChannelsSupported( int numChannels, DWORD formats )
{
  static const DWORD stereoFormats =
    WAVE_FORMAT_1S08 | WAVE_FORMAT_1S16 |
    WAVE_FORMAT_2S08 | WAVE_FORMAT_2S16 |
    WAVE_FORMAT_4S08 | WAVE_FORMAT_4S16 |
    WAVE_FORMAT_48S08 | WAVE_FORMAT_48S16 |
    WAVE_FORMAT_96S08 | WAVE_FORMAT_96S16;

  static const DWORD monoFormats =
    WAVE_FORMAT_1M08 | WAVE_FORMAT_1M16 |
    WAVE_FORMAT_2M08 | WAVE_FORMAT_2M16 |
    WAVE_FORMAT_4M08 | WAVE_FORMAT_4M16 |
    WAVE_FORMAT_48M08 | WAVE_FORMAT_48M16 |
    WAVE_FORMAT_96M08 | WAVE_FORMAT_96M16;

  int ret = 0;
  switch ( numChannels ) {
  case 1:
    ret = ((formats & stereoFormats) != 0) ? 1 : 0;
    break;

  case 2:
    ret = ((formats & monoFormats) != 0) ? 1 : 0;
    break;

  default:
    ret = 0; // not mono, not stereo --> definitely not supported!
  break;
  } // switch

  return ret;
}

//-----------------------------------------------------------------------------
//
// Returns 1 if the bit depth (8 or 16, basically) is supported by the
// format
int isBitDepthSupported( int bitDepth, DWORD formats )
{
  static const DWORD eightBitFormats =
    WAVE_FORMAT_1S08 | WAVE_FORMAT_1M08 |
    WAVE_FORMAT_2S08 | WAVE_FORMAT_2M08 |
    WAVE_FORMAT_4S08 | WAVE_FORMAT_4M08 |
    WAVE_FORMAT_48S08 | WAVE_FORMAT_48M08 |
    WAVE_FORMAT_96S08 | WAVE_FORMAT_96M08;

  static const DWORD sixteenBitFormats =
    WAVE_FORMAT_1S16 | WAVE_FORMAT_1M16 |
    WAVE_FORMAT_2S16 | WAVE_FORMAT_2M16 |
    WAVE_FORMAT_4S16 | WAVE_FORMAT_4M16 |
    WAVE_FORMAT_48S16 | WAVE_FORMAT_48M16 |
    WAVE_FORMAT_96S16 | WAVE_FORMAT_96M16;

  int ret = 0;
  switch ( bitDepth ) {
  case 8:
    ret = ((formats & eightBitFormats) != 0) ? 1 : 0;
    break;

  case 16:
    ret = ((formats & sixteenBitFormats) != 0) ? 1 : 0;
    break;

  default:
    ret = 0; // not 8-bit, not 16-bit --> definitely not supported!
  break;
  } // switch

  return ret;
}

//-----------------------------------------------------------------------------
//
// List all valid frame rates supported by the specified
// format. Returned as a pointer to a statically-allocated array (ie:
// do not call free on this pointer!) of integers expressing frame
// rates. The list is sorted from highest to lowest supported frame
// rates, and is zero-terminated.
//
// Note: NOT thread-safe (because of static array). Do not use in
// audio loop thread.
int* getSupportedFrameRates( DWORD formats )
{
  static const DWORD knownFormats[] = {
    WAVE_FORMAT_96S08| WAVE_FORMAT_96M08| WAVE_FORMAT_96S16| WAVE_FORMAT_96M16,
    WAVE_FORMAT_48S08| WAVE_FORMAT_48M08| WAVE_FORMAT_48S16| WAVE_FORMAT_48M16,
    WAVE_FORMAT_4S08 | WAVE_FORMAT_4M08 | WAVE_FORMAT_4S16 | WAVE_FORMAT_4M16,
    WAVE_FORMAT_2S08 | WAVE_FORMAT_2M08 | WAVE_FORMAT_2S16 | WAVE_FORMAT_2M16,
    WAVE_FORMAT_1S08 | WAVE_FORMAT_1M08 | WAVE_FORMAT_1S16 | WAVE_FORMAT_1M16,
    // Note: no define in mmsystem to support 8Khz
    0
  };

  static const int frameRates[] = {
    96000, 48000, 44100, 22050, 11025, 0
  };

  static int retFrameRates[ sizeof( frameRates ) ];

  int numSupported = 0, i;
  for ( i=0; knownFormats[i] != 0; ++i ) {
    assert( frameRates[i] != 0 );

    if ( (formats & knownFormats[i]) != 0 ) {
      // This frame rate is supported
      retFrameRates[ numSupported ] = frameRates[i];
      ++numSupported;
    }
  } // while knownFormats...

  retFrameRates[ numSupported ] = 0;
  return retFrameRates;
}

//-----------------------------------------------------------------------------
//
// Returns 1 if the frame rate (expressed as an integer, ie: 8000,
// 11025, 22050, 44100, etc.) is supported by the format
int isFrameRateSupported( int frameRate, DWORD formats )
{
  int i = 0;
  int ret = 0;
  int* supportedRates = getSupportedFrameRates( formats );

  while ( supportedRates[i] != 0 ) {
    if ( supportedRates[i] == frameRate ) {
      ret = 1;
      break;
    }
  } // while
  return ret;
}

//-----------------------------------------------------------------------------
//
// Turn a WAVE volume setting to an OpenML gain setting.
//
// The volume of 0 is mapped to a gain of -infinity, and volumes of 1
// to 65535 are mapped to gains of -60db to 0db (modelled after
// ossaudio module).
double waveVolumeToOpenMLGain( unsigned int volume )
{
  double gain, normVol;

  if ( volume == 0 ) {
#ifdef _MSC_VER
    // No useful constant with MSVC to denote infinity. Try using
    // HUGE_VAL, although this may not actually be "infinity"
    gain = -HUGE_VAL;
#else
    gain = -INFINITY;
#endif
  } else {
    normVol = (double)(volume - 1); // range 0..65534
    gain = -60.0 + (60.0 * normVol / 65534.0); 
  }

  return gain;
}

//-----------------------------------------------------------------------------
//
// Get the volume from the WAVE device specified in the AudioPath, and
// convert it to a Gain usable in OpenML. This will get both Left and
// Right gains for stereo formats.
//
// We use the volumeControl field of the AudioPath to determine what
// type of volume control is available in this device.
// - if no control, then simply return a gain of zero. Of course, if
//   there is no control, we should never end up here (see FIXME below)
//
// - if single control, return the one value for both left and right
//
// - if independent Left+Right controls, pretty straightforward
//
// FIXME: we should really fix our reporting of capabilities, and stop
// lying about the ability to control AUDIO_GAIN on WAVE devices that
// don't actually allow it (in particular, all input devices).
void getGainFromWAVEDevice( AudioPath* path, double* destArray )
{
  double left = 0.0, right = 0.0;

  // Now get volume from actual WAVE device, if possible
  if ( path->volumeControl != WAVE_VOLUME_NO_CONTROL ) {
    DWORD volume = 0;
    MMRESULT mmStatus;

    // Input device do not support volume... just make sure we're OK
    assert( path->direction == ML_DIRECTION_OUT );
    mmStatus = waveOutGetVolume( path->outHandle, &volume );

    // Assume there won't be errors... if there are, just stop, leave
    // the gains as-is at zero.
    if ( mmStatus != MMSYSERR_NOERROR ) {
      DEBG1( printf( "[winaudio]: waveOutGetVolume returned error %d\n",
		     mmStatus ) );
    } else {
      // As per WAVE docs, low word contains left, high word contains
      // right. If single control, low word contains both.
      left = waveVolumeToOpenMLGain( LOWORD( volume ) );
      if ( path->volumeControl == WAVE_VOLUME_L_R_CONTROL ) {
	right = waveVolumeToOpenMLGain( HIWORD( volume ) );
      } else {
	right = left;
      }
    }
  } // if volumeControl != WAVE_VOLUME_NO_CONTROL

  // Now store results in supplied array -- remember to check if we
  // actually need both results!

  // FIXME: I can't find anything in the spec that says whether left
  // or right is the first channel! Assume left is first.
  destArray[0] = left;

  if ( path->format.nChannels == 2 ) {
    destArray[1] = right;
  }
}

//-----------------------------------------------------------------------------
//
// Turn an OpenML gain into a WAVE volume setting.
//
// The volume of 0 is mapped to a gain of -infinity, and volumes of 1
// to 65535 are mapped to gains of -60db to 0db (modelled after
// ossaudio module).
unsigned int openMLGainToWAVEVolume( double gain )
{
  unsigned int volume;

#ifdef _MSC_VER
  if ( _finite( gain ) == 0 )
#else
  if ( gain == -INFINITY )
#endif
  {
    // Special case: silence (mute). WAVE uses volume of 0 for this.
    volume = 0;
  } else {
    double tmp = gain + 60.0;
    tmp = tmp * 65534.0 / 60.0;
    volume = (unsigned) tmp + 1;
  }

  return volume;
}

//-----------------------------------------------------------------------------
//
// Set the Volume on the WAVE device based on OpenML gain values
//
// If the device does not support setting the volume, this returns
// ML_STATUS_INVALID_PARAMETER
MLstatus setVolumeOnWAVEDevice( AudioPath* path, double* gainArray, int size )
{
  MLstatus status = ML_STATUS_NO_ERROR;

  assert( gainArray != 0 );
  assert( size >= 1 );

  if ( path->volumeControl == WAVE_VOLUME_NO_CONTROL ) {
    // Hmmm, not allowable on this device
    status = ML_STATUS_INVALID_PARAMETER;
  } else {
    DWORD volume;
    unsigned int left = 0, right = 0;
    MMRESULT mmstatus;

    // Assume first entry in the gain array is for the left
    // channel.
    left = openMLGainToWAVEVolume( gainArray[0] );

    // If device support independent left+right volume, use the second
    // entry, if there is one; otherwise, simply re-use the left
    // channel value.
    if ( path->volumeControl == WAVE_VOLUME_L_R_CONTROL ) {
      right = (size > 1) ? openMLGainToWAVEVolume( gainArray[1] ) : left;
    }

    // Make a DWORD out of the left and right (with the left in the
    // low-order word)
    volume = MAKELONG( left, right );

    // Before attempting to read handle from structure, acquire lock
    // to ensure exclusive access. (This may be overly paranoid...)
    EnterCriticalSection( &path->lock );

    // And make sure the handle is still valid (make sure it wasn't
    // closed by the other thread...)
    if ( path->outHandle != 0 ) {
      mmstatus = waveOutSetVolume( path->outHandle, volume );
      if ( mmstatus != MMSYSERR_NOERROR ) {
	DEBG1( printf( "[winaudio]: waveOutSetVolume returned error %d\n",
		       mmstatus ) );
      }
    }

    LeaveCriticalSection( &path->lock );
  }

  return status;
}

//-----------------------------------------------------------------------------
//
// Fill-in the "computed" fields of the WAVEFORMATEX structure. These
// fields depend on other settings within the structure, which should
// therefore be valid.
// The computed fields are:
//  nBlockAligh
//  nAvgBytesPerSec
void computeFormatFields( WAVEFORMATEX* format )
{
  // nBlockAlign is actually the size of a sample (both channels, for
  // stereo formats) in *bytes*. No need to worry about non-integer
  // results, since for now we only support 8- or 16-bit formats.
  format->nBlockAlign = (format->nChannels * format->wBitsPerSample) / 8;
  format->nAvgBytesPerSec = format->nSamplesPerSec * format->nBlockAlign;
}

//-----------------------------------------------------------------------------
//
// Open the WAVE device associated with the path -- assume the device
// is not already open.
// If the inFormat pointer is NULL, a default format will be used.
MLstatus waveDeviceOpen( AudioPath* path, WAVEFORMATEX* inFormat )
{
  MLstatus ret = ML_STATUS_NO_ERROR;
  WAVEFORMATEX waveFormat;
  WAVEFORMATEX* pFormat = 0;
  MMRESULT mmStatus;
  int* supportedRates = 0;

  if ( inFormat == 0 ) {
    // Fill in Wave format structure... at this point, the user has
    // not set any controls, so use reasonable defaults for the
    // variable portions of the structure

    // Use max number of channels supported
    waveFormat.nChannels =
      (isNumChannelsSupported( 2, path->capabilities ) == 1) ? 2 : 1;

    DEBG4( printf( "[winaudio]: waveDeviceOpen: nChannels = %d\n",
		   waveFormat.nChannels ) );

    // Use highest frame rate supported
    supportedRates = getSupportedFrameRates( path->capabilities );
    waveFormat.nSamplesPerSec = (DWORD)supportedRates[0];

    DEBG4( printf( "[winaudio]: waveDeviceOpen: nSamplesPerSec = %d\n",
		   (int)waveFormat.nSamplesPerSec ) );

    // Default to 8-bit if supported.
    waveFormat.wBitsPerSample =
      (isBitDepthSupported( 8, path->capabilities ) == 1) ? 8 : 16;

    DEBG4( printf( "[winaudio]: waveDeviceOpen: wBitsPerSample = %d\n",
		   waveFormat.wBitsPerSample ) );

    // Computed or invariable fields...
    waveFormat.wFormatTag = WAVE_FORMAT_PCM; // Only format supported
    waveFormat.cbSize = 0; // Ignored for PCM formats
    computeFormatFields( &waveFormat );

    // Point to this local structure
    pFormat = &waveFormat;

  } else {
    // Use supplied format. Assume it is valid.
    pFormat = inFormat;
  }

  // We are about to access the structure shared by the application
  // and audio loop threads. In theory, it is difficult to see how the
  // audio loop could be active if the device is not even open, but
  // for extra (thread-) safety, we need to acquire the lock.
  EnterCriticalSection( &path->lock );

  if ( path->direction == ML_DIRECTION_IN ) {
    mmStatus = waveInOpen( &path->inHandle, path->devId, pFormat,
			   (DWORD)path->childId, 0, CALLBACK_THREAD );
  } else {
    mmStatus = waveOutOpen( &path->outHandle, path->devId, pFormat,
			   (DWORD)path->childId, 0, CALLBACK_THREAD );
  }

  if ( mmStatus == MMSYSERR_NOERROR ) {
    // Copy WAVEFORMATEX over to AudioPath
    path->format = *pFormat;
  } else {
    DEBG1( printf( "[winaudio]: error (%d) opening Wave device:\n",
		   mmStatus ) );

    switch ( mmStatus ) {
    case MMSYSERR_ALLOCATED:
      DEBG1( printf( "\tMMSYSERR_ALLOCATED\n" ) );
      ret = ML_STATUS_DEVICE_BUSY;
      break;

    case MMSYSERR_BADDEVICEID:
      DEBG1( printf( "\tMMSYSERR_BADDEVICEID\n" ) );
      // This is an internal error -- we should never have come up
      // with this ID in the first place!
      ret = ML_STATUS_INTERNAL_ERROR;
      break;

    case MMSYSERR_NODRIVER:
      DEBG1( printf( "\tMMSYSERR_NODRIVER\n" ) );
      ret = ML_STATUS_DEVICE_ERROR;
      break;

    case MMSYSERR_NOMEM:
      DEBG1( printf( "\tMMSYSERR_NOMEM\n" ) );
      ret = ML_STATUS_OUT_OF_MEMORY;
      break;

    case WAVERR_BADFORMAT:
      DEBG1( printf( "\tWAVERR_BADFORMAT\n" ) );
      // What to do? We could go back and try a different
      // format... But how to know which part of the format is
      // invalid?
      ret = ML_STATUS_DEVICE_ERROR;
      break;

    case WAVERR_SYNC:
      DEBG1( printf( "\tWAVERR_SYNC\n" ) );
      ret = ML_STATUS_DEVICE_ERROR;
      break;

    default:
      // Hmmm?
      DEBG1( printf( "\tUnexpected (undocumented) error\n" ) );
      ret = ML_STATUS_INTERNAL_ERROR;
      break;
    } // switch
  } // if mmStatus != MMSYSERR_NOERROR

  // No buffers sent to the device yet, and no SEQUENCE_LOST in sight...
  path->numOutstandingBuffers = 0;

  // Remember to release the lock!!!
  LeaveCriticalSection( &path->lock );

  return ret;
}

//-----------------------------------------------------------------------------
//
// This attempts to open the WAVE device specified by the path (the
// device must not already be open), but only in "query" mode -- ie:
// the device is not actually opened, it is only a simulation to see
// if the device driver will allow the format.
//
// Returns 1 if the query succeeded, ie: the format is valid, 0
// otherwise (which either means the format is invalid, or some
// unspecified error occured).
int waveDeviceVerifyFormat( AudioPath* path, WAVEFORMATEX* format )
{
  int openOK = 0;
  MMRESULT mmStatus;

  // No need to acquire lock, since we are not modifying -- or even
  // reading -- any volatile part of the AudioPath structure.

  if ( path->direction == ML_DIRECTION_IN ) {
    HWAVEIN dummyHandle;
    mmStatus = waveInOpen( &dummyHandle, path->devId, format,
			   0, 0, WAVE_FORMAT_QUERY );
  } else {
    HWAVEOUT dummyHandle;
    mmStatus = waveOutOpen( &dummyHandle, path->devId, format,
			   0, 0, WAVE_FORMAT_QUERY );
  }

  if ( mmStatus == MMSYSERR_NOERROR ) {
    // This format seems OK
    openOK = 1;
  } else {
    DEBG1( printf( "[winaudio]: waveDeviceVerifyFormat: "
		   "error (%d) opening Wave device:\n", mmStatus ) );

    switch ( mmStatus ) {
    case MMSYSERR_ALLOCATED:
      DEBG1( printf( "\tMMSYSERR_ALLOCATED\n" ) );
      break;

    case MMSYSERR_BADDEVICEID:
      DEBG1( printf( "\tMMSYSERR_BADDEVICEID\n" ) );
      break;

    case MMSYSERR_NODRIVER:
      DEBG1( printf( "\tMMSYSERR_NODRIVER\n" ) );
      break;

    case MMSYSERR_NOMEM:
      DEBG1( printf( "\tMMSYSERR_NOMEM\n" ) );
      break;

    case WAVERR_BADFORMAT:
      DEBG1( printf( "\tWAVERR_BADFORMAT\n" ) );
      break;

    case WAVERR_SYNC:
      DEBG1( printf( "\tWAVERR_SYNC\n" ) );
      break;

    default:
      DEBG1( printf( "\tUnexpected (undocumented) error\n" ) );
      break;
    } // switch
  } // if mmStatus != MMSYSERR_NOERROR

  return openOK;
}


//-----------------------------------------------------------------------------
//
// Reset and close the WAVE device.
// Returns ML_STATUS_NO_ERROR, ML_STATUS_OUT_OF_MEMORY or
// ML_STATUS_INTERNAL_ERROR
MLstatus waveDeviceClose( AudioPath* path )
{
  MLstatus status = ML_STATUS_NO_ERROR;
  MMRESULT mmStatus = 0;

  // Acquire lock, to ensure we have exclusive access to the AudioPath
  // structure.
  EnterCriticalSection( &path->lock );

  if ( path->direction == ML_DIRECTION_IN ) {
    waveInReset( path->inHandle );
    mmStatus = waveInClose( path->inHandle );
    path->inHandle = 0;

  } else {
    waveOutReset( path->outHandle );
    mmStatus = waveOutClose( path->outHandle );
    path->outHandle = 0;
  }

  LeaveCriticalSection( &path->lock );

  if ( mmStatus != MMSYSERR_NOERROR ) {
    DEBG1( printf( "[winaudio]: waveDeviceClose: "
		   "error (%d) closing Wave device:\n", mmStatus ) );

    switch ( mmStatus ) {

    case MMSYSERR_INVALHANDLE:
      DEBG1( printf( "\tMMSYSERR_INVALHANDLE\n" ) );
      status = ML_STATUS_INTERNAL_ERROR;
      break;

    case MMSYSERR_NODRIVER:
      DEBG1( printf( "\tMMSYSERR_NODRIVER\n" ) );
      status = ML_STATUS_INTERNAL_ERROR;
      break;

    case MMSYSERR_NOMEM:
      DEBG1( printf( "\tMMSYSERR_NOMEM\n" ) );
      status = ML_STATUS_OUT_OF_MEMORY;
      break;

    case WAVERR_STILLPLAYING:
      DEBG1( printf( "\tWAVERR_STILLPLAYING\n" ) );
      status = ML_STATUS_INTERNAL_ERROR;
      break;

    default:
      DEBG1( printf( "\tUnexpected (undocumented) error\n" ) );
      status = ML_STATUS_INTERNAL_ERROR;
      break;
    } // switch mmStatus
  } // if

  return status;
}

//-----------------------------------------------------------------------------
//
MLstatus getAudioPathControls( AudioPath* path, MLpv *msg )
{
  MLstatus status = ML_STATUS_NO_ERROR;
  WAVEFORMATEX format;
  int i;

  for ( i=0; (msg[i].param != ML_END) && (status == ML_STATUS_NO_ERROR); ++i ){

    switch ( msg[i].param ) {

    case ML_QUEUE_SEND_COUNT_INT32:
      msg[i].value.int32 = mlDIQueueGetSendCount( path->pQueue );
      DEBG4( printf( "\tQUEUE_SEND_COUNT = %d\n", msg[i].value.int32 ) );
      break;

    case ML_QUEUE_RECEIVE_COUNT_INT32:
      msg[i].value.int32 = mlDIQueueGetReceiveCount( path->pQueue );
      DEBG4( printf( "\tQUEUE_RECEIVE_COUNT = %d\n", msg[i].value.int32 ) );
      break;

    case ML_QUEUE_SEND_WAITABLE_INT64:
      // How to cast the HANDLE (a pointer...) to an int64 without
      // warning...
      msg[i].value.int64 =
	(MLint64) (UINT_PTR) mlDIQueueGetSendWaitable( path->pQueue );
      DEBG4( printf( "\treturned QUEUE_SEND_WAITABLE\n" ) );
      break;

    case ML_QUEUE_RECEIVE_WAITABLE_INT64:
      msg[i].value.int64 =
	(MLint64) (UINT_PTR) mlDIQueueGetReceiveWaitable( path->pQueue );
      DEBG4( printf( "\treturned QUEUE_RECEIVE_WAITABLE\n" ) );
      break;

    case ML_AUDIO_FRAME_SIZE_INT32:
      // Can use nBlockAlign from WAVEFORMATEX structure directly --
      // that is already expressed in bytes.
      // But remember to obtain the format in a thread-safe manner.
      getWaveFormatEx( path, &format );
      msg[i].value.int32 = format.nBlockAlign;
      DEBG4( printf( "\tAUDIO_FRAME_SIZE = %d\n", msg[i].value.int32 ) );
      break;

    case ML_AUDIO_COMPRESSION_INT32:
      // For now, we only support un-compressed audio data...
      msg[i].value.int32 = ML_COMPRESSION_UNCOMPRESSED;
      DEBG4( printf( "\tAUDIO_COMPRESSION = %d\n", msg[i].value.int32 ) );
      break;

    case ML_AUDIO_FORMAT_INT32:
      // Look at wBitsPerSample in WAVEFORMATEX structure -- if 8,
      // then U8, if 16, then S16 -- the only 2 formats supported
      // currently.
      // Remember to obtain the format in a thread-safe manner.
      getWaveFormatEx( path, &format );
      msg[i].value.int32 = (format.wBitsPerSample == 8) ?
	ML_AUDIO_FORMAT_U8 : ML_AUDIO_FORMAT_S16;
      DEBG4( printf( "\tAUDIO_FORMAT = %d\n", msg[i].value.int32 ) );
      break;

    case ML_AUDIO_CHANNELS_INT32:
      // Available directly from nChannels in WAVEFORMATEX structure
      // Remember to obtain the format in a thread-safe manner.
      getWaveFormatEx( path, &format );
      msg[i].value.int32 = format.nChannels;
      DEBG4( printf( "\tAUDIO_CHANNELS = %d\n", msg[i].value.int32 ) );
      break;

    case ML_AUDIO_SAMPLE_RATE_REAL64:
      // Available from nSamplesPerSec in WAVEFORMATEX -- but must
      // convert DWORD to a double.
      // Remember to obtain the format in a thread-safe manner.
      getWaveFormatEx( path, &format );
      msg[i].value.real64 = (double) format.nSamplesPerSec;
      DEBG4( printf( "\tAUDIO_SAMPLE_RATE = %f\n",
		     (float) msg[i].value.real64 ) );
      break;

    case ML_AUDIO_GAINS_REAL64_ARRAY:
      // First, check if the request is actually to find out how big
      // of an array is required (as per spec, this happens when array
      // size is zero).

      // Remember to obtain the format in a thread-safe manner.
      getWaveFormatEx( path, &format );

      if ( msg[i].maxLength == 0 ) {
	// Required array size is the number of channels
	msg[i].maxLength = format.nChannels;
	msg[i].length = 0;
	DEBG4( printf( "\tAUDIO_GAINS array size = %d\n", msg[i].maxLength ) );
	break;
      }

      // Check space is properly allocated for return value
      if ( (msg[i].maxLength < format.nChannels) ||
	   (msg[i].value.pReal64 == 0) ) {
	DEBG1( printf( "[winaudio]: error getting AUDIO_GAINS control: "
		       "insufficient space allocated for result\n" ) );
	msg[i].length = -1;
	// Note: should we return ML_STATUS_INVALID_PARAMETER instead?
	// But we recognised the param... the spec should perhaps
	// reflect this error condition?
	status = ML_STATUS_INVALID_VALUE;
	break;
      }

      // Set length of array to be written
      msg[i].length = format.nChannels;

      getGainFromWAVEDevice( path, msg[i].value.pReal64 );
      DEBG4( printf( "\tAUDIO_GAINS first value = %f\n",
		     (float) msg[i].value.pReal64[0] ) );
      break;

    case ML_DEVICE_EVENTS_INT32_ARRAY: {
      // FIXME: for now we only support SEQUENCE_LOST events

      // Remember to obtain the current "events wanted" in a
      // thread-safe manner.
      int curEvents = getEventsWanted( path );

      // Determine how many events are currently requested
      int numEvents = 0, eventNo;
      if ( (curEvents & WAVE_SEQUENCE_LOST_EVENT) != 0 ) {
	++numEvents;
      }

      // First, check if the request is actually to find out how big
      // of an array is required (as per spec, this happens when array
      // size is zero).
      if ( msg[i].maxLength == 0 ) {
	// Required size is number of events currently selected
	msg[i].maxLength = numEvents;
	msg[i].length = 0;
	DEBG4( printf( "\tDEVICE_EVENTS array size = %d\n",
		       msg[i].maxLength ) );
	break;
      }

      // Check space is properly allocated for return value
      if ( (msg[i].maxLength < numEvents) || (msg[i].value.pInt32 == 0) ) {
	DEBG1( printf( "[winaudio]: error getting DEVICE_EVENTS control: "
		       "insufficient space allocated for result\n" ) );
	msg[i].length = -1;
	// Note: should we return ML_STATUS_INVALID_PARAMETER instead?
	// But we recognised the param... the spec should perhaps
	// reflect this error condition?
	status = ML_STATUS_INVALID_VALUE;
	break;
      }

      // Now go through list of valid events, check which are
      // requested
      eventNo = 0;
      if ( curEvents & WAVE_SEQUENCE_LOST_EVENT ) {
	msg[i].value.pInt32[ eventNo ] = ML_EVENT_AUDIO_SEQUENCE_LOST;
	++eventNo;
      }

      // Check that we found the same number of events as above
      assert( eventNo == numEvents );
      msg[i].length = eventNo;

      DEBG4( printf( "\tDEVICE_EVENTS first value = %d\n",
		     msg[i].value.pInt32[0] ) );
      } break;

    default:
      // Ignore user-defined controls -- anything else is an error
      if ( ML_PARAM_GET_CLASS( msg[i].param ) != ML_CLASS_USER ) {
	// Mark this param as invalid
	DEBG2( printf( "[winaudio]: Invalid param '%" FORMAT_LLD
		       "' ('0x%" FORMAT_LLX "') in ddGetControls\n",
		       msg[i].param, msg[i].param ) );
	msg[i].length = -1;
	status = ML_STATUS_INVALID_PARAMETER;
      } else {
	DEBG4( printf( "\tignoring USER_CLASS control request\n" ) );
      }
      break;

    } // switch msg[i].param

  } // for i=0...ML_END

  return status;
}

//-----------------------------------------------------------------------------
//
MLstatus setAudioPathControls( AudioPath* path, MLpv* msg )
{
  MLstatus status = ML_STATUS_NO_ERROR;
  int sampleRateIndex = -1, channelsIndex = -1, formatIndex = -1;
  int needReopen = 0;
  int setNewEvents = 0; // If 1, then need to set new events
  MLint32 newEvents = WAVE_NO_EVENTS;
  double* volumeRequest = 0;
  int volumeRequestSize = 0;
  int i;

  DEBG3( printf( "\tsetAudioPathControls:\n" ) );

  for ( i=0; msg[i].param != ML_END; ++i ) {

    // Start by validating the value of this part of the message, ie:
    // check that the enum is one of the enums we allow
    if ( validateParamValue( &msg[i] ) == 0 ) {
      // Ooops, not valid!
      msg[i].length = -1;
      status = ML_STATUS_INVALID_VALUE;
      break;
    }

    switch ( msg[i].param ) {

    case ML_AUDIO_COMPRESSION_INT32:
      // Nothing to do -- there is only 1 type of compression for now.
      // We could however check that we aren't in the middle of a
      // transfer, to be consistent with other params...
      break;

    case ML_AUDIO_CHANNELS_INT32:
      channelsIndex = i;
      needReopen = 1;
      break;

    case ML_AUDIO_SAMPLE_RATE_REAL64:
      sampleRateIndex = i;
      needReopen = 1;
      break;

    case ML_AUDIO_FORMAT_INT32:
      formatIndex = i;
      needReopen = 1;
      break;

    case ML_DEVICE_EVENTS_INT32_ARRAY: {
      // Determine which events are requested (make sure they are
      // valid events), but don't set them yet, in order to ensure
      // atomicity of the entire setControls message (according to the
      // spec, either the entire message is successful, or none of it
      // is).

      // This is easy to defer to the very end, since setting new
      // events can not fail -- so it can not cause us to back out of
      // any other change.

      int j;
      setNewEvents = 1; // flag that we need to set new events

      for ( j=0; j < msg[i].length; ++j ) {
	switch ( msg[i].value.pInt32[j] ) {

	case ML_EVENT_AUDIO_SEQUENCE_LOST:
	  newEvents |= WAVE_SEQUENCE_LOST_EVENT;
	  break;

	case ML_EVENT_AUDIO_SAMPLE_RATE_CHANGED:
	  newEvents |= RATE_CHANGED_EVENT;

	default:
	  // This is not one of the supported events, flag an error
	  msg[i].length = -1;
	  status = ML_STATUS_INVALID_VALUE;
	  DEBG1( printf( "\tsetControls: invalid event for "
			 "ML_DEVICE_EVENTS_INT32_ARRAY\n" ) );
	  break;
	} // switch
      } // for j=0...

    } break;

    case ML_AUDIO_GAINS_REAL64_ARRAY:
      // Here, simply determine if setting the volume is valid. The
      // actual setting will be done later, when we are certain that
      // no other part of the message has caused an error (to ensure
      // atomicity of the request)

      if ( path->volumeControl == WAVE_VOLUME_NO_CONTROL ) {
	// Hmmm, not allowable on this device
	status = ML_STATUS_INVALID_PARAMETER;

	msg[i].length = -1;
	DEBG1( printf( "\tsetControls: invalid parameter "
		       "ML_AUDIO_GAINS_REAL64_ARRAY\n" ) );
      } else {
	// Keep track of volume request
	volumeRequest = msg[i].value.pReal64;
	volumeRequestSize = msg[i].length;
      }
      break;

    default:
      if ( ML_PARAM_GET_CLASS( msg[i].param ) != ML_CLASS_USER ) {
	msg[i].length = -1;
	status = ML_STATUS_INVALID_PARAMETER;
	DEBG1( printf( "\tsetControls: invalid parameter %" FORMAT_LLX
		       "\n", msg[i].param ) );
      }
      break;

    } // switch msg[i].param

    if ( status != ML_STATUS_NO_ERROR ) {
      break;
    }
  } // for i=0...msg[i].param!=ML_END

  // Now check if there is anything left to do -- some controls
  // require re-opening the device, and that was deferred until we
  // determined everything that needed to be done.
  if ( (status == ML_STATUS_NO_ERROR) && (needReopen != 0) ) {

    // We are about to close & re-open the device, and change the
    // Audio Path structure -- this requires locking, to ensure
    // all changes are made atomically.
    EnterCriticalSection( &path->lock );

    // Can only do this if no transfer is in progress (can't close a
    // device while it is transferring). Also make sure the handle is
    // still valid -- just in case another thread closed it.

    // Note 1: We don't actually need to check for a transfer in
    // progress, since by the time we get here, the request message
    // has been validated -- which includes verifying that only
    // parameters with DURING_TRANSFER access controls are present
    // when a transfer is in progress. But we might as well
    // double-check...

    // Note 2: We have acquired the lock already, so it is OK to access
    // the AudioPath fields directly

    // Note 3: We look at the inHandle only, which is technically not
    // valid if this is an output path. But the outHandle occupies the
    // same field as the inHandle (it is a union), and we can safely
    // assume they are of the same underlying data type.
    if ( (path->inHandle == 0) ||
	 (path->state == ML_DEVICE_STATE_TRANSFERRING) ) {
      // Flag this as an INVALID_PARAMETER error. Flag all relevant
      // params in the msg -- the spec says the first param is
      // flagged, but it doesn't actually say none of the others are,
      // so this is OK (I think).
      if ( sampleRateIndex != -1 ) {
	msg[ sampleRateIndex ].length = -1;
      }
      if ( channelsIndex != -1 ) {
	msg[ channelsIndex ].length = -1;
      }
      if ( formatIndex != -1 ) {
	msg[ formatIndex ].length = -1;
      }

      status = ML_STATUS_INVALID_PARAMETER;

      DEBG1( printf( "\tsetControls: invalid handle (0x%x) "
		     "or transfer in progress (state = %d)\n",
		     (unsigned int)path->inHandle, path->state ) );

    } else {
      // OK to modify device params. Start by obtaining current device
      // format.
      WAVEFORMATEX format = path->format;
      MLstatus openStatus;

      // Now close device.
      // Note: this function also enters the critical section, but
      // that is OK -- a thread can not deadlock itself.
      waveDeviceClose( path );

      // Construct a new WAVEFORMATEX, based on the current one. Make
      // the changes incrementally, and check each time that the
      // change is valid -- this allows us to determine which
      // parameter (if any) is invalid.

      // Note: we try all params, even if we have already found one
      // which is not valid. This allows us to flag all invalid values
      // in the message -- and in particular, ensures that we have
      // flagged the *first* one.

      if ( sampleRateIndex != -1 ) {
	int changeOK;
	format.nSamplesPerSec = (DWORD)msg[ sampleRateIndex ].value.real64;
	computeFormatFields( &format );

	DEBG4( printf( "\tsetControls: setting AUDIO_SAMPLE_RATE to %f\n",
		       (float) msg[ sampleRateIndex ].value.real64 ) );
	changeOK = waveDeviceVerifyFormat( path, &format );
	if ( changeOK == 0 ) {
	  // Flag this as an invalid value
	  msg[ sampleRateIndex ].length = -1;
	  status = ML_STATUS_INVALID_VALUE;
	  DEBG1( printf( "\tsetControls: sample rate %f invalid\n",
			 (float) msg[ sampleRateIndex ].value.real64 ) );
	}
      } // if sampleRateIndex != -1

      if ( channelsIndex != -1 ) {
	int changeOK;
	format.nChannels =
	  (msg[ channelsIndex ].value.int32 == ML_CHANNELS_STEREO) ? 2 : 1;
	computeFormatFields( &format );

	DEBG4( printf( "\tsetControls: setting AUDIO_CHANNELS to %d "
		       "(%d channels)\n",
		       msg[ channelsIndex ].value.int32,
		       format.nChannels ) );
	changeOK = waveDeviceVerifyFormat( path, &format );
	if ( changeOK == 0 ) {
	  // Flag this as an invalid value
	  msg[ channelsIndex ].length = -1;
	  status = ML_STATUS_INVALID_VALUE;
	  DEBG1( printf( "\tsetControls: channel setting %d invalid\n",
			 msg[ channelsIndex ].value.int32 ) );
	}
      } // if channelsIndex != -1

      if ( formatIndex != -1 ) {
	int changeOK;
	format.wBitsPerSample =
	  (msg[ formatIndex ].value.int32 == ML_AUDIO_FORMAT_U8) ? 8 : 16;
	computeFormatFields( &format );

	DEBG4( printf( "\tsetControls: setting AUDIO_FORMAT to %d "
		       "(%d bits)\n",
		       msg[ formatIndex ].value.int32,
		       format.wBitsPerSample ) );
	changeOK = waveDeviceVerifyFormat( path, &format );
	if ( changeOK == 0 ) {
	  // Flag this as an invalid value
	  msg[ formatIndex ].length = -1;
	  status = ML_STATUS_INVALID_VALUE;
	  DEBG1( printf( "\tsetControls: format setting %d invalid\n",
			 msg[ formatIndex ].value.int32 ) );
	}
      } // if channelsIndex != -1

      // If there were no errors, re-open the device with the new
      // format. Otherwise (if one of the changes was invalid),
      // attempt to re-open the device with the old format.
      openStatus = waveDeviceOpen( path,
				   (status == ML_STATUS_NO_ERROR) ?
				   &format : &path->format );

      if ( openStatus != ML_STATUS_NO_ERROR ) {
	// In this case, whatever error occured overrides any possible
	// INVALID_VALUE error that we may have found.
	status = openStatus;
      }

    } // if path->inHandle==0 || ... else ...

    // Finally, remember to release lock on the Audio Path!
    LeaveCriticalSection( &path->lock );
  }

  // If all is still well, set new events and volume, if appropriate.
  // Neither of these 2 calls can fail (if there were any potential
  // error conditions, they have already been checked), so we will not
  // need to back-out of any of the previous changes.
  if ( status == ML_STATUS_NO_ERROR ) {
    if ( setNewEvents != 0 ) {
      setEventsWanted( path, newEvents );
    }

    if ( volumeRequest != 0 ) {
      setVolumeOnWAVEDevice( path, volumeRequest, volumeRequestSize );
    }
  }

  return status;
}

