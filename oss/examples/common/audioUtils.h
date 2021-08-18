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

#ifndef _AUDIOUTILS_H
#define _AUDIOUTILS_H


/*
	audioUtils.h

	Utilities for audio sample programs
*/

#include <ML/ml.h>
#include <audiofile.h>
#include <stdio.h>

/*
 * Audio UST / MSC utilities
 */


/* Get UST / MSC from message
 *
 * Retrieves the audio UST and MSC information from an MLpv message
 * (eg, such as a BUFFERS_COMPLETE message)
 *
 * If a pointer is NULL, that timestamp is not returned.
 *
 * If a timestamp is not present in the message, the corresponding
 * pointer is left un-affected.
 */
void getUstMscFromMsg( MLpv* msg, MLint64* ust, MLint64* msc );


/* Print UST / MSC contained in message
 *
 * Retrieves the audio UST and MSC information from an MLpv message
 * and prints it to the specified stream (generally stdout or stderr).
 *
 * Prints on 1 line, *without* adding a carriage return
 */
void printUstMscFromMsg( MLpv* msg, FILE* fp );


/* Structure used to track UST / MSC contained in buffer messages
 */
typedef struct AudioUstMscInfo_s {
  MLint64 veryFirstUST;
  MLint64 veryFirstMSC;

  MLint64 firstUST;
  MLint64 firstMSC;

  MLint64 lastUST;
  MLint64 lastMSC;

  MLint64 lastSeqLostMSC;
  MLint64 ustDiff;
  MLint64 mscDiff;
} AudioUstMscInfo;


/* Initialise the AudioUstMscInfo
 */
void initAudioUstMscInfo( AudioUstMscInfo* info );


/* Track the audio UST / MSC timestamps in a message
 *
 * Examines the supplied message based on its message type (both
 * assumed to be returned by the same call to mlReceiveMessage), and
 * keeps track of the UST / MSC timestamp information, in order to
 * ultimately be able to compute the effective transfer rate.
 *
 * The info structure *must* have been initialised (see above).
 */
void trackAudioUstMsc( MLpv* msg, MLint32 msgType, AudioUstMscInfo* info );


/* Compute effective transfer rate
 *
 * Using the UST / MSC info contained in the structure, compute the
 * effective transfer rate. Two different rates are computed:
 *
 * 1. overall rate, from start to finish
 * 2. rate of last un-interrupted sequence, ie: since the last SEQUENCE_LOST
 *
 * With a fully-compliant module, both rates should be approximately
 * equal.
 *
 * The result is printed to the specified stream (generally stdout or
 * stderr).
 */
void printEffectiveRate( AudioUstMscInfo* info, FILE* fp );


/*
 * Utilities to simplify creating, opening, reading and writing sound
 * files with libaudiofile
 */

/* Structure used to pass info to the utils
 */
typedef struct SoundFileInfo_s {
  AFfilehandle fHandle;
  int          channels;
  int          frameSize;
  AFframecount frameCount;
  double       rate;
} SoundFileInfo;

/* Create a sound file:
 *
 * Create the name file in a format appropriate for the sample
 * programs (WAVE format, 16-bit two's complement -- the number of
 * channels (1 or 2) and the rate, however, are set by the
 * caller). The SoundFileInfo structure is filled in with the relevant
 * info (the frameCount field is set to 0).
 *
 * Returns 0 on success, -1 on error.
 */
int soundFileCreate( SoundFileInfo* pInfo, const char* fName,
		     int numChannels, double rate );

/* Open a sound file:
 *
 * ensures the named file is in an appropriate format for the sample
 * programs (mono or stereo, 16-bit two's complement), and fills in
 * the SoundFileInfo structure.
 *
 * The file is opened for reading only.
 *
 * Returns 0 on success, -1 on error.
 */
int soundFileOpen( SoundFileInfo* pInfo, const char* fName );

/* Read from a sound file:
 *
 * Read the specified number of frames into the buffer. The
 * SoundFileInfo must refer to a previously-opened file.
 *
 * Returns the number of frames actually read.
 */
AFframecount soundFileRead( SoundFileInfo* pInfo, void* buffer,
			    AFframecount count );

/* Write to a sound file:
 *
 * Write the specified number of bytes from the buffer. The
 * SoundFileInfo must refer to a previously-created file.
 *
 * Returns the number of frames actually written.
 */
AFframecount soundFileWrite( SoundFileInfo* pInfo, void* buffer,
			     AFframecount count );

/* Close a sound file.
 */
int soundFileClose( SoundFileInfo* pInfo );


#endif // _AUDIOUTILS_H
