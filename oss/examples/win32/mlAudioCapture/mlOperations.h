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

#ifndef _ML_OPERATIONS_H
#define _ML_OPERATIONS_H

#include <Windows.h> // for MAX_PATH
#include <ml/ml.h>

// Structure to hold info about a usable path
typedef struct {
  char* pathName;
  MLint64 pathID;
  MLboolean monoOK;
  MLboolean stereoOK;
} PathInfo;

// Structure to hold info about a usable device
typedef struct {
  char* devName;
  MLint64 devID;
  int numPaths;
  PathInfo* paths;
} DevInfo;

// Structure to hold info about current system
typedef struct {
  char* sysName;
  int numDevs;
  DevInfo* devs;
} CurSysInfo;

// Structure to hold details about the audio-to-file transfer
// (Filled-in by the UI thread, passed to the child thread so it can
// perform the actual transfer)
typedef struct {
  MLint64 devID;
  MLint64 pathID;
  MLint32 numChannels;
  MLreal64 rate;
  char outFile[ MAX_PATH + 1 ];
  MLint32 numBuffers;
} TransferParams;

// Get the error caused by the last ML operation. This is not
// thread-safe -- it should only be called for the ML operations that
// are not performed in the child thread (ie: *not* for transfer
// operations, just for setup etc.)
//
// Returns a pointer to a static buffer that will be overwritten at
// the next ML operation.
const char* getMLError( void );

// Get the ML info for the current system -- including available
// devices and paths.
//
// The returned structure must be freed by the caller -- see below.
const CurSysInfo* getSysInfo( void );

// Free the system info structure allocated by the call to
// getSysInfo()
void freeSysInfo( const CurSysInfo* sysInfo );

// Transfer operations:
//
// - prepareTransfer: called by the UI thread to prepare the transfer,
//   according to the supplied parameters.  This creates the child
//   thread.
//
// - startTransfer / stopTransfer: instruct the child thread to start
//   and stop recording. When stopped, the child thread is idling.
//
// - finishTransfer: clean up resources (ie: buffers) and destroy child
//   thread.
//
// All functions return ML_TRUE on success, ML_FALSE on error.
MLboolean prepareTransfer( const TransferParams* params, HWND hwnd );
MLboolean startTransfer( void );
MLboolean stopTransfer( void );
MLboolean finishTransfer( void );

// Additional child thread states:
//
// - startMonitoring / stopMonitoring: instruct child thread to
//   examine the contents of every buffer, and send the UI thread a
//   message with the max signal level for each channel.
//
// Monitoring may be performed independently of transfering -- ie, it
// is not necessary to start a transfer in order to start
// monitoring. "FinishTransfer", on the other hand, will kill the
// child thread and will thus end the monitoring.
//
// All functions return ML_TRUE on success, ML_FALSE on error.
MLboolean startMonitoring( void );
MLboolean stopMonitoring( void );

#endif // _ML_OPERATIONS_H
