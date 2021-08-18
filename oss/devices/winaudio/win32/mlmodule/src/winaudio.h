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

#ifndef _WINAUDIO_H_
#define _WINAUDIO_H_

#include <mmsystem.h>

// I believe these definitions are valid, even though they do not
// appear in my header files...
#ifndef WAVE_FORMAT_48M08
#define WAVE_FORMAT_48M08 0x00001000
#define WAVE_FORMAT_48S08 0x00002000
#define WAVE_FORMAT_48M16 0x00004000
#define WAVE_FORMAT_48S16 0x00008000
#define WAVE_FORMAT_96M08 0x00010000
#define WAVE_FORMAT_96S08 0x00020000
#define WAVE_FORMAT_96M16 0x00040000
#define WAVE_FORMAT_96S16 0x00080000
#endif

#ifdef DEBUG
#include <stdlib.h>
extern int debugLevel;
// error printouts
#define DEBG1(block) if (debugLevel >= 1) { block; fflush(stdout); }
// basic function printouts
#define DEBG2(block) if (debugLevel >= 2) { block; fflush(stdout); }
// detail printouts
#define DEBG3(block) if (debugLevel >= 3) { block; fflush(stdout); }
// extreme debug printouts
#define DEBG4(block) if (debugLevel >= 4) { block; fflush(stdout); }
#else
#define DEBG1(block)
#define DEBG2(block)
#define DEBG3(block)
#define DEBG4(block)
#endif

// Requested device events (ie: those for which the client would like
// to receive notifications.
//
// Note: RATE_CHANGED_EVENT is when the input (record) rate has
// changed -- but this doesn't happen with WAVE devices. The rate only
// changes if the user asks for it. So this event is "supported", but
// never actually happens.
enum WaveRequestedEvents {
  WAVE_NO_EVENTS = 0x0,
  WAVE_SEQUENCE_LOST_EVENT = 0x1,
  RATE_CHANGED_EVENT = 0x2
};

// Volume-control support in the WAVE device
//
// Some devices do not support volume control at all, some support
// only "mono" volume (both channels at once), and some support
// independent L+R volume.
//
// Note that input devices NEVER support volume control.

enum WaveVolumeControl {
  WAVE_VOLUME_NO_CONTROL,
  WAVE_VOLUME_SINGLE_CONTROL,
  WAVE_VOLUME_L_R_CONTROL
};

// Description of Wave devices.
//
// We assume that devices have at most 1 input and 1 output -- this is
// valid, since we consider a WAVE device to be identified by its ID,
// and each ID can correspond to only 1 input and 1 output in the WAVE
// API. In other words, if a physical device supports multiple inputs
// or outputs, the WAVE API will report those as separate devices, and
// so will we.
//
// The devId field is the actual ID, used in calls to the WAVE API.
//
// It is guaranteed that all WAVE devices will have at least an input
// or an output, ie: the number of jacks is either 1 or 2.  To
// determine the number of jacks, it is valid to add the "hasInput"
// and "hasOutput" fields.

typedef struct _WaveDeviceDetails {
  UINT_PTR devId;
  char name[10];        // Mainly for debugging
  int numJacks;         // 1 or 2, depending on input and/or output caps
  int jackDir[2];       // ML_DIRECTION_IN or ML_DIRECTION_OUT
  DWORD jackFormats[2]; // Formats as per WAVEINCAPS / WAVEOUTCAPS
  DWORD outSupport;     // Supported features of output jack (if present)
} WaveDeviceDetails;

// Description of an open audio path.
//
// This structure is divided into two types of fields: the first half
// are accessed by a single thread, or are themselves thread-safe
// (such as the ML queuing mechanism: it is designed to be used by 1
// reader and 1 writer with no locking); this half is not protected by
// any lock. The second half of the fields may be accessed both by the
// application thread or the audio look thread, and must thus be
// placed under the protection of a lock.

typedef struct _AudioPath {
  // Fields that never change once the AudioPath is created and
  // initialised
  int direction;        // ML_DIRECTION_IN or ML_DIRECTION_OUT
  UINT devId;           // Needed in case we must re-open the device
  DWORD capabilities;   // as per WAVEINCAPS / WAVEOUTCAPS
  enum WaveVolumeControl volumeControl;

  // The "openId", as provided in all "dd" calls. Stored here (during
  // ddOpen) so that it is available to the audio loop: it may need it
  // to make certain mlDI... calls
  MLopenid openId;

  // Pointer to system-wide UST-generating function. Obtained and
  // cached during the ddOpen call.
  MLint32 (*USTSourceFunc)( MLint64* );

  // These 2 fields are only set once, when the child is created. They
  // are then reset when the child is terminated, and do not change
  // again.
  HANDLE childHandle;   // handle to child thread allocated to this device
  unsigned childId;     // thread ID of child (different from the Handle!)

  // The queue does not require locking, as it is designed for
  // one-reader, one-writer operation.
  MLqueueRec *pQueue;   // message queue

  // Event to signal that a new message is on the DI queue. This only
  // gets signaled when a new message is pushed -- as opposed to the
  // deviceWaitable in the DI queue itself, which remains signaled as
  // long as there are messages in the queue (not just new messages).
  HANDLE newDIMessage;  // Windows Event

  // Event to signal the end of an ABORTING state change -- sequence
  // is as follows: the application thread sets state to ABORTING and
  // waits on the event; eventually, the child thread notices the
  // state change, aborts the message queue, and signals the
  // event. The parent thread is released and may safely assume that
  // the ABORT is complete.
  HANDLE abortComplete; // Windows Event

  // Counter of outstanding buffers in the WAVE device queue (ie:
  // buffers supplied by the application, and passed on to the device
  // driver for reading or writing). Used to detect SEQUENCE_LOST
  // situations.
  //
  // No need for locking, this is used only by the audio loop.
  int numOutstandingBuffers;

  // Next MSC value:
  //
  // This is the MSC timestamp of the first slot following the last
  // buffer to be enqueued. In other words, it is the MSC of the first
  // sample of the next buffer.
  //
  // The MSC is maintained entirely by the module. It is not, for
  // instance, based on the WAVE device's "position" information.
  //
  // This is used solely by the audio loop, so no lock is required.
  MLint64 nextMSC;

  // Handling of idle periods:
  //
  // If the application fails to provide sufficient buffers to keep
  // the device busy -- ie: a buffer underflow, aka SEQUENCE_LOST --
  // the device becomes idle. The problem with this is that we can no
  // longer keep track of MSC slots in the usual way.
  //
  // So when we enter an idle period, we note the current UST. When we
  // detect that the idle period is about to end -- because we've
  // received a buffer from the application -- we use the new UST and
  // the saved UST to compute the idle time, and derive an MSC from
  // that.
  //
  // If this value is zero (which we assume will never be a valid
  // UST), then the module is not idle.
  MLint64 startOfIdleUST;

  // The remainder of this structure may be used both by the
  // application (parent) thread and the audio loop (child) thread,
  // and is used read-write. Thus, it requires locking.
  CRITICAL_SECTION lock;

  // The format of the device may change, if parameters are adjusted
  // (eg: changing the sample rate, etc.). Changing this requires
  // closing and re-opening the WAVE device, hence the handles may
  // change as well.
  union {
    HWAVEIN  inHandle;
    HWAVEOUT outHandle;
  };
  WAVEFORMATEX format;  // current format of the device, cf. WAVE API

  // State and "requested events" -- both can be changed via
  // setControls or sendControls.
  int state;            // I/O state
  int eventsWanted;     // Combination of the WaveRequestedEvents enums
} AudioPath;


// Helper functions to access fields in a thread-safe manner (they
// will acquire the lock)
extern int getState( AudioPath* path );
extern int setState( AudioPath* path, int newState );
extern int getEventsWanted( AudioPath* path );
extern int setEventsWanted( AudioPath* path, int newEventsWanted );
extern void getWaveFormatEx( AudioPath* path, WAVEFORMATEX* pFormat );
extern void* getDevHandle( AudioPath* path );

#endif // _WINAUDIO_H_
