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
 * Sample video to video application
 * 
 ****************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <errno.h>
#ifndef	ML_OS_IRIX
#define	oserror()	errno
#endif
#define	OSERROR	strerror(oserror())

#ifdef	ML_OS_NT
#include <ML/getopt.h>
#include <io.h>
#define	memalign(alignment,size)	malloc(size)	// FIXME
#else
#include <unistd.h>
#endif

#include <ML/ml.h>
#include <ML/mlu.h>

/*
 * Define a few convenience routines
 */
  
MLint32 allocateBuffers( void** buffers, MLint32 imageSize,
			 MLint32 maxBuffers, MLint32 memAlignment );
int print_controls( MLint64 openPath, FILE *f );

/*
 * Globals
 */
int debug = 0;

char *Usage = 
"usage: %s -d<indevice> -j<injack> -d<outdevice> -j<outjack> [options]\n"
"options:\n"
"    -b #                    number buffers to allocate (and preroll)\n"
"    -c #                    count of buffers to transfer (0 = indefinitely)\n"
"    -d <input device name>  1st -d/-j are input device/jack names\n"
"    -j <input jack name>\n"
"    -d <output device name> 2nd -d/-j are output device/jack names\n"
"    -j <output jack name>\n"
"    -s <timing>             set video standard\n"
"    -v <line #>             vitc display (use -2 to not specify line #)\n"
"    -y                      input and output YUV (CrYCb) pixels\n"
"    -V <param=value>        set \"param\" to \"value\"\n"
"    -D                      turn on debugging\n"
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


/*----------------------------------------------------------------------dparams
 */
static void dparams( MLint64 openPath, MLpv *controls, char *msg )
{
  MLpv *p;
  char buff[256];

  fprintf( stderr, "=== %s 0x%" FORMAT_LLX "\n", msg, openPath );
  for ( p = controls; p->param != ML_END; p++ ) {
    MLint32 size = sizeof( buff );
    MLstatus stat = mlPvToString( openPath, p, buff, &size );

    if ( stat != ML_STATUS_NO_ERROR ) {
      fprintf( stderr, "mlPvToString: %s\n", mlStatusName( stat ) );
    } else {
      fprintf( stderr, "\t%s", buff );
      if( p->length != 1 ) {
	fprintf( stderr, " (length %d)", p->length );
      }
      fprintf( stderr, "\n" );
    }
  }
}


/*-------------------------------------------------------------------event_wait
 */
MLstatus event_wait( MLwaitable inputPathWaitHandle,
		     MLwaitable outputPathWaitHandle,
		     MLint32 *input_message )
{
#ifdef	ML_OS_NT
  /* Needs to be WaitForMultipleObject but I don't have api
   * reference...  (I think wait handles need to be an array?)
   */
  if ( WaitForMultipleObject( inputPathWaitHandle | outputPathWaitHandle,
			      INFINITE ) != WAIT_OBJECT_0 ) {
    fprintf( stderr, "Error waiting for reply\n" );
    return ML_STATUS_INTERNAL_ERROR;
  }
  return ML_STATUS_NO_ERROR;

#else	/* ML_OS_UNIX */
  for ( ;; ) {
    fd_set fdset;
    int rc;
    int maxfd = inputPathWaitHandle > outputPathWaitHandle ?
      inputPathWaitHandle : outputPathWaitHandle;

    FD_ZERO( &fdset);
    FD_SET( inputPathWaitHandle, &fdset );
    FD_SET( outputPathWaitHandle, &fdset );

    rc = select( maxfd+1, &fdset, NULL, NULL, NULL );
    if ( debug ) {
      fprintf( stderr, "select %d returns %d\n", maxfd+1, rc );
    }

    if ( rc < 0 ) {
      fprintf( stderr, "select: %s\n", OSERROR );
      return ML_STATUS_INTERNAL_ERROR;
    }

    if ( rc > 0 ) {
      if ( input_message ) {
	*input_message = FD_ISSET( inputPathWaitHandle, &fdset );
      }
      return ML_STATUS_NO_ERROR;
    }
    /* Anything else, loop again */
  }
#endif
}


/* ---------------------------------------------------------------------CONVUST
 */
static char *CONVUST( MLint64 ns_time, char *str )
{
  MLint32 u = (MLint32) (( ns_time % (MLint64) 1000000000 ) / 1000);
  MLint32 s = (MLint32) (  ns_time / (MLint64) 1000000000 );
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
  MLint32 display_vitc = 0, vitc_inited = 0;
  char* timing_option = NULL;
  MLUTimeCode tc;

  MLint32 timing = -1, input_timing = -1;
  MLint32 transferred_buffers = 0;
  MLint32 requested_buffers = 10;
  MLint32 maxBuffers = 10;
  MLint32 yuv = 0;

  char* inputDevName = NULL;
  char* inputJackName = NULL;
  MLint64 inputDevId=0;
  MLint64 inputJackId=0;
  MLint64 inputPathId=0;
  MLopenid inputOpenPath;
  MLwaitable inputPathWaitHandle;
  MLint32 inputMemAlignment;

  char* outputDevName = NULL;
  char* outputJackName = NULL;
  MLint64 outputDevId=0;
  MLint64 outputJackId=0;
  MLint64 outputPathId=0;
  MLopenid outputOpenPath;
  MLwaitable outputPathWaitHandle;
  MLint32 outputMemAlignment;

  void** buffers;
  int c;
  MLint32 i;
  MLint32 imageSize;
  MLint32 output_going = 0;

  char* cmdArgs[200];
  int cmdArgsIndex = 0;

  /* Get command line args */
  while ( (c = getopt( argc, argv, "b:c:d:hj:s:v:yDV:" )) != EOF ) {
    switch ( c ) {
    case 'b':
      maxBuffers = atoi( optarg );
      break;

    case 'c':
      requested_buffers = atoi( optarg );
      break;

    case 'd':
      if ( inputDevName ) {
	outputDevName = optarg;
      } else {
	inputDevName = optarg;
      }
      break;

    case 'j':
      if ( outputDevName ) {	/* if following output name */
	outputJackName = optarg;
      } else {
	inputJackName = optarg;
      }
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
      yuv=1;
      break;

    case 'V':
      cmdArgs[ cmdArgsIndex++ ] = optarg;
      break;

    case 'D':
      debug++;
      break;

    case 'h':
    default:
      fprintf( stderr, Usage, argv[0] );
      exit( 0 );
    } /* switch */
  } /* while c = getopt */

  /* Insure number of buffers allocated/prerolled aren't more than the
   * number of requested buffers
   */
  if ( maxBuffers > requested_buffers ) {
    maxBuffers = requested_buffers;
  }

  buffers = malloc( sizeof( void* ) * maxBuffers );

  /* Get input path
   */
  if ( mluFindDeviceByName( ML_SYSTEM_LOCALHOST, inputDevName, &inputDevId )) {
    fprintf( stderr, "Cannot find input device named %s.\n", inputDevName );
    return -1;
  }

  if ( inputJackName == NULL ) {
    if ( mluFindFirstInputJack( inputDevId, &inputJackId ) ) {
      fprintf( stderr, "Cannot find a suitable input jack.\n" );
      return -1;
    }

  } else {
    if ( mluFindJackByName( inputDevId, inputJackName, &inputJackId ) ) {
      fprintf( stderr, "Cannot find jack: %s.\n", inputJackName );
      return -1;
    }
  }

  if ( mluFindPathFromJack( inputJackId, &inputPathId, &inputMemAlignment ) ) {
    fprintf( stderr, "Cannot find input path from jack\n" );
    return -1;
  }

  /* Get output path
   */
  if ( !outputDevName ) {
    outputDevName = inputDevName;
  }

  if ( mluFindDeviceByName( ML_SYSTEM_LOCALHOST, outputDevName,
			    &outputDevId ) ) {
    fprintf( stderr, "Cannot find output device named %s.\n", outputDevName );
    return -1;
  }

  if ( outputJackName == NULL ) {
    if ( mluFindFirstOutputJack( outputDevId, &outputJackId ) ) {
      fprintf( stderr, "Cannot find a suitable output jack.\n" );
      return -1;
    }

  } else {
    if ( mluFindJackByName( outputDevId, outputJackName, &outputJackId ) ) {
      fprintf( stderr, "Cannot find jack: %s.\n", outputJackName );
      return -1;
    }
  }

  if ( mluFindPathToJack( outputJackId, &outputPathId, &outputMemAlignment )) {
    fprintf( stderr, "Cannot find output path from jack\n" );
    return -1;
  }

  {
    MLpv options[2];
    memset( options, 0, sizeof( options ) );
    options[0].param = ML_OPEN_SEND_QUEUE_COUNT_INT32;
    options[0].value.int32 = maxBuffers + 2;
    options[0].length = 1;
    options[1].param = ML_END;

    if ( mlOpen( inputPathId, options, &inputOpenPath ) ) {
      fprintf( stderr, "Cannot open input path\n" );
      goto SHUTDOWN;
    }

    if ( mlOpen( outputPathId, options, &outputOpenPath ) ) {
      fprintf( stderr, "Cannot open output path\n" );
      goto SHUTDOWN;
    }
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
    MLpv controls[ 100 ], *cp;

    /* Define an array of events for use later (in this case, there is
     * only a single event, but we'll code it so we add more events by
     * just adding them to this array)
     */
    MLint32 events[] = { ML_EVENT_VIDEO_SEQUENCE_LOST };

    /* Clear pv variables - clears length (error) flags */
    memset( controls, 0, sizeof( controls ) );

    /* Video parameters, describing the signal at the jack */
#define	setV(cp,id,val)	cp->param = id; \
			    cp->value.int32 = val; \
			    cp->length = 1; \
			    cp++

    /* If timing not specified, see if we can discover the input
     * timing
     */
    cp = controls;
    setV( cp, ML_VIDEO_SIGNAL_PRESENT_INT32, timing );
    setV( cp, ML_END, 0 );

    if ( mlGetControls( inputOpenPath, controls ) != ML_STATUS_NO_ERROR ) {
      fprintf( stderr, "Couldn't get controls on input path\n" );
      dparams( inputOpenPath, controls, "Input mlGetControls video signal" );
      return -1;
    }

    input_timing = controls[0].value.int32;
    fprintf( stderr, "Input timing present = %s\n", timingtable[input_timing]);

    if ( timing == -1 ) {
      if ( controls[0].value.int32 != ML_TIMING_NONE &&
	   controls[0].value.int32 != ML_TIMING_UNKNOWN ) {
	timing = input_timing;
      }
    }

    /* If we can't discover the timing, then just get what it's
     * currently set to and use that.
     */
    if ( timing == -1 ) {
      cp = controls;
      setV( cp, ML_VIDEO_TIMING_INT32, timing );
      setV( cp, ML_END, 0 );
      if ( mlGetControls( inputOpenPath, controls ) != ML_STATUS_NO_ERROR ) {
	fprintf( stderr, "Couldn't get controls on input path\n" );
	dparams( inputOpenPath, controls, "Input mlGetControls timing" );
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
    if ( mlSetControls( inputOpenPath, controls ) != ML_STATUS_NO_ERROR ) {
      fprintf( stderr, "Couldn't set controls on input path\n" );
      dparams( inputOpenPath, controls, "Input mlSetControls timing" );
      return -1;
    }

    if ( debug ) {
      printf( "Timing %d\n", timing );
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
    } /* adjust-timing scope */

    /* Now we look at user cmd line options to see if they want to
     * change any default control setting.
     */
    for ( i = 0; i < cmdArgsIndex; i++ ) {
      char *p = cmdArgs[ i ];
      MLint32 size = (MLint32) strlen( p );
      MLint64 data[ 512 ];
      MLpv pv, *pvp;
      MLint32 found = 0;

      if ( debug > 1 ) {
	fprintf( stderr, "process cmdarg: %s\n", p );
      }

      mlPvFromString( inputOpenPath, p, &size, &pv, (MLbyte*) data,
		      sizeof( data ) );

      /* Look in our control list to see if we already have this
       * param...
       */
      for ( pvp = controls; pvp->param != ML_END; pvp++ ) {
	if ( pvp->param == pv.param ) {
	  /* If so, then replace the value */
	  *pvp = pv;
	  found = 1;
	  break;
	}
      }

      if ( !found ) {
	/* Otherwise, we need to append the param to our list */
	assert( pvp->param == ML_END );
	*pvp++ = pv;
	assert( ((MLbyte*) pvp - (MLbyte*) controls) < sizeof( controls ) );
	pvp->param = ML_END;
      }
    } /* for i=0..cmdArgsIndex */

    /* Now send the completed params to the video device */
    if ( mlSetControls( inputOpenPath, controls ) != ML_STATUS_NO_ERROR ) {
      fprintf( stderr, "Couldn't set controls on input path\n" );
      dparams( inputOpenPath, controls, "Input mlSetControls" );
      return -1;
    }

    if ( mlSetControls( outputOpenPath, controls ) != ML_STATUS_NO_ERROR ) {
      fprintf( stderr, "Couldn't set controls on output path\n" );
      dparams( outputOpenPath, controls, "Output mlSetControls" );
      return -1;
    }

    /* Display param values to user */
    if ( debug ) {
      dparams( inputOpenPath, controls, "Input" );
      dparams( outputOpenPath, controls, "Output" );
    }
  } /* set params scope */

  if ( mluGetImageBufferSize( inputOpenPath, &imageSize ) ) {
    fprintf( stderr, "Couldn't get buffer size\n" );
    return -1;
  }

  {
    MLint32 memAlignment = inputMemAlignment > outputMemAlignment ?
      inputMemAlignment : outputMemAlignment;
    if ( allocateBuffers( buffers, imageSize, maxBuffers, memAlignment ) ) {
      fprintf( stderr, "Cannot allocate memory for image buffers\n" );
      return -1;
    }
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

    setB( p, ML_IMAGE_BUFFER_POINTER, buffers[i], 0, imageSize );
    setV( p, ML_VIDEO_MSC_INT64, 0 );
    setV( p, ML_VIDEO_UST_INT64, 0 );
    setB( p, ML_USERDATA_BYTE_POINTER, (MLbyte*)msg, 0, 0 );

    if ( display_vitc ) {
      MLint32 timecode;
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
      setV( p, ML_VITC_USERDATA_INT32, 'GOLF' );
    }
    setV( p, ML_END, 0 );

    if ( mlSendBuffers( inputOpenPath, msg ) ) { 
      fprintf( stderr, "Error sending buffers.\n" );
      goto SHUTDOWN;
    }
  } /* for i=0..maxBuffers */

  /* Now start the video transfer */
  if ( mlBeginTransfer( inputOpenPath ) ) { 
    fprintf( stderr, "Error beginning input transfer.\n" );
    goto SHUTDOWN;
  }

  /* Don't start output until we have some buffers, unless we're only
   * transferring one buffer
   */
  if ( requested_buffers == 1 ) {
    if ( mlBeginTransfer( outputOpenPath ) ) { 
      fprintf( stderr, "Error beginning output transfer.\n" );
      goto SHUTDOWN;
    }
    output_going = 1;
  }

  /* Get wait handles */
  if ( mlGetReceiveWaitHandle( inputOpenPath, &inputPathWaitHandle ) ) {
    fprintf( stderr, "Cannot get input wait handle\n" );
    goto SHUTDOWN;
  }

  if ( mlGetReceiveWaitHandle( outputOpenPath, &outputPathWaitHandle ) ) {
    fprintf( stderr, "Cannot get output wait handle\n" );
    goto SHUTDOWN;
  }

  if ( debug ) {
    fprintf( stderr, " input wait handle %d output wait handle %d\n",
	     inputPathWaitHandle, outputPathWaitHandle );
  }

  while ( requested_buffers == 0 || transferred_buffers < requested_buffers ) {

    /* Wait for input or ouptut event... */
    MLint32 input_message;	/* boolean */
    MLstatus status = event_wait( inputPathWaitHandle, outputPathWaitHandle,
				  &input_message );
    if ( status != ML_STATUS_NO_ERROR ) {
      exit( 1 );
    }

    /* Let's see what reply message is ready... */
    { /* reply message scope */
      MLint32 messageType;
      MLpv* message;
      MLopenid OpenPath = input_message ? inputOpenPath : outputOpenPath;

      if ( mlReceiveMessage( OpenPath, &messageType, &message ) ) { 
	fprintf( stderr, "\nUnable to receive reply message\n" );
	goto SHUTDOWN;
      }

      switch ( messageType ) {

      case ML_BUFFERS_COMPLETE:
	transferred_buffers++;

	if ( input_message ) {
	  union { MLint32 word; char bytes[5]; } userdata = {0};
	  MLint64 theMSC = message[1].value.int64;
	  MLint64 theUST = message[2].value.int64;
	  MLpv *pv = mlPvFind( message, ML_VITC_TIMECODE_INT32 );
	  MLpv *uv = mlPvFind( message, ML_VITC_USERDATA_INT32 );
	  MLpv line[] = { { ML_VITC_INCOMING_LINE_NUMBER_INT32 },
			  { ML_END } };

	  assert( message[0].param == ML_IMAGE_BUFFER_POINTER );

	  if ( uv ) {
	    userdata.word = uv->value.int32;
	  }

	  if ( pv ) {
	    mluTCUnpack( &tc, pv->value.int32 );
	    mlGetControls( inputOpenPath, line );

	    if ( debug > 1 ) {
	      printf( " VITC %s %02d:%02d:%02d%s%02d on line %d\n",
		      userdata.bytes, tc.hours, tc.minutes, tc.seconds,
		      /* display a ":" for even field, "." for odd field 
		       * (just like sony monitors ;-)
		       */
		      tc.evenField? ":" : ".", tc.frames,
		      line[0].value.int32 );
	    }
	  }

	  if ( debug > 1 ) {
	    char ustb[100];
	    printf( "  transfer %d complete: length:%d MSC:%"
		    FORMAT_LLD " UST:%" FORMAT_LLD " %s\n",
		    transferred_buffers, message[0].length,
		    theMSC, theUST, CONVUST( theUST, ustb ) );

	    if ( debug > 2 ) {
	      int i, *p = message[0].value.pInt32;
	      for ( i = 0; i < 32; i++ ) {
		printf( "0x%08x%s", *p++, ( i & 3 ) == 3? " ":"\n" );
	      }
	    }

	  } else if ( debug && pv ) {
	    printf( "\rVITC %s %02d:%02d:%02d%s%02d line %d  ", 
		    userdata.bytes, tc.hours, tc.minutes, tc.seconds,
		    /* display a ":" for even field, "." for odd field 
		     * (just like sony monitors ;-)
		     */
		    tc.evenField? ":" : ".", tc.frames,
		    line[0].value.int32 );
	    fflush( stdout );

	  } else if ( ( transferred_buffers%10 ) == 0 ) {
	    fprintf( stderr, "." );
	    fflush( stderr );
	  }

	  /* Send filled input buffer to output path */
	  mlSendBuffers( outputOpenPath, message );
	  if ( output_going == 0 && transferred_buffers >= 2 ) {
	    if ( mlBeginTransfer( outputOpenPath ) ) { 
	      fprintf( stderr, "Error beginning output transfer.\n" );
	      goto SHUTDOWN;
	    }
	    output_going = 1;
	  }

	} /* if input_message */
	else {
	  /* Send empty output buffer to input path */
	  mlSendBuffers( inputOpenPath, message );
	}
	break;

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
	/* fprintf( stderr, "\nWarning, missed field\n" ); */
	break;

      case ML_EVENT_QUEUE_OVERFLOW:
	fprintf( stderr, "\nEvent queue overflowed, aborting\n" );
	goto SHUTDOWN;

      default:
	fprintf( stderr, "\nSomething went wrong - message is %s\n", 
		 mlMessageName( messageType ) );
	goto SHUTDOWN;
      } /* switch */

      fflush( stdout );
    } /* reply message scope */
  } /* while requested_buffers .... */

 SHUTDOWN:

  fprintf( stderr, "%d buffers transferred\nShutdown\n",
	   transferred_buffers );

  mlEndTransfer( inputOpenPath );
  mlEndTransfer( outputOpenPath );
  mlClose( inputOpenPath );
  mlClose( outputOpenPath );
  return 0;
}


/*--------------------------------------------------------------allocateBuffers
 *
 * Allocate an array of image buffers with specified alignment and size
 */
MLint32 allocateBuffers( void** buffers, MLint32 imageSize,
			 MLint32 maxBuffers, MLint32 memAlignment )
{
  int i;

  for ( i = 0; i < maxBuffers; i++ ) {
    buffers[i] = memalign( memAlignment, imageSize );
    if ( buffers[i] == NULL ) {
      fprintf( stderr, "Memory allocation failed\n" );
      return -1;
    }

    /* Here we touch the buffers, forcing the buffer memory to be
     * mapped this avoids the need to map the buffers the first time
     * they're used.  We could go the extra step and mpin them, but
     * choose not to here, trying a simpler approach first.
     */
    memset( buffers[i], 0xf0f0f0f0, imageSize );
    /*
     *{ int *t = &((int**)buffers)[i][1000];
     *  printf( "0x%p: 0x%x 0x%x 0x%x\n", t, t[0], t[1], t[2] ); }
     */
  }
  return 0;
}


/*---------------------------------------------------------------print_controls
 */
int print_controls( MLint64 openPath, FILE *f )
{
  /* First make a list of all the parameters we need to write */
  MLpv* devCap;
  MLpv* paramIds;
  char paramBuffer[60], valueBuffer[60];
  int i;

  if ( mlGetCapabilities( openPath, &devCap) ) {
    fprintf( stderr, "Unable to get device capabilities" );
    return -1;
  }

  paramIds = mlPvFind( devCap, ML_PARAM_IDS_INT64_ARRAY );
  if ( paramIds == NULL ) {
    fprintf( stderr, "Unable to find param id list" );
    return -1;
  }

  for ( i=0; i < paramIds->length; i++ ) {
    MLint32 class = ML_PARAM_GET_CLASS( paramIds->value.pInt64[i] );

    if ( class == ML_CLASS_VIDEO || class == ML_CLASS_IMAGE ||
	 class == ML_CLASS_AUDIO ) {
      int vs, ps;
      MLpv* paramCap;
      MLpv controls[2];
      MLstatus stat;

      if( paramIds->value.pInt64[i] == ML_VIDEO_UST_INT64 ||
	  paramIds->value.pInt64[i] == ML_VIDEO_MSC_INT64 ||
	  paramIds->value.pInt64[i] == ML_VIDEO_ASC_INT64 ||
	  paramIds->value.pInt64[i] == ML_AUDIO_UST_INT64 ||
	  paramIds->value.pInt64[i] == ML_AUDIO_MSC_INT64 ||
	  paramIds->value.pInt64[i] == ML_AUDIO_ASC_INT64 ) {
	continue;
      }

      controls[0].param = paramIds->value.pInt64[i];
      controls[1].param = ML_END;

      /* If we're writing to a file, we should only save those params
       * that can actually be set...
       */
      if ( f != stdout ) {
	if ( mlPvGetCapabilities( openPath, controls[0].param, &paramCap ) ) {
	  fprintf( stderr, "Unable to get param 0x%" FORMAT_LLX
		   " capabilities", controls[0].param );
	  return -1;
	}

	{
	  MLpv *pv = mlPvFind( paramCap, ML_PARAM_ACCESS_INT32 );
	  if ( !pv || !( pv->value.int32 & ML_ACCESS_W ) ) {
	    continue;
	  }
	}
      } /* if f != stdout */

      if ( stat = mlGetControls( openPath, controls ) ) {
	if ( debug ) {
	  char pname[64] = "";
	  int size = sizeof( pname );
	  mlPvParamToString( openPath, controls, pname, &size );
	  fprintf( stderr, "Unable to get param %s: %s",
		   pname, mlStatusName( stat ) );
	  continue;
	}
      }

      ps = sizeof( paramBuffer ) - 1;
      if ( mlPvParamToString( openPath, controls, paramBuffer, &ps ) ) {
	fprintf( stderr, "Unable to convert param 0x%" FORMAT_LLX " to string",
		 controls[0].param );
	continue;
      }

      vs = sizeof( valueBuffer ) - 1;
      if ( mlPvValueToString( openPath, controls, valueBuffer, &vs ) ) {
	fprintf( stderr, "Unable to convert %s value %d (0x%x) to string",
		 paramBuffer, controls[0].value.int32,
		 controls[0].value.int32 );
	continue;
      }

      if ( ps + vs < 72 ) {
	fprintf( f, "\t%s = %s\n", paramBuffer, valueBuffer );
      } else {
	fprintf( f, "\t%s =\n\t\t%s", paramBuffer, valueBuffer );
      }
    } /* if class == ML_CLASS_VIDEO */
  } /* for i=0..paramIds->legth */

  mlFreeCapabilities( devCap );
  return 0;
}

