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

/**********************************************************************
 * mlutimecode.c
 *
 *  Timecode related general purpose routines.
 *
 *  Stolen from IRIX libdmedia and munged - bloch 6 January 2000
 *
 **********************************************************************/

#include <stdio.h>	/* for NULL */
#include <stdlib.h>	/* for NULL */
#ifdef __sgi
#include <bstring.h>	/* for bcopy */
#endif
#include <string.h>     /* strdup, strrchr, etc */
#include <ctype.h>      /* for isdigit */
#include <math.h>       /* for floor */

#include <assert.h>     /* for assert */

#include <errno.h>	/* for interrupt condition */

#include <ML/ml.h>      
#include <ML/mlutimecode.h>

/*******************
 * Prototypes for static functions
 *******************/

#define MLboolean int
#define ML_TRUE 1
#define ML_FALSE 0

static MLboolean __SanityCheck( const MLUTimeCode * a );
static int       __FramesPerHour( const int type );
static int       __FramesPerDay ( const int type );
static int       __FramesPerSec ( const int type ); /* DO NOT CALL on a DROP 
                                                       FRAME type!!!!! */
static int       __FramesPastMidnight( const MLUTimeCode * a );
/* return the number of frames past midnight that a represents */
static void      __FramesPastMidnightToSmpte( MLUTimeCode * result,
                                             const MLU_TC_TYPE tc_type, 
                                             int  pastMid );
static MLboolean __iswholenum( const char * input );


/* ---------------------------------------------------------------mluTCToString
 */
MLstatus MLAPI 
mluTCToString( char *string, const MLUTimeCode *smpteTimecode )
{
  if ( !__SanityCheck( smpteTimecode ) ) {
    return ML_STATUS_INVALID_ARGUMENT;
  }
  sprintf( string, "%02d:%02d:%02d:%02d",
	   smpteTimecode->hours,
	   smpteTimecode->minutes,
	   smpteTimecode->seconds,
	   smpteTimecode->frames );
  return ML_STATUS_NO_ERROR;
}


/* -------------------------------------------------------------mluTCFromString
 */
MLstatus MLAPI 
mluTCFromString( MLUTimeCode * result, const char * inputString,
		 MLU_TC_TYPE tc_type )
{
  char *string;                 /* the input string should not be modified. */
  int posMarker;
  char *dividerloc, *tmp;

  assert( result && inputString );

  string = strdup( inputString ); /* make a local copy that we can modify */
  assert( string );

  result->hours   = 0;
  result->minutes = 0;
  result->seconds = 0;
  result->frames  = 0;
  result->tc_type = tc_type;

  posMarker = 0;

  for ( ;; ) {
    int val;
    dividerloc = strrchr( string, ':' );
    /* later on, we may want to change the dividerloc logic so that it
     * also accepts formats like the ones in the yellow book, ie,
     * ones that use periods, or ' or " as dividing marks. For now, we'll
     * just accept colons, since it seems to be the most common format.
     */
    
    if ( dividerloc == NULL ) {
      tmp = string;             
      /* if we couldn't find a divider, use the whole string!
       */
    } else {
      tmp = &dividerloc[1];
    }

    if ( !__iswholenum( tmp ) ) {
      free( string );
      return ML_STATUS_INVALID_ARGUMENT;
    }

    val = atoi( tmp );

    switch( posMarker++ ) {
    case 0:
      result->frames  = val; break;
    case 1:
      result->seconds = val; break;
    case 2:
      result->minutes = val; break;
    case 3: 
      result->hours   = val; break;
    default:
      free( string );
      return ML_STATUS_INVALID_ARGUMENT; /* user supplied too many fields! */
    }
    
    if ( dividerloc == NULL ) {
      break;                    /* no more to work with--we're done!*/
    }

    *dividerloc = '\0';         /* hack off last bit of string */
  }

  free( string );

  if ( __SanityCheck( result ) ) {
    return ML_STATUS_NO_ERROR;
  }

  return ML_STATUS_INVALID_ARGUMENT;
}


/* ------------------------------------------------------------mluTCFromSeconds
 */
MLstatus MLAPI mluTCFromSeconds( MLUTimeCode * result,
				 const MLU_TC_TYPE tc_type,
				 const double seconds )
{
  int  frames;
  double fps;
  
  if ( seconds < 0 || seconds > 24*60*60 ) {
    return ML_STATUS_INVALID_ARGUMENT;
  }

  /* Convert fractional seconds into time code by going via frames!
   */
  if ( (tc_type & MLU_TC_DROP_MASK) == MLU_TC_DROP ) {
      if ( tc_type == (MLU_TC_5994_8FIELD_DROP) ){
        fps = 59.94;
      } else {
        /* don't support brazil's M-PAL drop (30fps) */
        assert( tc_type == (MLU_TC_2997_4FIELD_DROP) );
        fps = 29.97;
     }
  } else {
    fps = __FramesPerSec( tc_type );
  }

  /* round to nearest frames!
   */
  frames = (MLint32) floor( 0.5 + seconds * fps );

  __FramesPastMidnightToSmpte( result, tc_type, frames );

  return ML_STATUS_NO_ERROR;
}


/* --------------------------------------------------------------mluTCToSeconds
 */
MLstatus MLAPI mluTCToSeconds( const MLUTimeCode * a, double * seconds )
{
  int  frames;
  MLUTimeCode midnight;
  double fps;

  /* Convert SMPTE to fractional seconds by going via frames
   */
  if ( !__SanityCheck( a ) ) {
    return ML_STATUS_INVALID_ARGUMENT;
  }

  memset( &midnight, 0, sizeof( midnight ) );
  midnight.tc_type = a->tc_type;

  mluTCFramesBetween( &frames, &midnight, a );

  if ( (a->tc_type & MLU_TC_DROP_MASK) == MLU_TC_DROP ) {
    /* don't support brazil's M-PAL drop (30fps)
     */
    assert ( a->tc_type == (MLU_TC_2997_4FIELD_DROP) );
    fps = 29.97;
  } else {
    fps = __FramesPerSec( a->tc_type );
  }

  *seconds = frames/fps;

  return ML_STATUS_NO_ERROR;
}


/* -------------------------------------------------------------------mluTCPack
 */
MLstatus MLAPI mluTCPack( MLint32 *word, const MLUTimeCode *tc )
{
  MLTimeCodePackedRaw packed = {0};
  MLint32 isPal = ( tc->tc_type & MLU_TC_FORMAT_MASK ) == MLU_TC_FORMAT_PAL;

  if ( !__SanityCheck( tc ) ) {
    return ML_STATUS_INVALID_ARGUMENT;
  }

  packed.bits.hrs1    = tc->hours / 10;
  packed.bits.hrs2    = tc->hours % 10;
  packed.bits.mins1   = tc->minutes / 10;
  packed.bits.mins2   = tc->minutes % 10;
  packed.bits.secs1   = tc->seconds / 10;
  packed.bits.secs2   = tc->seconds % 10;
  packed.bits.frames1 = tc->frames / 10;
  packed.bits.frames2 = tc->frames % 10;
  packed.bits.drop    = tc->dropFrame;
  packed.bits.color   = tc->colorLock;

  if ( isPal ) {
    packed.bits.bit75   = tc->evenField;
    packed.bits.bit74   = (tc->userType>>1) & 1;
    packed.bits.bit55   = (tc->userType>>2) & 1;
    packed.bits.bit35   = (tc->userType   ) & 1;
  } else /* is Ntsc */ {
    packed.bits.bit75   = (tc->userType>>2) & 1;
    packed.bits.bit74   = (tc->userType>>1) & 1;
    packed.bits.bit55   = (tc->userType   ) & 1;
    packed.bits.bit35   = tc->evenField;
  }

  *word = packed.word;
  return ML_STATUS_NO_ERROR;
}


/* -----------------------------------------------------------------mluTCUnpack
 */
MLstatus MLAPI mluTCUnpack( MLUTimeCode *tc, MLint32 word )
{
  MLTimeCodePackedRaw packed;
  MLint32 isPal = ( tc->tc_type & MLU_TC_FORMAT_MASK ) == MLU_TC_FORMAT_PAL;

  packed.word = word;

  tc->hours     = packed.bits.hrs1 * 10 + packed.bits.hrs2;
  tc->minutes   = packed.bits.mins1 * 10 + packed.bits.mins2;
  tc->seconds   = packed.bits.secs1 * 10 + packed.bits.secs2;
  tc->frames    = packed.bits.frames1 * 10 + packed.bits.frames2;
  tc->dropFrame = packed.bits.drop;
  tc->colorLock = packed.bits.color;

  if ( isPal ) {
    tc->evenField = packed.bits.bit75;
    tc->userType  = packed.bits.bit55 << 2 | packed.bits.bit74 << 1 |
      packed.bits.bit35;
  } else /* is Ntsc */ {
    tc->evenField = packed.bits.bit35;
    tc->userType  = packed.bits.bit75 << 2 | packed.bits.bit74 << 1 |
      packed.bits.bit55;
  }

  if ( !__SanityCheck( tc ) ) {
    return ML_STATUS_INVALID_ARGUMENT;
  }

  return ML_STATUS_NO_ERROR;
}


/* -----------------------------------------------------------mluTCFramesPerDay
 */
int MLAPI mluTCFramesPerDay( const int type )
{
  return __FramesPerDay( type );
}


/* ------------------------------------------------------------------mluTCAddTC
 */
MLstatus MLAPI mluTCAddTC( MLUTimeCode * result, const MLUTimeCode * a, 
			   const MLUTimeCode * b, int * overflow )
{
  int  totalFrames;
  if ( overflow ) {
    *overflow = 0;
  }

  if ( ! __SanityCheck( a ) || 
       ! __SanityCheck( b ) ) {
    return ML_STATUS_INVALID_ARGUMENT;
  }

  if ( a->tc_type != b->tc_type ) {
    return ML_STATUS_INVALID_ARGUMENT;
  }
  
  totalFrames  =  __FramesPastMidnight( a );
  totalFrames +=  __FramesPastMidnight( b );

  if ( totalFrames >= __FramesPerDay( a->tc_type ) ) {
    int daysOver = totalFrames/__FramesPerDay( a->tc_type );
    if ( overflow ) {
      *overflow = 1;
    }
    totalFrames -= __FramesPerDay( a->tc_type )*daysOver;
  }

  assert( totalFrames >= 0 && totalFrames < __FramesPerDay( a->tc_type ) );

  __FramesPastMidnightToSmpte( result, a->tc_type, totalFrames );

  return ML_STATUS_NO_ERROR;
}


/* --------------------------------------------------------------mluTCAddFrames
 *
 * mluTCAddFrames takes smpte time (a) and adds b frames (of smpte
 * type a.tc_type) to it.
 *
 * In order to convert frames into a smpte value, pass in a 0:0:0:0
 * smpte value as a, with tc_type set to the smpte style in which
 * you'd like the result!
 */
MLstatus MLAPI mluTCAddFrames( MLUTimeCode * result, const MLUTimeCode * a,
			       int  b, int * overflowunderflow )
{
  int  pastMid;
  if ( overflowunderflow ) {
    *overflowunderflow = 0;
  }

  if ( !__SanityCheck( a ) ) {
    return ML_STATUS_INVALID_ARGUMENT;
  }

  pastMid = __FramesPastMidnight( a );

  pastMid += b;

  if ( pastMid < 0 ) {
    int daysUnder = 1 + (-pastMid)/__FramesPerDay( a->tc_type );
    if ( overflowunderflow ) {
      *overflowunderflow = -1;
    }
    pastMid += __FramesPerDay( a->tc_type )*daysUnder;
  }
  
  if ( pastMid >= __FramesPerDay( a->tc_type ) ) {
    int daysOver = (pastMid)/__FramesPerDay( a->tc_type);
    if ( overflowunderflow ) {
      assert( *overflowunderflow != -1 );
      *overflowunderflow = 1;
    }
    pastMid -= __FramesPerDay( a->tc_type )*daysOver;
  }

  assert( pastMid >= 0 && pastMid < __FramesPerDay( a->tc_type ) );

  __FramesPastMidnightToSmpte( result, a->tc_type, pastMid );

  return ML_STATUS_NO_ERROR;
}


/* ----------------------------------------------------------mluTCFramesBetween
 *
 * mluTCFramesBetween returns the number of frames: b - a
 */
MLstatus MLAPI mluTCFramesBetween( int  * result, const MLUTimeCode * a, 
				   const MLUTimeCode * b )
{
  int  pastMida, pastMidb;

  if ( a->tc_type != b->tc_type ||
       !__SanityCheck( a ) ||
       !__SanityCheck( b ) ) {
    return ML_STATUS_INVALID_ARGUMENT;
  }

  pastMida = __FramesPastMidnight( a );
  pastMidb = __FramesPastMidnight( b );

  *result = pastMidb - pastMida;

  return ML_STATUS_NO_ERROR;
}


/*******************************************************
 * Implementations of static functions
 *******************************************************/

/* ---------------------------------------------------------------__SanityCheck
 *
 * Returns TRUE only on valid MLUTimeCode's.
 * 
 * It will return ML_FALSE if you pass it an invalid MLUTimeCode
 *
 * Thus, during calculations, internal Sanity checks can say
 * assert( __SanityCheck( foo ) );
 *
 * But for Parameter checking of inputs, you can say
 * if ( !__SanityCheck( foo ) )
 *     return MLU_ERROR;
 */
static MLboolean __SanityCheck( const MLUTimeCode *a )
{
  if ( a->hours   < 0 || a->hours   > 23  ||
       a->minutes < 0 || a->minutes > 59  ||
       a->seconds < 0 || a->seconds > 59  ||
       a->frames  < 0 ) {
    return ML_FALSE;
  }

  switch( a->tc_type ) {
  case MLU_TC_24_ND: return a->frames < 24;
  case MLU_TC_25_ND: return a->frames < 25;
  case MLU_TC_30_ND: return a->frames < 30;
  case MLU_TC_60_ND: return a->frames < 60;
  case MLU_TC_2997_4FIELD_DROP:
    if ( a->frames > 29 ) {
	return ML_FALSE;
    }
    if ( a->seconds == 0 && ( a->frames == 0 || a->frames == 1 ) ) {
      /* SMPTE INTERNAL ERROR -- INSANITY: 30_DROP sanity check!
       */
      if ((a->minutes%10) != 0) {
	return ML_FALSE;
      }
    }
    return ML_TRUE;

  case MLU_TC_5994_8FIELD_DROP:
    if ( a->frames > 59 ) {
	return ML_FALSE;
    }
    if ( a->seconds == 0 && 
	 ( a->frames == 0 || a->frames == 1 || a->frames == 2 ||
	   a->frames == 3 ) ) {
      /* SMPTE INTERNAL ERROR -- INSANITY: 60_DROP sanity check!
       */
      if ( (a->minutes%10) != 0 ) {
	return ML_FALSE;
      }
    }
    return ML_TRUE;

  default:
    /* we don't support M-PAL drop (brazil 30fps drop-frames)
     */
#ifdef DEBUG
    fprintf( stderr, "SMPTE INTERNAL ERROR -- INSANITY: unknown tc_type\n" );
    abort();
#endif
    return ML_FALSE;
  }
}


/* -------------------------------------------------------------__FramesPerHour
 */
static int  __FramesPerHour( const int type )
{
  switch( type ) {
  case MLU_TC_24_ND: return 24*60*60;
  case MLU_TC_25_ND: return 25*60*60;
  case MLU_TC_30_ND: return 30*60*60;
  case MLU_TC_2997_4FIELD_DROP: return 107892; /* 29.97*(60*60) */
  case MLU_TC_5994_8FIELD_DROP: return 215784; /* 59.94*(60*60) */
  case MLU_TC_60_ND: return 60*60*60;
  default:
    abort();
  }
  return 0;                     /* make compiler happy */
}


/* --------------------------------------------------------------__FramesPerDay
 */
static int  __FramesPerDay( const int type )
{
  return 24*__FramesPerHour( type );
}


/* --------------------------------------------------------------__FramesPerSec
 *
 * DO NOT CALL __FramesPerSec on a DROP-FRAME type!!!!!
 */
static int  __FramesPerSec( const int type )
{
  switch( type ) {
  case MLU_TC_24_ND: return 24;
  case MLU_TC_25_ND: return 25;
  case MLU_TC_30_ND: return 30;
  case MLU_TC_60_ND: return 60;
  default:
    fprintf( stderr, "SMPTE INTERNAL ERROR -- can't get fps for df type!\n" );
    abort();
  }
  return 0;
}


/* --------------------------------------------------------__FramesPastMidnight
 */
static int  __FramesPastMidnight( const MLUTimeCode * a )
{
  MLUTimeCode time;
  int  result;

  memcpy( &time, a, sizeof( time ) );
  assert( __SanityCheck( &time ) ); /* XXX This is parameter checking */

  result = time.hours * __FramesPerHour( time.tc_type );
  time.hours = 0;

  if ( (time.tc_type & MLU_TC_DROP_MASK) == MLU_TC_DROP ) {
    int  fps;
    /* Drop frames require additional work
     */

    assert( time.tc_type != (MLU_TC_2997_8FIELD_DROP) );
    /* We don't support M-PAL drop (brazil; 30 fps drop frames)
     */

    /* now, let's clean out the ten's column of the minutes in time
     */
    if ( time.minutes >= 10 ) {
      int  framesPerTenMinutes;
      int  tmp;
      /* in smpte 30D, 
       * there is always a fixed number of frames per ten minutes in 30_DROP
       * 
       * So, first thing we'll do is get time to look like 00:0x:yy:zz.
       * Doing this will speed up the remaining part (which is slower),
       * because we'll have at most 10 minutes of SMPTE to figure out.
       *
       * there are 1800 frames in the first minutes of every 10 minutes,
       * and there are 1798 frames in the remaining 9 minutes of each 10
       * minutes.
       */

      framesPerTenMinutes = 9*(1798) + (1800);
      if ( time.tc_type == (MLU_TC_5994_8FIELD_DROP) ) {
          framesPerTenMinutes *= 2;
      }

      tmp          = time.minutes/10;
      result      += framesPerTenMinutes * tmp;
      time.minutes -= tmp * 10;
    }

    assert( time.minutes < 10 );

    /* minutes 1,2,3,4,5,6,7,8,9 all have a fixed number of frames in them:
     * 1798. So, let's chop time down to something that looks like
     * 00:00:yy:zz
     */

    if ( time.minutes > 0 ) {
      /* the first minutes of each 10 minutes has 1800 frames.
       */
      int  mins = time.minutes;
      if (time.tc_type == (MLU_TC_5994_8FIELD_DROP)) {
	result += 3600;
	mins -= 1;
	/* Chop off the dropped frames--at this point, we need to
	 * loose the four frames
	 */
	time.frames -= 4;

      } else {
	result += 1800;
	mins -= 1;
        /* Chop off the dropped frames--at this point, we need to
	 * loose the two frames
	 */
	time.frames -= 2;
      }
      
      /* all other minutes of each 10 minutes have 1798 frames
       */
      if ( mins > 0 ) {
        if (time.tc_type == (MLU_TC_5994_8FIELD_DROP)) {
          result += mins * 3596;
        } else {
          result += mins * 1798;
        }
      }
    }

#ifndef NDEBUG
    /* make sure our -= 2 above is ok (this code needs rewrite)
     */
    if ( time.tc_type == (MLU_TC_2997_4FIELD_DROP) ) {
      if ( time.seconds == 0 ) {
        assert( time.frames >= 0 );
      } else {
        assert( time.frames >= -2 );
      }
    } else { /* MLU_TC_5994_8FIELD_DROP */
      if ( time.seconds == 0 ) {
        assert( time.frames >= 0 );
      } else {
        assert( time.frames >= -4 );
      }
    } 
#endif

    /* All seconds values except for 00 have 28 frames per seconds, so
     * let's pare down to get something that looks like 00:00:00:zz
     */

    if ( time.tc_type == (MLU_TC_2997_4FIELD_DROP) ) {
      fps = 30;
    } else {
      fps = 60;
    }

    if ( time.seconds > 0 ) {
      int  secs = time.seconds;
      result += fps;
      secs -= 1;
      if ( secs > 0 ) {
        result += secs * fps;
        time.seconds = 0;
      }
    }

    /* now, we can just add in the frames!!!
     */
    result += time.frames;
    time.frames = 0;

  } else {
    /* For non-drop, there is always a fixed number of frames per minutes,
     * So minutes are just as easy to do as seconds.
     * And there are a fixed number of frames per seconds.
     * And we can just add in the frames.
     * So this code is dirt simple!
     */
    int  fps;

    fps = __FramesPerSec( time.tc_type );

    /* we only do the above if non-drop!
     */

    result += time.minutes * 60 * fps;
    result += time.seconds * fps;
    result += time.frames;
    time.minutes = 0;
    time.seconds = 0;
    time.frames = 0;
  }

  return result;
}


/* -------------------------------------------------__FramesPastMidnightToSmpte
 */
static void __FramesPastMidnightToSmpte( MLUTimeCode * result, 
                                         const MLU_TC_TYPE tc_type, 
                                         int  pastMid )
{
  int  fph;
  int  tmp;

  result->hours   = 0;
  result->minutes = 0;
  result->seconds = 0;
  result->frames  = 0;
  result->tc_type = tc_type;

  fph = __FramesPerHour( tc_type );

  tmp = pastMid / fph;
  result->hours = tmp;
  pastMid -= tmp*fph;

  /* Now we have less than 1 hours worth of smpte to deal with
   *
   * If we're non-drop, then it's easy.
   * If we're drop, then we have more work to do
   */
  if ( (tc_type & MLU_TC_DROP_MASK) == MLU_TC_DROP ) {
    int  fpm, fps;

    assert( tc_type != (MLU_TC_2997_8FIELD_DROP) ); 

    /* we don't support M-PAL drop (30 fps, drop, used in Brazil)
     */

    /* let's see how many tens of minutes we have in pastMid
     */
    {
      int  framesPerTenMinutes;
      int  tmp;

      /* there are 1800 frames in the first minutes of every 10
       * minutes, and there are 1798 frames in the remaining 9 minutes
       * of each 10 minutes.
       */
      framesPerTenMinutes = 9*1798 + 1800;
      if (tc_type == (MLU_TC_5994_8FIELD_DROP)) {
	framesPerTenMinutes *= 2;
      }

      tmp = pastMid/framesPerTenMinutes;
      result->minutes += 10*tmp;
      pastMid -= tmp * framesPerTenMinutes;
    }

    /* Now we have less than 10 minutes worth of frames
     */
    fpm = 60*30;
    if ( tc_type == (MLU_TC_5994_8FIELD_DROP )) {
        fpm *= 2;
    }
    if ( pastMid >= fpm ) {
      result->minutes += 1;
      pastMid -= fpm;
      if ( tc_type == (MLU_TC_5994_8FIELD_DROP) ) {
        fpm = 59*60+56;
      } else {
        fpm = 59*30+28;
      }
      while ( pastMid >= fpm ) {
        result->minutes += 1;
        pastMid -= fpm;
      }
    }

    /* now we have less than 1 minutes worth of frames
     */
    if ( (result->minutes%10) == 0 ) {
      fps = 30;
    } else {
      fps = 28;
    }
    if ( tc_type == (MLU_TC_5994_8FIELD_DROP) ) {
      fps *= 2;
    }
    if ( pastMid >= fps ) {
      result->seconds += 1;
      pastMid -= fps;

      fps = 30;
      if ( tc_type == (MLU_TC_5994_8FIELD_DROP) ) {
        fps *= 2;
      }

      while ( pastMid >= fps ) {
        result->seconds += 1;
        pastMid -= fps;
      }
    }

    /* Now, we have the correct number of frames
     */
    if ( (result->minutes%10)!=0 && result->seconds == 0 ) {
      result->frames = pastMid;
      if ( tc_type == (MLU_TC_5994_8FIELD_DROP) ) {
	result->frames += 4;
      } else {
	result->frames += 2;
      }
    } else {
      result->frames = pastMid;
    }

    assert( __SanityCheck( result ) );

  } else {
    /* HEY! Non-drop is EASY!!!!!
     */
    int  fps, fpm;

    fps = __FramesPerSec( tc_type ); /* only call this if nondrop! */
    fpm = 60 * fps;

    tmp = pastMid / fpm;
    assert( tmp < 60 );
    result->minutes = tmp;
    pastMid -= tmp * fpm;

    tmp = pastMid / fps;
    assert( tmp < 60 );
    result->seconds = tmp;
    pastMid -= tmp * fps;

    result->frames = pastMid;
  }

  assert( __SanityCheck( result ) );
}


/* ----------------------------------------------------------------__iswholenum
 */
static MLboolean __iswholenum( const char * input )
{
  char * p;

  for( p = (char*) input; *p != 0; p++ ) {
    if ( ! isdigit( *p ) ) {
      return ML_FALSE;
    }
  }

  return ML_TRUE;
}
