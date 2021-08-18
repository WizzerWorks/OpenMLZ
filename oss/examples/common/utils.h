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

#ifndef _UTILS_H
#define _UTILS_H

#include <ML/ml.h>
#include <stdio.h>

/*
	utils.h

	Utilities for sample programs
*/


/* Wait on event
 *
 * Wait on an ML waitable event. Used, for instance, to wait for the
 * message queue to be non-empty.
 */
MLstatus event_wait( MLwaitable pathWaitHandle );


/* Print params contained in an MLpv message
 *
 * This prints out the param/value pairs contained in the message,
 * along with the length, if that isn't 1 (in particular, this is
 * useful if the length is -1, as that denotes a param error)
 *
 * This uses the id of the (open) path with which this MLpv message is
 * associated; this is used to obtain the string representation of
 * each param. If the param is not valid for the path, an error
 * message will be printed instead.
 *
 * The output is sent to the specified stream -- generally stdout or
 * stderr.
 */
void printParams( MLint64 pathId, MLpv* msg, FILE* fp );


/* Print jack name
 *
 * This prints out the message "Using jack: <jack name>", to the supplied
 * stream (generally stdout or stderr).
 *
 * The jack name is obtained by querying the capabilities of the
 * supplied jack ID. If an error occurs, a message is sent to the
 * stream.
 */
void printJackName( MLint64 jackId, FILE* fp );


/* Check if the path supports the parameter, and optionally the specified
 * access mode to the parameter
 *
 * Queries the specified path to determine if the parameter (control)
 * is supported.
 *
 * Optionally, if "desiredAccess" is non-zero, it is interpreted as a
 * combination of access mode flags (eg, READ, WRITE, DURING_TRANSFER,
 * etc). In this case, the routine also verifies that the parameter
 * supports the desired access mode.
 *
 * Returns 1 if the param is supported (and the desired access, if
 * any, is allowed), 0 if it isn't or if an error occured.
 */
int checkPathSupportsParam( MLint64 pathId, MLint64 param,
			    MLint32 desiredAccess );


/*
 * Buffer management utilities
 */

/* Allocate buffers
 *
 * Allocated buffers of the specified size (in bytes), with the given
 * alignment.
 *
 * All buffers are touched after allocation, to ensure they are mapped
 * -- however, the memory is not pinned (so the pages could
 * potentially be un-mapped).
 *
 * Returns 0 on success, -1 on error.
 */
MLint32 allocateBuffers( void** buffers, MLint32 bufferSize,
			 MLint32 maxBuffers, MLint32 memAlignment );

/* Free buffers
 *
 * Free buffers that were originally allocated by allocateBuffers. The
 * maxBuffers parameter should match the one used in the call to
 * allocateBuffers.
 *
 * Returns 0 on success, -1 on error.
 */
MLint32 freeBuffers( void** buffers, MLint32 maxBuffers );


#endif /* _UTILS_H */
