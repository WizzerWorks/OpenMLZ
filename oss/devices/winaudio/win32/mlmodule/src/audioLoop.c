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
#include "audioLoop.h"
#include "devOps.h"
#include "generic.h"

//-----------------------------------------------------------------------------
//
// Compile-time option:
//
// We can compute the UST associated with a buffer at 2 different
// points in time:
//
//  1 when the buffer is received by the module, just before it is
//    sent on to the WAVE device. This is somewhat analogous to the
//    way things are done in ossaudio. The problem is that the WAVE
//    device implements "un-limited" buffering (as many buffers as we
//    wish to pass to it), so that the latency between us sending a
//    buffer and it actually going through the jack is highly
//    variable. This means that there is actually little correlation
//    between the UST and the actual jack time of the samples.
//
//  2 when the buffer is returned by the device, after it has been
//    processed. In this case, there is no buffering -- we simply must
//    contend with the latency of the messaging process by which the
//    buffer is returned. This latency is variable, but likely less so
//    that the latency described above, so this method of measuring
//    UST may provide more accurate results. However, the spec
//    requires that the reported time be that of the beginning of the
//    first sample of the buffer. Since we obtain a time close to the
//    end of the buffer, we must do some math to work back to the time
//    of the start of the buffer.
//
// It isn't clear at this point which method provides better results
// (in any case, neither of the methods provides true spec-compliant
// results). So both methods are coded, and the following macro
// selects one of them.
//
// MSC determination is not as complex: since we keep track of all MSC
// slots within this module (with no help from the WAVE device), there
// is no element of uncertainty: there is exactly 1 MSC slot per
// sample sent to the device. Thus, when we receive a new buffer, we
// know exactly what the MSC value is for its first sample -- and we
// can simply record it right then.
//
// The difficult with MSC is when we experience buffer underflow (ie:
// SEQUENCE_LOST) situations. In this case, we are no longer sending
// samples to the device, so we are no longer updating the MSC. To
// work around this problem (the spec states that the MSC must
// continue to be updated even when no data is being transferred), we
// send dummy buffers (filled with silence) to the device. This allows
// us to continue incrementing the MSC.
//
// Comment out this line to use method #1, un-comment it for method #2.
#define UST_WHEN_BUFFER_COMPLETE

//-----------------------------------------------------------------------------
//
// Wave header entry -- contains the WAVEHDR structure and information
// required by our own algorithm.
//
// Note that this stores a pointer to the outstanding message in the
// DI queue (so that when the buffer has been processed by the WAVE
// device, we are able to complete processing of the queue message).
// For input buffers, it is also necessary to store a pointer to the
// portion of the send-buffers message that contains the size
// parameter, so that we can write the number of bytes read in from
// the device. (This is not required for output buffers, so in that
// case, the pointers will remain 0).
typedef struct _WaveHdrEntry {
  MLqueueEntry* qEntry; // Pointer to queue entry -- if 0, this record
                        // is not in use
  WAVEHDR waveHdr;      // WAVE header, sent to device
  MLint32* msgLengthValue;  // Pointer to length field in MLpv message
                            // Used only for input buffers (0 otherwise)

  // The following two fields are only required if we are computing
  // UST / MSC after the buffer is complete (see comments above) -- in
  // this case, we need to keep track of the part of the application
  // message that is to contain the timestamps.
#ifdef UST_WHEN_BUFFER_COMPLETE
  MLint64* msgUSTValue; // Pointer to UST field in MLpv message
                        // Used only if message requested UST (0 otherwise)
#endif
} WaveHdrEntry;

//-----------------------------------------------------------------------------
//
// Circular queue of WaveHdrEntry structures -- these structures are
// used to wrap the buffers sent to us by the application before we
// pass them on to the WAVE device. For each entry in the circular
// queue that is "in use", there is an outstanding mlSendBuffers()
// message in the DI queue.
typedef struct _WaveHdrPool {
  int size;              // Number of headers in pool
  int inUse;             // Number of headers currently in use
  int nextAvail;         // Index of next available header
  WaveHdrEntry* headers; // Array of header structures (see above)
} WaveHdrPool;

//-----------------------------------------------------------------------------
//
// Allocate a Wave header pool and init the structure.
//
// The pool is sized according to the DI queue in the AudioPath: since
// there is always 1 outstanding sendBuffers msg in the DI queue for
// every WAVEHDR in use, there is no need to make the pool larger than
// the queue.
//
// The pool pointer must point to a pre-allocated structure. This
// function only allocates the space for the headers themselves (and
// for the "inUse" array)
void allocateWaveHdrPool( AudioPath* path, WaveHdrPool* pool )
{
  int i;

  assert( path != 0 );
  assert( pool != 0 );

  pool->size = path->pQueue->sendMaxCount;
  DEBG4( printf( "[winaudio audio loop]: allocating pool of %d headers\n",
		 pool->size ) );

  pool->headers = (WaveHdrEntry*) malloc( sizeof(WaveHdrEntry) * pool->size );
  assert( pool->headers != 0 ); // No graceful recovery here...

  // Init the array... we use the dwUser field of the wave header to
  // store the index number of the entry (to help us locate that entry
  // inside the array). Set all entries to "not in use", by clearing
  // the queue entry pointers.
  for ( i=0; i < pool->size; ++i ) {
    pool->headers[i].waveHdr.dwUser = (DWORD) i;
    pool->headers[i].qEntry = 0;
  }

  // Init the "next available entry" field
  pool->nextAvail = 0;
  pool->inUse = 0;
}

//-----------------------------------------------------------------------------
//
// Free the wave header pool
void freeWaveHdrPool( WaveHdrPool* pool )
{
  // First make sure no entry is in use
  assert( pool->inUse == 0 );

  free( pool->headers );
}

//-----------------------------------------------------------------------------
//
// Get the next available wave header from the pool, and mark it as
// "in use" by the specified queue entry.
//
// The "lenPtr" may be 0, if the header is for an output buffer,
// otherwise it should be a pointer to the 
WAVEHDR* getNextWaveHdr( WaveHdrPool* pool, MLqueueEntry* qEntry )
{
  WAVEHDR* ret;

  // Get next available entry
  WaveHdrEntry* entry = &(pool->headers[ pool->nextAvail ]);

  // Make sure the "next available entry" really is available.
  assert( entry->qEntry == 0 );

  // Mark entry as "in-use"
  entry->qEntry = qEntry;

  // Init optional fields to zero
  entry->msgLengthValue = 0;
#ifdef UST_WHEN_BUFFER_COMPLETE
  entry->msgUSTValue = 0;
#endif

  ret = &(entry->waveHdr);

  // Increment "next available" index
  ++pool->inUse;
  ++pool->nextAvail;
  if ( pool->nextAvail == pool->size ) {
    pool->nextAvail = 0;
  }

  return ret;
}

//-----------------------------------------------------------------------------
//
// Set the msgLengthValue pointer associated with the specified
// WAVEHDR object. This pointer is used to access the "msg.length"
// field of the ML_AUDIO_BUFFER_POINTER portion of the application's
// "send-buffers" message.
//
// This is only useful for input buffers, when the module must inform
// the application how many bytes have been read into the buffer
// (which can only be done after the buffer has been processed).
void setMsgLengthValue( WaveHdrPool* pool, WAVEHDR* waveHdr,
			MLint32* msgLengthValue )
{
  int idx = (int) waveHdr->dwUser;

  // Sanity-checking
  assert( &(pool->headers[ idx ].waveHdr) == waveHdr );

  pool->headers[ idx ].msgLengthValue = msgLengthValue;
}

//-----------------------------------------------------------------------------
//
// Get the msgLengthValue pointer associated with the specified
// WAVEHDR object.
//
// This pointer is only valid for input buffers; if this function is
// called for an output buffer, the pointer returned will be zero.
//
// This function should not be called after the header has been
// released to the pool.
MLint32* getMsgLengthValue( WaveHdrPool* pool, WAVEHDR* waveHdr )
{
  int idx = (int) waveHdr->dwUser;

  // Sanity-checking
  assert( &(pool->headers[ idx ].waveHdr) == waveHdr );

  return pool->headers[ idx ].msgLengthValue;
}

#ifdef UST_WHEN_BUFFER_COMPLETE
//-----------------------------------------------------------------------------
//
// Set the msgUSTValue pointer associated with the specified WAVEHDR
// object.
//
// Only required if the application requested UST in its message.
//
// This function only needs to be defined if we are measuring UST upon
// buffer completion. See comments at top of file.
void setMsgUSTValue( WaveHdrPool* pool, WAVEHDR* waveHdr,
		     MLint64* msgUSTValue )
{
  int idx = (int) waveHdr->dwUser;

  // Sanity-checking
  assert( &(pool->headers[ idx ].waveHdr) == waveHdr );

  pool->headers[ idx ].msgUSTValue = msgUSTValue;
}

//-----------------------------------------------------------------------------
//
// Get the msgUSTValue pointer associated with the specified WAVEHDR
// object.
//
// The pointer may be zero, if the application did not request the
// timestamp in its send-buffers message.
//
// This functions should not be called after the header has been
// released to the pool.
MLint64* getMsgUSTValue( WaveHdrPool* pool, WAVEHDR* waveHdr )
{
  int idx = (int) waveHdr->dwUser;

  // Sanity-checking
  assert( &(pool->headers[ idx ].waveHdr) == waveHdr );

  return pool->headers[ idx ].msgUSTValue;
}
#endif

//-----------------------------------------------------------------------------
//
// Release the wave header -- ie: return it to the free pool -- and
// return the queue entry that was associated with it.
MLqueueEntry* releaseWaveHdr( WaveHdrPool* pool, WAVEHDR* waveHdr )
{
  MLqueueEntry* ret;
  int idx = (int) waveHdr->dwUser;

  // Sanity-checking
  assert( &(pool->headers[ idx ].waveHdr) == waveHdr );

  // Get queue entry associated with this header, and make sure the
  // header really was "in use"
  ret = pool->headers[ idx ].qEntry;
  assert( ret != 0 );

  // Mark as no longer in use
  pool->headers[ idx ].qEntry = 0;
  --pool->inUse;

  return ret;
}


//-----------------------------------------------------------------------------
//
// Compute the number of MSC slots used by the specified buffer. This
// is computed based on the length (duration) of the buffer, which in
// turn is computed based on:
//  - the number of bytes in the buffer
//  - the number of bytes per sample (as provided by the current format)
MLint64 computeMSCFromBuffer( AudioPath* path, int bufferLength )
{
  // The WAVEFORMATEX structure can tell us the number of bytes per
  // sample -- the field is actually called nBlockAlign, and takes
  // into account the number of channels for each sample.
  int buffSamples = bufferLength / path->format.nBlockAlign;

  assert( path != 0 );

  return (MLint64) buffSamples;
}

#ifdef UST_WHEN_BUFFER_COMPLETE
//-----------------------------------------------------------------------------
//
// Compute the UST timestamp for a buffer that has just completed.
//
// The timestamp is adjusted -- as best we can -- to account for the
// fact that we are at the *end* of the buffer, whereas the UST
// *should* be for the beginning of the first sample of the buffer
// (according to the spec). This requires knowing the number of
// samples in the buffer, which is computed from the bufferLength
// parameter (expressed in bytes).
//
// This function is only relevant if we are measuring UST at
// buffer-completion time (rather than when we receive a buffer from
// the app). See also the comments at the top of this file.
MLint64 computeUSTBufferComplete( AudioPath* path, int bufferLength )
{
  MLint64 ust = 0;
  static MLint64 secsToNanos = 1000000000; // 10^9 nanos in 1 second
  MLint64 buffDuration;

  assert( path != 0 );

  // Get actual UST timestamp, from system
  path->USTSourceFunc( &ust );

  // Now try to adjust the value to account for the buffer
  // duration. This requires knowledge of the buffer as well as the
  // frame rate (but of course, the frame rate is theoretical -- the
  // actual frame rate may be slightly different, in which case this
  // computation is inexact)

  // UST: expressed in nano-seconds. The WAVEFORMATEX structure (in
  // the AudioPath) can tell us the number of bytes per second.
  // Dividing the size of the buffer (in bytes) by the rate gives us
  // the duration in seconds; we then multiply this by 10^9 to get the
  // duration in nano-secs. This is what we can subtract from the
  // current UST.
  buffDuration = (MLint64) bufferLength * secsToNanos;
  buffDuration = buffDuration / (MLint64) path->format.nAvgBytesPerSec;

  return (ust - buffDuration);
}
#endif


//-----------------------------------------------------------------------------
//
// Compute the current MSC value to account for an idle period.
//
// The new MSC is computed as follows:
//   new MSC = old MSC + (new UST - old UST) * rate
// where:
//   old MSC / UST = timestamp of first sample of the idle period
//                   Obtained from the AudioPath
//   new UST = current timestamp, assumed to be first sample after the
//             idle period (this is not exact, of course)
//   rate = sample rate, expressed in Hz
//
// Note that UST is expressed in nano-secs and rate is in Hz -- so
// don't forget to convert!
MLint64 computeMSCAfterIdle( AudioPath* path )
{
  MLint64 curMSC;
  MLint64 curUST = 0;
  MLint64 ustDiff;
  static MLint64 secsToNanos = 1000000000; // 10^9 nanos in 1 second

  // Make sure we really are in an idle period
  assert( path->startOfIdleUST != 0 );

  // Get current UST
  path->USTSourceFunc( &curUST );
  ustDiff = curUST - path->startOfIdleUST;

  // Use sampling rate from the WAVEFORMATEX structure (expressed in
  // Hz). Hope that this first multiplication doesn't overflow the
  // 64-bit int...
  curMSC = ustDiff * path->format.nSamplesPerSec;
  curMSC /= secsToNanos;
  curMSC += path->nextMSC;

  return curMSC;
}


//-----------------------------------------------------------------------------
//
// Compute the amount of time -- in milli-seconds -- that we should
// wait before processing this message.
//
// We only need to wait if the message contains a predicate:
//   ML_WAIT_FOR_AUDIO_UST_INT64
//   ML_WAIT_FOR_AUDIO_MSC_INT64
//
// If no predicate is found in the message, the function returns 0,
// ie: no wait. Otherwise, the returned value represents the delta
// between the current time (UST or MSC) and the requested time.
//
// Note that if the message contains both predicates -- a case not
// covered in the OpenML spec -- the MSC delta is used.
DWORD computeWaitForMessage( AudioPath* path, MLpv* msg )
{
  static MLint64 nanosPerMilli = 1000000; // 10^6 nanos in 1 milli
  static MLint64 millisPerSec = 1000; // 10^3 millis in 1 sec
  DWORD waitTime = 0;

  // Scan through the entire message looking for these particular
  // params -- not terribly efficient, these are linear searches, but
  // then we don't expect the messages to be that long.
  MLpv* predicate = mlPvFind( msg, ML_WAIT_FOR_AUDIO_UST_INT64 );
  if ( predicate != NULL ) {
    // Get the current UST, and compute difference with requested UST
    // -- this is how long we must wait, in nano-seconds
    MLint64 curUST = 0, diffUST;
    path->USTSourceFunc( &curUST );

    diffUST = predicate->value.int64 - curUST;
    if ( diffUST > 0 ) {
      // Yes, the requested UST is in the future, we must wait.
      // Convert the waiting time to milliseconds, which is the unit
      // used by the WaitForMultipleObjects() call for its timeout.
      waitTime = (DWORD) (diffUST / nanosPerMilli);
      DEBG1( printf( "[winaudio audio loop]: wait for UST = %" FORMAT_LLD
		     ", cur UST = %" FORMAT_LLD ", waiting %d millisecs\n",
		     predicate->value.int64, curUST, waitTime ) );

    } else {
      DEBG2( printf( "[winaudio audio loop]: wait for UST = %" FORMAT_LLD
		     ", cur UST = %" FORMAT_LLD ", NOT waiting\n",
		     predicate->value.int64, curUST ) );

    }
  } // if predicate != NULL

  // Again, for MSC
  predicate = mlPvFind( msg, ML_WAIT_FOR_AUDIO_MSC_INT64 );
  if ( predicate != NULL ) {
    // Get the current MSC -- note that if we are in the middle of an
    // idle period, we need to adjust the MSC. However, we won't save
    // the adjusted MSC to the AudioPath structure: we get better
    // precision when we compute the adjusted MSC over longer periods
    // of time (reduces the influence of system call overhead etc.),
    // so we will wait until the end of the period to save the final
    // adjusted MSC.
    MLint64 curMSC = 0;

    MLint64 diffMSC;
    if ( path->startOfIdleUST != 0 ) {
      curMSC = computeMSCAfterIdle( path );
    } else {
      curMSC = path->nextMSC;
    }

    diffMSC = predicate->value.int64 - curMSC;
    if ( diffMSC > 0 ) {
      // Yes, the requested MSC is in the future, we must
      // wait. Convert the waiting time from MSC slots to
      // milli-seconds, using the frame rate. Frame rate is in samples
      // per sec, we want milli-secs.
      waitTime = (DWORD) ((millisPerSec * diffMSC) /
			  path->format.nSamplesPerSec);

      DEBG1( printf( "[winaudio audio loop]: wait for MSC = %" FORMAT_LLD
		     ", cur MSC = %" FORMAT_LLD ", waiting %d millisecs\n",
		     predicate->value.int64, curMSC, waitTime ) );

    } else {
      DEBG2( printf( "[winaudio audio loop]: wait for MSC = %" FORMAT_LLD
		     ", cur MSC = %" FORMAT_LLD ", NOT waiting\n",
		     predicate->value.int64, curMSC ) );

    }
  } // if predicate != NULL

  return waitTime;
}


//-----------------------------------------------------------------------------
//
// Handle buffers sent by the application
// Description of arguments:
//   path : AudioPath structure, used to obtain direction of transfer,
//          device handle, and MSC. Remember to access fields of this
//          structure using thread-safe functions!
//   state : current state of device. Could also be determined from path,
//           but this allows the caller to supply a cached value, which
//           may be preferrable to avoid race conditions
//   hdrPool : Wave Header Pool, used to wrap buffers before sending to dev.
//   qEntry, msg : queue entry and associated message of BUFFERS_IN_PROGRESS
//                 message
//   deferTimeout : see below. Not modified if return status is not
//                  ML_STATUS_WINAUDIO_DEFERRED.
//
// In certain circumstances, it is impossible to handle the buffer
// right away. For instance:
//  - if the transfer has not yet started
//  - if the buffer request includes a predicate such as wait for UST or MSC,
//    and that time has not yet been reached
//
// In this situation, the buffer is deferred: the function returns
// ML_STATUS_WINAUDIO_DEFERRED, and the deferTimeout value is set to
// the number of milli-seconds before it becomes possible to handle
// the buffer (ie: before the desired UST or MSC is reached). If
// handling the buffer requires an external event (ie: start of
// transfer), the deferTimeout value will be set to INFINITE.

// Defined ML_STATUS_WINAUDIO_DEFERRED
// This is a bit of a hack -- but we assume that no other ML status code
// will ever use this value...
#define ML_STATUS_WINAUDIO_DEFERRED (ML_STATUS_INTERNAL_ERROR+1)

MLstatus handleIncomingBuffers( AudioPath* path, int state,
				WaveHdrPool* hdrPool,
				MLqueueEntry* qEntry, MLpv* msg,
				DWORD* deferTimeout )
{
  MLstatus status = ML_STATUS_NO_ERROR;
  int i, direction;
  MLint64* msgUSTValue = 0;
  MMRESULT mmStatus;
  WAVEHDR* wHdr = 0;       // ptr to WAVE header used for this buffer
  MLint64 bufMSC;
  DWORD waitTime = 0;

#ifndef UST_WHEN_BUFFER_COMPLETE
  MLint64 bufUST = 0;
#endif

  // First check: are we transferring? If not, must defer this buffer
  // for an INFINITE time, ie: until transfer begins.
  if ( state != ML_DEVICE_STATE_TRANSFERRING ) {
    DEBG2( printf( "[winaudio audio loop]: Can not use buffer on "
		   "output path now, not transferring (MSC = %"
		   FORMAT_LLD ")\n", path->nextMSC ) );
    *deferTimeout = INFINITE;
    return ML_STATUS_WINAUDIO_DEFERRED;
  }

  // Second check: is there a predicate associated with this buffer?
  // eg: instructions to wait for a particular UST or MSC?
  waitTime = computeWaitForMessage( path, msg );
  if ( waitTime > 0 ) {
    *deferTimeout = waitTime;
    return ML_STATUS_WINAUDIO_DEFERRED;
  }

  direction = path->direction; // no thread-safety issue on this field

  // Get MSC corresponding to the first sample of this buffer.
  //
  // First check to see if this buffer comes after an idle period. If
  // that is the case, we first need to update the MSC based on the
  // duration of the idle period.
  if ( path->startOfIdleUST != 0 ) {
    path->nextMSC = computeMSCAfterIdle( path );
    // Idle period is over!
    path->startOfIdleUST = 0;
    DEBG2( printf( "[winaudio audio loop]: Adjust MSC for idle period, "
		   "new value = %" FORMAT_LLD "\n", path->nextMSC ) );
  }
  bufMSC = path->nextMSC;

  for ( i=0; msg[i].param != ML_END; ++i ) {

    switch ( msg[i].param ) {

    case ML_AUDIO_BUFFER_POINTER:

#ifndef UST_WHEN_BUFFER_COMPLETE
      // Get the UST that corresponds (roughly) to the point when we
      // send out this buffer to the WAVE device.
      //
      // Note that this is NOT COMPLIANT with the OpenML spec: in
      // theory, the UST should correspond to the time at which the
      // first sample of the buffer passes through the (physical)
      // jack. But we don't know when that time will be -- the
      // buffering inside the WAVE device is unknown to us. So this is
      // the best we can do.
      //
      // This is only done if we are NOT measuring UST when the buffer
      // is *complete*. See comments at top of file.
      path->USTSourceFunc( &bufUST );
#endif

      // Allocate a Wave header for this buffer, and associate it with
      // the ml queue entry
      wHdr = getNextWaveHdr( hdrPool, qEntry );

      if ( direction == ML_DIRECTION_IN ) {
	// Input path. Send this buffer on to the Wave device (OK to
	// do this even if we are not transferring -- the Wave device
	// will simply hang on to it until we call waveInStart)
	//
	HWAVEIN inHandle = (HWAVEIN) getDevHandle( path );

	// Remember to keep track of the pointer to the length portion
	// of this message, so that when the buffer has been filled,
	// we can write the number of bytes written to it.
	setMsgLengthValue( hdrPool, wHdr, &msg[i].length );

	DEBG3( printf( "[winaudio audio loop]: Got input buffer, %d bytes;"
		       " using header #%d (MSC = %" FORMAT_LLD ")\n",
		       msg[i].maxLength, (int) wHdr->dwUser, bufMSC ) );

	// "Prepare" the header. Note: it isn't clear from the WAVE
	// documentation whether it is actually necessary to
	// prepare/unprepare the header each time we use it. I suspect
	// it is, however, since it seems that the buffer pointer is
	// required -- and this will change for each use of the
	// header.
	//
	// According to WAVE docs, before preparing the header, the
	// following fields must be set:
	wHdr->lpData = (LPSTR) msg[i].value.pByte;
	wHdr->dwBufferLength = (DWORD) msg[i].maxLength;
	wHdr->dwFlags = 0;
	mmStatus = waveInPrepareHeader( inHandle, wHdr, sizeof( WAVEHDR ) );
	if ( mmStatus != MMSYSERR_NOERROR ) {
	  DEBG1( printf( "[winaudio audio loop]: waveInPrepareHeader "
			 "failed, code = %d\n", mmStatus ) );
	  status = ML_STATUS_INTERNAL_ERROR;
	  break;
	}

	// Finally, add buffer to the WAVE device queue
	mmStatus = waveInAddBuffer( inHandle, wHdr, sizeof( WAVEHDR ) );
	if ( mmStatus != MMSYSERR_NOERROR ) {
	  DEBG1( printf( "[winaudio audio loop]: waveInAddBuffer "
			 "failed, code = %d\n", mmStatus ) );
	  status = ML_STATUS_INTERNAL_ERROR;
	}

      } else {
	// Output path. Same logic as above for input paths, except we
	// do not need to supply the pointer to msg.length -- no need
	// to write the number of bytes output to the device when the
	// buffer is complete.
	HWAVEOUT outHandle = (HWAVEOUT) getDevHandle( path );

	DEBG3( printf( "[winaudio audio loop]: Got output buffer, %d bytes;"
		       " using header #%d (MSC = %" FORMAT_LLD ")\n",
		       msg[i].length, (int) wHdr->dwUser, bufMSC ) );

	wHdr->lpData = (LPSTR) msg[i].value.pByte;
	wHdr->dwBufferLength = (DWORD) msg[i].length;
	wHdr->dwFlags = 0;

	mmStatus = waveOutPrepareHeader( outHandle, wHdr, sizeof( WAVEHDR ) );
	if ( mmStatus != MMSYSERR_NOERROR ) {
	  DEBG1( printf( "[winaudio audio loop]: waveOutPrepareHeader "
			 "failed, code = %d\n", mmStatus ) );
	  status = ML_STATUS_INTERNAL_ERROR;
	  break;
	}

	// Finally, add buffer to the WAVE device queue
	mmStatus = waveOutWrite( outHandle, wHdr, sizeof( WAVEHDR ) );
	if ( mmStatus != MMSYSERR_NOERROR ) {
	  DEBG1( printf( "[winaudio audio loop]: waveOutWrite "
			 "failed, code = %d\n", mmStatus ) );
	  status = ML_STATUS_INTERNAL_ERROR;
	}
      } // if ML_DIRECTION_IN ... else ...

      // If all went well, one more buffer was sent to the
      // device... that's one more outstanding buffers. Also, if a
      // SEQUENCE_LOST condition existed, it has been resolved, since
      // the device is once again busy with a buffer.
      //
      // We can also increment the "next MSC" value by the number of
      // MSC slots in the buffer that was jus sent.
      //
      // NOTE: this is actually *incorrect* for input buffers, because
      // the length of the buffer is not necessarily the number of
      // samples that will be read in. For input, we should really be
      // updating MSC upon *completion* of the buffer. But in reality,
      // the buffer is generally filled, so this isn't a problem.
      if ( status == ML_STATUS_NO_ERROR ) {
	path->numOutstandingBuffers += 1;

	path->nextMSC += computeMSCFromBuffer( path, wHdr->dwBufferLength );
      }
      break;

    case ML_WAIT_FOR_AUDIO_UST_INT64:
    case ML_WAIT_FOR_AUDIO_MSC_INT64:
      // These were handled above, if we got this far, then no waiting
      // is necessary
      break;

    case ML_AUDIO_UST_INT64:
      // Record request for UST, but don't process right now -- see
      // details below.
      msgUSTValue = &msg[ i ].value.int64;
      break;

    case ML_AUDIO_MSC_INT64:
      // Request for MSC: fill in the information right now, using the
      // value that was cached up above. The point of caching is that
      // if the AUDIO_MSC field appears *after* the BUFFER_POINTER
      // field, we still want to use the MSC that was current at the
      // start of the buffer (when we process the buffer pointer, we
      // increment the MSC, and it is no longer valid for the current
      // buffer)
      msg[ i ].value.int64 = bufMSC;
      break;

    default:
      DEBG1( printf( "[winaudio audio loop]: sendBuffers msg: param #%d "
		     "unknown type: 0x%" FORMAT_LLX "\n",
		     i, msg[i].param ) );
      msg[i].length = -1;
      status = ML_STATUS_INVALID_PARAMETER;
      break;
    } // switch msg[i].param

    if ( status != ML_STATUS_NO_ERROR ) {
      break;
    }
  } // for i=0...msg[i].param!=ML_END

  // Make sure the message really did contain a buffer pointer --
  // otherwise, that's an error. Check the wHdr pointer: it was set
  // when we parsed the buffer part of the message (so if there was no
  // such part, it is still zero)
  if ( wHdr == 0 ) {
    DEBG1( printf( "[winaudio audio loop]: sendBuffers msg did not contain "
		   "a buffer pointer\n" ) );
    // Set some error -- which exact error doesn't really matter,
    // since in any case it can not be passed back to the application
    // via the response message (the only code we can use is
    // BUFFERS_FAILED).
    status = ML_STATUS_INVALID_PARAMETER;
  }
  else {

    // Check if the message contained a request for UST. If so,
    // we face two options (see also comments at top of file):
    //  - store the UST value obtained above. This is a (rough)
    //    approximation of the timestamp of the beginning of the buffer.
    //
    //  - keep track of the location of the UST request within
    //    the user message, so that we may save the timestamp *after*
    //    the buffer has been processed by the WAVE device.

#ifdef UST_WHEN_BUFFER_COMPLETE
    // The pointer msgUSTValue points to the portion of the message
    // containing the request for UST *or* is zero, if no request was
    // made. Either way, we can write it directly to the
    // WaveHeaderEntry structure.
    setMsgUSTValue( hdrPool, wHdr, msgUSTValue );

#else
    // If there was a request for UST, complete it now. It was not
    // completed when we parsed it out of the message, because the
    // order of parameters within a message is not imposed by
    // OpenML. Thus, the request for UST *could* come before the
    // actual buffer pointer, which is when UST is recorded.
    if ( msgUSTValue != 0 ) {
      *msgUSTValue = bufUST;
    }
#endif
  } // if wHdr == 0 ... else ...

  return status;
}


//-----------------------------------------------------------------------------
//
int handleStateChange( AudioPath* path, int prevState )
{
  int newState;
  void* devHandle;

  // Get the new device state and device handle (either in- or out-),
  // in a thread-safe manner
  newState = getState( path );
  devHandle = getDevHandle( path );

  // If the state has just changed, deal with the transition
  if ( newState != prevState ) {

    switch ( newState ) {

    case ML_DEVICE_STATE_ABORTING: {
      // If the state has just changed to ABORTING, we must abort the
      // message queue
      DEBG1( printf( "[winaudio audio loop]: Transition to aborting (MSC = %"
		     FORMAT_LLD ")\n", path->nextMSC ) );
      mlDIQueueAbortMessages( path->pQueue );

      // Also need to abort the WAVE device
      if ( devHandle != 0 ) {
	if ( path->direction == ML_DIRECTION_IN ) {
	  // Have a choice between calling waveInStop and
	  // waveInReset... the former leaves buffers in the queue,
	  // which is probably not what we want.
	  waveInReset( (HWAVEIN) devHandle );
	} else {
	  waveOutReset( (HWAVEOUT) devHandle );
	}
      } // if devHandle != 0
    } break;

    case ML_DEVICE_STATE_TRANSFERRING: {
      DEBG1( printf( "[winaudio audio loop]: Transition to transferring "
		     "(MSC = %" FORMAT_LLD ")\n", path->nextMSC ) );

      // On output, there is nothing to do here -- we will simply
      // start sending buffers to the WAVE device (in the loop that
      // picks up DI messages), and that will start the transfer.
      //
      // But on input, we need to explicitly start the device.
      if ( path->direction == ML_DIRECTION_IN ) {
	assert( devHandle != 0 );

	if ( waveInStart( (HWAVEIN) devHandle ) != MMSYSERR_NOERROR ) {
	  // Can't do much -- no way to return an error code to
	  // caller. Hope it doesn't happen...
	  DEBG1( printf( "[winaudio audio loop]: Error in waveInStart\n" ) );
	}
      } // if path->direction == ML_DIRECTION_IN
    } break;

    default:
      // Other transitions may happen, but for now, we have no special
      // action to take.
      DEBG1( printf( "[winaudio audio loop]: Transition to state %s "
		     "(MSC = %" FORMAT_LLD ")\n",
		     mlDeviceStateName( newState ), path->nextMSC ) );
      break;
    } // switch newState

  } // if newState != prevState

  // return new state
  return newState;
}

//-----------------------------------------------------------------------------
//
// Wake up audio loop -- the child thread blocks on the thread message
// queue. When the client thread wishes to wake up the child (because
// the state changed, or new buffers were made available for
// instance), it needs to post a message to the thread's queue to wake
// it up.
MLstatus wakeUpAudioLoop( AudioPath* path )
{
  MLstatus status = ML_STATUS_NO_ERROR;

  // Post a thread message, of type WM_USER -- the params don't
  // matter, the thread won't look at them. The only point of this
  // message is to cause the thread to return from its GetMessage
  // call.

  // Note: this assumes that the audio loop thread has already created
  // its message queue. As detailed in the API docs for
  // PostThreadMessage, there are situations where that may not be
  // true. But in our case -- where the child thread goes immediately
  // to its GetMessage call -- we can safely assume (I hope!) that
  // we'll be OK.
  DEBG4( printf( "[winaudio]: wakeUpAudioLoop -- posting WM_USER\n" ) );

  if ( PostThreadMessage( path->childId, WM_USER, 0, 0 ) == 0 ) {
    DEBG1( printf( "[winaudio]: PostThreadMessage failed, error = %lu\n",
		   GetLastError() ) );
    status = ML_STATUS_INTERNAL_ERROR;
  }
  return status;
}

//-----------------------------------------------------------------------------
//
// Main audio loop, child thread entry point
// Returns 0 on normal exit, 1 on error exit
unsigned __stdcall audioLoop( void* arg )
{
  AudioPath* path = (AudioPath*) arg;
  int doneLoop = 0;
  HANDLE diQueueWaitable = 0;
  WaveHdrPool hdrPool;
  MSG threadMsg;
  MLstatus status = ML_STATUS_NO_ERROR;
  int state;
  int checkDeviceIdle = 0;

  // Pending messages:
  //
  // Inside the loop, whenever we get woken up, we check the DI
  // queue. By doing so -- using mlDIQueueNextMessage -- we mark the
  // oldest message as "read", and that message is not returned to us
  // again. However, if that message is a buffer (mlSendBuffers) for
  // an output path and we are not currently transferring, we can not
  // deal with it immediately (because calling "waveOutWrite" would
  // actually start the transfer). So we must hang on to the message
  // -- and make sure that we check it before we perform another
  // mlDIQueueNextMessage.

  enum mlMessageTypeEnum pendingMsgType; // pending message type
  MLqueueEntry* pendingQEntry = 0;       // queue entry of pending message
  MLpv* pendingMsg = 0;                  // actual message

  // Timeout value for wait call:
  //
  // In general we wish to perform a blocking wait until we have
  // messages to process -- either messages from the application (eg,
  // new buffers) or messages from the WAVE device (eg, buffers
  // complete). However, if the application has sent a buffer with
  // instructions to wait for a specific UST or MSC, we need to be
  // unblocked at the specified time, whether or not messages have
  // arrived.
  //
  // To handle this, we use this timeout variable: it will generally
  // be set to the constant "INFINITE", meaning no timeout, but can be
  // reset to a specific timeout value, expressed in milli-seconds.
  DWORD timeoutValue = INFINITE;

  DEBG1( printf( "[winaudio audio loop]: In audio %s loop thread\n",
		 (path->direction == ML_DIRECTION_IN) ? "input" : "output" ) );

  // Before we go any further, call PeekMessage to ensure that the
  // thread message queue is created.
  PeekMessage( &threadMsg, 0, 0, 0, PM_NOREMOVE );

  // Allocate storage for the WAVEHDR structures that will be used to
  // wrap the OpenML buffers before they are sent to the WAVE device.
  allocateWaveHdrPool( path, &hdrPool );

  // Get an event on which to block while waiting for the DI queue to
  // contain new messages.
  //
  // Note: we could use mlDIQueueGetDeviceWaitable here, but that
  // event remains signaled as long as there are messages in the queue
  // -- even if we have already seen the messages using
  // mlDIQueueNextMessage. As a result, when the queue contains
  // messages that we are unable to process (eg: sendBuffers when the
  // device is not yet transferring), we are constantly woken up, only
  // to find that there are no new messages. This is a form of
  // busy-waiting, and is wasteful.
  //
  // Instead, we have our own event, which is only signaled when the
  // queue contains new messages -- see ddSendBuffers and
  // ddSendControls.
  diQueueWaitable = path->newDIMessage;

  // Get the device state; this will be updated inside the loop, when
  // we notice that there has been a state change.
  // Note: we assume here that it is not possible for the state to be
  // ABORTING yet...
  state = getState( path );

  // The device is now operational, but it is not transferring -- it
  // is idle. So keep track of the UST, so that we can properly
  // increment the MSC (in other words: start rolling the MSC right
  // now, don't wait for the first transfer to start)
  path->USTSourceFunc( &(path->startOfIdleUST) );

  while ( doneLoop == 0 ) {
    int thMsgAvailable = 0;
    int chkDIMessages;

    // Wait for either a thread message (from WAVE device, or perhaps
    // a WM_USER message from the client thread), OR a message in the
    // DI queue (also from the client thread)
    DWORD waitStatus =
      MsgWaitForMultipleObjects( 1, &diQueueWaitable,
				 FALSE /* bWaitAll */,
				 timeoutValue, /* INFINITE or time in msecs */
				 QS_ALLINPUT /* dwWakeMask */ );

    switch ( waitStatus ) {
    case WAIT_FAILED:
      DEBG1( printf( "[winaudio audio loop]: Wait failed, error = %lu\n",
		     GetLastError() ) );
      status = ML_STATUS_INTERNAL_ERROR;
      break;

    case WAIT_OBJECT_0:
      DEBG4( printf( "[winaudio audio loop]: Woke up on DI queue msg\n" ) );
      break;

    case WAIT_OBJECT_0+1:
      DEBG4( printf( "[winaudio audio loop]: Woke up on thread message\n" ) );
      thMsgAvailable = 1;
      break;

    case WAIT_TIMEOUT:
      DEBG4( printf( "[winaudio audio loop]: Woke up on timeout\n" ) );
      break;

    default:
      DEBG1( printf( "[winaudio audio loop]: Unknown reason for wake-up: %d\n",
		     waitStatus ) );
      status = ML_STATUS_INTERNAL_ERROR;
      break;
    } // switch waitStatus

    if ( status != ML_STATUS_NO_ERROR ) {
      break;
    }

    // Reset timeout value -- in general we don't want to timeout. If
    // later on we determine that we need to time-out, we'll set the
    // value to whatever is appropriate.
    timeoutValue = INFINITE;

    // If we were told there is something available in the thread
    // message queue, get that now. This is actually a loop, because
    // we wish to retrieve *all* messages in the queue -- because,
    // even if we don't retrieve them, all messages are marked as "not
    // new anymore" after the first call to GetMessage/PeekMessage --
    // and MsgWaitForMultipleObjects will *not* wake us up for those
    // messages anymore.
    while ( thMsgAvailable == 1 ) {
      WAVEHDR* wHdr = 0;

#ifdef UST_WHEN_BUFFER_COMPLETE
      int bufferLength = 0;
#endif

      DEBG4( printf( "[winaudio audio loop]: calling PeekMessage...\n" ) );
      if ( PeekMessage( &threadMsg, 0, 0, 0, PM_REMOVE ) == 0 ) {
	DEBG4( printf( "[winaudio audio loop]: no more messages\n" ) );
	break;
      }

      switch ( threadMsg.message ) {

      case WM_USER:
	DEBG3( printf( "[winaudio audio loop]: got WM_USER message\n" ) );
	// This is perhaps an indication from the app thread that the
	// state has changed, so handle that possibility now.
	state = handleStateChange( path, state );

	// Check if we are told to shutdown
	if ( state == ML_DEVICE_STATE_FINISHING ) {
	  DEBG3( printf( "[winaudio audio loop]: requesting shutdown\n" ) );
	  doneLoop = 1;
	}
	break;

      case MM_WIM_OPEN:
	DEBG3( printf( "[winaudio audio loop]: got MM_WIM_OPEN message\n" ) );
	assert( path->direction == ML_DIRECTION_IN );
	break;

      case MM_WOM_OPEN:
	DEBG3( printf( "[winaudio audio loop]: got MM_WOM_OPEN message\n" ) );
	assert( path->direction == ML_DIRECTION_OUT );
	break;

      case MM_WIM_CLOSE:
	DEBG3( printf( "[winaudio audio loop]: got MM_WIM_CLOSE message\n" ) );
	assert( path->direction == ML_DIRECTION_IN );
	break;

      case MM_WOM_CLOSE:
	DEBG3( printf( "[winaudio audio loop]: got MM_WOM_CLOSE message\n" ) );
	assert( path->direction == ML_DIRECTION_OUT );
	break;

      case MM_WIM_DATA: {
	HWAVEIN inHandle;
	int* lengthPtr;

	assert( path->direction == ML_DIRECTION_IN );
	inHandle = (HWAVEIN) threadMsg.wParam;
	wHdr = (WAVEHDR*) threadMsg.lParam;

	DEBG3( printf( "[winaudio audio loop]: got MM_WIM_DATA message "
		       "for header #%d\n\t%d bytes written\n",
		       (int) wHdr->dwUser, (int) wHdr->dwBytesRecorded ) );

	// Un-prepare the header, so it is ready for the next time
	waveInUnprepareHeader( inHandle, wHdr, sizeof( WAVEHDR ) );

	// Record number of bytes written to the buffer, using the
	// pointer associated with this Wave header
	lengthPtr = getMsgLengthValue( &hdrPool, wHdr );
	assert( lengthPtr != 0 );
	*lengthPtr = (MLint32) wHdr->dwBytesRecorded;

	// Our handling of MSC is not quite correct for input buffers:
	// we assume we know beforehand how many bytes will be written
	// to the buffer, and update the MSC accordingly. But in
	// reality, now -- when the buffer is complete -- is the only
	// time we can really know how many MSC slots were used.
	//
	// I believe that in reality, the input buffer will almost
	// always be filled by the WAVE device, so that our estimate
	// is correct. But just to make sure, check that here, and
	// output a warning if the assumption is wrong.
	if ( wHdr->dwBytesRecorded < wHdr->dwBufferLength ) {
	  DEBG1( printf( "[winaudio audio loop]: WARNING: used %d "
			 "bytes from a %d byte input buffer (MSC = %"
			 FORMAT_LLD ")\n", wHdr->dwBytesRecorded,
			 wHdr->dwBufferLength, path->nextMSC
			 ) );

	  // Any point in trying to adjust the MSC? It will have been
	  // recorded incorrectly for all buffers currently in the
	  // queue; adjusting it now will cause a further glitch. Best
	  // to leave it alone.
	}

#ifdef UST_WHEN_BUFFER_COMPLETE
	// In order to compute UST, we will need to know the number of
	// bytes recorded to this buffer
	bufferLength = wHdr->dwBytesRecorded;
#endif
      } break;

      case MM_WOM_DONE: {
	HWAVEOUT outHandle;

	assert( path->direction == ML_DIRECTION_OUT );
	outHandle = (HWAVEOUT) threadMsg.wParam;
	wHdr = (WAVEHDR*) threadMsg.lParam;

	DEBG3( printf( "[winaudio audio loop]: got MM_WOM_DONE message "
		       "for header #%d\n", (int) wHdr->dwUser ) );

	// Un-prepare the header, so it is ready for the next time
	waveOutUnprepareHeader( outHandle, wHdr, sizeof( WAVEHDR ) );

#ifdef UST_WHEN_BUFFER_COMPLETE
	// In order to compute UST, we will need to know the number of
	// bytes played out from this buffer
	bufferLength = wHdr->dwBufferLength;
#endif
      } break;

      default:
	DEBG1( printf( "[winaudio audio loop]: got unknown message %d\n",
		       threadMsg.message ) );
	break;
      } // switch threadMsg.message

      // Did we just get a completed buffer? If so, process it and
      // send it back on the OpenML queue.
      if ( wHdr != 0 ) {
	MLqueueEntry* qEntry = 0;

#ifdef UST_WHEN_BUFFER_COMPLETE
	// Start by getting pointer to the UST request portion of the
	// application's message -- so that we may write the
	// timestamp. If the app did not request UST, the pointer will
	// be zero.
	//
	// Note that this needs to be done *before* we release the
	// header back to the pool, to ensure that the value is still
	// valid.
	MLint64* msgUSTValue;

	msgUSTValue = getMsgUSTValue( &hdrPool, wHdr );
#endif

	// Release header back to pool, and get associated Queue
	// entry.
	qEntry = releaseWaveHdr( &hdrPool, wHdr );

	// Note that if we are aborting, we should not attempt to
	// respond to the message -- it has already been aborted.
	if ( state != ML_DEVICE_STATE_ABORTING ) {

#ifdef UST_WHEN_BUFFER_COMPLETE
	  // Compute UST, if requested by the app (ie: if the pointer
	  // is non-zero).
	  if ( msgUSTValue != 0 ) {
	    *msgUSTValue = computeUSTBufferComplete( path, bufferLength );
	  }
#endif

	  DEBG4( printf( "[winaudio audio loop]: Updating messages, "
			 "send count = %d\n", path->pQueue->sendCount ) );
	  status = mlDIQueueUpdateMessage( qEntry, ML_BUFFERS_COMPLETE );
	  if ( status != ML_STATUS_NO_ERROR ) {
	    DEBG1( printf( "[winaudio audio loop]: UpdateMessage failed, "
			   "status = %d\n", status ) );
	    // But what to do? Hope for the best and keep going??? 
	    // Abort for now
	    break;
	  }

	  // That's one less buffer outstanding in the device
	  path->numOutstandingBuffers -= 1;

	  // It is possible that the WAVE device may now have run out
	  // of buffers -- ie: that the device has become idle. This
	  // may also be a SEQUENCE_LOST event (or it may not -- see
	  // below).
	  //
	  // But rather than check it here (inside the loop), we
	  // should wait until we have processed all our messages. So
	  // simply keep track of the fact that we want to check, and
	  // do the actual check after the loop.
	  checkDeviceIdle = 1;

	} // if state != ABORTING
      } // if qEntry != 0

    } // while thMsgAvailable == 1

    if ( (status != ML_STATUS_NO_ERROR) || (doneLoop != 0) ) {
      break;
    }

    // Check if the WAVE device has become idle (see inside the loop,
    // above).
    //
    // How many buffers are left outstanding? If there are none left,
    // the WAVE device is idle -- and this is a SEQUENCE_LOST,
    // *unless* we have a buffer waiting to be sent to the WAVE device
    // (most likely because it has requested to wait for a UST or
    // MSC). If a buffer is waiting, then this is not a loss of
    // sequence: the app has provided sufficient buffers, it just
    // isn't time to process them yet.
    //
    // However, even if it isn't a loss of sequence, it is still the
    // start of an idle period, so don't forget to handle that.
    if ( checkDeviceIdle != 0 ) {
      checkDeviceIdle = 0; // reset flag for next round

      if ( path->numOutstandingBuffers == 0 ) {
	int evWanted = getEventsWanted( path );
	MLstatus diQStatus = ML_STATUS_NO_ERROR;

	// This is the start of an idle period. In order to properly
	// update the MSC when we get the next buffer, we will need to
	// know the duration of the idle period. So take note of the
	// UST now.
	//
	// But first, sanity-check: we should not already be in the
	// middle of an idle period. Thus, startOfIdleUST should be
	// zero (when non-zero, we are idle)
	assert( path->startOfIdleUST == 0 );
	path->USTSourceFunc( &(path->startOfIdleUST) );

	DEBG1( printf( "[winaudio audio loop]: start of idle period, "
		       "UST = %" FORMAT_LLD ", MSC = %" FORMAT_LLD "\n",
		       path->startOfIdleUST, path->nextMSC ) );

	// Figure out if we should report the condition: did the
	// application request it? And is it really a loss of
	// sequence, or do we actually have a buffer waiting to be
	// sent at the appropriate time? Check 'pendingMsg' for this
	// -- for now, this is only non-NULL if a buffers message is
	// waiting (it can currently never point to a controls
	// message, for instance).
	if ( (pendingMsg == 0) &&
	     ((evWanted & WAVE_SEQUENCE_LOST_EVENT) != 0) ) {
	  MLpv eventMsg[3];

	  // Report the UST/MSC at which we have detected the event.
	  eventMsg[0].param = ML_AUDIO_UST_INT64;
	  path->USTSourceFunc( &eventMsg[0].value.int64 );
	  eventMsg[0].length = 1;
	  eventMsg[1].param = ML_AUDIO_MSC_INT64;
	  eventMsg[1].value.int64 = path->nextMSC;
	  eventMsg[1].length = 1;
	  eventMsg[2].param = ML_END;

	  DEBG2( printf( "[winaudio audio loop]: "
			 "Reporting SEQUENCE_LOST (MSC = %" FORMAT_LLD
			 ")\n", path->nextMSC ) );
	  diQStatus = mlDIQueueReturnEvent( path->pQueue,
					    ML_EVENT_AUDIO_SEQUENCE_LOST,
					    eventMsg );

	  if ( diQStatus != ML_STATUS_NO_ERROR ) {
	    // Not much we can do in this case, except output some
	    // debugging info...
	    DEBG1( printf( "[winaudio audio loop]: mlDIQueueReturnEvent "
			   "returned error: '%s'\n",
			   mlStatusName( diQStatus )) );
	  }
	} // if evWanted & WAVE_SEQUENCE_LOST_EVENT ...
      } // if numOutstandingBuffers <= 1
    } // if checkDeviceIdle

    // Manage state changes
    //
    // When the state transitions to "ABORTING", it stays there until
    // all pending WAVE headers are returned by the device. Once that
    // is done, we can move to "READY". Since we have just processed
    // messages from the device, now is a good time to check if we
    // should make that state change.
    if ( (state == ML_DEVICE_STATE_ABORTING) && (hdrPool.inUse == 0) ) {
      // Yup, all buffers have been returned
      int oldState;

      DEBG3( printf( "[winaudio audio loop]: Transition from ABORTING "
		     "to READY (MSC = %" FORMAT_LLD ")\n", path->nextMSC ) );
      oldState = setState( path, ML_DEVICE_STATE_READY );

      // Sanity-checking: this is one state that should not have been
      // changed by anybody else.
      assert( oldState == ML_DEVICE_STATE_ABORTING );

      // Signal the parent thread that the ABORT operation is complete
      if ( SetEvent( path->abortComplete ) == 0 ) {
	DEBG1( printf( "[winaudio audio loop]: setEvent abortComplete "
		       "failed, error code = %d\n", (int)GetLastError() ) );
      }

      // Update our local cache
      state = getState( path );
    }


    // Check if there are any messages in the DI queue -- we can
    // safely do this even if we weren't woken up for this reason,
    // because this is a non-blocking call (even if the queue is
    // empty).
    chkDIMessages = 1;
    while ( chkDIMessages != 0 ) {
      // If we already have de-queued a message, use it instead of
      // de-queueing a new one.
      if ( pendingMsg == 0 ) {
	status = mlDIQueueNextMessage( path->pQueue, &pendingQEntry,
				       &pendingMsgType,
				       &pendingMsg, NULL, NULL );

	if ( status == ML_STATUS_RECEIVE_QUEUE_EMPTY ) {
	  DEBG4( printf( "[winaudio audio loop]: DI queue empty "
			 "(send count = %d)\n", path->pQueue->sendCount ) );
	  pendingMsg = 0; // just to make sure
	  status = ML_STATUS_NO_ERROR;
	  chkDIMessages = 0;
	} else if ( status != ML_STATUS_NO_ERROR ) {
	  DEBG1( printf( "[winaudio audio loop]: DI queue error %d\n",
			 status ) );
	  pendingMsg = 0;
	  chkDIMessages = 0;
	}
      } else {
	DEBG4( printf( "[winaudio audio loop]: re-using pending message\n" ) );
      }

      if ( pendingMsg != 0 ) {
	// Got a valid message, see if we can process it.
	MLint32 replyMsgType = 0;

	switch ( pendingMsgType ) {

	case ML_CONTROLS_IN_PROGRESS: {
	  MLint32 accessNeeded = ML_ACCESS_WRITE | ML_ACCESS_QUEUED;

	  DEBG3( printf( "[winaudio audio loop]: got DI queue msg "
			 "ML_CONTROLS_IN_PROGRESS\n" ) );

	  if ( getState( path ) == ML_DEVICE_STATE_TRANSFERRING ) {
	    accessNeeded |= ML_ACCESS_DURING_TRANSFER;
	  }

	  // Check that all parts of the request are valid at this time
	  status = mlDIvalidateMsg( path->openId, accessNeeded,
				    ML_FALSE /* not used in get-controls */,
				    pendingMsg );
	  if ( status == ML_STATUS_NO_ERROR ) {
	    status = setAudioPathControls( path, pendingMsg );
	  } else {
	    DEBG2( printf( "[winaudio audio loop]: setControls: "
			   "invalid request message\n" ) );
	  }

	  replyMsgType = (status == ML_STATUS_NO_ERROR) ?
	    ML_CONTROLS_COMPLETE : ML_CONTROLS_FAILED;

	  // Mark this message as complete in the DI queue
	  status = mlDIQueueUpdateMessage( pendingQEntry, replyMsgType );
	  if ( status != ML_STATUS_NO_ERROR ) {
	    DEBG1( printf( "[winaudio audio loop]: UpdateMessage failed, "
			   "status = %d\n", status ) );
	  }

	  // Reset pendingMsg pointer to indicate it was handled
	  pendingMsg = 0;
	} break;

	case ML_QUERY_IN_PROGRESS: {
	  MLint32 accessNeeded = ML_ACCESS_READ | ML_ACCESS_QUEUED;

	  DEBG3( printf( "[winaudio audio loop]: got DI queue msg "
			 "ML_QUERY_IN_PROGRESS\n" ) );

	  if ( getState( path ) == ML_DEVICE_STATE_TRANSFERRING ) {
	    accessNeeded |= ML_ACCESS_DURING_TRANSFER;
	  }

	  // Check that all parts of the request are valid at this time
	  status = mlDIvalidateMsg( path->openId, accessNeeded,
				    ML_TRUE /* used in get-controls */,
				    pendingMsg );
	  if ( status == ML_STATUS_NO_ERROR ) {
	    status = getAudioPathControls( path, pendingMsg );
	  } else {
	    DEBG2( printf( "[winaudio audio loop]: getControls: "
			   "invalid request message\n" ) );
	  }

	  replyMsgType = (status == ML_STATUS_NO_ERROR) ?
	    ML_QUERY_CONTROLS_COMPLETE : ML_QUERY_CONTROLS_FAILED;

	  // Mark this message as complete in the DI queue
	  status = mlDIQueueUpdateMessage( pendingQEntry, replyMsgType );
	  if ( status != ML_STATUS_NO_ERROR ) {
	    DEBG1( printf( "[winaudio audio loop]: UpdateMessage failed, "
			   "status = %d\n", status ) );
	  }

	  // Reset pendingMsg pointer to indicate it was handled
	  pendingMsg = 0;
	} break;

	case ML_BUFFERS_IN_PROGRESS:
	  DEBG3( printf( "[winaudio audio loop]: got DI queue msg "
			 "ML_BUFFERS_IN_PROGRESS\n" ) );

	  // Attempt to handle this buffer now. This might not be
	  // possible -- we might need to defer this buffer (see
	  // handleIncomingBuffers() for details).
	  status = handleIncomingBuffers( path, state, &hdrPool,
					  pendingQEntry, pendingMsg,
					  &timeoutValue );

	  if ( status == ML_STATUS_WINAUDIO_DEFERRED ) {
	    // Do not handle right now.
	    //
	    // timeoutValue has been updated -- it is set to the
	    // number of milli-seconds before we can process the
	    // buffer, if applicable.
	    //
	    // Do NOT resent pendingMsg!!!
	    //
	    // Stop looking for new messages, since we can't process
	    // anything util this buffers message can be handled.
	    chkDIMessages = 0;

	    // This isn't actually an error! Make sure we reset the
	    // status
	    status = ML_STATUS_NO_ERROR;

	  } else {
	    if ( status != ML_STATUS_NO_ERROR ) {
	      // In case of error, reply to the message right now.
	      status =
		mlDIQueueUpdateMessage( pendingQEntry, ML_BUFFERS_FAILED );
	      if ( status != ML_STATUS_NO_ERROR ) {
		DEBG1( printf( "[winaudio audio loop]: UpdateMessage "
			       "(ML_BUFFERS_FAILED) failed, status = %d\n",
			       status ) );
	      }
	    }

	    // Reset pendingMsg to indicated it was handled
	    pendingMsg = 0;
	  } // if status == ML_STATUS_WINAUDIO_DEFERRED ... else ...
	  break;

	default:
	  DEBG1( printf( "[winaudio audio loop]: invalid msg type %d\n",
			 pendingMsgType ) );
	  status = ML_STATUS_INTERNAL_ERROR;
	  break;
	} // switch

      } // if pendingMsg != 0

    } // while chkDIMessages != 0

    if ( status != ML_STATUS_NO_ERROR ) {
      // On error, break out of the loop and terminate the child
      // thread
      break;
    }

    // Finally, attempt to "advance" the message queue -- ie: take all
    // processed messages from the head, and send them back to the
    // application. This is done here, at the end, because we may
    // complete the processing of messages in 2 different places: once
    // when receiving WW_WOM_DONE and WW_WIM_DATA thread messages, and
    // once when receiving ML_CONTROLS_IN_PROGRESS DI queue messages.
    status = mlDIQueueAdvanceMessages( path->pQueue );
    DEBG4( printf( "[winaudio audio loop]: Queue was advanced, "
		   "new send count = %d\n", path->pQueue->sendCount ) );
    if ( status != ML_STATUS_NO_ERROR ) {
      DEBG1( printf( "[winaudio audio loop]: AdvanceMessages failed\n" ) );
      // Break out of loop
      break;
    }

  } // while doneLoop == 0

  // Release parent thread if it is waiting on the abortComplete event
  // -- in particular, this event is "recycled" as a "Finishing
  // complete" event for proper and orderly shutdown.
  DEBG3( printf( "[winaudio audio loop]: signalling finish complete\n" ) );
  SetEvent( path->abortComplete );

  if ( status == ML_STATUS_NO_ERROR ) {
    // Free wave header pool
    freeWaveHdrPool( &hdrPool );
    DEBG1( printf( "[winaudio audio loop]: audio loop thread "
		   "exiting normally\n" ) );
  } else {
    // Do not free wave header pool -- there might be headers still in
    // use (because we aborted due to error), and that would cause an
    // assert, which simply muddles the issue.
    DEBG1( printf( "[winaudio audio loop]: audio loop thread "
		   "exiting on error\n" ) );
  }

  return (status == ML_STATUS_NO_ERROR) ? 0 : 1;
}
