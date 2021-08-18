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
 * Sample video output application
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
MLint32 fillBuffers( char *fileName, void** buffers, MLint32 imageSize,
		     MLint32 maxBuffers );

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
"    -v <line #>      place vitc on line # (-2 to dynamically move)\n"
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


/*--------------------------------------------------------------------next_word
 */
MLint32 next_word( FILE *f )
{
  if ( f ) {
    int rewound = 0;
    union {
      char line[80];
      int word;
    } data;

    while ( rewound < 2 ) {
      while ( fgets( data.line, sizeof( data.line ), f ) ) {
	if ( strlen( data.line ) == 5 ) {
	  data.line[4] = '\0';
	  return data.word;
	}
      }
      rewind( f );
      rewound++;
    }
  }
  return (('U' << 3) | ('S' << 2) | ('E' << 1) | 'R');
}


/*-------------------------------------------------------------------------main
 */
int main( int argc, char **argv )
{
  MLstatus status = ML_STATUS_NO_ERROR;
  MLint32 imageSize;

  MLint32 transferred_buffers = 0;
  MLint32 requested_buffers = 1000;
  MLint32 maxBuffers = 10;
  void** buffers;

  int c;
  int debug = 0;
  char* jackName = NULL;
  char* fileName = 0;

  MLUTimeCode tc;
  FILE *wordfile = 0;
  MLint32 display_vitc = -5,
    vitc_inited = 0,
    vitc_line = 0,
    vitc_line_max = 0,
    user_data = 0;
  MLint32 i;
  MLint64 devId=0;
  MLint64 jackId=0;
  MLint64 pathId=0;
  MLint32 memAlignment;
  MLopenid openPath;
  MLwaitable pathWaitHandle;

  MLint32 timing = -1, input_timing = ML_TIMING_UNKNOWN;
  MLint32 yuv = 0;
  char *timing_option = NULL;
  char *devName = 0;

  /* Get command line args */
  while ( (c = getopt( argc, argv, "b:c:d:Df:hj:s:v:y" )) != EOF ) {
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

    case 'D':
      debug++;
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
	   !strcmp( optarg, "486i" ) ) {
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
      vitc_line = display_vitc;
      user_data = (('U' << 3) | ('S' << 2) | ('E' << 1) | 'R');
      if ( vitc_line == -2 ) {
	vitc_line = 4;
      }
      memset( &tc, 0, sizeof( tc ) );
      wordfile = fopen( "/usr/lib/dict/words", "ro" );
      break;

    case 'y':
      yuv = 1;
      break;

    case 'h':
    default:
      fprintf( stderr, Usage, argv[0] );
      exit( 0 );
    } /* switch */
  } /* while c = getopt ... */

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
    status = mluFindFirstOutputJack( devId, &jackId );
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

  status = mluFindPathToJack( jackId, &pathId, &memAlignment );
  if ( status != ML_STATUS_NO_ERROR ) {
    fprintf( stderr, "Cannot find a path to jack: %s\n",
	     mlStatusName( status ) );
    return -1;
  }

  {
    /* Get path name and print it out, to show user what is going on.
     */
    MLpv* pathCap;
    MLpv* pathName;

    status = mlGetCapabilities( pathId, &pathCap );
    if ( status != ML_STATUS_NO_ERROR ) {
      fprintf( stderr, "Cannot get path capabilities: %s\n",
	       mlStatusName( status ) );
      return -1;
    }

    pathName = mlPvFind( pathCap, ML_NAME_BYTE_ARRAY );
    if ( pathName == NULL ) {
      fprintf( stderr, "Unable to find path name\n" );
      return -1;
    }

    fprintf( stderr, "Using path '%s'\n", pathName->value.pByte );
    mlFreeCapabilities( pathCap );
  }

  status = mlOpen( pathId, NULL, &openPath );
  if ( status != ML_STATUS_NO_ERROR ) {
    fprintf( stderr, "Cannot open path: %s\n", mlStatusName( status ) );
    goto SHUTDOWN;
  }

  /* Set the path parameters.  In this case, we wish to transfer
   * high-definition or standard-definition, we want the data to be
   * stored with 8-bits-per-component, and we want it to have an RGB
   * colorspace in memory.  We also wish to be notified of
   * sequence-lost events (these correspond to cases where the
   * application was unable to send enough buffers quickly enough to
   * keep the video hardware busy).
   */
  { /* set params scope */
    MLpv controls[30], *cp;

    /* Define an array of events for use later (in this case, there is
     * only a single event, but we'll code it so we add more events by
     * just adding them to this array)
     */
    MLint32 events[] = { ML_EVENT_VIDEO_SEQUENCE_LOST };

    /* Clear pv variables - clears length (error) flags */
    memset( controls, 0, sizeof( controls ) );

    /* Video parameters, describing the signal at the jack */
#define	setV(cp,id,val,typ)\
			cp->param = id; \
			cp->value.typ = val; \
			cp->length = 1; \
			cp++

    /* If timing not specified, see if we can discover the input
     * timing -- ONLY on boards that support this (ie: some boards are
     * output only, so we can't query the VIDEO_SIGNAL_PRESENT)
     */
    if ( checkPathSupportsParam( openPath, ML_VIDEO_SIGNAL_PRESENT_INT32, 0 )
	 != 0 ) {
      cp = controls;
      setV( cp, ML_VIDEO_SIGNAL_PRESENT_INT32, timing, int32 );
      setV( cp, ML_END, 0, int32 );
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
    } /* if checkPathSupportsParam ... */

    /* If we can't discover the timing, then just get what it's
     * currently set to and use that.
     */
    cp = controls;
    setV( cp, ML_VIDEO_TIMING_INT32, timing, int32 );
    setV( cp, ML_END, 0, int32 );
    status = mlGetControls( openPath, controls );
    if ( status != ML_STATUS_NO_ERROR ) {
      fprintf( stderr, "Couldn't get controls on path: %s\n",
	       mlStatusName( status ) );
      printParams( openPath, controls, stderr );
      return -1;
    }

    fprintf( stderr, "Current video timing = %s\n",
	     timingtable[controls[0].value.int32] );
    if ( timing == -1 ) {
      timing = controls[0].value.int32;
    }

    if ( input_timing != ML_TIMING_UNKNOWN && input_timing != timing ) {
      fprintf( stderr, "Cannot set requested timing = %s\n",
	       timingtable[timing] );
      exit( 1 );
    }

    /* Now set the timing and video precision */
    cp = controls;
    setV( cp, ML_VIDEO_TIMING_INT32, timing, int32 );
    setV( cp, ML_VIDEO_PRECISION_INT32, 10, int32 );
    setV( cp, ML_END, 0, int32 );
    status = mlSetControls( openPath, controls );
    if ( status != ML_STATUS_NO_ERROR) {
      fprintf( stderr, "Couldn't set controls on path: %s\n",
	       mlStatusName( status ) );
      printParams( openPath, controls, stderr );
      return -1;
    }

    if ( debug ) {
      printf( " Timing %d\n", timing );
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
    setV( cp, ML_IMAGE_COMPRESSION_INT32, ML_COMPRESSION_UNCOMPRESSED, int32 );
    setV( cp, ML_IMAGE_COLORSPACE_INT32, yuv ?
	  ML_COLORSPACE_CbYCr_601_HEAD : ML_COLORSPACE_RGB_601_FULL, int32 );
    setV( cp, ML_IMAGE_SAMPLING_INT32, yuv ?
	  ML_SAMPLING_422 : ML_SAMPLING_444, int32 );
    setV( cp, ML_IMAGE_PACKING_INT32, ML_PACKING_8, int32 );
    setV( cp, ML_IMAGE_INTERLEAVE_MODE_INT32,
	  ML_INTERLEAVE_MODE_INTERLEAVED, int32 );

    /* Did the user request VITC processing? */
    if ( display_vitc != -5 ) {
      MLint32 v = display_vitc;
      if ( v == -2 ) {
	v = 4;
      }
      setV( cp, ML_VITC_LINE_NUMBER_INT32, v, int32 );

      if ( timing == ML_TIMING_525 ) {
	vitc_line_max = 19;
      } else if ( timing == ML_TIMING_625 ) {
	vitc_line_max = 21;
      } else {
	vitc_line_max = 25;	/* no idea what this should be */
      }
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
    } /* adjust-size scope */

    /* Now send the completed params to the video device */
    status = mlSetControls( openPath, controls );
    if ( status != ML_STATUS_NO_ERROR ) {
      fprintf( stderr, "Couldn't set controls on path (%s)\n",
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

  if ( fillBuffers( fileName, buffers, imageSize, maxBuffers ) ) {
    fprintf( stderr, "Cannot get image data into buffers\n" );
    return -1;
  }

  /* Preroll - send buffers to the path in preparation for beginning
   * the transfer.
   */
  printf( "Prerolling %d buffers: ", maxBuffers );
  fflush( stdout );

  for ( i = 0; i < maxBuffers; i++ ) {
    MLpv msg[8], *pv = msg;

#define	setB(cp,id,val,len,mlen)\
				cp->param = id; \
				cp->value.pByte = (MLbyte*)val; \
				cp->length = len; \
				cp->maxLength = mlen; \
				cp++

    setB( pv, ML_IMAGE_BUFFER_POINTER, buffers[i], imageSize, imageSize );
    setV( pv, ML_VIDEO_MSC_INT64, 0, int64 );
    setV( pv, ML_VIDEO_UST_INT64, 0, int64 );

    if ( display_vitc != -5 ) {
      MLint32 timecode;

      MLU_TC_TYPE tc_type;
      if ( !vitc_inited ) {
	if ( timing == ML_TIMING_525 ) {
	  tc_type = MLU_TC_2997_4FIELD_DROP;
	} else {
	  tc_type = MLU_TC_25_ND;
	}
	mluTCFromSeconds( &tc, tc_type, (double) 0.0 );
      }

      mluTCPack( &timecode, &tc );
      if ( debug > 1 ) {
	printf( " VITC %02d:%02d:%02d%s%02d -> 0x%08x\n", 
		tc.hours, tc.minutes, tc.seconds,
		/* display a ":" for even field, "." for odd field 
		 * (just like sony monitors ;-)
		 */
		tc.evenField? ":" : ".",
		tc.frames, timecode );
      }
      setV( pv, ML_VITC_LINE_NUMBER_INT32, vitc_line, int32 );
      setV( pv, ML_VITC_TIMECODE_INT32, timecode, int32 );
      setV( pv, ML_VITC_USERDATA_INT32, user_data, int32 );

      mluTCAddFrames( &tc, &tc, 1, NULL );
    }
    setV( pv, ML_END, 0, int64 );

    status = mlSendBuffers( openPath, msg );
    if ( status != ML_STATUS_NO_ERROR ) { 
      fprintf( stderr, "Error sending buffers (pre-roll): %s.\n",
	       mlStatusName( status ) );
      goto SHUTDOWN;
    }
    fprintf( stdout, "." ); fflush( stdout );
  } /* for i=0..maxBuffers */

  fprintf( stdout, "done.\n" );

  /* Now start the video transfer
   */
  fprintf( stdout, "Begin transfer %d buffers\n", requested_buffers );
  fflush( stdout );

  status = mlBeginTransfer( openPath );
  if ( status != ML_STATUS_NO_ERROR ) { 
    fprintf( stderr, "Error beginning transfer: %s\n", mlStatusName( status ));
    goto SHUTDOWN;
  }

  /* We can either poll mlReceiveMessage, or allow a wait variable to
   * be signalled.  Polling is undesirable -- it;s much better to wait
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
      fprintf( stderr, "Cannot wait on handle: %s\n", mlStatusName( status ) );
      exit( 1 );
    }

    /* Let's see what reply message is ready...
     */
    { /* reply message scope */
      MLint32 messageType;
      MLpv* message;

      status = mlReceiveMessage( openPath, &messageType, &message );
      if ( status != ML_STATUS_NO_ERROR ) { 
	fprintf( stderr, "Unable to receive reply message: %s\n",
		 mlStatusName( status ) );
	goto SHUTDOWN;
      }

      switch ( messageType ) {

      case ML_BUFFERS_COMPLETE: {
	/* Here we could do something with the result of the transfer.
	 *
	 * MLbyte* theBuffer = message[0].value.pByte;
	 * MLint64 theMSC    = message[1].value.int64;
	 * MLint64 theUST    = message[2].value.int64;
	 */
	transferred_buffers++;
	fprintf( stdout, "\r  transfer %d complete!  ", transferred_buffers );
	fflush( stdout );

	/* Send the buffer back to the path so another field can be
	 * transferred from it.
	 */
	if ( display_vitc != -5 ) {
	  MLpv *p = mlPvFind( message, ML_VITC_TIMECODE_INT32 );
	  MLpv *d = mlPvFind( message, ML_VITC_USERDATA_INT32 );
	  MLpv *l = mlPvFind( message, ML_VITC_LINE_NUMBER_INT32 );
	  mluTCAddFrames( &tc, &tc, 1, NULL );
	  mluTCPack( &p->value.int32, &tc );

	  if ( d ) {
	    if ( display_vitc == -2 && (transferred_buffers % 5) == 0 ) {
	      /* Rotate word */
	      user_data = next_word( wordfile );
	    }
	    d->value.int32 = user_data;
	  }

	  if ( l && display_vitc == -2 && (transferred_buffers % 10) == 0 ) {
	    if ( ++l->value.int32 > vitc_line_max ) {
	      l->value.int32 = 4;
	    }
	    fprintf( stdout, " vitc line %d ", l->value.int32 );
	    fflush( stdout );
	  }
	}

	status = mlSendBuffers( openPath, message );
	if ( status != ML_STATUS_NO_ERROR ) {
	  fprintf( stderr, "Error sending buffers: %s\n",
		   mlStatusName( status ) );
	  goto SHUTDOWN;
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

  printf( "\nShutdown\n" );

  mlEndTransfer( openPath );
  mlClose( openPath );
  freeBuffers( buffers, maxBuffers );

  return 0;
}


/*------------------------------------------------------------------fillBuffers
 *
 * Fill each buffer with a recognizable pattern
 */
MLint32 fillBuffers( char *fileName, void** buffers, MLint32 imageSize,
		     MLint32 maxBuffers )
{
  int i;
  printf( "Filling buffers: " );
  if ( fileName ) {
    printf( "from %s: ", fileName );
  }
  fflush( stdout );

  if ( !fileName ) {
    for ( i=0; i < maxBuffers; i++ ) {
      MLbyte* p;
      int x,y;

      for ( p=(MLbyte*) buffers[i]; p < (MLbyte*) buffers[i]+imageSize/5; ) {
	*p++ = 0x0;
	*p++ = 0x0;
	*p++ = 0x0;
      }

      for ( ; p < (MLbyte*) buffers[i]+imageSize*2/5; ) {
	*p++ = (MLbyte)0xe0;
	*p++ = (MLbyte)0x00;
	*p++ = (MLbyte)0x00;
      }
      
      for ( ; p < (MLbyte*) buffers[i]+imageSize*3/5; ) {
	*p++ = (MLbyte)0x00;
	*p++ = (MLbyte)0xe0;
	*p++ = (MLbyte)0x0;
      }
      
      for ( ; p < (MLbyte*) buffers[i]+imageSize*4/5; ) {
	*p++ = (MLbyte)0x00;
	*p++ = (MLbyte)0x00;
	*p++ = (MLbyte)0xe0;
      }
      
      for ( ; p < (MLbyte*) buffers[i]+imageSize; ) {
	*p++ = (MLbyte)0xe0;
	*p++ = (MLbyte)0xe0;
	*p++ = (MLbyte)0xe0;
      }

      for ( y=0; y < 100; y++ ) {
	for ( x=0; x < i*10+100; x++ ) {
	  *((MLbyte*) buffers[i]+((y*720)+x)*3) = (MLbyte) 0x80;
	}
      }

      printf( "." );
      fflush( stdout );
    } /* for i=0..maxBuffers */

  } else {
    int i, fd = 0;
    FILE *fp = 0;
    int imagefiles = (strchr( fileName, '%' ) != NULL);
    int ppmfile = (strstr( fileName, ".ppm" ) != NULL);

    if ( !imagefiles ) {
      if ( ppmfile ) {
	fp = fopen( fileName, "rb" );
	if ( fp == 0 ) {
	  perror( fileName );
	  return -1;
	}

      } else {
	fd = open( fileName, O_RDONLY, 0666 );
	if ( fd < 0 ) {
	  perror( fileName );
	  return -1;
	}
      }
    } /* if !imagefiles */

    for ( i = 0; i < maxBuffers; i++ ) {
      int rc;
      if ( imagefiles ) {
	char fn[100];
	sprintf( fn, fileName, i );

	if ( ppmfile ) {
	  fp = fopen( fn, "rb" );
	  if ( fp == 0 ) {
	    perror( fn );
	    return -1;
	  }

	} else {
	  fd = open( fn, O_CREAT|O_RDONLY, 0666 );
	  if ( fd < 0 ) {
	    perror( fn );
	    return -1;
	  }
	}
      } /* if imagefiles */

      if ( ppmfile ) {
	int readPPMfile( FILE *fp, char *buffer, int buffersize );
	rc = readPPMfile( fp, buffers[i], imageSize );
	if ( rc != 1 ) {
	  perror( "fread" );
	  return -1;
	}

      } else {
	rc = (int) read( fd, buffers[i], (size_t) imageSize );
	if ( rc == 0 ) {
	  lseek( fd, 0, SEEK_SET );
	  rc = (int) read( fd, buffers[i], imageSize );
	}
	if ( rc != imageSize ) {
	  perror( "read" );
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

      printf( "." );
      fflush( stdout );
    }
  }
  printf( "done.\n" );
  return 0;
}	  


/*---------------------------------------------------------------------_readInt
 */
static int _readInt( FILE* fptr, int* result )
{
  int c;

  while ( (c = fgetc( fptr )) > 0 ) {
    switch( c ) {
    case '#': {
      /* Ignore rest of comment line */
      char line[71];
      fgets(line, 71, fptr);
      continue;
    }

    case ' ':
    case '\t':
    case '\n':
    case '\r':
      continue;

    default:
      ungetc( c, fptr );
      return fscanf( fptr, "%d", result ) == 1;
    } /* switch */
  } /* while c = fgetc ... */

  return -1;
}


/*------------------------------------------------------------------readPPMfile
 */
int readPPMfile( FILE *fp, char *buffer, int buffersize )
{
  char filetype[3];
  int maxValue, imageWidth, imageHeight, isize;
  int rc;

  if ( fgets( filetype, 3, fp ) == 0 ) {
    fprintf( stderr, "can't get filetype\n" );
    exit( 2 );
  }

  if ( (filetype[0] != 'P' && filetype[0] != 'S') || filetype[1] != '6' ) {
    fprintf( stderr, "not a ppm file: %s\n", filetype );
    exit( 2 );
  }

  _readInt( fp, &imageWidth );
  _readInt( fp, &imageHeight );
  _readInt( fp, &maxValue );
  fgetc( fp ); /* skip over single whitespace char after maxValue */

  isize = imageWidth * imageHeight * 3;

  if ( isize > buffersize ) {
    fprintf( stderr, "Error: image size %d buffer size %d\n",
	     isize, buffersize );
    fprintf( stderr, "imageWidth %d imageHeight %d maxValue %d\n",
	     imageWidth, imageHeight, maxValue );
    exit( 2 );
  }

  if ( (rc = (int) fread( buffer, (size_t) isize, (size_t) 1, fp )) != 1 ) {
    fprintf( stderr, "fread returns %d\n", rc );
    exit( 2 );
  }

  if ( isize < buffersize ) {
    memset( &buffer[ isize ], 0, buffersize - isize );
  }

  return rc;
}

