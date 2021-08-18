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
	memtoaudio.c

	memtoaudio is a sample audio playback application.
*/

#include <ML/ml.h>
#include <ML/mlu.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <malloc.h>
#include <math.h>

#ifdef	ML_OS_NT
#include <ML/getopt.h>
#include <io.h>
#else
#include <unistd.h>
#endif

#include "utils.h"
#include "audioUtils.h"

#define MAX_BUFFERS 8
#define FRAMES 1024

/* Global verbosity level -- accessible to all functions */
int verbose;

/*----------------------------------------------------------setMsgWaitPredicate
 *
 * Fill in the wait predicate (ML_WAIT_FOR_AUDIO_UST or _MSC) portion
 * of the message, as appropriate:
 *   - if neither predicate has been requested, simply return (message is
 *     un-modified)
 *   - if the UST interval has been reached, compute the desired UST and
 *     write it to the message
 *   - otherwise, if the MSC interval has been reached, compute and set that
 *   - otherwise, set a wait time of 0, effectively disabling the predicate
 *
 * The number of buffers, the number of frames per buffer and the frame
 * rate are all required in order to compute UST / MSC:
 *   - starting point is the UST / MSC currently stored in the msg
 *     ***Note***: this assumes the message is a BUFFER_COMPLETE reply!
 *     This is the timestamp of the beginning of the buffer -- we must
 *     add 1 buffer-length to get the end timestamp
 *   - we know how many buffers are enqueued for processing at any given
 *     time (includes the current buffer, since we must add the time taken by
 *     this buffer to obtain its end-point), we know how many samples each
 *     buffer contains
 *   - from this, we can compute the expected MSC of the first sample of
 *     the current buffer -- and we can increment that to force a wait
 *   - we also know the frame rate (expressed in samples per sec), so we can
 *     compute the expected UST of the current buffer -- and increment it.
 */
void setMsgWaitPredicate( MLpv* msg, int bufNo,
			  int ustInterval, int mscInterval,
			  int numBufsInQueue, int numFramesPerBuf,
			  MLreal64 sampleRate )
{
  MLint64 numSamplesInFuture = 0;
  MLpv* predicate = 0;
  MLint64 curMSC = 0, curUST = 0;

  /* Time to wait -- expressed in number of buffers -- before
   * processing this buffer (after all previous buffers have been
   * processed). ie: idle time to enforce.
   */
  static const int numBufsIdle = 10;
  static const MLint64 nanosPerSec = 1000000000; /* 10^9 nanos in 1 sec */

  if ( (ustInterval == 0) && (mscInterval == 0) ) {
    /* No waiting of any kind has been requested */
    return;
  }

  /* Find the predicate portion of the message. This could be either
   * an MSC or a UST predicate, so must look for both. Also find the
   * UST and MSC timestamps of the current (just-completed)
   * buffer. Could use mlPvFind (multiple times), but doing it
   * ourselves is more efficient.
   */
  while ( msg->param != ML_END ) {
    if ( (msg->param == ML_WAIT_FOR_AUDIO_UST_INT64) ||
	 (msg->param == ML_WAIT_FOR_AUDIO_MSC_INT64) ) {
      predicate = msg;

    } else if ( msg->param == ML_AUDIO_UST_INT64 ) {
      curUST = msg->value.int64;

    } else if ( msg->param == ML_AUDIO_MSC_INT64 ) {
      curMSC = msg->value.int64;
    }
    ++msg;
  }

  /* Did we find the predicate? If not, can't do much */
  if ( predicate == 0 ) {
    fprintf( stderr, "Warning: Wait for UST / MSC requested, but message "
	     "contains no predicate\n" );
    return;
  }

  /* Compute how many samples in the future at which we wish to
   * process this buffer -- this is the basis of both the UST and MSC.
   */
  numSamplesInFuture = (MLint64)
    ((numBufsInQueue + numBufsIdle) * numFramesPerBuf);

  /* It is important to test the UST interval first: it is twice the
   * MSC interval, so when it is reached, the MSC interval would also
   * be reached -- if we test MSC first, we will never get a chance to
   * wait for UST
   */
  if ( (ustInterval > 0) && ((bufNo % ustInterval) == 0) ) {
    /* Make sure message contains appropriate predicate */
    predicate->param = ML_WAIT_FOR_AUDIO_UST_INT64;

    /* Convert number of samples into a UST by multiplying by the
     * frame rate (and converting from secs to nanosecs), and add to
     * current UST to obtain desired timestamp.
     */
    predicate->value.int64 = curUST +
      ((numSamplesInFuture * nanosPerSec) / (MLint64) sampleRate);

    if ( verbose > 0 ) {
      fprintf( stdout, "Buffer %d: will wait for UST = %" FORMAT_LLD "\n",
	       bufNo, predicate->value.int64 );
    }

  } else if ( (mscInterval > 0) && ((bufNo % mscInterval) == 0) ) {
    /* Make sure message contains appropriate predicate */
    predicate->param = ML_WAIT_FOR_AUDIO_MSC_INT64;

    /* Using the MSC of the current message as a starting point,
     * compute the MSC at which we want to start our current buffer
     */
    predicate->value.int64 = curMSC + numSamplesInFuture;

    if ( verbose > 0 ) {
      fprintf( stdout, "Buffer %d: will wait for MSC = %" FORMAT_LLD "\n",
	       bufNo, predicate->value.int64 );
    }

  } else {
    /* Zero-out the predicate, so that the buffer does NOT wait.  It
     * doesn't really matter whether this is a wait for UST or MSC.
     */
    predicate->value.int64 = 0;
  }
}

/*------------------------------------------------------------------------usage
 */
void usage( const char* name )
{
  fprintf( stderr, "usage: %s -d <device name> [ -v ] [ -n <num buffers> ] "
	   "[ -p <pause interval> ] [ -w <wait interval> ] [ -r <rate> ]"
	   " [ -s ] [ -a ]\n\n"
	   "device name    : run mlquery to find valid names\n"
	   "pause interval : number of buffers between pauses, ie: between\n"
	   "                 SEQUENCE_LOST events\n"
	   "wait interval  : number of buffers between ML_WAIT predicates\n"
	   "                 Will alternate between WAIT_FOR_AUDIO_UST and\n"
	   "                 WAIT_FOR_AUDIO_MSC predicates, if supported\n"
	   "rate           : sample rate, in Hz (default 44100)\n"
	   "-s             : stereo (default is mono)\n"
	   "-a             : animate volume during transfer\n"
	   ,
	   name );
  exit( 1 );
}

/*-------------------------------------------------------------------------main
 */
int main( int argc, char **argv )
{
#ifndef ML_OS_NT
  extern char *optarg;
#endif
  /*
   * The following would typically come from a UI or a file, but for
   * this example, we just hard-code them in.
   */
  MLstatus status = ML_STATUS_NO_ERROR;
  MLint32 bufferSize;
  MLint32 delivered_buffers = 0;
  MLint32 returned_buffers = 0;
  MLint32 requested_buffers = 1000;
  MLint32 maxBuffers = MAX_BUFFERS;

  void *buffers[MAX_BUFFERS];
  MLint32 i;
  MLint32 memAlignment;
  MLint64 devId = 0, pathId = 0, jackId = 0;
  MLopenid hPath = 0;
  MLwaitable pathWaitHandle;
  char* deviceName = 0;
  int pauseInterval = 0;
  int waitForUSTInterval = 0, waitForMSCInterval = 0;
  MLreal64 sampleRate = 44100.0; /* hard-coded default */
  int numChannels = 1;           /* by default: mono */
  int animVolume = 0;            /* by default: do not animate */
  const MLreal64 minGain = -48.0; /* decibels */
  const MLreal64 maxGain = -12.0;
  MLreal64 gainIncrement =  -2.0;
  MLreal64 gain = maxGain;

  AudioUstMscInfo ustMscInfo;

  verbose = 0;

  initAudioUstMscInfo( &ustMscInfo );

  while ( (i = getopt( argc, argv, "d:n:p:w:avsr:" )) != -1 ) {
    switch ( i ) {
    case 'd':
      deviceName = optarg;
      break;

    case 'n':
      requested_buffers = atoi( optarg );
      break;

    case 'p':
      pauseInterval = atoi( optarg );
      break;

    case 'w':
      /* Wait intervals: MSC interval is as specified by user; UST
       * interval is computed as twice the MSC interval. Then, in the
       * loop, we test first for the UST interval, and only test for
       * MSC if the UST test failed -- the effect of this is to
       * alternate between UST and MSC.
       */
      waitForMSCInterval = atoi( optarg );
      waitForUSTInterval = waitForMSCInterval * 2;
      break;

    case 'a':
      animVolume = 1;
      break;

    case 'v':
      ++verbose;
      break;

    case 's':
      numChannels = 2;
      break;

    case 'r':
      sampleRate = atof( optarg );
      break;

    default:
      usage( argv[0] );
    } /* switch */
  } /* while */

  if ( deviceName == 0 ) {
    usage( argv[0] );
  }

  fprintf( stdout, "Requesting %d buffers\n", requested_buffers );
  fprintf( stdout, "Sampling rate: %.0f Hz, %s\n", sampleRate,
	   (numChannels == 1) ? "Mono" : "Stereo" );
  if ( pauseInterval > 0 ) {
    fprintf( stdout, "Pausing every %d buffers\n", pauseInterval );
  }
  if ( animVolume != 0 ) {
    fprintf( stdout, "Changing volume during transfer\n" );
  }
  fflush( stdout );

  status = mluFindDeviceByName( ML_SYSTEM_LOCALHOST, deviceName, &devId );
  if ( status != ML_STATUS_NO_ERROR ) {
    fprintf( stderr, "Cannot find device '%s': %s\n", deviceName,
	     mlStatusName( status ) );
    return -1;
  }

  status = mluFindFirstOutputJack( devId, &jackId );
  if ( status != ML_STATUS_NO_ERROR ) {
    fprintf( stderr, "Cannot find a suitable output jack: %s\n",
	     mlStatusName( status ) );
    return -1;
  }

  printJackName( jackId, stdout );
  fflush( stdout );

  status = mluFindPathToJack( jackId, &pathId, &memAlignment );
  if ( status != ML_STATUS_NO_ERROR ) {
    fprintf( stderr, "Cannot find a path from memory to that jack: %s\n",
	     mlStatusName( status ) );
    return -1;
  }

  bufferSize = FRAMES * sizeof( short ) * numChannels;

  /* In order to do capture, we send buffers to the path so they can
   * be filled in. Here we allocate buffers with desired alignment.
   */
  if ( allocateBuffers( buffers, bufferSize, maxBuffers, memAlignment ) ) {
    fprintf( stderr, "Cannot allocate memory for input buffers\n" );
    return -1;
  }

  /* Fill first buffer with a normalized sine wave and copy to
   * remaining buffers.
   */
  {
    short *sbuf = (short *) buffers[0];
    int n, idx;
    for ( n = 0, idx = 0; n < FRAMES; ++n ) {
      sbuf[idx] = (short) (32767.0 * sin(3.1415926585 * 8.0 *
					 (n / (double) FRAMES)));
      ++idx;
      if ( numChannels == 2 ) {
	/* If stereo, copy same value to second channel */
	sbuf[idx] = sbuf[idx-1];
	++idx;
      }
    }
    for ( n = 1; n < MAX_BUFFERS; ++n ) {
      memcpy(buffers[n], buffers[0], bufferSize);
    }
  }

  status = mlOpen( pathId, NULL, &hPath );
  if ( status != ML_STATUS_NO_ERROR ) {
    fprintf( stderr, "Cannot open path: %s\n", mlStatusName( status ) );
    goto SHUTDOWN;
  }

  /* Figure out if the device supports the WAIT_FOR_AUDIO_UST and/or
   * WAIT_FOR_AUDIO_MSC predicates (if requested by user)
   */
  if ( waitForMSCInterval > 0 ) {
    int waitUSTOK = 0, waitMSCOK = 0;

    waitUSTOK =
      checkPathSupportsParam( hPath, ML_WAIT_FOR_AUDIO_UST_INT64, 0 );
    waitMSCOK =
      checkPathSupportsParam( hPath, ML_WAIT_FOR_AUDIO_MSC_INT64, 0 );

    if ( waitUSTOK == 0 ) {
      waitForUSTInterval = 0;
    }
    if ( waitMSCOK == 0 ) {
      waitForMSCInterval = 0;
      /* In this case, we must divide the UST interval by 2, to make
       * sure we wait as often as the user requested (since we can't
       * alternate between both methods)
       */
      waitForUSTInterval /= 2;
    }

    if ( waitForMSCInterval == 0 ) {
      if ( waitForUSTInterval == 0 ) {
	fprintf( stdout, "Device does not support waiting\n" );
      } else {
	fprintf( stdout, "Device does not support waiting for MSC: "
		 "will wait for UST only\n" );
      }
    } else if ( waitForUSTInterval == 0 ) {
      fprintf( stdout, "Device does not support waiting for UST: "
	       "will wait for MSC only\n" );
    }
  } /* if waitForMSCInterval > 0 */

  /* If the user requested animated volume, make sure it is possible
   * to change the volume using sendControls during a transfer
   */
  if ( animVolume > 0 ) {
    MLint32 access =
      ML_ACCESS_W | ML_ACCESS_QUEUED | ML_ACCESS_DURING_TRANSFER;
    animVolume =
      checkPathSupportsParam( hPath, ML_AUDIO_GAINS_REAL64_ARRAY, access );

    if ( animVolume == 0 ) {
      /* Can't access the audio gains the way we'd like to */
      fprintf( stdout, "Device does not support changing the volume during "
	       "transfer\n" );
    }
  } /* if animVolume > 0 */

  /* Now set the playback attributes for 16 bit 2's complement, 1
   * channel, no compression, 44.1K sampling rate, -12dB gain.
   *
   * Also request SEQUENCE_LOST events
   */
  {
    MLpv controls[7];
    MLint32 eventsReq[1];

    eventsReq[0] = ML_EVENT_AUDIO_SEQUENCE_LOST;

    controls[0].param = ML_AUDIO_FORMAT_INT32;
    controls[0].value.int32 = ML_AUDIO_FORMAT_S16;
    controls[0].length = 1; /* to help debugging */
    controls[1].param = ML_AUDIO_CHANNELS_INT32;
    controls[1].value.int32 = numChannels;
    controls[1].length = 1;
    controls[2].param = ML_AUDIO_GAINS_REAL64_ARRAY;
    controls[2].value.pReal64 = &gain;
    controls[2].length = controls[2].maxLength = 1;
    controls[3].param = ML_AUDIO_COMPRESSION_INT32;
    controls[3].value.int32 = ML_COMPRESSION_UNCOMPRESSED;
    controls[3].length = 1;
    controls[4].param = ML_AUDIO_SAMPLE_RATE_REAL64;
    controls[4].value.real64 = sampleRate;
    controls[4].length = 1;
    controls[5].param = ML_DEVICE_EVENTS_INT32_ARRAY;
    controls[5].value.pInt32 = eventsReq;
    controls[5].length = sizeof( eventsReq ) / sizeof( MLint32 );
    controls[5].maxLength = controls[5].length;
    controls[6].param = ML_END;

    status = mlSetControls( hPath, controls );
    if ( status != ML_STATUS_NO_ERROR ) {
      fprintf( stderr, "Couldn't set controls: %s\n", mlStatusName( status ) );
      printParams( hPath, controls, stderr );
      goto SHUTDOWN;
    }
  }

  /* In verbose mode, get the controls. This shows how controls can be
   * obtained synchronously (not during a transfer, in this case)
   */
  if ( verbose > 0 ) {
    MLpv msg[5];

    /* Set the length params to 1, even if ML ignores it for scalar
     * values: that way, upon return of the message, the field will be
     * 1 (unless an error occurred), and printParams will consider it
     * "normal"
     */
    msg[0].param = ML_AUDIO_FORMAT_INT32;
    msg[0].length = 1;
    msg[1].param = ML_AUDIO_CHANNELS_INT32;
    msg[1].length = 1;
    msg[2].param = ML_AUDIO_SAMPLE_RATE_REAL64;
    msg[2].length = 1;
    msg[3].param = ML_AUDIO_FRAME_SIZE_INT32;
    msg[3].length = 1;
    msg[4].param = ML_END;

    status = mlGetControls( hPath, msg );
    if ( status != ML_STATUS_NO_ERROR ) {
      fprintf( stderr, "Error getting controls: %s\n",
	       mlStatusName( status ) );

    } else {
      fprintf( stdout, "Got controls:\n" );
      printParams( hPath, msg, stdout );
    }
  } /* if verbose > 0 */

  /* We can either poll mlReceiveMessage, or allow a wait variable to
   * be signalled. Polling is undesirable - its much better to wait
   * for an event - that way the OS can swap us out and make full use
   * of the processor until the event occurs.
   */
  status = mlGetReceiveWaitHandle(hPath, &pathWaitHandle );
  if ( status != ML_STATUS_NO_ERROR ) {
    fprintf( stderr, "Cannot get wait handle: %s\n", mlStatusName( status ) );
    goto SHUTDOWN;
  }

  /* Now preroll the queue. An assumption here is that the send queue
   * is larger than the preroll size.
   */
  for ( i = 0; i < maxBuffers; i++ ) {
    MLpv msg[5];
    int idx = 0;

    msg[idx].param = ML_AUDIO_BUFFER_POINTER;
    msg[idx].value.pByte = (MLbyte *) buffers[i];
    msg[idx].length = bufferSize;
    ++idx;

    msg[idx].param = ML_AUDIO_UST_INT64;
    ++idx;

    msg[idx].param = ML_AUDIO_MSC_INT64;
    ++idx;

    /* If the user requested 'wait predicates' (wait for UST and/or
     * MSC), and if they are supported, allocate space in the message
     * for one such predicate. No need to allocate space for both,
     * since they will never be used together.
     *
     * Set the initial wait time to 0 -- ie: no wait
     */
    if ( waitForMSCInterval > 0 ) {
      msg[idx].param = ML_WAIT_FOR_AUDIO_MSC_INT64;
      msg[idx].value.int64 = 0;
      ++idx;
    } else if ( waitForUSTInterval > 0 ) {
      msg[idx].param = ML_WAIT_FOR_AUDIO_UST_INT64;
      msg[idx].value.int64 = 0;
      ++idx;
    }
    msg[idx].param = ML_END;

    status = mlSendBuffers( hPath, msg );
    if ( status != ML_STATUS_NO_ERROR ) {
      fprintf( stderr, "Error sending buffers (pre-roll): %s\n",
	       mlStatusName( status ) );
      goto SHUTDOWN;
    }
    ++delivered_buffers;

    if ( verbose > 0 ) {
      fprintf( stdout, "Delivering buffer %d\n", delivered_buffers );
    }
  } /* for i=0..maxBuffers */

  /* Now start the transfer. */
  status = mlBeginTransfer( hPath );
  if ( status != ML_STATUS_NO_ERROR ) {
    fprintf( stderr, "Error beginning transfer: %s\n",
	     mlStatusName( status ) );
    goto SHUTDOWN;
  }

  /* In verbose mode, send a QueryControls message. This shows how
   * controls can be queried asynchronously during a transfer.
   *
   * Note: some of these controls may not be readable "in-band" (ie:
   * with QueryControls), or may not be readable during a transfer. In
   * that case, the entire message will fail. We will handle this when
   * we receive the response.
   */
  if ( verbose > 0 ) {
    MLpv msg[5];

    /* Set the length params to 1, even if ML ignores it for scalar
     * values: that way, upon return of the message, the field will be
     * 1 (unless an error occurred), and printParams will consider it
     * "normal"
     */
    msg[0].param = ML_AUDIO_FORMAT_INT32;
    msg[0].length = 1;
    msg[1].param = ML_AUDIO_CHANNELS_INT32;
    msg[1].length = 1;
    msg[2].param = ML_AUDIO_SAMPLE_RATE_REAL64;
    msg[2].length = 1;
    msg[3].param = ML_AUDIO_FRAME_SIZE_INT32;
    msg[3].length = 1;
    msg[4].param = ML_END;

    status = mlQueryControls( hPath, msg );
    if ( status != ML_STATUS_NO_ERROR ) {
      fprintf( stderr, "Error querying controls: %s\n",
	       mlStatusName( status ) );
    }
  } /* if verbose > 0 */

  while ( delivered_buffers < requested_buffers ) {
    MLstatus status = event_wait( pathWaitHandle );
    if ( status != ML_STATUS_NO_ERROR ) {
      fprintf( stderr, "Error waiting on event: %s\n",
	       mlStatusName( status ) );
      exit( 1 );
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
	++returned_buffers;
	if ( verbose > 1 ) {
	  MLint64 curUST = 0;
	  mlGetSystemUST( ML_SYSTEM_LOCALHOST, &curUST );
	  fprintf( stdout, "Buffer %d complete (", returned_buffers );
	  printUstMscFromMsg( message, stdout );
	  fprintf( stdout, "), current UST = %" FORMAT_LLD "\n", curUST );
	}

	/* Fill in the wait predicate of the message, if necessary
	 */
	++delivered_buffers;
	setMsgWaitPredicate( message, delivered_buffers, waitForUSTInterval,
			     waitForMSCInterval, maxBuffers, FRAMES,
			     sampleRate );

	/* Send the buffer back to the path so more data can be
	 * captured into it.
	 */
	if ( verbose > 0 ) {
	  fprintf( stdout, "Delivering buffer %d\n", delivered_buffers );
	}
	status = mlSendBuffers(hPath, message);
	if ( status != ML_STATUS_NO_ERROR ) {
	  fprintf( stderr, "Error sending buffers: %s\n",
		   mlStatusName( status ) );
	  goto SHUTDOWN;
	}

	/* If the user requested the volume be changed (animated),
	 * send controls to that effect
	 */
	if ( animVolume != 0 ) {
	  MLpv gainControls[2];

	  gain += gainIncrement;
	  if ( gain > maxGain ) {
	    gain = maxGain;
	    gainIncrement = -gainIncrement; /* switch directions */
	  } else if ( gain < minGain ) {
	    gain = minGain;
	    gainIncrement = -gainIncrement; /* switch directions */
	  }

	  gainControls[0].param = ML_AUDIO_GAINS_REAL64_ARRAY;
	  gainControls[0].value.pReal64 = &gain;
	  gainControls[0].length = gainControls[0].maxLength = 1;
	  gainControls[1].param = ML_END;

	  status = mlSendControls( hPath, gainControls );
	  if ( status != ML_STATUS_NO_ERROR ) {
	    fprintf( stderr, "Couldn't send controls: %s\n",
		     mlStatusName( status ) );
	    printParams( hPath, gainControls, stderr );
	    /* don't abort */
	  }
	} /* if animVolume != 0 */

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
      } break; /* case ML_BUFFERS_COMPLETE */

      case ML_QUERY_CONTROLS_COMPLETE:
      case ML_QUERY_CONTROLS_FAILED: {
	/* Print out result of query */
	fprintf( stdout, "Received result of %s query-controls:\n",
		 (messageType == ML_QUERY_CONTROLS_COMPLETE) ?
		 "successful" : "FAILED" );
	printParams( hPath, message, stdout );

	/* If the query failed, check through the message to find the
	 * parts that have *not* been marked in error -- these parts
	 * may be valid, so compose a new message with just them, and
	 * try again.
	 */
	if ( messageType == ML_QUERY_CONTROLS_FAILED ) {
	  int nextSlot = 0;
	  int i;
	  for ( i=0; message[i].param != ML_END; ++i ) {
	    if ( message[i].length != -1 ) {
	      /* This looks like it would have been valid, so keep it */
	      message[nextSlot].param = message[i].param;
	      message[nextSlot].length = 1;
	      ++nextSlot;
	    }
	  } /* for i=0... */

	  /* If we found anything valid, finalise the new message and
	   * send it off.
	   */
	  if ( nextSlot > 0 ) {
	    message[nextSlot].param = ML_END;
	    status = mlQueryControls( hPath, message );
	    if ( status != ML_STATUS_NO_ERROR ) {
	      fprintf( stderr, "Error querying controls: %s\n",
		       mlStatusName( status ) );
	    }
	  }
	} /* if messageType == ML_CONTROLS_FAILED */
      } break;

      case ML_CONTROLS_COMPLETE:
      case ML_CONTROLS_FAILED: {
	/* Print out result of send-controls, in verbose mode only */
	if ( verbose > 0 ) {
	  fprintf( stdout, "Received result of %s send-controls:\n",
		   (messageType == ML_CONTROLS_COMPLETE) ?
		   "successful" : "FAILED" );
	  printParams( hPath, message, stdout );
	}
      } break;

      case ML_EVENT_AUDIO_SEQUENCE_LOST: {
	fprintf( stdout, "Got a sequence lost event (" );
	printUstMscFromMsg( message, stdout );
	fprintf( stdout, ")\n" );
	fflush( stdout );
      } break;

      default:
	fprintf( stderr, "Unexpected reply %s\n", mlMessageName(messageType) );
	goto SHUTDOWN;

      } /* switch */
    } /* reply message scope */
  } /* while delivered_buffers < requested_buffers ... */

 SHUTDOWN:

  if ( hPath ) {
    mlEndTransfer( hPath );
    mlClose( hPath );
  }

  freeBuffers( buffers, maxBuffers );

  printEffectiveRate( &ustMscInfo, stdout );

  return 0;
}


