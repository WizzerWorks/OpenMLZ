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

#ifndef _ML_AUDIO_CAPTURE_H_
#define _ML_AUDIO_CAPTURE_H_

#include <Windows.h>

// Application-private messages

// WMA_MLAC_ERROR: error encountered in child thread
//   wParam: not used
//   lParam: pointer to error string. Must be freed after use.
#define WMA_MLAC_ERROR (WM_APP + 1)

// WMA_MLAC_DROPPED_BUFF: buffer dropped ('ML_SEQUENCE_LOST')
//   wParam: number dropped since start of transfer
//   lParam: time (in milli-secs) of latest dropped buff
#define WMA_MLAC_DROPPED_BUFF (WM_APP + 2)

// WMA_MLAC_TOTAL_TIME: total transferred time
//   wParam: not used
//   lParam: total accumulated transfer time (in milli-secs)
#define WMA_MLAC_TOTAL_TIME (WM_APP + 3)

// WMA_MLAC_EFFECTIVE_RATE: effective transfer rate
//   wParam: transfer rate (in Hz)
//   lParam: not used
#define WMA_MLAC_EFFECTIVE_RATE (WM_APP + 4)

// WMA_MLAC_RSIGNAL_STRENGTH:
// WMA_MLAC_LSIGNAL_STRENGTH: max signal strength of right & left
// channel in last buffer
//   wParam: max strength
//   lParam: max possible value
// In other words, to normalize the strength to the [0,1] range:
//   normStrength = (float)wParam / (float)lParam  
#define WMA_MLAC_RSIGNAL_STRENGTH (WM_APP + 5)
#define WMA_MLAC_LSIGNAL_STRENGTH (WM_APP + 6)

#endif // _ML_AUDIO_CAPTURE_H_
