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

/*
	audioUtils.c

	Utilities for audio sample programs
*/

#include "audioUtils.h"

#include <assert.h>
#include <stdio.h>


/*-------------------------------------------------------------getUstMscFromMsg
 */
void getUstMscFromMsg( MLpv* msg, MLint64* ust, MLint64* msc )
{
  MLpv* param;

  param = mlPvFind( msg, ML_AUDIO_UST_INT64 );
  if ( (param != NULL) && (param->length != -1) ) {
    *ust = param->value.int64;
  }

  param = mlPvFind( msg, ML_AUDIO_MSC_INT64 );
  if ( (param != NULL) && (param->length != -1) ) {
    *msc = param->value.int64;
  }
}


/*-----------------------------------------------------------printUstMscFromMsg
 */
void printUstMscFromMsg( MLpv* msg, FILE* fp )
{
  MLint64 ust = -1, msc = -1;

  getUstMscFromMsg( msg, &ust, &msc );

  fprintf( fp, "UST = " );
  if ( ust == -1 ) {
    fprintf( fp, "(N/A)" );
  } else {
    fprintf( fp, "%" FORMAT_LLD, ust );
  }
  fprintf( fp, ", " );

  fprintf( fp, "MSC = " );
  if ( msc == -1 ) {
    fprintf( fp, "(N/A)" );
  } else {
    fprintf( fp, "%" FORMAT_LLD, msc );
  }
}


/*----------------------------------------------------------initAudioUstMscInfo
 */

void initAudioUstMscInfo( AudioUstMscInfo* info )
{
  info->veryFirstUST = -1;
  info->veryFirstMSC = -1;
  info->firstUST = -1;
  info->firstMSC = -1;
  info->lastUST = -1;
  info->lastMSC = -1;
  info->lastSeqLostMSC = -1;
  info->ustDiff = 0;
  info->mscDiff = 0;
}


/*-------------------------------------------------------------trackAudioUstMsc
 *
 * Keeping track of UST and MSC in order to compute effective rate
 * when the transfer is complete.
 *
 * The effective rate is: (lastMSC - firstMSC) / (lastUST - firstUST)
 *
 * However, if we get a SEQUENCE_LOST, we reset the "first" timestamps
 * to be those of the first buffer *after* the loss of sync.
 *
 * Note that the device may deliver the SEQUENCE_LOST message
 * out-of-order with respect to the return of buffers (ie: the last
 * buffer prior to the loss of sync may be returned *after* the event
 * message). So we also keep track of the timsestamp of the last even
 * message, to know when we are processing the first message *after*
 * the event.
 */
void trackAudioUstMsc( MLpv* msg, MLint32 msgType, AudioUstMscInfo* info )
{
  switch ( msgType ) {

  case ML_BUFFERS_COMPLETE: {
    MLint64 ust, msc;
    getUstMscFromMsg( msg, &ust, &msc );

    if ( info->veryFirstUST == -1 ) {
      info->veryFirstUST = ust;
      info->veryFirstMSC = msc;
    }

    if ( info->firstUST == -1 ) {
      /* Make sure this buffer comes after the last SEQUENCE_LOST
       * event, otherwise, don't use its timestamps.
       */
      if ( msc > info->lastSeqLostMSC ) {
	info->firstUST = ust;
	info->firstMSC = msc;
      }
    } else {
      info->lastUST = ust;
      info->lastMSC = msc;
    }
  } break;

  case ML_EVENT_AUDIO_SEQUENCE_LOST: {
    MLint64 dummy;

    /* If we have enough info -- ie: some buffers transferred since
     * last sequence loss -- compute the ust and msc delta for that
     * part of the transfer.
     */
    if ( (info->firstUST != -1) && (info->lastUST != -1) ) {
      info->ustDiff += (info->lastUST - info->firstUST);
      info->mscDiff += (info->lastMSC - info->firstMSC);
    }

    /* Reset 'first' timestamps, which correspond to the first
     * time-stamp of an un-interrupted transfer (a sequence loss is an
     * interruption).
     */
    info->firstUST = -1;
    info->firstMSC = -1;

    /* Keep track of the MSC of the sequence loss, so that we can tell
     * if a buffer comes before or after it (we don't care about the
     * UST in this case).
     */
    getUstMscFromMsg( msg, &dummy, &info->lastSeqLostMSC );
  } break;

  default:
    /* Nothing to do for this type of message
     */
    break;
  } /* switch */
}


/*-----------------------------------------------------------printEffectiveRate
 */
void printEffectiveRate( AudioUstMscInfo* info, FILE* fp )
{
  /* Compute overall effective rate (includes periods of loss of sync)
   * using UST and MSC, if we have the appropriate data (ie: first and
   * last UST / MSC pair)
   */
  if ( (info->veryFirstUST != -1) && (info->lastUST != -1) ) {
    float ustDiff = (float) (info->lastUST - info->veryFirstUST);
    float mscDiff = (float) (info->lastMSC - info->veryFirstMSC);

    if ( (mscDiff == 0) || (ustDiff == 0) ) {
      fprintf( fp, "UST / MSC information appears invalid, can not compute "
	       "transfer rate:\nUST diff = %.0f, MSC diff = %.0f\n",
	       ustDiff, mscDiff );

    } else {
      double rate = mscDiff * 1.0E9 / ustDiff;
      fprintf( fp, "Effective transfer rate = %.2f Hz\n", rate );
    }
  } else {
    fprintf( fp, "Insufficient data to compute effective transfer rate.\n" );
  }

  if ( (info->ustDiff != 0) && (info->mscDiff != 0) ) {
    double rate = (double) info->mscDiff * 1.0E9 / (double) info->ustDiff;
    fprintf( fp, "Effective transfer rate ignoring "
	     "sequence losses = %.2f Hz\n", rate );
  }
}


/*--------------------------------------------------------------soundFileCreate
 */
int soundFileCreate( SoundFileInfo* pInfo, const char* fName,
		     int numChannels, double rate )
{
  AFfilesetup outputFileSetup;
  AFfilehandle outputFile;

  assert( pInfo != 0 );
  assert( (numChannels == 1) || (numChannels == 2) );

  outputFileSetup = afNewFileSetup();
  if( outputFileSetup == AF_NULL_FILESETUP ) {
    fprintf( stderr, "Could not allocated setup data\n" );
    return -1;
  }

  afInitChannels( outputFileSetup, AF_DEFAULT_TRACK, numChannels );
  afInitFileFormat( outputFileSetup, AF_FILE_WAVE );
  afInitSampleFormat( outputFileSetup, AF_DEFAULT_TRACK,
		     AF_SAMPFMT_TWOSCOMP, 16 );
  afInitRate( outputFileSetup, AF_DEFAULT_TRACK, rate );

  outputFile = afOpenFile( fName, "w", outputFileSetup );

  afFreeFileSetup( outputFileSetup );

  if ( outputFile == AF_NULL_FILEHANDLE ) {
    fprintf( stderr, "Could not open output file '%s'\n", fName );
    return -1;
  }

  pInfo->fHandle = outputFile;
  pInfo->channels = numChannels;
  pInfo->frameSize = (int) afGetFrameSize( outputFile, AF_DEFAULT_TRACK, 0 );
  pInfo->frameCount = 0;
  pInfo->rate = afGetRate( outputFile, AF_DEFAULT_TRACK );

  return 0;
}


/*----------------------------------------------------------------soundFileOpen
 */
int soundFileOpen( SoundFileInfo* pInfo, const char* fName )
{
  AFfilehandle inputFile = 0;
  int channels;
  int sampleFormat, sampleWidth;

  inputFile = afOpenFile( fName, "r", NULL );
  if ( inputFile == AF_NULL_FILEHANDLE ) {
    fprintf( stderr, "Can not open sound file '%s'\n", fName );
    return -1;
  }

  channels = afGetChannels( inputFile, AF_DEFAULT_TRACK );
  if ( channels != 1 && channels != 2 ) {
    fprintf( stderr, "Only monophonic and stereophonic audio files "
	     "are supported.\n" );
    return -1;
  }

  afGetSampleFormat( inputFile, AF_DEFAULT_TRACK, &sampleFormat,
		     &sampleWidth );
  if ( sampleFormat != AF_SAMPFMT_TWOSCOMP || sampleWidth != 16 ) {
    fprintf( stderr, "only 16-bit two's complement audio files "
	     "are supported.\n" );
    return -1;
  }

  pInfo->fHandle = inputFile;
  pInfo->channels = channels;
  pInfo->frameSize = (int) afGetFrameSize( inputFile, AF_DEFAULT_TRACK, 1 );
  pInfo->frameCount = afGetFrameCount( inputFile, AF_DEFAULT_TRACK );
  pInfo->rate = afGetRate( inputFile, AF_DEFAULT_TRACK );

  return 0;
}


/*----------------------------------------------------------------soundFileRead
 */
AFframecount soundFileRead( SoundFileInfo* pInfo, void* buffer,
			    AFframecount count )
{
  assert( pInfo != 0 );

  return afReadFrames( pInfo->fHandle, AF_DEFAULT_TRACK, buffer, count );
}


/*---------------------------------------------------------------soundFileWrite
 */
AFframecount soundFileWrite( SoundFileInfo* pInfo, void* buffer,
			     AFframecount count )
{
  AFframecount numWritten = 0;

  assert( pInfo != 0 );

  numWritten = afWriteFrames( pInfo->fHandle,
			      AF_DEFAULT_TRACK, buffer, count );
  pInfo->frameCount += numWritten;

  return numWritten;
}

/*---------------------------------------------------------------soundFileClose
 */
int soundFileClose( SoundFileInfo* pInfo )
{
  assert( pInfo != 0 );

  return afCloseFile( pInfo->fHandle );
}

