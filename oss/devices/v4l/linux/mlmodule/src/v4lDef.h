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
 ***************************************************************************/


#ifndef V4LDEF_H
#define V4LDEF_H

#include <pthread.h>
#include <ML/ml_didd.h>
#include "generic.h"

#define V4L_MAX_EVENTS 1

/* -----------------------------------------------------------------v4lSpecific
 */

typedef struct
{
  MLint32 videotiming;
  MLint32 videocolorspace;
  MLint32 videoprecision;

  MLint32 imagewidth;
  MLint32 imageheight1;
  MLint32 imageheight2;
  MLint32 imagecompression;
  MLint32 imagetemporal;
  MLint32 imageinterleave;
  MLint32 imagerowbytes;
  MLint32 imageskiprows;
  MLint32 imageskippixels;

  MLint32 imageorientation;
  MLint32 imagecolorspace;
  MLint32 imagepacking;
  MLint32 imagesampling;
  MLint32 imageSize;

  MLint32 deviceState;

  MLint32 events[V4L_MAX_EVENTS];
  MLint32 eventCount;

} v4lPathParams;

typedef struct 
{
  v4lPathParams      pathParams;   /* local copy */

  int                videofd;      /* video device file descriptor */
  pthread_t          thread;       /* transfer thread - only used for paths */
  pthread_mutex_t    mutex;        /* mutex for access to parameters */
  MLqueueRec*        pQueue;       /* the queue between app and device */
                                   /* currently used only for paths. */
} v4lOpen;

typedef struct
{
  char name[128];
  MLint32 type;
  MLint32 index;
} v4lChannelInfo;

typedef struct
{
  MLint32 size;
  char name[128];
  char location[256];
  MLint32 index;
  MLint32 numChannels;
  v4lChannelInfo channels[10];
} v4lDeviceInfo;


#endif /* V4LDEF_H */

