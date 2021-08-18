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
	audiotomem.c

	audiotomem is a simple audio input application.
*/

#include <ML/ml.h>
#include <ML/mlu.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <math.h>
#include <errno.h>
#include <string.h>

#ifdef	ML_OS_NT
#include <ML/getopt.h>
#include <io.h>
#else
#include <unistd.h>
#endif

#include "utils.h"


/*------------------------------------------------------------------------usage
 */
void usage( char *prog )
{
  fprintf( stderr,
	   "usage: %s -d <device name> [-I device_index] [-j <input jack>] "
	   "[-n <num buffers>] [ -v ]\n", prog );
  exit( 0 );
}


/*-------------------------------------------------------------------------main
 */
int main( int argc, char **argv )
{
#ifndef ML_OS_NT
  extern char *optarg;
#endif
  MLint32 bufferSize = 0;
  MLint32 captured_buffers = 0;

  /* Program runs till this number read: */
  MLint32 requested_buffers = 800;
  /* Rotate through this many buffers: */
  MLint32 maxBuffers = 2;

  char *deviceName = NULL;
  char *jackName = NULL;
  MLint32 deviceIndex = -1;

  void *buffers[2];
  MLint32 i;
  MLopenid hPath;
  MLint64 deviceID = 0, jackId = 0, pathId = 0;
  MLint32 memAlignment;
  MLwaitable pathWaitHandle;
  MLstatus status = ML_STATUS_NO_ERROR;
  int verbose = 0;

  while ( (i = getopt( argc, argv, "d:I:j:n:v" )) != -1 ) {
    switch ( i ) {
    case 'd':
      deviceName = optarg;
      break;

    case 'I':
      deviceIndex = atoi(optarg);
      break;

    case 'j':
      jackName = optarg;
      break;

    case 'n':
      requested_buffers = atoi(optarg);
      break;

    case 'v':
      ++verbose;
      break;

    default:
      usage( argv[0] );
    } /* switch */
  } /* while i = getopt ... */

  if ( !deviceName ) {
    usage( argv[0] );
  }

  if ( verbose > 0 ) {
    fprintf( stdout, "Requesting %d buffers\n", requested_buffers );
  }

  if ( deviceIndex == -1 ) {
    status = mluFindDeviceByName( ML_SYSTEM_LOCALHOST, deviceName, &deviceID );
    if ( status != ML_STATUS_NO_ERROR ) {
      fprintf( stderr, "Cannot find device '%s': %s\n", deviceName,
	       mlStatusName( status ) );
      return -1;
    }

  } else {
    if ( mluFindDeviceByNameAndIndex( ML_SYSTEM_LOCALHOST, deviceName,
				      deviceIndex, &deviceID )
	 != ML_STATUS_NO_ERROR ) {
      fprintf( stderr, "Cannot find device %s with index %d\n",
	       deviceName, deviceIndex );
      return -1;
    }
  }

  if ( jackName ) {
    if ( mluFindJackByName( deviceID, jackName, &jackId ) ) {
      fprintf( stderr, "Cannot find input jack %s\n", jackName );
      return -1;
    }

  } else {
    if ( mluFindFirstInputJack( deviceID, &jackId ) ) {
      fprintf( stderr, "Cannot find suitable input jack\n" );
      return -1;
    }
  }

  if ( mluFindPathFromJack( jackId, &pathId, &memAlignment ) ) {
    fprintf( stderr, "Cannot find a path from that jack to memory\n" );
    return -1;
  }

  /* We will read 1024 samples per buffer, so calculate size of that
   * buffer in bytes.  This will also be the length passed in for the
   * ML_AUDIO_BUFFER_POINTER param.
   */
  bufferSize = 1024 * sizeof( short );

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

  /* Now set the capture attributes.  We will expect 16 bit 2's
   * complement, 1-channel data at 44.1k sampling rate.  Don't set the
   * gain, because some devices may not support that (in particular:
   * WAVE devices on NT). Doesn't matter in this particular case
   * anyway, we're not doing anything with the captured audio.
   */
  {
    MLpv controls[4];

    controls[0].param = ML_AUDIO_FORMAT_INT32;
    controls[0].value.int32 = ML_AUDIO_FORMAT_S16;
    controls[0].length = 1; /* to help debugging */
    controls[1].param = ML_AUDIO_CHANNELS_INT32;
    controls[1].value.int32 = 1;
    controls[1].length = 1;
    controls[2].param = ML_AUDIO_SAMPLE_RATE_REAL64;
    controls[2].value.real64 = 44100.0;
    controls[2].length = 1;
    controls[3].param = ML_END;

    if ( mlSetControls( hPath, controls ) ) {
      fprintf( stderr, "Couldn't set controls on path\n" );
      goto SHUTDOWN;
    }
  }

  /* Set event mask: we're interested in sequence lost events.
   */
  {
    MLint32 events[] = { ML_EVENT_AUDIO_SEQUENCE_LOST };
    MLpv controls[2];

    controls[0].param = ML_DEVICE_EVENTS_INT32_ARRAY;
    controls[0].value.pInt32 = events;
    controls[0].length = 1;
    controls[0].maxLength = controls[0].length;

    controls[1].param = ML_END;

    if ( mlSetControls( hPath, controls ) ) {
      fprintf( stderr, "Cannot set events for path\n" );
      return -1;
    }
  }

  /* We can either poll mlReceiveMessage, or allow a wait variable to
   * be signalled.  Polling is undesirable; it's much better to wait
   * for an event.  That way the OS can swap us out and make full use
   * of the processor until the event occurs.
   */
  if ( mlGetReceiveWaitHandle( hPath, &pathWaitHandle ) ) {
    fprintf( stderr, "Cannot get wait handle\n" );
    goto SHUTDOWN;
  }

  /* Now preroll the queue. Our assumption here that the send queue is
   * larger than the preroll size.
   *
   * At this point you could also add the UST AND MSC parameters
   * (ML_AUDIO_UST_INT64 and ML_AUDIO_MSC_INT64) to monitor the timing
   * of the buffers as they are read in.
   */
  for ( i = 0; i < maxBuffers; i++ ) {
    MLpv msg[2];

    msg[0].param = ML_AUDIO_BUFFER_POINTER;
    msg[0].value.pByte = (MLbyte *) buffers[i];
    msg[0].maxLength = bufferSize;
    msg[1].param = ML_END;

    if ( mlSendBuffers( hPath, msg ) ) {
      fprintf( stderr, "Error sending buffers\n" );
      goto SHUTDOWN;
    }
  }

  /* Now start the audio transfer. */
  if ( mlBeginTransfer( hPath ) ) {
    fprintf( stderr, "Error beginning transfer\n" );
    goto SHUTDOWN;
  }

  while ( captured_buffers < requested_buffers ) {
    MLstatus status = event_wait( pathWaitHandle );
    if ( status != ML_STATUS_NO_ERROR ) {
      exit( 1 );
    }

    /* Let's see what reply message is ready... */
    { /* reply message scope */
      MLint32 status;
      MLpv *message;

      /* Read the top message on the receive queue. */
      if ( mlReceiveMessage( hPath, &status, &message ) ) {
	fprintf( stderr, "Error receiving reply message\n" );
	goto SHUTDOWN;
      }

      switch ( status ) {

      case ML_BUFFERS_COMPLETE: {
	/* Here we could do something with the result of the transfer.
	 */
	printf( "Got buffer\n" );

	/* Send the buffer back to the path so another field can be
	 * captured into it.
	 */
	if ( verbose > 0 ) {
	  fprintf( stdout, "Sending buffer %d\n", captured_buffers + 1 );
	}

	mlSendBuffers( hPath, message );
	captured_buffers++;
      } break; /* case ML_BUFFERS_COMPLETE */

      case ML_EVENT_AUDIO_SEQUENCE_LOST:
	printf( "Warning - we dropped a buffer!\n" );
	break;

      default:
	fprintf( stderr, "Something went wrong - status code is %s\n",
		 mlMessageName( status ) );
	goto SHUTDOWN;
      } /* switch */
    } /* reply message scope */
  } /* while captured_buffers ... */

 SHUTDOWN:

  mlEndTransfer( hPath );
  mlClose( hPath );
  freeBuffers( buffers, maxBuffers );

  return 0;
}

