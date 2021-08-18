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

/****************************************************************************
 *
 * Sample video input application
 * 
 ****************************************************************************/

#include <ML/ml.h>
#include <ML/mlu.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>

#ifdef	ML_OS_NT
#include <ML/getopt.h>
#include <io.h>
#else
#include <unistd.h>
#endif

#include "utils.h"

/*
 * Define a few convenience routines
 */
  
MLint32 writeBuffers( MLopenid openPath, char *fileName,
		      void** buffers, MLint32 imageSize, MLint32 maxBuffers );

/*
 * Globals
 */
int debug = 0;

char *Usage = 
"usage: %s -d <device> -f <filename> [options]\n"
"options:\n"
"    -b #             number buffers to allocate (and preroll)\n"
"    -c #             count of buffers to transfer (0 = indefinitely)\n"
"    -d <device name> (run mlquery to find device names)\n"
"    -f <filename>    save pixels in 'filename'\n"
"                     (use '%%d' in filename to make seperate image files)\n"
"                     (use .ppm extension to make PPM files)\n"
"    -j <jack name>   (run mlquery to find jack names)\n"
"    -s <timing>      set video standard\n"
"    -v <line #>      enable vitc display (use -2 to not specify line number)\n"
"    -D               turn on debugging\n"
"\n"
"available timings:\n"
"     480i (or) 486i (or) 487i (or) NTSC\n"
"     576i (or) PAL\n"
"    1080i (or) SMPTE274/29I\n"
"    1080p24 (or) SMPTE274/24P\n"
;

char *timingtable[] = {
  "ML_TIMING_NONE",
  "ML_TIMING_UNKNOWN",
  "ML_TIMING_525",
  "ML_TIMING_625",
  "ML_TIMING_525_SQ_PIX",
  "ML_TIMING_625_SQ_PIX",
  "ML_TIMING_1125_1920x1080_60p",
  "ML_TIMING_1125_1920x1080_5994p",
  "ML_TIMING_1125_1920x1080_50p",
  "ML_TIMING_1125_1920x1080_60i",
  "ML_TIMING_1125_1920x1080_5994i",
  "ML_TIMING_1125_1920x1080_50i",
  "ML_TIMING_1125_1920x1080_30p",
  "ML_TIMING_1125_1920x1080_2997p",
  "ML_TIMING_1125_1920x1080_25p",
  "ML_TIMING_1125_1920x1080_24p",
  "ML_TIMING_1125_1920x1080_2398p",
  "ML_TIMING_1125_1920x1080_24PsF",
  "ML_TIMING_1125_1920x1080_2398PsF",
  "ML_TIMING_1125_1920x1080_30PsF",
  "ML_TIMING_1125_1920x1080_2997PsF",
  "ML_TIMING_1125_1920x1080_25PsF",
  "ML_TIMING_1250_1920x1080_50p",
  "ML_TIMING_1250_1920x1080_50i",
  "ML_TIMING_1125_1920x1035_60i",
  "ML_TIMING_1125_1920x1035_5994i",
  "ML_TIMING_750_1280x720_60p",
  "ML_TIMING_750_1280x720_5994p",
  "ML_TIMING_525_720x483_5994p",
};


/*----------------------------------------------------------------------CONVUST
 */
static char *CONVUST( MLint64 ns_time, char *str )
{
  MLint32 u = (MLint32)(( ns_time % (MLint64)1000000000 ) / 1000);
  MLint32 s = (MLint32)(  ns_time / (MLint64)1000000000 );
  MLint32 m =  s / 60;
  MLint32 h =  m / 60;
  /* MLint32 d =  h / 24; XXX - (don't need days yet) */

  s = s % 60;
  m = m % 60;
  /* d = d % 24; */

  sprintf( str, "%03d:%02d:%02d.%06d", h, m, s, u );
  return str;
}


/*-------------------------------------------------------------------------main
 */
int main( int argc, char **argv )
{
  MLstatus status = ML_STATUS_NO_ERROR;

  MLint32 imageSize;

  MLint32 transferred_buffers = 0;
  MLint32 sent_buffers = 0;
  MLint32 requested_buffers = 10;
  MLint32 maxBuffers = 10;

  void**  buffers;
  char*   jackName = NULL;
  char*	  fileName = "/dev/null";
  int	  c;

  MLint32 display_vitc = 0, vitc_inited = 0;
  MLUTimeCode tc;

  MLint32 i;
  MLint64 devId=0;
  MLint64 jackId=0;
  MLint64 pathId=0;
  MLint32 memAlignment;
  MLopenid openPath;
  MLwaitable pathWaitHandle;
  MLint32 timing = -1, input_timing = -1;
  MLint32 yuv = 0;
  char *timing_option = NULL;
  char *devName = 0;

  /* Get command line args */
  while ( (c = getopt(argc, argv, "b:c:d:f:j:s:v:yDh") ) != EOF ) {
    switch ( c ) {
    case 'b':
      maxBuffers = atoi( optarg );
      break;

    case 'c':
      requested_buffers = atoi( optarg );
      break;

    case 'd':
      devName = optarg;
      break;

    case 'f':
      fileName = optarg;
      break;

    case 'j':
      jackName = optarg;
      break;

    case 's':
      timing_option = optarg;
      if ( !strcmp( optarg, "NTSC" ) || !strcmp( optarg, "480i" ) ||
	   !strcmp( optarg, "486i" ) || !strcmp( optarg, "487i" ) ) {
	timing = ML_TIMING_525;

      } else if ( !strcmp( optarg, "PAL" ) || !strcmp( optarg, "576i" ) ) {
	timing = ML_TIMING_625; /* PAL */

      } else if ( !strcmp( optarg, "SMPTE274/29I" ) ||
		  !strcmp( optarg, "1080i" ) ) {
	timing = ML_TIMING_1125_1920x1080_5994i; /* HD */

      } else if ( !strcmp( optarg, "SMPTE274/24P" ) ||
		  !strcmp( optarg, "1080p24" ) ) {
	timing = ML_TIMING_1125_1920x1080_24p; /* HD */

      } else {
	fprintf( stderr, Usage, argv[0] );
	exit( 0 );
      }
      break;

    case 'v':
      display_vitc = atoi( optarg );
      memset( &tc, 0, sizeof( tc ) );
      break;

    case 'y':
      yuv = 1;
      break;

    case 'D':
      debug++;
      break;

    case 'h':
    default:
      fprintf( stderr, Usage, argv[0] );
      exit( 0 );
    } /* switch */
  } /* while c = getopt ... */

  /* Don't allocate more buffers than the requested i/o count */
  if ( requested_buffers > 0 && maxBuffers > requested_buffers ) {
    maxBuffers = requested_buffers;
  }

  buffers = malloc( sizeof( void * ) * maxBuffers );

  /* Find out about this system */
  if ( devName ) {
    status = mluFindDeviceByName( ML_SYSTEM_LOCALHOST, devName, &devId );
    if ( status != ML_STATUS_NO_ERROR ) {
      fprintf( stderr, "Cannot find device '%s': %s\n", devName,
	       mlStatusName( status ) );
      return -1;
    }
  }

  if ( jackName == NULL ) {
    status = mluFindFirstInputJack( devId, &jackId );
    if ( status != ML_STATUS_NO_ERROR ) {
      fprintf( stderr, "Cannot find a suitable output jack: %s\n",
	       mlStatusName( status ) );
      return -1;
    }

  } else {
    status = mluFindJackByName( devId, jackName, &jackId );
    if ( status != ML_STATUS_NO_ERROR ) {
      fprintf( stderr, "Cannot find jack '%s': %s\n", jackName,
	       mlStatusName( status ) );
      return -1;
    }
  }

  status = mluFindPathFromJack( jackId, &pathId, &memAlignment );
  if ( status != ML_STATUS_NO_ERROR ) {
    fprintf( stderr, "Cannot find a path to jack: %s\n",
	     mlStatusName( status ) );
    return -1;
  }

  {
    MLpv options[2];

    memset( options, 0, sizeof( options ) );
    options[0].param = ML_OPEN_SEND_QUEUE_COUNT_INT32;
    options[0].value.int32 = maxBuffers + 2;
    options[0].length = 1;
    options[1].param = ML_END;

    status = mlOpen( pathId, options, &openPath );
    if ( status != ML_STATUS_NO_ERROR ) {
      fprintf( stderr, "Cannot open path: %s\n", mlStatusName( status ) );
      goto SHUTDOWN;
    }
  }

  /* Set the path parameters.  In this case, we wish to transfer
   * high-definition or standard-definition, we want the data to be
   * stored with 8-bits-per-component, and we want it to have an RGB
   * colorspace (or YUV if 'y' option).  We also wish to be notified
   * of sequence-lost events (these correspond to cases where the
   * application was unable to send enough buffers quickly enough to
   * keep the video hardware busy).
   */
  { /* set params scope */
    MLpv controls[21], *cp;

    /* Define an array of events for use later (in this case, there is
     * only a single event, but we'll code it so we add more events by
     * just adding them to this array)
     */
    MLint32 events[] = { ML_EVENT_VIDEO_SEQUENCE_LOST };

    /* Clear pv variables - clears length (error) flags */
    memset( controls, 0, sizeof( controls ) );

    /* Video parameters, describing the signal at the jack */
#define	setV(cp,id,val)	cp->param = id; \
				cp->value.int32 = (val); \
				cp->length = 1; \
				cp++

    /* If timing not specified, see if we can discover the input
     * timing
     */
    cp = controls;
    setV( cp, ML_VIDEO_SIGNAL_PRESENT_INT32, timing );
    setV( cp, ML_END, 0 );

    status = mlGetControls( openPath, controls );
    if ( status != ML_STATUS_NO_ERROR ) {
      fprintf( stderr, "Couldn't get controls on path: %s\n",
	       mlStatusName( status ) );
      printParams( openPath, controls, stderr );
      return -1;
    }
    input_timing = controls[0].value.int32;

    fprintf( stderr, "Input timing present = %s\n",
	     timingtable[input_timing] );

    if ( timing == -1 ) {
      if ( controls[0].value.int32 != ML_TIMING_NONE &&
	   controls[0].value.int32 != ML_TIMING_UNKNOWN ) {
	timing = input_timing;
      }
    }

    /* if we can't discover the timing, then just get what it's
     * currently set to and use that.
     */
    if ( timing == -1 ) {
      cp = controls;
      setV( cp, ML_VIDEO_TIMING_INT32, timing );
      setV( cp, ML_END, 0 );
      status = mlGetControls( openPath, controls );
      if ( status != ML_STATUS_NO_ERROR ) {
	fprintf( stderr, "Couldn't get controls on path: %s\n",
		 mlStatusName( status ) );
	printParams( openPath, controls, stderr );
	return -1;
      }
      timing = controls[0].value.int32;
    }

    if ( input_timing != timing ) {
      fprintf( stderr, "Cannot set requested timing = %s\n",
	       timingtable[timing] );
      exit( 1 );
    }

    /* Now set the timing and video precision */
    cp = controls;
    setV( cp, ML_VIDEO_TIMING_INT32, timing );
    setV( cp, ML_VIDEO_PRECISION_INT32, 10 );
    setV( cp, ML_END, 0 );
    status = mlSetControls( openPath, controls );
    if ( status != ML_STATUS_NO_ERROR ) {
      fprintf( stderr, "Couldn't set controls on path: %s\n",
	       mlStatusName( status ) );
      printParams( openPath, controls, stderr );
      return -1;
    }

    if ( debug ) {
      printf( "Timing: %s\n", timingtable[timing] );
    }

    /* Now set remainder of controls */
    cp = controls;

    /* The following controls are dependent on TIMING, we'll use a
     * utility routine to fill in the actual values
     */
    (cp++)->param = ML_VIDEO_COLORSPACE_INT32;
    (cp++)->param = ML_VIDEO_WIDTH_INT32;
    (cp++)->param = ML_VIDEO_HEIGHT_F1_INT32;
    (cp++)->param = ML_VIDEO_HEIGHT_F2_INT32;
    (cp++)->param = ML_VIDEO_START_X_INT32;
    (cp++)->param = ML_VIDEO_START_Y_F1_INT32;
    (cp++)->param = ML_VIDEO_START_Y_F2_INT32;

    /* Image parameters, describing the signal in memory.  These too
     * are dependent on TIMING
     */
    (cp++)->param = ML_IMAGE_WIDTH_INT32;
    (cp++)->param = ML_IMAGE_HEIGHT_1_INT32;
    (cp++)->param = ML_IMAGE_HEIGHT_2_INT32;
    (cp++)->param = ML_IMAGE_TEMPORAL_SAMPLING_INT32;
    (cp++)->param = ML_IMAGE_DOMINANCE_INT32;

    /* Set timing dependent parameters */
    (cp  )->param = ML_END;
    mluComputePathParamsFromTiming( timing, controls,
				    MLU_TIMING_IGNORE_INVALID );

    /* The following are variable -- set for our need to display on
     * gfx
     */
    setV( cp, ML_IMAGE_COMPRESSION_INT32, ML_COMPRESSION_UNCOMPRESSED );
    setV( cp, ML_IMAGE_COLORSPACE_INT32, yuv ?
	  ML_COLORSPACE_CbYCr_601_HEAD : ML_COLORSPACE_RGB_601_FULL );
    setV( cp, ML_IMAGE_SAMPLING_INT32, yuv ?
	  ML_SAMPLING_422 : ML_SAMPLING_444 );
    setV( cp, ML_IMAGE_PACKING_INT32, ML_PACKING_8 );
    setV( cp, ML_IMAGE_INTERLEAVE_MODE_INT32, ML_INTERLEAVE_MODE_INTERLEAVED );

    /* Honor user request for VITC processing */
    if ( display_vitc && display_vitc != -2 ) {
      setV( cp, ML_VITC_LINE_NUMBER_INT32, display_vitc );
    }

    /* Device events, describing which events we want sent to us */
    (cp  )->param = ML_DEVICE_EVENTS_INT32_ARRAY;
    (cp  )->value.pInt32 = events;
    (cp  )->length=sizeof(events)/sizeof(MLint32);
    (cp++)->maxLength=sizeof(events)/sizeof(MLint32);

    /* That's everything, now signal the end of the list */
    (cp  )->param = ML_END;

    /* Adjust sizes for interleaved capture */
    { /* adjust-size scope */
      MLpv *sp  = mlPvFind( controls, ML_VIDEO_START_Y_F1_INT32 );
      MLpv *vp  = mlPvFind( controls, ML_VIDEO_HEIGHT_F1_INT32 );
      MLpv *ip  = mlPvFind( controls, ML_IMAGE_HEIGHT_1_INT32 );

      /* Adjust video/image sizes to be what the user requested */
      if ( timing_option ) {
	if ( strcmp( timing_option, "487i" ) == 0 ) {
	  sp[0].value.int32 = 20;
	  vp[0].value.int32 = 244;
	  vp[1].value.int32 = 243;
	  ip[0].value.int32 = 487;
	  ip[1].value.int32 = 0;

	} else if ( strcmp( timing_option, "486i" ) == 0 ) {
	  sp[0].value.int32 = 21;
	  vp[0].value.int32 = 243;
	  vp[1].value.int32 = 243;
	  ip[0].value.int32 = 486;
	  ip[1].value.int32 = 0;

	} else if ( strcmp( timing_option, "480i" ) == 0 ) {
	  sp[0].value.int32 = 21;
	  vp[0].value.int32 = 240;
	  vp[1].value.int32 = 240;
	  ip[0].value.int32 = 480;
	  ip[1].value.int32 = 0;

	} else {
	  ip[0].value.int32 += ip[1].value.int32;
	  ip[1].value.int32 = 0;
	}

      } else {
	ip[0].value.int32 += ip[1].value.int32;
	ip[1].value.int32 = 0;
      }
    } /* ajust-size scope */

    /* Now send the completed params to the video device */
    status = mlSetControls( openPath, controls );
    if ( status != ML_STATUS_NO_ERROR ) {
      fprintf( stderr, "Couldn't set controls on path: %s\n",
	       mlStatusName( status ) );
      printParams( openPath, controls, stderr );
      return -1;
    }

    /* Display param values to user */
    if ( debug ) {
      printParams( openPath, controls, stdout );
    }
  } /* set params scope */

  if ( mluGetImageBufferSize( openPath, &imageSize ) ) {
    fprintf( stderr, "Couldn't get buffer size\n" );
    return -1;
  }

  if ( allocateBuffers( buffers, imageSize, maxBuffers, memAlignment ) ) {
    fprintf( stderr, "Cannot allocate memory for image buffers\n" );
    return -1;
  }

  /* Preroll - send buffers to the path in preparation for beginning
   * the transfer.
   */
  for ( i = 0; i < maxBuffers; i++ ) {
    MLpv msg[10], *p = msg;
#define	setB(cp,id,val,len,mlen)\
		cp->param = id; \
		cp->value.pByte = val; \
		cp->length = len; \
		cp->maxLength = mlen; \
		cp++

    if ( debug ) {
      int d;
      int *b = (int *) buffers[i];
      for ( d = 0; d < 64; d++ ) {
	*b++ = 0xdeadbeaf;
      }
    }

    setB( p, ML_IMAGE_BUFFER_POINTER, buffers[i], 0, imageSize );
    setV( p, ML_VIDEO_MSC_INT64, 0 );
    setV( p, ML_VIDEO_UST_INT64, 0 );
    setB( p, ML_USERDATA_BYTE_POINTER, (MLbyte*) msg, 0, 0 );

    if ( display_vitc ) {
      MLint32 timecode, userdata;
      MLU_TC_TYPE tc_type;

      if ( !vitc_inited ) {
	if ( timing == ML_TIMING_525 ) {
	  tc_type = MLU_TC_2997_4FIELD_DROP;
	} else {
	  tc_type = MLU_TC_25_ND;
	}
	mluTCFromSeconds( &tc, tc_type, (double)0.0 );
      }

      mluTCPack( &timecode, &tc );
      setV( p, ML_VITC_TIMECODE_INT32, timecode );

      userdata = ('G' << 3) | ('O' << 2) | ('L' << 1) | 'F';
      setV( p, ML_VITC_USERDATA_INT32, userdata );
    }
    setV( p, ML_END, 0 );

    if ( debug ) {
      fprintf( stderr, "send buffer 0x%p/%d\n", 
	       msg[0].value.pByte, msg[0].maxLength );
    }

    status = mlSendBuffers( openPath, msg );
    if ( status != ML_STATUS_NO_ERROR ) {
      fprintf( stderr, "Error sending buffers (pre-roll): %s\n",
	       mlStatusName( status ) );
      goto SHUTDOWN;
    }

    sent_buffers++;
  } // for i=0..maxBuffers

  /* Now start the video transfer
   */
  status = mlBeginTransfer( openPath );
  if ( status != ML_STATUS_NO_ERROR ) { 
    fprintf( stderr, "Error beginning transfer: %s\n",
	     mlStatusName( status ) );
    goto SHUTDOWN;
  }

  /* We can either poll mlReceiveMessage, or allow a wait variable to
   * be signalled.  Polling is undesirable -- it's much better to wait
   * for an event -- that way the OS can swap us out and make full use
   * of the processor until the event occurs.
   */
  status = mlGetReceiveWaitHandle( openPath, &pathWaitHandle );
  if ( status != ML_STATUS_NO_ERROR ) {
    fprintf( stderr, "Cannot get wait handle: %s\n", mlStatusName( status ) );
    goto SHUTDOWN;
  }

  while ( requested_buffers == 0 || transferred_buffers < requested_buffers ) {
    status = event_wait( pathWaitHandle );
    if ( status != ML_STATUS_NO_ERROR ) {
      fprintf( stderr, "Cannot wait on event: %s\n", mlStatusName( status ) );
      exit( 1 );
    }

    /* Let's see what reply message is ready...
     */
    { /* reply message scope */
      MLint32 messageType;
      MLpv* message;

      status = mlReceiveMessage(openPath, &messageType, &message );
      if ( status != ML_STATUS_NO_ERROR ) { 
	fprintf( stderr, "\nUnable to receive reply message: %s\n",
		 mlStatusName( status ) );
	goto SHUTDOWN;
      }

      switch ( messageType ) {
      case ML_BUFFERS_COMPLETE: {
	/* Here we could do something with the result of the transfer.
	 */
	/* MLbyte* theBuffer = message[0].value.pByte; */

	union { MLint32 word; char bytes[5]; } userdata = {0};
	MLint64 theMSC = message[1].value.int64;
	MLint64 theUST = message[2].value.int64;
	MLpv *pv = mlPvFind( message, ML_VITC_TIMECODE_INT32 );
	MLpv *uv = mlPvFind( message, ML_VITC_USERDATA_INT32 );
	MLpv line[] = { { ML_VITC_INCOMING_LINE_NUMBER_INT32 },
			{ ML_END } };

	assert( message[0].param == ML_IMAGE_BUFFER_POINTER );
	transferred_buffers++;

	if ( uv ) {
	  userdata.word = uv->value.int32;
	}

	if ( pv ) {
	  mluTCUnpack( &tc, pv->value.int32 );
	  mlGetControls( openPath, line );

	  if ( debug > 1 ) {
	    printf( " VITC %s %02d:%02d:%02d%s%02d on line %d\n",
		    userdata.bytes, tc.hours, tc.minutes, tc.seconds,
		    /* display a ":" for even field, "." for odd field
		     * (just like sony monitors ;-)
		     */
		    tc.evenField ? ":" : ".", tc.frames,
		    line[0].value.int32 );
	  }
	}

	if ( debug > 1 ) {
	  char ust[100];
	  printf( "  transfer %d complete: buffer 0x%p/%d MSC:%" FORMAT_LLD
		  " UST:%s\n", transferred_buffers, 
		  message[0].value.pByte, message[0].length,
		  theMSC, CONVUST( theUST, ust ) );

	} else if ( debug && pv ) {
	  printf( "\rVITC %s %02d:%02d:%02d%s%02d line %d  ", 
		  userdata.bytes, tc.hours, tc.minutes, tc.seconds,
		  /* display a ":" for even field, "." for odd field
		   * (just like sony monitors ;-)
		   */
		  tc.evenField ? ":" : ".", tc.frames,
		  line[0].value.int32 );
	  fflush( stdout );

	} else if ( ( transferred_buffers % 10 ) == 0 ) {
	  fprintf( stderr, "." );
	  fflush( stderr );
	}

	/* Send the buffer back to the path so another frame can be
	 * transferred into it.
	 */
	if ( requested_buffers == 0 || sent_buffers < requested_buffers ) {
	  status = mlSendBuffers( openPath, message );
	  if ( status != ML_STATUS_NO_ERROR ) { 
	    fprintf( stderr, "Error sending buffers: %s\n",
		     mlStatusName( status ) );
	    goto SHUTDOWN;
	  }
	  sent_buffers++;
	}
      } break; /* case ML_BUFFERS_COMPLETE */

      case ML_BUFFERS_FAILED: {
	/* Here we could do something with the result of the failed
	 * transfer.
	 *
	 * MLbyte* theBuffer = message[0].value.pByte;
	 */

	fprintf( stderr, "\nTransfer failed, aborting.\n" );
	goto SHUTDOWN;
      }

      case ML_EVENT_VIDEO_SEQUENCE_LOST: 
	fprintf( stderr, "\nWarning, missed field\n" );
	break;

      case ML_EVENT_QUEUE_OVERFLOW:
	fprintf( stderr, "\nEvent queue overflowed, aborting\n" );
	goto SHUTDOWN;
	    
      default:
	fprintf( stderr, "\nSomething went wrong - message is %s\n", 
		 mlMessageName( messageType ) );
	goto SHUTDOWN;
      } /* switch */

      fflush(stdout);
    } /* reply message scope */
  } /* while requested_buffers .... */

 SHUTDOWN:

  fprintf( stderr, "%d buffers transferred\nShutdown\n", transferred_buffers );

  writeBuffers( openPath, fileName, buffers, imageSize, maxBuffers ); 
  mlEndTransfer( openPath );

  mlClose( openPath );
  freeBuffers( buffers, maxBuffers );

  return 0;
}


/*-----------------------------------------------------------------writeBuffers
 */
MLint32 writeBuffers( MLopenid openPath, char *fileName, void** buffers,
		      MLint32 imageSize, MLint32 maxBuffers )
{
  int i, fd = 0;
  FILE *fp = 0;
  int imagefiles = (strchr( fileName, '%' ) != NULL);
  int ppmfile = (strstr( fileName, ".ppm" ) != NULL);
  int xsize = 0, ysize = 0;

  if ( ppmfile ) {
    /* Get the x/y size for the file */
    MLpv controls[3];

    memset( controls, 0, sizeof( controls ) );
    controls[0].param = ML_IMAGE_WIDTH_INT32;
    controls[1].param = ML_IMAGE_HEIGHT_1_INT32;
    controls[2].param = ML_END;

    if ( mlGetControls( openPath, controls ) != ML_STATUS_NO_ERROR ) {
      fprintf( stderr, "couldn't get height/width\n" );
      exit( 1 );
    }

    xsize = controls[0].value.int32;
    ysize = controls[1].value.int32;
    if ( debug ) {
      fprintf( stderr, "image %d x %d\n", xsize, ysize );
    }
  } /* if ppmfile */

  if ( !imagefiles ) {
    if ( ppmfile ) {
      fp = fopen( fileName, "wb" );
      if ( fp == 0 ) {
	perror( fileName );
	return -1;
      }
      fprintf( fp, "P6\n# vidtomem PPM file\n%d %d\n255\n", xsize, ysize );

    } else {
      fd = open( fileName, O_CREAT|O_WRONLY, 0666 );
      if ( fd < 0 ) {
	perror( fileName );
	return -1;
      }
    }

    if ( debug ) {
      fprintf( stderr, "write %d bytes to %s\n", imageSize, fileName );
    }
  } /* if !imagefiles */

  for ( i = 0; i < maxBuffers; i++ ) {
    int rc;
    /*
     *{ int *t = &((int**)buffers)[i][1000];
     *  printf( "0x%p: 0x%x 0x%x 0x%x\n", t, t[0], t[1], t[2] ); }
     */

    if ( imagefiles ) {
      char fn[100];

      sprintf( fn, fileName, i );
      if ( ppmfile ) {
	fp = fopen( fn, "wb" );
	if ( fp == 0 ) {
	  perror( fn );
	  return -1;
	}
	fprintf( fp, "P6\n# vidtomem PPM file\n%d %d\n255\n", xsize, ysize );

      } else {
	fd = open( fn, O_CREAT|O_WRONLY, 0666 );
	if ( fd < 0 ) {
	  perror( fn );
	  return -1;
	}
      }

      if ( debug ) {
	fprintf( stderr, "write %d bytes to %s\n", imageSize, fn );
      }
    } /* if imagefiles */

    if ( ppmfile ) {
      rc = (int) fwrite( buffers[i], (size_t) imageSize, (size_t) 1, fp );
      if ( rc != 1 ) {
	perror( "fwrite" );
	return -1;
      }

    } else {
      rc = (int) write( fd, buffers[i], (size_t) imageSize );
      if ( rc != imageSize ) {
	perror( "write" );
	return -1;
      }
    }

    if ( imagefiles ) {
      if ( ppmfile ) {
	fclose( fp );
      } else {
	close( fd );
      }
    }
  } /* for i=0..maxBuffers */
  return 0;
}

