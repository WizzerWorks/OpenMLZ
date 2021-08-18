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

#include <string.h>

#include <ML/ml_private.h>

#ifndef ML_OS_NT
#include <dlfcn.h>
#endif

#ifdef ML_OS_LINUX
/* Linux dlopen() will call _init() before returning. */
#ifdef ML_ARCH_IA64
void oinit( void )  __attribute__ ((constructor)); /* XXXjaya tbcleanup */

void oinit( void )
{
  mlDIGenesis();
}
#else 
void _init( void )
{
  mlDIGenesis();
}
#endif
#endif

#if defined(ML_OS_LINUX) && defined(ML_ARCH_IA64)
#include <string.h> /* for strcpy and memcpy */
#endif


/* -----------------------------------------------------------------mlDIGenesis
 *
 * This function begins the life of the mlSDK in a process address
 * space. It must be called before any other routine in the mlSDK, and
 * it must be called just once in the life of the address space (not
 * once per thread!). These constraints can be met by having OS
 * dependent mechanisms call here immediately upon load of the mlSDK
 * .dll or .dso.
 * [e.g. dllmain.c on NT, and the "-init" arg to "ld" on IRIX.]
 */
void mlDIGenesis( void )
{
  MLstatus status;

  status = _mlInit(); /* For early, non-OS specific init */
  if ( status != ML_STATUS_NO_ERROR ) {
    _mlOSError( "Failed OS-independent initialization\n" );
    return; 
  }

  status = _mlOSInit(); /* For early, OS specific, initializations. */
  if ( status != ML_STATUS_NO_ERROR ) {
    _mlOSError( "Failed OS-dependent initialization\n" );
    return; 
  }

#ifdef ML_OS_UNIX
  atexit( mlDITearDown );
  /* On NT, mlDITearDown is called from dllmain() */
#endif
}


/* -------------------------------------------------------mlDINewPhysicalDevice
 */
MLstatus MLAPI mlDINewPhysicalDevice( MLsystemContext systemContext,
				      MLmoduleContext moduleContext,
				      const MLbyte* ddCookie,
				      MLint32 ddCookieSize )
{
  MLsystemRec* pSystem = (MLsystemRec*) systemContext;
  MLint32 moduleIndex = (MLint32) moduleContext;
  MLphysicalDeviceRec* pDevice = 
    pSystem->physicalDevices + pSystem->physicalDeviceCount;

  if ( pSystem->physicalDeviceCount == ML_MAX_PHYSICAL_DEVICES ) {
    fprintf( stderr, "[mlDINewPhysicalDevice] in current implmentation, "
             "there is a limitation of %d physical devices\n",
             ML_MAX_PHYSICAL_DEVICES );
    return ML_STATUS_INSUFFICIENT_RESOURCES;
  }

  pDevice->id = (MLint64) (ML_REF_TYPE_DEVICE | 
			   (pSystem->physicalDeviceCount+1)<<12)<<32;

  if ( getenv( "MLOS_DEBUG" ) ) {
    MLmoduleRec* pModule = pSystem->modules + moduleIndex;
    _mlOSErrPrintf( "New Device #%d %s [%d] [0x%" FORMAT_LLX "] pDevice 0x%p "
		    "cookie -> 0x%p (%d bytes) devpriv -> 0x%p\n",
		    pSystem->physicalDeviceCount+1,
		    pModule->dsoname,
		    moduleIndex,
		    pDevice->id,
		    pDevice,
		    &pDevice->ddCookie,
		    ddCookieSize,
		    &pDevice->ddDevicePriv );
  }

  if ( ( ddCookieSize < 0 ) || ( ddCookieSize > ML_MAX_COOKIE_SIZE ) ) {
    fprintf( stderr, "[mlDINewPhysicalDevice] in current implmentation, "
	     "ddCookie must be no larger than %d bytes\n", ML_MAX_COOKIE_SIZE );
    return ML_STATUS_INVALID_ARGUMENT;
  }

  if ( ddCookie == NULL ) {
    pDevice->ddCookieSize = -1;
  } else {
    memcpy( pDevice->ddCookie, ddCookie, ddCookieSize );
    pDevice->ddCookieSize = ddCookieSize;
  }

  pDevice->connected=0;
  pDevice->moduleIndex = moduleIndex;
  pSystem->physicalDeviceCount++;

  return ML_STATUS_NO_ERROR;
}


/* ------------------------------------------------------------mlDINewUSTSource
 */
MLstatus MLAPI mlDINewUSTSource( MLsystemContext systemContext,
				 MLmoduleContext moduleContext,
				 MLint32 updatePeriod,
				 MLint32 latencyVariation,
				 const char* name,
				 MLint32 nameSize,
				 const char* description,
				 MLint32 descriptionSize )
{
  MLsystemRec* pSystem = (MLsystemRec*) systemContext;
  MLint32 moduleIndex = (MLint32) moduleContext;
  MLmoduleRec* pModule = pSystem->modules + moduleIndex;

  /* Make sure supplied metrics are sane.
   */
  if ( updatePeriod <= 0 ) {
    fprintf( stderr, "[mlDINewUSTSource] module '%s': invalid "
	     "updatePeriod %d: must be > 0\n",
	     pModule->dsoname, updatePeriod );
    return ML_STATUS_INVALID_ARGUMENT;
  }

  if ( latencyVariation < 0 ) {
    fprintf( stderr, "[mlDINewUSTSource] module '%s': invalid "
	     "latencyVariation %d: must be >= 0\n",
	     pModule->dsoname, latencyVariation );
    return ML_STATUS_INVALID_ARGUMENT;
  }

  /* Make sure the module has the appropriate entry point for
   * obtaining UST
   */
  {
#ifdef ML_OS_NT
    FARPROC ustEntryPoint = GetProcAddress( pModule->dsohandle, "ddGetUST" );
#else
    void* ustEntryPoint = dlsym( pModule->dsohandle, "ddGetUST" );
#endif
    if ( ustEntryPoint == NULL ) {
      fprintf( stderr, "[mlDINewUSTSource] module '%s': no 'ddGetUST' "
	       "entry point\n", pModule->dsoname );
      return ML_STATUS_INVALID_ARGUMENT;
    }
  }

  /* Now check that no UST source has yet been registered for this
   * module (a single source is permitted per module).
   */
  if ( pModule->ustUpdatePeriod > 0 ) {
    fprintf( stderr, "[mlDINewUSTSource] UST source already registered for "
	     "module '%s' [%d]\n",
	     pModule->dsoname, moduleIndex );
    return ML_STATUS_INVALID_ARGUMENT;
  }

  /* Copy over name and description -- make sure they aren't too large
   * (leave room for terminating null, which may or may not be present
   * in the client-supplied data)
   */
  if ( nameSize > (MLint32) (sizeof( pModule->ustName ) - 1) ) {
    fprintf( stderr, "[mlDINewUSTSource] in current implementation, "
	     "name must be no larger than %d bytes\n",
	     (MLint32) sizeof( pModule->ustName ) - 1 );
    return ML_STATUS_INVALID_ARGUMENT;
  }
  if ( descriptionSize > (MLint32) (sizeof( pModule->ustDescription ) - 1) ) {
    fprintf( stderr, "[mlDINewUSTSource] in current implementation, "
	     "description must be no larger than %d bytes\n",
	     (MLint32) sizeof( pModule->ustDescription ) - 1 );
    return ML_STATUS_INVALID_ARGUMENT;
  }

  if ( (name == NULL) || (nameSize == 0) ) {
    fprintf( stderr, "[mlDINewUSTSource] UST source must have a name\n" );
    return ML_STATUS_INVALID_ARGUMENT;
  } else {
    memcpy( pModule->ustName, name, nameSize );
    /* Ensure it is NULL-terminated
     */
    if ( pModule->ustName[ nameSize-1 ] != '\0' ) {
      pModule->ustName[ nameSize ] = '\0';
      ++nameSize;      
    }
    pModule->ustNameSize = nameSize;
  }

  if ( (description == NULL) || (descriptionSize == 0) ) {
    pModule->ustDescription[0] = '\0';
    pModule->ustDescriptionSize = 1;
  } else {
    memcpy( pModule->ustDescription, description, descriptionSize );
    /* Ensure it is NULL-terminated
     */
    if ( pModule->ustDescription[ descriptionSize-1 ] != '\0' ) {
      pModule->ustDescription[ descriptionSize ] = '\0';
      ++descriptionSize;      
    }
    pModule->ustDescriptionSize = descriptionSize;
  }

  /* Copy over UST source metrics
   */
  pModule->ustUpdatePeriod = updatePeriod;
  pModule->ustLatencyVar = latencyVariation;

  if( getenv( "MLOS_DEBUG" ) ) {
    _mlOSErrPrintf( "New UST source '%s', from module '%s' [%d],\n"
		    "updatePeriod = %d ns, latencyVariation = %d ns\n"
		    "Description: %s\n",
		    pModule->ustName,
		    pModule->dsoname, moduleIndex,
		    updatePeriod, latencyVariation,
		    pModule->ustDescription );
  }

  return ML_STATUS_NO_ERROR;
}


/* ----------------------------------------------------------------mlDITearDown
 */
void mlDITearDown(void)
{
  (void) _mlOSExiting();
  (void) _mlExiting();
}

