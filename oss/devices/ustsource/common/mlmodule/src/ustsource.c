/***************************************************************************
 * License Applicability. Except to the extent portions of this file are 
 * made subject to an alternative license as permitted in the Khronos 
 * Free Software License Version 1.0 (the "License"), the contents of 
 * this file are subject only to the provisions of the License. You may 
 * not use this file except in compliance with the License. You may obtain 
 * a copy of the License at 
 * 
 * The Khronos Group Inc.:  PO Box 1019, Clearlake Park CA 95424 USA or at
 *  
 * http://www.Khronos.org/licenses/KhronosOpenSourceLicense1_0.pdf
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

#include <ML/ml_didd.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#ifdef ML_OS_LINUX
#include <sys/time.h>
#endif

#ifdef ML_OS_IRIX
#include <dmedia/dmedia.h>
#endif

#ifdef DEBUG
static int debugLevel = -1;
/* error printouts */
#define DEBG1(block) if (debugLevel >= 1) { block; fflush(stdout); }
/* basic function printouts */
#define DEBG2(block) if (debugLevel >= 2) { block; fflush(stdout); }
/* detail printouts */
#define DEBG3(block) if (debugLevel >= 3) { block; fflush(stdout); }
/* extreme debug printouts */
#define DEBG4(block) if (debugLevel >= 4) { block; fflush(stdout); }
#else
#define DEBG1(block)
#define DEBG2(block)
#define DEBG3(block)
#define DEBG4(block)
#endif

static void initDebugLevel(void)
{
#ifdef DEBUG
  if ( debugLevel == -1 ) {
    char *e = getenv("USTSOURCE_DEBUG");
    if ( e ) {
      debugLevel = atoi(e);
    } else {
      debugLevel = 0;
    }
  }
#endif

  return;
}


/*=============================================================================
 *
 * Entry points called by the OpenML SDK
 *
 *===========================================================================*/


/*----------------------------------------------------------------ddInterrogate
 *
 * This is the first device dependent entry point called by the mlSDK.
 * This routine would normally call mlDINewPhysicalDevice(), but this
 * module doesn't provide any devices -- so it jus calls
 * mlDINewUSTSource()
 */
MLstatus ddInterrogate( MLsystemContext systemContext,
			MLmoduleContext moduleContext )
{
  MLstatus status = ML_STATUS_NO_ERROR;
  static const char USTName[] = "ustsource UST";
  static const char USTDescription[] = "ML SDK sample software UST source "
    "in module ustsource; for demo purposes only.";

  initDebugLevel();

  DEBG1( printf( "[ustsource]: in ddInterrogate\n" ) );

  /* Add a UST source only if explicitly enabled by an environment
   * variable. Use bogus metrics, but allow them to be overriden by
   * environment variables. This allows us to test various
   * combinations of modules.
   */
  if ( getenv( "USTSOURCE_ENABLE" ) ) {
    MLint32 updatePeriod = 1800;
    MLint32 latencyVar = 9000;

    char* e = getenv( "USTSOURCE_UPDATE_PERIOD" );
    if ( e ) {
      updatePeriod = atoi( e );
    }
    e = getenv( "USTSOURCE_LATENCY_VAR" );
    if ( e ) {
      latencyVar = atoi( e );
    }

    DEBG1( printf( "[ustsource]: registering UST source, update period = %d"
		   ", latency var = %d\n", updatePeriod, latencyVar ) );
    status = mlDINewUSTSource( systemContext, moduleContext,
			       updatePeriod, latencyVar,
			       USTName, (MLint32) strlen( USTName ),
			       USTDescription,
			       (MLint32) strlen( USTDescription ) );
  }
  return status;
}


/*--------------------------------------------------------------------ddConnect
 *
 * ddConnect(3dm) will be called in every process that accesses a
 * physical device exposed by your device-dependent module.  It
 * establishes the connection between the process independent
 * description of a physical device and process specific addresses for
 * entry points and private memory structures.
 *
 * It should never be called in our case, since we don't expose any
 * devices (but it must be present for the module to be valid)
 */
/* ARGSUSED */
MLstatus ddConnect( MLbyte* physicalDeviceCookie,
		    MLint64 staticDeviceId,
		    MLphysicalDeviceOps* pOps,
		    MLbyte** ddDevicePriv )
{
  initDebugLevel();

  DEBG1( printf( "[ustsource]: in ddConnect. Should never be called!\n" ) );
  return ML_STATUS_INVALID_ARGUMENT;
}


/*---------------------------------------------------------------------ddGetUST
 *
 * ddGetUST will be called by other modules and the SDK to obtain UST
 * timestamps, *if* the current module was selected as the system-wide
 * source for UST.
 */
MLint32 ddGetUST( MLint64* UST )
{
  static MLint64 USTBase = 0;
  MLint64 rawUST;

  initDebugLevel();

#ifdef ML_OS_IRIX
  {
    dmGetUST((unsigned long long*) &rawUST);
  }
#endif

#ifdef ML_OS_LINUX
  {
    struct timeval tv;
    struct timezone tz;
    if ( gettimeofday( &tv, &tz ) ) {
      return -1;
    }
    rawUST = ((MLint64)tv.tv_sec)*1e9 + ((MLint64)tv.tv_usec)*1e3;
  }
#endif

#ifdef ML_OS_NT
  {
    static LONGLONG PerfFreq = 0, PerfCount = 0;
    static MLint64 secsToNanos = 1000000000;
    MLint64 secs, remain;

    if (!PerfFreq ) {
      QueryPerformanceFrequency( (LARGE_INTEGER*)&PerfFreq );

      /* Check that our math won't overflow a 64-bit integer. See
       * below for details.
       */
      assert( PerfFreq < 18446744073 );
    }

    QueryPerformanceCounter( (LARGE_INTEGER*)&PerfCount );

    /* The performance counter frequency is expressed in ticks per
     * second -- but this can be quite high (eg: approx 1.7 * 10^9 on
     * my P4 system). So the performance counter values end up being
     * quite large as well. Thus, the naive approach to obtain a UST
     * in nanoseconds:
     *   nanos = (PerfCount * secsToNanos) / PerfFreq
     * will not work: it will overflow a 64-bit integer.
     *
     * To manage this, we start by computing the number of seconds
     * represented by the counter -- by dividing the counter by the
     * frequency, effectively rounding down to the nearest
     * second. Then we take the remainder of this division, which
     * gives us the number of ticks since the last second.
     */

    secs = PerfCount / PerfFreq;
    remain = PerfCount % PerfFreq;

    /* The remainder of the division is smaller than the number of
     * ticks per second (since it represents a time smaller than 1
     * second). We can safely multiply this by secsToNanos, as long
     * as:
     *
     * secsToNanos * PerfFreq < max value representable in 64 bits
     *
     * Concretely, this is true if:
     *      PerfFreq < (2^64 - 1) / 10^9
     * ie:  PerfFreq < 18446744073  (1.84 * 10^10)
     *
     * This should be fairly safe for a while... But eventually we may
     * need to review it. For now, the assert above will provide us
     * with the necessary warm fuzzy feeling.
     */

    /* Convert remainder (still expressed as counts) into nanoseconds,
     * according to counter frequency.
     */
    remain = (remain * secsToNanos) / PerfFreq;

    /* To get the full UST, simply add the number of seconds
     * (converted to nanos) to the number of nanos since the last
     * second.
     */
    rawUST = (secs * secsToNanos) + remain;
  }
#endif

  if ( USTBase == 0 ) {
    USTBase = rawUST;
    *UST = 1; /* Don't return 0, just in case someone thinks that's an error */
  } else {
    *UST = rawUST - USTBase;
  }

  DEBG4( printf( "[ustsource]: in ddGetUST, raw UST = %" FORMAT_LLD
		 ", base = %" FORMAT_LLD ", adjusted = %" FORMAT_LLD "\n",
		 rawUST, USTBase, *UST ) );

  return 0;
}


