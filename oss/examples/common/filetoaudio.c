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
	filetoaudio.c

	Play a file through a specified mlSDK path.
*/

#include <ML/ml.h>
#include <ML/mlu.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <malloc.h>

#ifndef	ML_OS_NT
#include <unistd.h>
#else
#include <ML/getopt.h>
#endif

#include "utils.h"
#include "audioUtils.h"

#define MAX_BUFFERS 8
#define FRAMES 1024


/*------------------------------------------------------------------------usage
 */
void usage( char *prog )
{
  fprintf(stderr,
	  "usage: %s -d <device name> [ -I <device index> ] "
	  " [-j <output jack> ] [ -p <pause interval> ] [ -v ] <sound file>\n",
	  prog);
  exit( 0 );
}


/*-------------------------------------------------------------------------main
 */
int main( int argc, char **argv )
{
#ifndef ML_OS_NT
  extern char *optarg;
#endif

  /* The following would typically come from a UI or a file, but for
   * this example, we just hard-code them in.
   */
  MLstatus status = ML_STATUS_NO_ERROR;
  MLint32 bufferSize;
  MLint32 delivered_buffers = 0;
  MLint32 maxBuffers = MAX_BUFFERS;
  int buffersComplete = 0;

  void *buffers[MAX_BUFFERS];
  MLint32 i;
  MLint32 memAlignment;
  MLint64 deviceID = 0, pathId = 0, jackId = 0;
  MLopenid hPath = 0;
  MLwaitable pathWaitHandle;
  int verbose = 0;

  int deviceIndex = -1;
  char* deviceName = 0;
  char* jackName = 0;
  char* input_file_name;
  SoundFileInfo sfInfo;
  AFframecount deliveredFrames = 0;

  int pauseInterval = 0;
  AudioUstMscInfo ustMscInfo;

  initAudioUstMscInfo( &ustMscInfo );

  while ( (i = getopt( argc, argv, "d:I:j:p:v" )) != -1 ) {
    switch ( i ) {
    case 'd':
      deviceName = optarg;
      break;

    case 'I':
      deviceIndex = atoi( optarg );
      break;

    case 'j':
      jackName = optarg;
      break;

    case 'p':
      pauseInterval = atoi( optarg );
      break;

    case 'v':
      ++verbose;
      break;

    default:
      usage( argv[0] );

    } /* switch */
  } /* while i = getopt ... */

  if ( (deviceName == 0) || (optind >= argc) ) {
    usage( argv[0] );
  }

  input_file_name = argv[ optind ];

  fprintf( stdout, "Reading from file '%s'\n", input_file_name );
  if ( pauseInterval > 0 ) {
    fprintf( stdout, "Pausing every %d buffers\n", pauseInterval );
  }

  if ( soundFileOpen( &sfInfo, input_file_name ) ) {
    fprintf( stderr, "Error accessing sound file\n" );
    return -1;
  }

  fprintf( stdout, "Sampling rate: %.0f Hz, %s\n", sfInfo.rate,
	   (sfInfo.channels == 2) ? "Stereo" : "Mono" );

  if ( deviceIndex == -1 ) {
    status = mluFindDeviceByName( ML_SYSTEM_LOCALHOST, deviceName, &deviceID );
    if ( status != ML_STATUS_NO_ERROR ) {
      fprintf( stderr, "Cannot find device '%s': %s\n", deviceName,
	       mlStatusName( status ) );
      return -1;
    }

  } else {
    status = mluFindDeviceByNameAndIndex( ML_SYSTEM_LOCALHOST, deviceName,
					  deviceIndex, &deviceID );
    if ( status != ML_STATUS_NO_ERROR ) {
      fprintf(stderr, "Cannot find device '%s' with index %d: %s\n",
	      deviceName, deviceIndex, mlStatusName( status ) );
      return -1;
    }
  }

  if ( jackName ) {
    status = mluFindJackByName( deviceID, jackName, &jackId );
    if ( status != ML_STATUS_NO_ERROR ) {
      fprintf( stderr, "Cannot find output jack '%s': %s\n", jackName,
	       mlStatusName( status ) );
      return -1;
    }

  } else {
    status = mluFindFirstOutputJack( deviceID, &jackId );
    if ( status != ML_STATUS_NO_ERROR ) {
      fprintf( stderr, "Cannot find a suitable output jack: %s\n",
	       mlStatusName( status ) );
      return -1;
    }
  }

  printJackName( jackId, stdout );
  fflush( stdout );

  status = mluFindPathToJack( jackId, &pathId, &memAlignment );
  if ( status != ML_STATUS_NO_ERROR ) {
    fprintf( stderr, "Cannot find a path from memory to that jack: %s\n",
	     mlStatusName( status ) );
    return -1;
  }

  bufferSize = FRAMES * sfInfo.frameSize;

  /* In order to do capture, we send buffers to the path so they can
   * be filled in. Here we allocate buffers with desired alignment.
   */
  if ( allocateBuffers( buffers, bufferSize, maxBuffers, memAlignment ) ) {
    fprintf( stderr, "Cannot allocate memory for input buffers\n" );
    return -1;
  }

  status = mlOpen( pathId, NULL, &hPath );
  if ( status != ML_STATUS_NO_ERROR ) {
    fprintf( stderr, "Cannot open path: %s\n", mlStatusName( status ) );
    goto SHUTDOWN;
  }

  /* Now set the playback attributes for 16 bit 2's complement, 1
   * channel, no compression, 44.1 kHz sampling rate, -6dB gain.
   */
  { /* set attributes scope */
    MLpv controls[6];
    MLreal64 gain = -6;	/* decibels */

    controls[0].param = ML_AUDIO_FORMAT_INT32;
    controls[0].value.int32 = ML_AUDIO_FORMAT_S16;
    controls[0].length = 1; /* to help debugging */
    controls[1].param = ML_AUDIO_CHANNELS_INT32;
    controls[1].value.int32 = sfInfo.channels;
    controls[1].length = 1;
    controls[2].param = ML_AUDIO_GAINS_REAL64_ARRAY;
    controls[2].value.pReal64 = &gain;
    controls[2].length = controls[3].maxLength = 1;
    controls[3].param = ML_AUDIO_COMPRESSION_INT32;
    controls[3].value.int32 = ML_COMPRESSION_UNCOMPRESSED;
    controls[3].length = 1;
    controls[4].param = ML_AUDIO_SAMPLE_RATE_REAL64;
    controls[4].value.real64 = sfInfo.rate;
    controls[4].length = 1;
    controls[5].param = ML_END;

    status = mlSetControls( hPath, controls );
    if ( status != ML_STATUS_NO_ERROR ) {
      fprintf( stderr, "Couldn't set controls: %s\n", mlStatusName( status ) );
      printParams( hPath, controls, stderr );
      goto SHUTDOWN;
    }
  } /* set attributes scope */

  /* Set event mask; we're interested in sequence lost events.
   */
  { /* set events scope */
    MLint32 events[] = { ML_EVENT_AUDIO_SEQUENCE_LOST };
    MLpv controls[2];

    controls[0].param = ML_DEVICE_EVENTS_INT32_ARRAY;
    controls[0].value.pInt32 = events;
    controls[0].length = 1;
    controls[0].maxLength = controls[0].length;

    controls[1].param = ML_END;

    status = mlSetControls( hPath, controls );
    if ( status != ML_STATUS_NO_ERROR ) {
      fprintf( stderr, "Cannot set events for path: %s\n",
	       mlStatusName( status ) );
      return -1;
    }
  } /* set events scope */

  /* We can either poll mlReceiveMessage, or allow a wait variable to
   * be signalled. Polling is undesirable -- it's much better to wait
   * for an event - that way the OS can swap us out and make full use
   * of the processor until the event occurs.
   */
  status = mlGetReceiveWaitHandle( hPath, &pathWaitHandle );
  if ( status != ML_STATUS_NO_ERROR ) {
    fprintf( stderr, "Cannot get wait handle: %s\n", mlStatusName( status ) );
    goto SHUTDOWN;
  }

  /* And then preroll the queue. Our assumption here that the send
   * queue is larger than the preroll size.
   */
  for ( i = 0; i < maxBuffers; i++ ) {
    MLpv msg[4];
    int framesRead;

    framesRead = (int) soundFileRead( &sfInfo, buffers[i], FRAMES );
    if ( framesRead == 0 ) {
      break;
    }

    msg[0].param = ML_AUDIO_BUFFER_POINTER;
    msg[0].value.pByte = (MLbyte *) buffers[i];
    msg[0].length = framesRead * sfInfo.frameSize;
    msg[1].param = ML_AUDIO_UST_INT64;
    msg[2].param = ML_AUDIO_MSC_INT64;
    msg[3].param = ML_END;

    status = mlSendBuffers( hPath, msg );
    if ( status != ML_STATUS_NO_ERROR ) {
      fprintf( stderr, "Error sending buffers (pre-roll): %s\n",
	       mlStatusName( status ) );
      goto SHUTDOWN;
    }
  } /* for i=0..maxBuffers */

  /* Now start the video transfer. */
  status = mlBeginTransfer( hPath );
  if ( status != ML_STATUS_NO_ERROR ) {
    fprintf( stderr, "Error beginning transfer: %s\n",
	     mlStatusName( status ) );
    goto SHUTDOWN;
  }

  while ( deliveredFrames < sfInfo.frameCount ) {
    MLint32 messageType;
    MLpv *message;

    status = event_wait( pathWaitHandle );
    if ( status != ML_STATUS_NO_ERROR ) {
      fprintf( stderr, "Error waiting on event: %s\n", mlStatusName( status ));
      goto SHUTDOWN;
    }

    /* Let's see what reply message is ready...  Read the top message
     * on the receive queue.
     */
    status = mlReceiveMessage( hPath, &messageType, &message );
    if ( status != ML_STATUS_NO_ERROR ) {
      fprintf( stderr, "Error receiving reply message: %s\n",
	       mlStatusName( status ) );
      goto SHUTDOWN;
    }

    trackAudioUstMsc( message, messageType, &ustMscInfo );

    switch ( messageType ) {

    case ML_BUFFERS_COMPLETE: {
      AFframecount framesRead;

      ++buffersComplete;
      if ( verbose > 0 ) {
	fprintf( stdout, "Buffer #%d complete (", buffersComplete );
	printUstMscFromMsg( message, stdout );
	fprintf( stdout, ")\n" );
      }

      deliveredFrames += message[0].length / sfInfo.frameSize;
      framesRead = soundFileRead( &sfInfo, message[0].value.pByte, FRAMES );
      if ( framesRead == 0 ) {
	// No more frames -- send silence to keep the device queue
	// full until we are done, otherwise we will get a spurious
	// SEQUENCE_LOST at the end (the device will detect a buffer
	// underflow before we get a chance to tell it we're done)
	if ( verbose > 0 ) {
	  fprintf( stdout, "No more samples to send, using silence buffer\n" );
	}
	memset( message[0].value.pByte, 0, bufferSize );
	message[0].length = bufferSize;
      } else {
	message[0].length = (MLint32)framesRead * (MLint32)sfInfo.frameSize;
      }

      message[0].param = ML_AUDIO_BUFFER_POINTER;
      message[1].param = ML_AUDIO_UST_INT64;
      message[2].param = ML_AUDIO_MSC_INT64;
      message[3].param = ML_END;

      /* Send the buffer back to the path so another field can be
       * captured into it.
       */

      status = mlSendBuffers( hPath, message );
      if ( status != ML_STATUS_NO_ERROR ) {
	fprintf( stderr, "Error sending buffers: %s\n",
		 mlStatusName( status ) );
	goto SHUTDOWN;
      }

      delivered_buffers++;

      /* Check if the user requested pauses (to cause SEQUENCE_LOST
       * events), and if so, check if now is the time...
       */
      if ( (pauseInterval > 0) &&
	   ((delivered_buffers % pauseInterval) == 0) ) {
	if ( verbose > 1 ) {
	  fprintf( stdout, "Pausing for 1 sec.\n" );
	}

#ifdef ML_OS_NT
	Sleep( 1000 ); /* 1000 milli-secs == 1 sec */
#else
	sleep( 1 ); /* 1 sec */
#endif
      }
    } break;

    case ML_EVENT_AUDIO_SEQUENCE_LOST:
      fprintf( stdout, "Warning - buffer dropped (" );
      printUstMscFromMsg( message, stdout );
      fprintf( stdout, ")\n" );
      fflush( stdout );
      break;

    default:
      fprintf( stderr, "Unexpected reply %s\n",	mlMessageName( messageType ) );
      goto SHUTDOWN;

    } /* switch */
  } /* while deliveredFrames < sfInfo.frameCount */

 SHUTDOWN:

  if ( hPath ) {
    mlEndTransfer( hPath );
    mlClose( hPath );
  }

  freeBuffers( buffers, maxBuffers );
  soundFileClose( &sfInfo );

  printEffectiveRate( &ustMscInfo, stdout );

  return 0;
}
