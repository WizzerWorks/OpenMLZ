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
	audiotofile.c

	audiotofile captures audio into an audio file.
*/

#include <ML/ml.h>
#include <ML/mlu.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <math.h>

#ifdef	ML_OS_NT
#include <ML/getopt.h>
#else
#include <unistd.h>
#include <string.h>
#endif

#include "utils.h"
#include "audioUtils.h"


#define DEFAULT_SAMPLE_RATE 44100.0


/*------------------------------------------------------------------------usage
 */
void usage( char *prog )
{
  fprintf(stderr,
	  "usage: %s -d <device name> [ -I <device index> ] "
	  "[ -j <input jack> ] [ -p <pause interval> ] [ -n <num buffers> ] "
	  "[ -v ] [ -s ] [ -r <rate in Hz> ] <sound file>\n",
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
  MLstatus status = ML_STATUS_NO_ERROR;
  MLint32 bufferSize = 0;
  MLint32 captured_buffers = 0;
  /* Program runs till this number read: */
  MLint32 requested_buffers = 800;
  /* Rotate through this many buffers: */
  MLint32 maxBuffers = 2;

  char *deviceName = NULL;
  char *jackName = NULL;
  MLint32 deviceIndex = -1;

  char *output_file_name;
  SoundFileInfo sfInfo;
  int pauseInterval = 0;

  void *buffers[2];
  MLint32 i;
  MLopenid hPath;
  MLint64 deviceID = 0, jackId = 0, pathId = 0;
  MLint32 memAlignment;
  MLwaitable pathWaitHandle;
  int verbose = 0;
  double rate = DEFAULT_SAMPLE_RATE;
  int numChannels = 1; /* By default, mono */

  AudioUstMscInfo ustMscInfo;

  initAudioUstMscInfo( &ustMscInfo );

  while ( (i = getopt( argc, argv, "d:I:j:n:p:vsr:" )) != -1 ) {
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

    case 'n':
      requested_buffers = atoi( optarg );
      break;

    case 'p':
      pauseInterval = atoi( optarg );
      break;

    case 'v':
      ++verbose;
      break;

    case 's':
      numChannels = 2;
      break;

    case 'r':
      rate = atof( optarg );
      break;

    default:
      usage( argv[0] );

    } /* switch */
  } /* while i = getopt ... */

  if ( !deviceName ) {
    usage( argv[0] );
  }

  if ( optind >= argc ) {
    usage( argv[0] );
  }

  output_file_name = argv[optind];

  fprintf( stdout, "Writing to file '%s'\n", output_file_name );
  fprintf( stdout, "Requesting %d buffers\n", requested_buffers );
  fprintf( stdout, "Sampling rate: %.0f Hz, %s\n", rate,
	   (numChannels == 1) ? "Mono" : "Stereo" );
  if ( pauseInterval > 0 ) {
    fprintf( stdout, "Pausing every %d buffers\n", pauseInterval );
  }

  if ( soundFileCreate( &sfInfo, output_file_name, numChannels, rate ) ) {
    fprintf( stderr, "Could not create output file\n" );
    exit( -1 );
  }

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
      fprintf( stderr, "Cannot find input jack '%s': %s\n", jackName,
	       mlStatusName( status ) );
      return -1;
    }

  } else {
    status = mluFindFirstInputJack( deviceID, &jackId );
    if ( status != ML_STATUS_NO_ERROR ) {
      fprintf( stderr, "Cannot find suitable input jack: %s\n",
	       mlStatusName( status ) );
      return -1;
    }
  }

  printJackName( jackId, stdout );
  fflush( stdout );

  status = mluFindPathFromJack( jackId, &pathId, &memAlignment );
  if ( status != ML_STATUS_NO_ERROR ) {
    fprintf( stderr, "Cannot find a path from that jack to memory: %s\n",
	     mlStatusName( status ) );
    return -1;
  }

  /* We will read 1024 samples per buffer, so calculate size of that
   * buffer in bytes.  This will also be the length passed in for the
   * ML_AUDIO_BUFFER_POINTER param.
   */
  bufferSize = 1024 * sizeof( short ) * numChannels;

  /* In order to do capture, we send buffers to the path so they can
   * be filled in. Here we allocate buffers with desired alignment.
   */
  if ( allocateBuffers( buffers, bufferSize, maxBuffers, memAlignment ) ) {
    fprintf( stderr, "Cannot allocate memory for input buffers\n" );
    return -1;
  }

  /* Now open and set up the path. */
  status = mlOpen( pathId, NULL, &hPath );
  if ( status != ML_STATUS_NO_ERROR ) {
    fprintf( stderr, "Cannot open path: %s\n", mlStatusName( status ) );
    goto SHUTDOWN;
  }

  /* Now set the capture attributes. We will expect 16bit 2's
   * complement, 1-channel data at 44.1k sampling rate. Must check if
   * the device support AUDIO_GAINS before setting that.
   */
  { /* set attributes scope */
    MLpv controls[5];
    MLreal64 gain[] = { -12 };	/* gain in dB */
    int audioGainsSupported = 0;

    audioGainsSupported =
      checkPathSupportsParam( hPath, ML_AUDIO_GAINS_REAL64_ARRAY, 0 );

    if ( (audioGainsSupported == 0) && (verbose > 0) ) {
      fprintf( stdout, "Device does not support AUDIO_GAINS\n" );
    }

    controls[0].param = ML_AUDIO_FORMAT_INT32;
    controls[0].value.int32 = ML_AUDIO_FORMAT_S16;
    controls[0].length = 1; /* makes debugging easier */
    controls[1].param = ML_AUDIO_CHANNELS_INT32;
    controls[1].value.int32 = numChannels;
    controls[1].length = 1;
    controls[2].param = ML_AUDIO_SAMPLE_RATE_REAL64;
    controls[2].value.real64 = rate;
    controls[2].length = 1;
    if ( audioGainsSupported != 0 ) {
      controls[3].param = ML_AUDIO_GAINS_REAL64_ARRAY;
      controls[3].value.pReal64 = gain;
      controls[3].length = sizeof( gain ) / sizeof( MLreal64 );
      controls[4].param = ML_END;

    } else {
      controls[3].param = ML_END;
    }

    status = mlSetControls( hPath, controls );
    if ( status != ML_STATUS_NO_ERROR ) {
      fprintf( stderr, "Couldn't set controls on path: %s\n",
	       mlStatusName( status ) );
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
   * for an event -- that way the OS can swap us out and make full use
   * of the processor until the event occurs.
   */
  status = mlGetReceiveWaitHandle( hPath, &pathWaitHandle );
  if ( status != ML_STATUS_NO_ERROR ) {
    fprintf( stderr, "Cannot get wait handle: %s\n", mlStatusName( status ) );
    goto SHUTDOWN;
  }

  /* And then preroll the queue -- assumption here that the send queue
   * is larger than the preroll size.
   */
  for ( i = 0; i < maxBuffers; i++ ) {
    MLpv msg[4];

    msg[0].param = ML_AUDIO_BUFFER_POINTER;
    msg[0].value.pByte = (MLbyte *) buffers[i];
    msg[0].maxLength = bufferSize;

    /* Request UST and MSC for every buffer, so we can monitor
     * progress (and, when all is done, compute the effective rate)
     */
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

  /* Now start the audio transfer. */
  status = mlBeginTransfer( hPath );
  if ( status != ML_STATUS_NO_ERROR ) {
    fprintf( stderr, "Error beginning transfer: %s\n", mlStatusName( status ));
    goto SHUTDOWN;
  }

  while ( captured_buffers < requested_buffers ) {
    status = event_wait( pathWaitHandle );
    if ( status != ML_STATUS_NO_ERROR ) {
      fprintf( stderr, "Error waiting on event: %s\n", mlStatusName( status ));
      goto SHUTDOWN;
    }

    /* Let's see what reply message is ready... */
    { /* reply message scope */
      MLint32 messageType;
      MLpv *message;

      /* Read the top message on the receive queue. */
      status = mlReceiveMessage( hPath, &messageType, &message );
      if ( status != ML_STATUS_NO_ERROR ) {
	fprintf( stderr, "Error receiving reply message: %s\n",
		 mlStatusName( status ) );
	goto SHUTDOWN;
      }

      trackAudioUstMsc( message, messageType, &ustMscInfo );

      switch ( messageType ) {

      case ML_BUFFERS_COMPLETE: {
	/* Here we could do something with the result of the transfer.
	 */
	if ( verbose > 0 ) {
	  fprintf( stdout, "Got buffer #%d (", captured_buffers+1 );
	  printUstMscFromMsg( message,stdout );
	  fprintf( stdout, ")\n" );
	}
	soundFileWrite( &sfInfo, message[0].value.pByte,
			message[0].length / sfInfo.frameSize );

	/* Send the buffer back to the path so another field can be
	 * captured into it.
	 */
	status = mlSendBuffers( hPath, message );
	if ( status != ML_STATUS_NO_ERROR ) {
	  fprintf( stderr, "Error sending buffers: %s\n",
		   mlStatusName( status ) );
	  goto SHUTDOWN;
	}

	captured_buffers++;

	/* Check if the user requested pauses (to cause SEQUENCE_LOST
	 * events), and if so, check if now is the time...
	 */
	if ( (pauseInterval > 0) &&
	     ((captured_buffers % pauseInterval) == 0) ) {
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
	fprintf( stderr, "Something went wrong - reply message is %s\n",
		 mlMessageName( messageType ) );
	goto SHUTDOWN;

      } /* switch */
    } /* reply message scope */
  } /* while captured_buffers < requested_buffers */

 SHUTDOWN:

  mlEndTransfer( hPath );
  mlClose( hPath );
  soundFileClose( &sfInfo );

  freeBuffers( buffers, maxBuffers );

  printEffectiveRate( &ustMscInfo, stdout );

  return 0;
}
