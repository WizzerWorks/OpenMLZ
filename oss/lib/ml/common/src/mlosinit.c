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

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include <ML/ml_private.h>
#include "mldebug.h"

#ifndef ML_OS_NT
#include <dlfcn.h>
#endif


/* ============================================================================
 *
 * Forward references
 *
 * ==========================================================================*/

static MLstatus _mlOSDiscoverModules( MLsystemRec* pSystem );
static MLint32 _mlOSGetUSTinternal( MLint64* UST );

static MLint32 assertionFailed = 0;


/* ============================================================================
 *
 * Implementation
 *
 * ==========================================================================*/

void _mlOSDebugDrivers( char *s )
{
  /* A chance for debuggers to install breakpoints in drivers */
  mlDebug( "_mlOSDebugDrivers called from %s\n", s );
}

void
_mlOSErrPrintf( const char *format, ... )
{
  va_list args;
  va_start( args, format );
  vfprintf( stderr, (char *)format, args );
  va_end( args );
}

void _mlOSError( const char *msg )
{
  _mlOSErrPrintf( "mlSDK Error: %s", msg );
}


/* ------------------------------------------------------- _mlOSSelectUSTSource
 *
 * Select the "best" UST source from among all those registered by the
 * modules. The selection is done based on the metrics of update
 * period and maximum variation in sampling latency.
 *
 * The index of the selected source's module is written to the
 * supplied system rec, for later use by applications.
 *
 * This must be called *after* all modules have been interrogated.
 *
 * Note: it is possible to override the selection of the UST source by
 * setting the "_ML_UST_SOURCE" env var before running the daemon. In
 * this case, the UST source named by the env var will be chosen (if
 * it is found). The env var must use the name provided by the module
 * when registering the source (which is also the name that is
 * displayed by the mlquery utility)
 */
MLstatus _mlOSSelectUSTSource( MLsystemRec* pSystem )
{
  MLint32 i;
  MLstatus status = ML_STATUS_NO_ERROR;
  MLmoduleRec* selModule = NULL;
  MLint32 selModuleIdx = -1;
  char* srcOverride = getenv( "_ML_UST_SOURCE" );

  for ( i=0; i < pSystem->moduleCount; ++i ) {
    MLmoduleRec* pModule = pSystem->modules+i;

    /* If the current module doesn't have a UST source (if the update
     * period appears invalid), move on to the next one.
     */
    if ( pModule->ustUpdatePeriod <= 0 ) {
      continue;
    }

    /* Check if there is a selection override, and if so, does it
     * match the current source?
     */
    if ( srcOverride && !strcmp( srcOverride, pModule->ustName ) ) {
      /* Force this to be the selected source, regardless of metrics.
       */
      selModule = pModule;
      selModuleIdx = i;
      break;
    }

    /* If we haven't yet found a valid UST source, then this one is
     * the best one so far! Keep track of it and move on.
     */
    if ( selModuleIdx == -1 ) {
      selModule = pModule;
      selModuleIdx = i;
      continue;
    }

    /* We now have a choice between two sources, so we must select the
     * best one. First, check if this source is acceptable according
     * to these rules of thumb (from OpenML spec):
     *
     * - freq should be 5 times greater than highest media stream
     * freq.  Assume highest media is 96Khz audio, then UST should be
     * at 500Khz. So period should be no more than 2000 (nano-secs)
     *
     * - latency variation should be less than highest media
     * frequency. So this should be no more than 10000 (nano-secs).
     *
     * Anything else, and we reject right away.
     *
     * Note: the flaw with this is that the very first source we find
     * will not be rejected this way -- but it might in fact be even
     * worse than subsequent sources that we do reject. We could
     * improve the heuristic here.
     */
#define MAX_ACCEPTABLE_UPDATE_PERIOD 2000
#define MAX_ACCEPTABLE_LATENCY_VAR  10000
    if ( (pModule->ustUpdatePeriod > MAX_ACCEPTABLE_UPDATE_PERIOD) ||
	 (pModule->ustLatencyVar > MAX_ACCEPTABLE_LATENCY_VAR) ) {
      continue;
    }

    /* Source is acceptable, so compare it to the previous one --
     * simplest way is to compute the sum of the 2 metrics. The lowest
     * sum "wins".
     */
    if ( (pModule->ustUpdatePeriod + pModule->ustLatencyVar) <
	 (selModule->ustUpdatePeriod + selModule->ustLatencyVar) ) {
      selModule = pModule;
      selModuleIdx = i;
    }
  }

  /* Keep track of the index of the module with the best source. If no
   * source was registered, this will be '-1' (an invalid index), and
   * we will eventually fall back on the software-only UST source.
   */
  pSystem->ustSrcModuleIndex = selModuleIdx;

  if ( getenv( "MLOS_DEBUG" ) ) {
    if ( selModuleIdx == -1 ) {
      _mlOSErrPrintf( "ML: no UST source registered. Falling back "
		      "on software-only solution.\n" );
    } else {
      _mlOSErrPrintf( "ML: using UST source '%s' from module '%s' [%d]\n",
		      selModule->ustName, selModule->dsoname, selModuleIdx );
    }
  }

  return status;
}


/* -------------------------------------------------- _mlOSDiscoverLocalDevices
 *
 * Discover all the devices on the local system Accomplish this by
 * first discovering all the modules on this system, then ask each
 * driver to enumerate all the physical devices (boards) it can
 * control. Each physical device must be identified to the DI layer by
 * a call back to NewPhysicalDevice().
 */
MLstatus _mlOSDiscoverLocalDevices( MLsystemRec* pSystem )
{
  MLstatus status;
  MLint32 i;

  status = _mlOSDiscoverModules( pSystem );

  /* A chance for debuggers to install breakpoints in drivers: */
  _mlOSDebugDrivers( "_mlOSDiscoverLocalDevices" );

  if ( status != ML_STATUS_NO_ERROR ) {
    return status;
  }
	
  for ( i = 0; i < pSystem->moduleCount; ++i ) {
    MLmoduleRec* pModule = pSystem->modules+i;
    MLint32 before = pSystem->physicalDeviceCount;
      
    if ( !(pModule->interrogate) ) {
      status = ML_STATUS_INTERNAL_ERROR;
      return status;
    }

    status = (*(pModule->interrogate))(pSystem, i);
    if ( status != ML_STATUS_NO_ERROR ) {
      /* If the device added at least one physical device, then its a
       * fatal error if the discovery fails (the capabilities list may
       * have inconsistencies).
       */
      if ( pSystem->physicalDeviceCount - before > 0 ) {
	_mlOSErrPrintf( "[_mlOSDiscoverDevices] fatal error - "
			"device discovery failed for device: %s.\n",
			pModule->dsoname );
	return status;

      } else {
	/* not fatal - print warning and continue -- but make sure we
	 * don't consider the module as a source of UST
	 */
	pModule->ustUpdatePeriod = 0;
	_mlOSErrPrintf( "[_mlOSDiscoverDevices] warning - "
			"device discovery failed for: %s.\n",
			pModule->dsoname );
      }
    } /* if status != ML_STATUS_NO_ERROR */

    /* Could issue a warning here if the module added no devices and
     * no UST source (ie: seemingly useless module) -- but just
     * because a device module is present does not mean a physical
     * device is present, so there should be no warning
     */
  } /* for i=0..pSystem->moduleCount */
	  
  /* Now that all modules have been interrogated, we can select the
   * best UST source.
   */
  return _mlOSSelectUSTSource( pSystem );
}


/* ----------------------------------------------------- _mlOSInitUSTSourceFunc
 *
 * Open the module that was selected to provide UST for the system,
 * and obtain the address of the UST generating function. If no UST
 * source was registered, use the address of the built-in
 * software-only UST function.
 *
 * This must be called in the app, typically at the end of the
 * "_mlOSBootstrapSystem" function.
 */
MLstatus _mlOSInitUSTSourceFunc( MLsystemRec* pSystem )
{
  MLstatus status = ML_STATUS_NO_ERROR;

  if ( pSystem->ustSrcModuleIndex == -1 ) {
    /* No registered UST source, use built-in routine instead.
     * FIXME: should we issue a warning message?
     */
    pSystem->getUST = _mlOSGetUSTinternal;
    mlDebug( "[Bootstrap] using fall-back s/w UST source\n" );

  } else {
    MLmoduleRec* pModule = pSystem->modules + pSystem->ustSrcModuleIndex;
    mlDebug( "[Bootstrap] using UST source from module '%s' [%d]\n",
	     pModule->dsoname, pSystem->ustSrcModuleIndex );

    /* Remember, the module library has not yet been opened in this
     * process -- that normally happens in _mlOSBootstrapDevice(),
     * which is only called when a device is accessed by the app. So
     * we must first open the lib.
     */
#ifdef ML_OS_NT
    if ( (pModule->dsohandle = LoadLibrary( (const char*)pModule->dsopath ))
	 == NULL ) {
      _mlOSErrPrintf( "%s: %d\n",
		      (const char*)pModule->dsopath, GetLastError() );
      return ML_STATUS_INTERNAL_ERROR;
    }
    pSystem->getUST =
      (mlDDGetUSTPP)GetProcAddress( pModule->dsohandle, "ddGetUST" );
#else
    if ( (pModule->dsohandle = dlopen( (const char *)pModule->dsopath,
				       RTLD_LAZY) ) == NULL ) {
      _mlOSErrPrintf( "%s: %s\n", pModule->dsopath, dlerror() );
      return ML_STATUS_INTERNAL_ERROR;
    }
    pSystem->getUST = (mlDDGetUSTPP)dlsym( pModule->dsohandle, "ddGetUST" );
#endif

    if ( pSystem->getUST == NULL ) {
      _mlOSErrPrintf( "mlSDK Error: could not obtain UST function "
		      "'ddGetUST' from module '%s'\n", pModule->dsoname );
      status = ML_STATUS_INTERNAL_ERROR;
    }
  }

  return status;
}


/* ------------------------------------------------------- _mlOSBootstrapDevice
 */
MLstatus _mlOSBootstrapDevice( MLsystemRec* pSystem,
			       MLphysicalDeviceRec* pDevice )
{
  MLstatus status;

  MLmoduleRec* pModule = pSystem->modules + pDevice->moduleIndex;

  if ( pDevice->connected != 0 ) {
    /* Already connected, don't need to do anything. */
    return ML_STATUS_NO_ERROR;
  }

  /* No need to open the module, or search for the connect entry
   * point: that was all done in _mlOSDiscoverModules (the module info
   * can't be included in the MLsystemRec unless the lib was opened
   * and all entry points were found)
   */
  _mlOSDebugDrivers( "_mlOSBootstrapDevice" );
  if ( getenv( "MLOS_DEBUG" ) ) {
    _mlOSErrPrintf( "Connecting Module %s (%d) via %s to 0x%p "
		    "cookie 0x%p devpriv 0x%p\n",
		    pModule->dsoname,
		    pDevice->moduleIndex,
		    pModule->connectName,
		    pDevice,
		    pDevice->ddCookie,
		    &(pDevice->ddDevicePriv) );
  }

  /* Call ddConnect with the DD cookie and get back an ops pointer. */
  status = (pModule->connect)(pDevice->ddCookie, pDevice->id,
			      &(pDevice->ops), &(pDevice->ddDevicePriv));

  if ( status != ML_STATUS_NO_ERROR ) {
    return status;
  }
  pDevice->connected = 1;
  return ML_STATUS_NO_ERROR;
}


/* ------------------------------------------------------- _mlOSBootstrapSystem
 */
MLstatus _mlOSBootstrapSystem( MLsystemRec* pSystem )
{
  MLstatus status;

  if ( pSystem == NULL ) {
    return ML_STATUS_INTERNAL_ERROR;
  }

  if ( pSystem->id == 0 ) {
    MLsystemRec sysRec;

    /* If we are here then system has not been initialized so let's
     * clear everything before we begin, then init globals for the
     * local system.
     */
    _mlOSClearLocalSystem( &sysRec );

    status = _mlOSDiscoverLocalSystem( &sysRec );
    if ( status != ML_STATUS_NO_ERROR ) {
      fprintf( stderr,"discoverLocalSystem returned %s\n",
	       mlStatusName( status ) );
      return status;
    }

    status = _mlOSDiscoverLocalDevices( &sysRec );
    if ( status != ML_STATUS_NO_ERROR ) {
      fprintf( stderr,"discoverLocalDevices returned %s\n",
	       mlStatusName( status ) );
      return status;
    }

    status = _mlOSInitUSTSourceFunc( &sysRec );
    if ( status != ML_STATUS_NO_ERROR ) {
      fprintf( stderr,"initUSTSourceFunc returned %s\n",
	       mlStatusName( status ) );
      return status;
    }

    /* Initialize supplied system capabilities from the structure
     * returned by the 'discover' routines
     */
    *pSystem = sysRec;
  }

  return ML_STATUS_NO_ERROR;
}


/* ------------------------------------------------------ _mlOSClearLocalSystem
 */
MLstatus _mlOSClearLocalSystem( MLsystemRec* pSystem )
{
  memset( pSystem, 0, sizeof( MLsystemRec ) );

  pSystem->id = 0; /* signals unitialized */
  strcpy( (char*)pSystem->name, "Unitialized" );
  pSystem->moduleCount = 0;
  pSystem->physicalDeviceCount = 0;
  pSystem->openDeviceCount = 0;

  return ML_STATUS_NO_ERROR;
}


/* --------------------------------------------------- _mlOSDiscoverLocalSystem
 */
MLstatus _mlOSDiscoverLocalSystem( MLsystemRec* pSystem )
{
  char *s = (char*) pSystem->name;
#ifndef ML_OS_NT
  char *se = s + sizeof( pSystem->name );
#endif

  /* The following gets the 32-bit IP address for this host both the
   * IP address and ML_SYSTEM_LOCALHOST are accepted ... as synomyms
   * for "this system"
   */
#ifdef ML_OS_UNIX
  pSystem->id = mlDImakeSystemId( (MLuint32)gethostid() );
#else
  pSystem->id = mlDImakeSystemId( (MLuint32)ML_SYSTEM_LOCALHOST );
#endif

#ifdef ML_OS_NT
  {
    unsigned long length = sizeof( pSystem->name )-1;
    GetComputerName(s,&length);
    /* Is the domain name included?  if not, needs to be added */
  }
#else
  gethostname( s, (size_t)(se - s) );

  /* If domain name not already in hostname, then get it */
  if ( !strchr( s, '.' ) ) {
    s = strchr( s, '\0' );
    *s++ = '.';
    getdomainname( s, (int)(se - s) );
  }
#endif

  pSystem->moduleCount = 0;
  pSystem->physicalDeviceCount = 0;
  pSystem->openDeviceCount = 0;

  return ML_STATUS_NO_ERROR;
}


/* ============================================================================
 *
 * Windows NT Implementations
 *
 * ==========================================================================*/

#ifdef ML_OS_NT

MLstatus _mlOSInit( void )
{
  return ML_STATUS_NO_ERROR;
}

MLstatus _mlOSExiting( void )
{
  return ML_STATUS_NO_ERROR;
}


/* ----------------------------------------------------------------------------
 *
 * Simple, user mode, single process, lightweight locks intended to
 * avoid collision of errant application threads.
 */

MLstatus _mlOSNewLockLite( _mlOSLockableLite *pLock )
{
  InitializeCriticalSection( pLock ); /* No return value */
  return ML_STATUS_NO_ERROR;
}

MLstatus _mlOSFreeLockLite( _mlOSLockableLite *pLock )
{
  DeleteCriticalSection( pLock ); /* No return value */
  return ML_STATUS_NO_ERROR;
}

static
MLstatus __mlOSLockLiteBlock( _mlOSLockableLite *pLock )
{
  EnterCriticalSection( pLock ); /* No return value */
  return ML_STATUS_NO_ERROR;
}

MLstatus _mlOSLockLite( _mlOSLockableLite *pLock )
{
  if ( 0 == TryEnterCriticalSection( pLock ) ) {
#if defined(DEBUG) || defined(_DEBUG)
    /*
    _mlOSErrPrintf( "Detected thread collision. "
		    "Blocking until all clear.\n");
    */
#endif
    return __mlOSLockLiteBlock( pLock );
  }
  return ML_STATUS_NO_ERROR;
}

MLstatus _mlOSUnlockLite( _mlOSLockableLite *pLock )
{
  LeaveCriticalSection( pLock ); /* No return value */
  return ML_STATUS_NO_ERROR;
}


/* ----------------------------------------------------------------------------
 *
 * More general, user+kernel, multiprocess, heavyweight locks intended
 * to coordinate access to system wide global resources.
 *
 * N.B.: at present, this implementation is *not* adequate for
 * multiprocess protection!
 */

MLstatus _mlOSNewLock( _mlOSLockable *pLock )
{
#if defined(DEBUG) || defined(_DEBUG)
  if ( getenv( "MLOS_DEBUG" ) ) {
    _mlOSErrPrintf( "_mlOSNewLock() creating a lock *not* suitable "
		    "for multi-process synchronization!\n" );
  }
#endif
  InitializeCriticalSection( pLock ); /* No return value */
  return ML_STATUS_NO_ERROR;
}

MLstatus _mlOSFreeLock( _mlOSLockable *pLock )
{
  DeleteCriticalSection( pLock ); /* No return value */
  return ML_STATUS_NO_ERROR;
}

MLstatus _mlOSLock( _mlOSLockable *pLock )
{
  EnterCriticalSection( pLock ); /* No return value */
  return ML_STATUS_NO_ERROR;
}

MLstatus _mlOSUnlock( _mlOSLockable *pLock )
{
  LeaveCriticalSection( pLock ); /* No return value */
  return ML_STATUS_NO_ERROR;
}

MLstatus _mlOSSemaNew( _mlOSSema *pSema, MLint32 value )
{
  mlAssert( pSema );
  if ( pSema ==  NULL ) {
    return ML_STATUS_INTERNAL_ERROR;
  }

  /* Unnamed semaphore, *not* shared across process boundaries */
  *pSema = CreateSemaphore( NULL, value, INT_MAX, NULL );
  if ( *pSema ) {
    return ML_STATUS_NO_ERROR;
  } else {
    perror( "_mlOSSemaNew CreateSemaphore" );
    mlAssert( assertionFailed );
    return ML_STATUS_INTERNAL_ERROR;
  }
}

MLstatus _mlOSSemaFree( _mlOSSema *pSema )
{
  if ( CloseHandle( *pSema ) ) {
    return ML_STATUS_NO_ERROR;
  } else {
    perror( "_mlOSSemaFree CloseHandle" );
    mlAssert( assertionFailed );
    return ML_STATUS_INTERNAL_ERROR;
  }
}

MLstatus _mlOSSemaWait( _mlOSSema *pSema )
{
  if ( WaitForSingleObjectEx( *pSema, INFINITE, TRUE) == WAIT_OBJECT_0 ) {
    return ML_STATUS_NO_ERROR;
  } else {
    perror( "_mlOSSemaWait WaitForSingleObjectEx" );
    mlAssert( assertionFailed );
    return ML_STATUS_INTERNAL_ERROR;
  }
}

MLstatus _mlOSSemaTryWait( _mlOSSema *pSema )
{
  MLint32 ret = WaitForSingleObjectEx( *pSema, 0, TRUE );
  if ( ret == WAIT_TIMEOUT ) {
    return ML_STATUS_INSUFFICIENT_RESOURCES;
  } else if ( ret == WAIT_OBJECT_0 ) {
    return ML_STATUS_NO_ERROR;
  } else {
    perror( "_mlOSSemaTryWait WaitForSingleObjectEx" );
    mlAssert( assertionFailed );
    return ML_STATUS_INTERNAL_ERROR;
  }
}

MLstatus _mlOSSemaPost( _mlOSSema *pSema )
{
  if ( ReleaseSemaphore( *pSema, 1, NULL ) ) {
    return ML_STATUS_NO_ERROR;
  } else {
    perror( "_mlOSSemaPost ReleaseSemaphore" );
    mlAssert( assertionFailed );
    return ML_STATUS_INTERNAL_ERROR;
  }
}


/* ----------------------------------------------------------------------------
 *
 * this proto implementation is based on Linux implementation
 * _mlOSThreadCreate
 * _mlOSThreadPrepare
 * _mlOSThreadJoin
 * _mlOSThreadExit
 * _mlOSThreadReap
 */

MLstatus _mlOSThreadCreate( _mlOSThread *thread,
			    _mlOSThreadRetValue (*entry)(void *), void *arg )
{
  mlAssert( thread );
  if ( ! thread ) {
    return ML_STATUS_INTERNAL_ERROR;
  }

  *thread =
    (_mlOSThread)_beginthreadex( NULL, 0,
				 (_mlOSThreadRetValue (WINAPI*)(LPVOID)) entry,
				 (LPVOID)arg, 0, NULL );
  if ( *thread == NULL ) {
    perror( "_mlOSThreadCreate CreateThread" );
    mlAssert( assertionFailed );
    return ML_STATUS_INTERNAL_ERROR;
  } else {
    return ML_STATUS_NO_ERROR;
  }
}

MLstatus _mlOSThreadPrepare( _mlOSThread *thread )
{
  mlAssert( thread );
  if ( ! thread ) {
    return ML_STATUS_INTERNAL_ERROR;
  }

  return ML_STATUS_NO_ERROR;
}

MLstatus _mlOSThreadJoin( _mlOSThread *thread, MLint32 *exitState )
{
  mlAssert( thread );
  if ( ! thread ) {
    return ML_STATUS_INTERNAL_ERROR;
  }

  if ( WaitForSingleObjectEx( *thread, INFINITE, TRUE) == WAIT_OBJECT_0 ) {
    CloseHandle( *thread );
    return ML_STATUS_NO_ERROR;
  } else {
    perror( "_mlOSThreadJoin WaitForSingleObjectEx" );
    mlAssert( assertionFailed );
    return ML_STATUS_INTERNAL_ERROR;
  }
}

void _mlOSThreadExit( _mlOSThread *thread, MLstatus exitState )
{
  mlAssert( thread );
  if ( ! thread ) {
    return;
  }

  _endthreadex( (DWORD)exitState );
}

MLstatus _mlOSThreadReap( void )
{
  return ML_STATUS_NO_ERROR;
}


/* ----------------------------------------------------------------------------
 *
 * Following is a *semi-colon* delimited list
 */
static const char* MLDSOPATH = "openml\\mlmodule";


/* ------------------------------------------------------- _mlOSDiscoverModules
 *
 * This function (which manipulates per System global state) runs
 * under the lock obtained on the mlDISystemRecPtr in mlGetCapabilities().
 */
static MLstatus _mlOSDiscoverModules( MLsystemRec* pSystem )
{
  /* Scan a well known location for driver dll's. */
  char *paths;
  char *path;
  char *rootstr;
  char sysDir[512];
	
  pSystem->moduleCount = 0;

  /* strtok *modifies* its first argument, so copy! */
  paths = strdup( MLDSOPATH );

  rootstr = getenv( "_MLMODULE_ROOT" );
  if ( rootstr ) {
    strncpy( sysDir, rootstr, sizeof( sysDir ) );
  } else {
    if ( 0 == GetSystemDirectory( sysDir, sizeof( sysDir ) ) ) {
      _mlOSErrPrintf( "_mlOSDiscoverModules GetSystemDirectory failed: %d\n",
		      GetLastError() );
      return ML_STATUS_INTERNAL_ERROR;
    }
  }

  for ( path = strtok( paths, ";" ); path; path = strtok( NULL,";" ) ) {
    char mlmodpath[MAX_PATH];
    HANDLE hFind;
    WIN32_FIND_DATA FindData;
    MLboolean pressOn;

    sprintf( mlmodpath, "%s\\%s\\*.dll", sysDir, path );

    hFind = FindFirstFile( mlmodpath, &FindData );
    if ( hFind == INVALID_HANDLE_VALUE ) {
      _mlOSErrPrintf( "ML Error: failed to FindFirstFile %s\n", mlmodpath );
      return ML_STATUS_INTERNAL_ERROR;
    }
		
    for ( pressOn = ML_TRUE; pressOn;
	  pressOn = FindNextFile( hFind, &FindData ) ) {
      char dsopath[MAX_PATH];
      HANDLE dsohandle;
      MLmoduleRec newModule;
				
      sprintf( dsopath, "%s\\%s\\%s", sysDir, path, FindData.cFileName );

      if ( (dsohandle = LoadLibrary( dsopath )) == NULL ) {
	_mlOSErrPrintf( "%s: %d\n", dsopath, GetLastError() );
	continue;
      }

      newModule.interrogate =
	(mlDDInterrogatePP) GetProcAddress( dsohandle, "ddInterrogate" );
      newModule.connect =
	(mlDDConnectPP) GetProcAddress( dsohandle, "ddConnect" );

      if ( newModule.interrogate == NULL || newModule.connect == NULL) {
	_mlOSErrPrintf( "Missing driver entry address %s.\n", dsopath );
	(void) FreeLibrary( dsohandle );
	continue;
      }
	
      if ( getenv( "MLOS_DEBUG" ) ) {
	_mlOSErrPrintf( "Found driver entry address %s: %s\n",
			dsopath, "ddInterrogate" );
      }

      {
	/* filter out redundant mlmodules, 
	 * e.g. mlmmodule/foo.so mlmodule/debug/foo.so
	 */
	MLint32 i;
		
	for ( i = 0; i < pSystem->moduleCount; ++i ) {
	  if ( 0 == strcmp(FindData.cFileName, pSystem->modules[i].dsoname) ) {
	    _mlOSErrPrintf( "Ignoring redundant mlmodule %s\n", dsopath );
	    newModule.interrogate = NULL;
	    break;
	  }
	}

	if ( newModule.interrogate == NULL ) {
	  (void) FreeLibrary( dsohandle );
	  continue;
	}
      } /* filter-out scope */
				
      strcpy( newModule.dsopath, dsopath );
      strcpy( newModule.dsoname, FindData.cFileName );
      newModule.dsohandle = dsohandle;
      newModule.ustUpdatePeriod = 0; /* initial (invalid) setting */
      newModule.ustLatencyVar = -1;

      if ( pSystem->moduleCount < ML_MAX_MODULES -1 ) {
	pSystem->modules[pSystem->moduleCount] = newModule;
	pSystem->moduleCount++;
      } else {
	_mlOSErrPrintf( "WARNING - max module count exceeded!\n" );
      }

    } /* for pressOn ... */
    FindClose( hFind );
  } /* for path = strtok... */

  if ( paths ) {
    free( paths ); /* free() (rather than mlFree) to match malloc in strdup. */
  }

  return ML_STATUS_NO_ERROR;
}

#endif /* ML_OS_NT */


/* ============================================================================
 *
 * UN*X Implementations
 *
 * ==========================================================================*/

#ifdef ML_OS_UNIX

#include <stdlib.h>
#include <sys/types.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>


/* Simple, user mode, single process, lightweight locks intended to
 * avoid collision of errant application threads.
 */

MLstatus _mlOSNewLockLite( _mlOSLockableLite *pLock )
{
  mlAssert( pLock );
  if ( pLock == NULL ) {
    return ML_STATUS_INTERNAL_ERROR;
  }

  /* Unnamed semaphore, *not* shared across process boundaries */
  if ( sem_init( pLock, 0, 1 ) ) {
    perror( "sem_init" );
    mlAssert( assertionFailed );
    return ML_STATUS_INTERNAL_ERROR;
  }

  return ML_STATUS_NO_ERROR;
}

MLstatus _mlOSFreeLockLite( _mlOSLockableLite *pLock )
{
  if ( sem_destroy( pLock ) ) {
    perror( "sem_destroy" );
    mlAssert( assertionFailed );
    return ML_STATUS_INTERNAL_ERROR;
  }
  return ML_STATUS_NO_ERROR;
}

static
MLstatus __mlOSLockLiteBlock( _mlOSLockableLite *pLock )
{
  for (;;) {
    if ( sem_wait( pLock ) ) {
      if ( oserror() == EINTR ) {
        continue;
      } else {
        perror( "sem_wait" );
        mlAssert( assertionFailed );
        return ML_STATUS_INTERNAL_ERROR;
      }
    } else {
      break;
    }
  }
  return ML_STATUS_NO_ERROR;
}

MLstatus _mlOSLockLite( _mlOSLockableLite *pLock )
{
  for (;;) {
    if ( sem_trywait( pLock ) ) {
      int err = oserror();
      if ( err == EINTR ) {
        continue;
      } else if ( err == EAGAIN ) {
#if defined(DEBUG) || defined(_DEBUG)
	/*
	_mlOSErrPrintf( "Detected thread collision. "
			"Blocking until all clear.\n" );
	*/
#endif
        return __mlOSLockLiteBlock( pLock );
      } else {
        perror( "sem_wait" );
        mlAssert( assertionFailed );
        return ML_STATUS_INTERNAL_ERROR;
      }
    } else {
      break;
    }
  }
  return ML_STATUS_NO_ERROR;
}

MLstatus _mlOSUnlockLite( _mlOSLockableLite *pLock )
{
  if ( sem_post( pLock ) ) {
    perror( "sem_post" );
    mlAssert( assertionFailed );
    return ML_STATUS_INTERNAL_ERROR;
  }
  return ML_STATUS_NO_ERROR;
}


/* ----------------------------------------------------------------------------
 *
 * More general, user+kernel, multiprocess, heavyweight locks intended
 * to coordinate access to system wide global resources.
 *
 * N.B. at present, this implementation is *not* adequate for
 * multiprocess protection!
 */

MLstatus _mlOSNewLock( _mlOSLockable *pLock )
{
#if defined(DEBUG) || defined(_DEBUG)
  if ( getenv( "MLOS_DEBUG" ) ) {
    _mlOSErrPrintf( "_mlOSNewLock() creating a lock *not* suitable"
		    " for multi-process synchronization!\n" );
  }
#endif
  mlAssert( pLock );
  if ( pLock == NULL ) {
    return ML_STATUS_INTERNAL_ERROR;
  }

#ifdef ML_OS_IRIX
  /* XXX If pLock is known to reside in shared memory, then this
   * XXX unnamed semaphore, which is create "shared", may be suitable
   * XXX for locking global resources.
   */
  if (sem_init(pLock, 1, 1)) {
    perror("sem_init");
    mlAssert(assertionFailed);
    return ML_STATUS_INTERNAL_ERROR;
  }
#endif
#ifdef ML_OS_LINUX
  /* XXX Linux does not implement process-sharable semas */
  if ( sem_init( pLock, 0, 1 ) ) {
    perror( "sem_init" );
    mlAssert( assertionFailed );
    return ML_STATUS_INTERNAL_ERROR;
  }
#endif

  return ML_STATUS_NO_ERROR;
}

MLstatus _mlOSFreeLock( _mlOSLockable *pLock )
{
  if ( sem_destroy( pLock ) ) {
    perror( "sem_destroy" );
    mlAssert( assertionFailed );
    return ML_STATUS_INTERNAL_ERROR;
  }
  return ML_STATUS_NO_ERROR;
}

MLstatus _mlOSLock( _mlOSLockable *pLock )
{
  for (;;) {
    if ( sem_wait( pLock ) ) {
      if ( oserror() == EINTR ) {
        continue;
      } else {
        perror( "sem_wait" );
        mlAssert( assertionFailed );
        return ML_STATUS_INTERNAL_ERROR;
      }
    } else {
      break;
    }
  }
  return ML_STATUS_NO_ERROR;
}

MLstatus _mlOSUnlock( _mlOSLockable *pLock )
{
  if ( sem_post( pLock ) ) {
    perror( "sem_post" );
    mlAssert( assertionFailed );
    return ML_STATUS_INTERNAL_ERROR;
  }
  return ML_STATUS_NO_ERROR;
}

MLstatus _mlOSSemaNew( _mlOSSema *pSema, MLint32 value )
{
  mlAssert( pSema );
  if ( pSema== NULL ) {
    return ML_STATUS_INTERNAL_ERROR;
  }

  /* Unnamed semaphore, *not* shared across process boundaries */
  if ( sem_init( pSema, 0, value ) ) {
    perror( "sem_init" );
    mlAssert( assertionFailed );
    return ML_STATUS_INTERNAL_ERROR;
  }
#ifdef ML_OS_IRIX /* Bug 763586 */
  /* Ensure that sema has zero resources recorded */
  for (;;) {
    if ( sem_trywait( pSema ) ) {
      int err = oserror();
      if ( err == EINTR ) {
        continue;
      } else if ( err == EAGAIN ) {
        break; /* sema is now at zero */
      } else {
        perror( "mlOSSemaNew sem_trywait" );
        return ML_STATUS_INTERNAL_ERROR;
      }
    } else {
      continue; /* reach here having decremented this sema! */
    }
  }
#endif

  return ML_STATUS_NO_ERROR;
}

MLstatus _mlOSSemaFree( _mlOSSema *pSema )
{
  if ( sem_destroy( pSema ) ) {
    perror( "sem_destroy" );
    mlAssert( assertionFailed );
    return ML_STATUS_INTERNAL_ERROR;
  }
  return ML_STATUS_NO_ERROR;
}

MLstatus _mlOSSemaWait( _mlOSSema *pSema )
{
  for (;;) {
    if ( sem_wait( pSema ) ) {
      if ( oserror() == EINTR ) {
        continue;
      } else {
        perror( "sem_wait" );
        mlAssert( assertionFailed );
        return ML_STATUS_INTERNAL_ERROR;
      }
    } else {
      break;
    }
  }
  return ML_STATUS_NO_ERROR;
}

MLstatus _mlOSSemaTryWait( _mlOSSema *pSema )
{
#ifndef ML_OS_LINUX /* XXX GNU-Linux sem_trywait() BLOCKS! XXX */
  for (;;) {
    if ( sem_trywait( pSema ) ) {
      int err = oserror();
      if ( err == EINTR ) {
        continue;
      } else if ( err == EAGAIN ) {
        return ML_STATUS_INSUFFICIENT_RESOURCES;
      } else {
        perror( "sem_wait" );
        mlAssert( assertionFailed );
        return ML_STATUS_INTERNAL_ERROR;
      }
    } else {
      break;
    }
  }
#endif
  return ML_STATUS_NO_ERROR;
}

MLstatus _mlOSSemaPost( _mlOSSema *pSema )
{
  if ( sem_post( pSema ) ) {
    perror( "sem_post" );
    mlAssert( assertionFailed );
    return ML_STATUS_INTERNAL_ERROR;
  }
  return ML_STATUS_NO_ERROR;
}


/* ============================================================================
 *
 * IRIX Implementations
 *
 * ==========================================================================*/

#ifdef ML_OS_IRIX
#include <mplib.h>
#include <ulocks.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>

MLstatus _mlOSInit( void )
{
  if ( _mplib_get_thread_type() == MT_PTHREAD ) {
  } else {
    int maxcpus, users, wantusers, err;

    maxcpus = (int) prctl( PR_MAXPPROCS );
    if ( maxcpus < 0 ) {
      perror( "_mlOSInit prctl(PR_MAXPPROCS)" );
      return ML_STATUS_INTERNAL_ERROR;
    }

    users = (int) usconfig( CONF_INITUSERS, 8 );
    if ( users < 0 ) {
      perror( "_mlOSInit get usconfig" );
      return ML_STATUS_INTERNAL_ERROR;
    }

    /* Main thread, queue service thread, worker threads */
    wantusers = MAX( users, maxcpus + 2 );

    err = (int) usconfig( CONF_INITUSERS, wantusers );
    if ( err < 0 ) {
      perror( "_mlOSInit set usconfig" );
      return ML_STATUS_INTERNAL_ERROR;
    }
  }

  return ML_STATUS_NO_ERROR;
}

MLstatus _mlOSExiting( void )
{
  return ML_STATUS_NO_ERROR;
}

static MLboolean _mlOSUsePthreads( void )
{
  if ( _mplib_get_thread_type() == MT_PTHREAD ||
       getenv( "MLOS_USE_PTHREADS" ) ) {
    return ML_TRUE;
  }
  return ML_FALSE;
}

MLstatus _mlOSThreadCreate( _mlOSThread *thread, void *(*entry)(void *),
			    void *arg )
{
  mlAssert( thread );
  if ( ! thread ) {
    return ML_STATUS_INTERNAL_ERROR;
  }

  if ( _mlOSUsePthreads() ) {
    int stat = pthread_create( &(thread->pthread), NULL, entry, arg );
    if ( getenv("MLOS_DEBUG") ) {
      _mlOSErrPrintf( "[_mlOSThreadCreate] creating pthread: %s (%d,%d)\n",
		      strerror(stat), stat, _mplib_get_thread_type() );
    }
    if ( stat ) {
      if ( stat == ENOSYS ) {
        fprintf( stderr, "mlOSThreadCreate pthread lib not loaded? %s\n",
		 strerror( stat ) );
        return ML_STATUS_INTERNAL_ERROR;
      } else if ( stat == EAGAIN ) {
        fprintf( stderr, "mlOSThreadCreate exhausted pthread_create: %s\n",
		 strerror( stat ) );
        return ML_STATUS_INSUFFICIENT_RESOURCES;
      } else {
        fprintf( stderr, "mlOSThreadCreate pthread_create: %s\n",
		 strerror( stat ) );
        return ML_STATUS_INTERNAL_ERROR;
      }
    } else {
      return ML_STATUS_NO_ERROR;
    }
  } else {
    int stat;
    stat = sem_init( &(thread->join_sema), 0, 0 );
    if ( stat == -1 ) {
      perror( "mlOSThreadCreate sem_init" );
      return ML_STATUS_INTERNAL_ERROR;
    }

#ifdef ML_OS_IRIX /* Bug 763586 */
    /* Ensure that sema has zero resources recorded */
    for (;;) {
      if (sem_trywait(&(thread->join_sema))) {
        int err = oserror();
        if (err == EINTR)
          continue;
        else if (err == EAGAIN)
          break; /* sema is now at zero */
        else {
          perror("mlOSThreadCreate sem_trywait");
          return ML_STATUS_INTERNAL_ERROR;
        }
      } else
        continue; /* reach here having decremented this sema! */
    }
#endif /* ML_OS_IRIX */

    thread->pid = sproc( (void (*)(void *))entry, PR_SADDR | PR_SFDS, arg );
    if ( getenv( "MLOS_DEBUG" ) ) {
      _mlOSErrPrintf( "[_mlOSThreadCreate] sproc pid: %d (%s)\n",
		      thread->pid,
		      thread->pid == -1? strerror(oserror()): "ok" );
    }
    if ( thread->pid == -1 ) {
      int err = oserror();
      if (err == EAGAIN ||
          err == ENOMEM ||
          err == ENOLCK ||
          err == EACCES ||
          err == ENOSPC) {
        _mlOSErrPrintf( "mlOSThreadCreate sproc oserror() is %d\n", err );
        perror( "mlOSThreadCreate exhausted sproc" );
        return ML_STATUS_INSUFFICIENT_RESOURCES;
      } else {
        _mlOSErrPrintf( "mlOSThreadCreate sproc oserror() is %d\n", err );
        perror( "mlOSThreadCreate sproc" );
        return ML_STATUS_INTERNAL_ERROR;
      }
    } 
    return ML_STATUS_NO_ERROR;
  } /* if _mlOSUsePthreads() ... else ... */
}

MLstatus _mlOSThreadPrepare( _mlOSThread *thread )
{
  mlAssert( thread );
  if ( ! thread ) {
    return ML_STATUS_INTERNAL_ERROR;
  }

  if ( _mlOSUsePthreads() ) {
    /* Establish self identity in case this child runs before parent
     * completes assignement in pthread_create
     */
    thread->pthread = pthread_self();
  } else {
    /* Establish self identity in case this child runs before parent
     * completes assignement after sproc
     */
    thread->pid = getpid();
    /* set up hang up handler */
    signal( SIGHUP, SIG_DFL );
    /* get hangup signal when parent terminates */
    prctl( PR_TERMCHILD );
  }
  return ML_STATUS_NO_ERROR;
}

MLstatus _mlOSThreadJoin( _mlOSThread *thread, MLint32 *exitState )
{
  mlAssert( thread );
  if ( ! thread ) {
    return ML_STATUS_INTERNAL_ERROR;
  }

  if ( _mlOSUsePthreads() ) {
    static int (*_real_pthread_join)(pthread_t, void **) = NULL;
    int err;

    if ( ! _real_pthread_join ) {
      void *handle;
      handle = dlopen( NULL, RTLD_LAZY|RTLD_LOCAL );
      _real_pthread_join =
	( int (*)(pthread_t, void **) ) dlsym( handle, "pthread_join" );
    }
    assert( _real_pthread_join != NULL );
    err = _real_pthread_join( thread->pthread, (void **)exitState );
    if ( err != 0 && err != ESRCH ) {
      return ML_STATUS_INTERNAL_ERROR;
    } else {
      return ML_STATUS_NO_ERROR;
    }
  } else {
    if ( getenv( "MLOS_DEBUG" ) ) {
      _mlOSErrPrintf("[_mlOSThreadJoin] kill pid: %d\n", thread->pid);
    }
    if ( kill( thread->pid, 0 ) == -1 ) {
      if ( oserror() != ESRCH ) {
        perror( "_mlOSThreadJoin kill" );
        return ML_STATUS_INTERNAL_ERROR;
      }
      /* Fall through when pid has terminated, don't bother with the
       * sem_wait(). Its safer this way, the process may have
       * terminated without post-ing the join_sema
       */
    } else {
      for (;;) {
        if ( sem_wait( &(thread->join_sema) ) ) {
          if ( oserror() == EINTR ) {
            continue;
	  }
          perror( "mlOSThreadJoin sem_wait" );
          return ML_STATUS_INTERNAL_ERROR;
        } else {
          break;
	}
      }
    }
    sem_destroy( &(thread->join_sema) );
    return ML_STATUS_NO_ERROR;
  }
}

void _mlOSThreadExit( _mlOSThread *thread, MLstatus exitState )
{
  mlAssert( thread );
  if ( ! thread ) {
    return;
  }

  if ( _mlOSUsePthreads() ) {
    /* Cast to size_t quiets remark 1413 */
    pthread_exit( (void *)((MLsize_t) exitState) );
  } else {
    int stat = sem_post( &(thread->join_sema) );
    if ( stat == -1 ) {
      perror( "mlOSThreadExit sem_post" );
    }
    exit( (int)exitState );
  }
}

MLstatus _mlOSThreadReap( void )
{
  if ( _mlOSUsePthreads() ) {
  } else {
    waitpid( -1, NULL, WNOHANG );
  }
  return ML_STATUS_NO_ERROR;
}

#endif /* ML_OS_IRIX */


/* ============================================================================
 *
 * LINUX Implementations
 *
 * ==========================================================================*/

#ifdef ML_OS_LINUX
#include <pthread.h>

MLstatus _mlOSInit( void )
{
  return ML_STATUS_NO_ERROR;
}

MLstatus _mlOSExiting( void )
{
  return ML_STATUS_NO_ERROR;
}

MLstatus _mlOSThreadCreate( _mlOSThread *thread,
			    void *(*entry)(void *), void *arg )
{
  int stat;

  mlAssert( thread );
  if ( ! thread ) {
    return ML_STATUS_INTERNAL_ERROR;
  }

  stat = pthread_create( &(thread->pthread), NULL, entry, arg );
  if ( stat ) {
    if ( oserror() == EAGAIN ) {
      perror( "mlOSThreadCreate exhausted pthread_create" );
      return ML_STATUS_INSUFFICIENT_RESOURCES;
    } else {
      perror( "mlOSThreadCreate pthread_create" );
      return ML_STATUS_INTERNAL_ERROR;
    }
  } else {
    return ML_STATUS_NO_ERROR;
  }
}

MLstatus _mlOSThreadPrepare( _mlOSThread *thread )
{
  mlAssert( thread );
  if ( ! thread ) {
    return ML_STATUS_INTERNAL_ERROR;
  }

  /* Establish self identity in case this child runs before parent
   * completes assignement in pthread_create
   */
  thread->pthread = pthread_self();

  return ML_STATUS_NO_ERROR;
}

MLstatus _mlOSThreadJoin( _mlOSThread *thread, MLint32 *exitState )
{
  int err;

  mlAssert( thread );
  if ( ! thread ) {
    return ML_STATUS_INTERNAL_ERROR;
  }

  err = pthread_join( thread->pthread, (void **)exitState );
  if ( err != 0 && err != ESRCH ) {
    return ML_STATUS_INTERNAL_ERROR;
  } else {
    return ML_STATUS_NO_ERROR;
  }
}

void _mlOSThreadExit( _mlOSThread *thread, MLstatus exitState )
{
  mlAssert( thread );
  if ( ! thread ) {
    return;
  }

#ifndef _LP64
  pthread_exit( (void *)exitState );
#else
  pthread_exit( (void *)(MLint64) exitState );
#endif
}

MLstatus _mlOSThreadReap( void )
{
  return ML_STATUS_NO_ERROR;
}

#endif /* ML_OS_LINUX */


/* ----------------------------------------------------------------------------
 *
 * Following is a *colon* delimited list
 */
#ifdef ML_OS_IRIX
#if (_MIPS_SIM == _MIPS_SIM_ABI64)
static const char* MLDSOPATH = \
	"/usr/lib64/mlmodule";

#elif (_MIPS_SIM == _MIPS_SIM_NABI32)
static const char* MLDSOPATH = \
	"/usr/lib32/mlmodule";

#else	/* O32 is actually obsolete */
static const char* MLDSOPATH = \
	"/usr/lib/mlmodule";
#endif /* ABI CHECK */
#endif /* IRIX */

#ifdef ML_OS_LINUX
static const char* MLDSOPATH = \
	"/usr/lib/mlmodule";
#endif


/* ------------------------------------------------------- _mlOSDiscoverModules
 *
 * This function (which manipulates per System global state) runs
 * under the lock obtained on the mlDISystemRecPtr in mlGetCapabilities().
 */
static MLstatus _mlOSDiscoverModules( MLsystemRec* pSystem )
{
  /* Scan a well known location (/usr/lib{,32,64}/mlmodule) for driver
   * dso's.
   */
  char *paths;
  char *path;
  char *rootstr;
  char *debugstr;
  char *purestr = "";
	
  pSystem->moduleCount = 0;
  /* strtok *modifies* its first argument, so copy! */
  paths = strdup( MLDSOPATH );
 
  rootstr = getenv( "_MLMODULE_ROOT" );
  if ( ! rootstr ) {
    rootstr = "";
  }

  if ( getenv( "MLMODULE_DEBUG" ) != NULL ) {
    if ( getenv( "MLOS_DEBUG" ) ) {
      _mlOSErrPrintf( "[ml module debugging on]\n" );
    }
    debugstr = "/debug";
  } else {
    debugstr = "";
  }

  {
    char *s;
    if ( (s = getenv( "MLMODULE_PURIFY" )) ) {
      purestr = s;
    }
  }

  for ( path = strtok( paths, ":" ); path; path = strtok( NULL, ":" ) ) {
    char mlmodpath[PATH_MAX];
    DIR *dirp;
    struct direct *dp;

    sprintf( mlmodpath, "%s%s%s", rootstr, path, debugstr );
    if ( getenv( "MLOS_DEBUG" ) ) {
      _mlOSErrPrintf("[_mlOSDiscoverModules] module directory: %s\n",
		     mlmodpath );
    }

    if ( (dirp = opendir( mlmodpath )) == NULL ) {
      _mlOSErrPrintf( "ML Error: failed to open directory %s\n", mlmodpath );
      return ML_STATUS_INTERNAL_ERROR;
    }
		
    for ( dp = readdir( dirp ); dp; dp = readdir( dirp ) ) {
      char dsopath[PATH_MAX];
      char interrogateName[PATH_MAX];
      char connectName[PATH_MAX];
      struct stat buf;
      void *dsohandle;   
      unsigned long  d_namlen = strlen( dp->d_name );
      MLmoduleRec newModule;
      int found_generic = 0;

      memset( &newModule, 0, sizeof( newModule ) );

      if ( (d_namlen <= 0) || 
	   (dp->d_name[0] == '.') ||
	   (0 != strcmp(&(dp->d_name[d_namlen-3]),".so")) ) {
	continue;
      }

      sprintf( dsopath, "%s/%s%s", mlmodpath, dp->d_name, purestr );

      if ( stat( (const char *) dsopath, &buf) ) {
	if ( getenv( "MLOS_DEBUG" ) ) {
	  _mlOSErrPrintf( "mlOSDiscoverDrivers failed to stat %s: ", dsopath );
	  perror( "" );
	  _mlOSErrPrintf( "Ignoring mlmodule %s\n", dsopath );
	}
	continue;
      }

      if ( (dsohandle = dlopen( (const char *) dsopath, RTLD_LAZY ) )
	   == NULL ) {
	_mlOSErrPrintf( "%s: %s\n", dsopath, dlerror() );
	continue;
      }

      /* Try using module prefix for symbol names first */
      { 
	char *d = dsopath, *s;
	strcpy( interrogateName, ( s = strrchr( d, '/' ))? s + 1 : d );
	strcpy( connectName,     ( s = strrchr( d, '/' ))? s + 1 : d );
      }
      *strchr( interrogateName, '.' ) = '\0';
      *strchr( connectName, '.' ) = '\0';
      strcat( interrogateName, "Interrogate" );
      strcat( connectName, "Connect" );
      newModule.interrogate =
	(mlDDInterrogatePP) dlsym( dsohandle, interrogateName );
      newModule.connect =
	(mlDDConnectPP) dlsym( dsohandle, connectName );

      /* If that didn't work, then try the generic names */
      if ( newModule.interrogate == NULL || newModule.connect == NULL) {
	found_generic = 1;
	newModule.interrogate =
	  (mlDDInterrogatePP) dlsym( dsohandle, "ddInterrogate" );
	newModule.connect = (mlDDConnectPP) dlsym( dsohandle, "ddConnect" );
      }

      if ( newModule.interrogate == NULL || newModule.connect == NULL) {
	_mlOSErrPrintf( "Missing driver entry address %s: %s, %s, %s\n", 
			dsopath, interrogateName, connectName,
			"ddInterrogate or ddConnect" );

	(void) dlclose( dsohandle );
	continue;

      } else {
	if ( getenv( "MLOS_DEBUG" ) ) {
	  _mlOSErrPrintf( "Found driver entry address %s: %s\n", 
			  dsopath,
			  found_generic? "ddInterrogate" : interrogateName );
	}

	{
	  /* filter out redundant mlmodules, 
	   * e.g. mlmmodule/foo.so mlmodule/debug/foo.so
	   */
	  MLint32 i;

	  for ( i = 0; i < pSystem->moduleCount; ++i ) {
	    if ( 0 == strcmp( dp->d_name, pSystem->modules[i].dsoname ) ) {
	      _mlOSErrPrintf( "Ignoring redundant mlmodule %s\n", dsopath );
	      newModule.interrogate = NULL;
	      break;
	    }
	  }
	  if ( newModule.interrogate == NULL ) {
	    (void) dlclose( dsohandle );
	    continue;
	  }
	} /* filter-out scope */

#ifdef ML_OS_LINUX /* XXX Linux workaround!?! XXX */
	{
	  MLint32 i;

	  for ( i = 0; i < pSystem->moduleCount; ++i ) {
	    if ( newModule.interrogate == pSystem->modules[i].interrogate ) {
	      _mlOSErrPrintf( "Duplicate driver entry address %s: %s\n", 
			      dsopath, 
			      found_generic? "ddInterrogate":interrogateName );
	      newModule.interrogate = NULL;
	      break;
	    }
	  }
	  if ( newModule.interrogate == NULL ) {
	    (void) dlclose( dsohandle );
	    continue;
	  }
	}
#endif /* ML_OS_LINUX */
			
	newModule.dsohandle = dsohandle;
	strcpy( newModule.dsopath, dsopath );
	strcpy( newModule.dsoname, dp->d_name );
	strcpy( newModule.connectName,
		found_generic? "ddConnect" : connectName );
	newModule.ustUpdatePeriod = 0; /* initial (invalid) setting */
	newModule.ustLatencyVar = -1;

	if ( pSystem->moduleCount < ML_MAX_MODULES -1 ) {
	  pSystem->modules[pSystem->moduleCount] = newModule;
	  pSystem->moduleCount++;
	  if ( getenv( "MLOS_DEBUG" ) ) {
	    _mlOSErrPrintf( "Driver Module %d at 0x%p\n",
			    pSystem->moduleCount,
			    &pSystem->modules[pSystem->moduleCount] );
	  }
	} else {
	  _mlOSErrPrintf( "WARNING - max module count exceeded!\n");
	}
      }
    } /* for dp = readdir() ... */
    closedir( dirp );
  }

  if ( paths ) {
    free( paths ); /* free() (rather than mlFree) to match malloc in strdup. */
  }

  return ML_STATUS_NO_ERROR;
}

#endif /* ML_OS_UNIX */


/* -------------------------------------------------------- _mlOSGetUSTinternal
 */
static MLint32 _mlOSGetUSTinternal( MLint64* ust )
{
#ifdef ML_OS_IRIX
#include <sys/syssgi.h>
  if ( syssgi( SGI_GET_UST, ust, 0 ) == 0 ) {
    return 0;
  } else {
    return -1;
  }
#endif

#ifdef ML_OS_LINUX
#include <sys/time.h>

  struct timeval tv;
  struct timezone tz;
  if ( gettimeofday( &tv, &tz ) ) {
    return -1;
  }
  *ust = ((MLint64) tv.tv_sec) * 1e9 + ((MLint64) tv.tv_usec) * 1e3;
  return 0;
#endif

#ifdef ML_OS_NT
  static LONGLONG PerfFreq = 0, PerfCount = 0;
  static MLint64 secsToNanos = 1000000000;
  MLint64 secs, remain;

  if ( !PerfFreq ) {
    QueryPerformanceFrequency( (LARGE_INTEGER*)&PerfFreq );

    /* Check that our math won't overflow a 64-bit integer. See below
     * for details.
     */
    assert( PerfFreq < 18446744073 );
  }

  QueryPerformanceCounter( (LARGE_INTEGER*)&PerfCount );

  /* The performance counter frequency is expressed in ticks per
   * second -- but this can be quite high (eg: approx 1.7 * 10^9 on
   * my P4 system). So the performance counter values end up being
   * quite large as well. Thus, the naive approach to obtain a UST in
   * nanoseconds:
   *   nanos = (PerfCount * secsToNanos) / PerfFreq
   * will not work: it will overflow a 64-bit integer.
   *
   * To manage this, we start by computing the number of seconds
   * represented by the counter -- by dividing the counter by the
   * frequency, effectively rounding down to the nearest second. Then
   * we take the remainder of this division, which gives us the number
   * of ticks since the last second.
   */
  secs = PerfCount / PerfFreq;
  remain = PerfCount % PerfFreq;

  /* The remainder of the division is smaller than the number of ticks
   * per second (since it represents a time smaller than 1 second). We
   * can safely multiply this by secsToNanos, as long as:
   *
   * secsToNanos * PerfFreq < max value representable in 64 bits
   *
   * Concretely, this is true if:
   *      PerfFreq < (2^64 - 1) / 10^9
   * ie:  PerfFreq < 18446744073  (1.84 * 10^10)
   *
   * This should be fairly safe for a while... But eventually we may
   * need to review it. For now, the assert above will provide us with
   * the necessary warm fuzzy feeling.
   */

  /* Convert remainder (still expressed as counts) into nanoseconds,
   * according to counter frequency.
   */
  remain = (remain * secsToNanos) / PerfFreq;

  /* To get the full UST, simply add the number of seconds (converted
   * to nanos) to the number of nanos since the last second.
   */
  *ust = (secs * secsToNanos) + remain;

  return 0;
#endif
}


/* ---------------------------------------------------------------- _mlOSGetUST
 *
 * This function should never be called. But old modules may still be
 * using it, so we can't just remove it. Instead, issue a warning
 * message and call the new function.
 *
 * The correct thing to do for a module is to obtain a function
 * address using mlDIGetUSTSource().
 */
MLint32 _mlOSGetUST( MLint64* ust )
{
  _mlOSError( "_mlOSGetUST called. Should obtain func ptr using "
	      "mlDIGetUSTSource() instead\n" );
  return _mlOSGetUSTinternal( ust );
}

