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


/* This is an example showing how to connect a single-stream transform to
 * the mlSDK DI layer.  This transform simply copies its input to its output.
 */

#ifndef _nullxcode_h_
#define _nullxcode_h_ 1

#include <ML/ml.h>
#include <ML/mlqueue.h>
#include <ML/ml_didd.h>
#include <ML/mlu.h>

typedef MLint32 bool;
static const bool false = 0;
static const bool true = 1;

/* We only support the two required transcoder events,
 * ML_EVENT_XCODE_FAILED and ML_EVENT_DEVICE_UNAVAILABLE
 */
#define MAX_EVENTS 2

/* General detail of a ml param
 */
typedef struct {
    MLint64  id;
    char*    name;
    MLint32  type;
    MLint32  access;
    MLint32* deflt;
    MLint32* mins; 
    MLint32  minsLength;
    MLint32* maxs; 
    MLint32  maxsLength;
    MLint32* enumValues; 
    MLint32  enumValuesLength;
    char*    enumNames; 
    MLint32  enumNamesLength;
} MLDDparamDetails;


/* A struct describing the current settings on each of
 * our pipes.  This will change depending on what controls
 * are important to a particular transcoder.
 */
typedef struct {
    MLint32 compression;
    MLint32 colorspace;
    MLint32 packing;
    MLint32 sampling;
    MLint32 temporalSampling;
    MLint32 width;
    MLint32 height;
    MLint32 orientation;
} MLDDxcodeParams;
  
/* This struct is passed to each thread at startup.  It contains
 * an alias pointer to the shared MLDDopen and some thread-specific
 * state.
 */
typedef struct {
    _mlOSThread   threadId;
    void*       pOpen; /* really a MLDDopen* */
    char        stateData[100];
} ThreadData;


/* An opaque pointer to this struct is passed about as the ddPriv
 * for each open transcoder.  It contains all of the instance-specific
 * data needed by an instance of our transcoder.
 */
typedef struct {
    bool		 isAsync;
    bool                 useCopyEngine;
    bool                 buffersSent;
    int                  deviceState;
    MLopenDeviceContext diContext;
    MLopenid             myId;
    MLint64              ust;
    MLint64              msc;
    MLint32              events[MAX_EVENTS];
    MLint32              eventCount;
    
    int                  numThreads;
    ThreadData*          threadData;
    
    MLDDxcodeParams	 inParams;
    MLDDxcodeParams	 outParams;
    
    _mlOSLockableLite      mutex;
    
    MLqueueRec*          q;
} MLDDopen;
  

#endif /* _nullxcode_h_ */
