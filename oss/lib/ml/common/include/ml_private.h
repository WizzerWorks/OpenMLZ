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

#ifndef _ML_PRIVATE_H
#define _ML_PRIVATE_H
#ifdef __cplusplus
extern "C" {
#endif

#include <ML/ml.h>

#if !defined(_KERNEL)
#include <limits.h>

#ifndef ML_OS_NT
#include <sys/param.h>          /* for MAXHOSTNAMELEN */
#else
#define MAXHOSTNAMELEN    (MAX_COMPUTERNAME_LENGTH + 1)
#endif

#include <ML/ml_didd.h>
#include <ML/ml_oswrap.h>


typedef MLstatus (*NoopProcPtr)(void);
typedef MLstatus (*mlDDInterrogatePP)(MLsystemContext,
				      MLmoduleContext moduleContext);
typedef MLstatus (*mlDDConnectPP)(MLbyte *physicalDeviceCookie, 
				  MLint64 staticDeviceId,
				  MLphysicalDeviceOps *pOps,
				  MLbyte** retddDevicePriv);
typedef MLint32 (*mlDDGetUSTPP)(MLint64* ust);

#define ML_MAX_MODULES 32
#define ML_MAX_PHYSICAL_DEVICES 16
#define ML_MAX_OPEN_DEVICES 64

#ifdef ML_OS_NT
#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif
#endif

typedef struct _MLmoduleRec
{
  char dsopath[PATH_MAX];
  char dsoname[128];
  char connectName[128];
  mlDDInterrogatePP interrogate;
  mlDDConnectPP connect;
  void* dsohandle;
  MLint32 ustUpdatePeriod;
  MLint32 ustLatencyVar;
  char ustName[ML_MAX_USTNAME + 1];
  MLint32 ustNameSize; /* includes terminating NULL */
  char ustDescription[ML_MAX_USTDESCRIPTION + 1];
  MLint32 ustDescriptionSize; /* includes terminating NULL */
} MLmoduleRec;

typedef struct _MLphysicalDeviceRec
{
  MLint64 id;
  MLbyte ddCookie[ML_MAX_COOKIE_SIZE];
  MLint32 ddCookieSize;
  MLint32 moduleIndex;
  MLint32 connected;
  MLphysicalDeviceOps ops;
  MLbyte* ddDevicePriv;
} MLphysicalDeviceRec;

/* An open instance of a logical device
 */
typedef struct _MLopenDeviceRec {
  MLopenid id;
  MLphysicalDeviceRec* device;
  MLint64  objectid;
  MLbyte*  ddPriv;
} MLopenDeviceRec;

/*
 * per system OS privates - this should be kept under 128KB in size to
 *                          stay within default pthread stack limits
 */
typedef struct _MLsystemRec {
  MLint64 id;
  MLbyte name[ MAXHOSTNAMELEN ];
  MLint32 moduleCount;
  MLmoduleRec modules[ML_MAX_MODULES];
  MLint32 physicalDeviceCount;
  MLphysicalDeviceRec physicalDevices[ML_MAX_PHYSICAL_DEVICES];
  MLint32 openDeviceCount;
  MLopenDeviceRec openDevices[ML_MAX_OPEN_DEVICES];
  MLint32 ustSrcModuleIndex;
  mlDDGetUSTPP getUST;
} MLsystemRec;

/* OS independent routines: first-time init, and 'atexit' clean-up
 */
extern MLstatus _mlInit(void);
extern MLstatus _mlExiting(void);

/* OS dependent routines exposed to the DI layer.
 */
extern MLstatus _mlOSInit(void);
extern MLstatus _mlOSExiting(void);
extern MLstatus _mlOSClearLocalSystem(MLsystemRec* pSystem);
extern MLstatus _mlOSDiscoverLocalSystem(MLsystemRec* pSystem);
extern MLstatus _mlOSDiscoverLocalDevices(MLsystemRec* pSystem);
extern MLstatus _mlOSBootstrapSystem(MLsystemRec* pSystem);
extern MLstatus _mlOSBootstrapDevice(MLsystemRec* pSystem,
				     MLphysicalDeviceRec* pDevice);

extern void _mlOSErrPrintf( const char *format, ... );
extern void _mlOSError(const char *msg);

/* Following structure must be a mulitple of 8 bytes in length
 */
typedef struct {
  MLint32 generation; /* -1 while constructing, >= 0 when ready for use. */
  MLint32 major;
  MLint32 minor;
  MLint32 segLength; /* length of shared seg *including* this header */
} _mlOSDaemonHead;

#ifdef ML_OS_NT
#include <process.h>
#define MLOSDAEMON_KEY (MLbyte*)"mlDaemonSharedCapabilities"
#define MLOSDAEMON_DAEMON_ACCESS (FILE_MAP_READ | FILE_MAP_WRITE)
#define MLOSDAEMON_CLIENT_ACCESS FILE_MAP_READ
typedef HANDLE _mlOSShmSegHandle;

#endif

#ifdef ML_OS_IRIX
#include <sys/mman.h> /* PROT_READ, PROT_WRITE */
#if (_MIPS_SIM == _MIPS_SIM_ABI64)
    #define MLOSDAEMON_KEY "/tmp/.ml64DaemonSharedCapabilites"
#else
    #define MLOSDAEMON_KEY "/tmp/.ml32DaemonSharedCapabilites"
#endif
#define MLOSDAEMON_DAEMON_ACCESS (PROT_READ | PROT_WRITE)
#define MLOSDAEMON_CLIENT_ACCESS PROT_READ
typedef int _mlOSShmSegHandle;

#endif

#ifdef ML_OS_LINUX

#define oserror() errno

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#define MLOSDAEMON_KEY ((MLbyte *)271828182)
#define MLOSDAEMON_DAEMON_ACCESS 0 /* attach read-write */
#define MLOSDAEMON_CLIENT_ACCESS SHM_RDONLY /* attach read-only */
typedef int _mlOSShmSegHandle;

#endif


/* More general, user+kernel, multiprocess, heavyweight locks intended
 * to coordinate access to system wide global resources.
 *
 * FIXME: at present, this implementation is *not* adequate for
 * multiprocess protection!
 */

#ifdef ML_OS_IRIX
#include <semaphore.h>
typedef sem_t _mlOSLockable;
#endif
#ifdef ML_OS_LINUX
#include <semaphore.h>
typedef sem_t _mlOSLockable;
#endif
#ifdef ML_OS_NT
typedef CRITICAL_SECTION _mlOSLockable;
#endif

MLstatus _mlOSNewLock(_mlOSLockable *pLock);
MLstatus _mlOSFreeLock(_mlOSLockable *pLock);
MLstatus _mlOSLock(_mlOSLockable *pLock);
MLstatus _mlOSUnlock(_mlOSLockable *pLock);

/*** L E V E L  1   F I L E   I / O ***/

#if defined(ML_OS_UNIX)

/* include system header files */
#include <unistd.h>
#include <fcntl.h>

#define	_mlFileOpen		open
#define	_mlFileClose		close
#define	_mlFileRead		read
#define	_mlFileWrite		write
#define	_mlFileSeek		lseek
#define	_mlFileAccess	access
#define	_mlFileRewind(fd)	lseek(fd,SEEK_SET,0)	

/* define permission modes for _mlFileOpen() */
#define ML_O_RDONLY   O_RDONLY
#define ML_O_WRONLY   O_WRONLY
#define ML_O_RDWR     O_RDWR
#define ML_O_APPEND   O_APPEND
#define ML_O_CREATE   O_CREAT
#define ML_O_TRUNC    O_TRUNC
#define ML_O_EXCL     O_EXCL
#define ML_O_NONBLOCK O_NONBLOCK

#ifdef ML_OS_LINUX /* O_DIRECT not present in linux:/usr/include/fcntlbits.h */
#define ML_O_DIRECT   0 /* XXX _BOGUS_ XXX */
#else
#define ML_O_DIRECT   O_DIRECT
#endif

/* define whence modes for _mlFileSeek() */
#define ML_SEEK_SET   SEEK_SET
#define ML_SEEK_CUR   SEEK_CUR
#define ML_SEEK_END   SEEK_END

/* define access modes for _mlFileAccess() */
#define ML_R_OK       R_OK    /* test for read permission */
#define ML_W_OK       W_OK    /* test for write permission */
#define ML_X_OK       X_OK    /* test for execute permission */
#define ML_F_OK       X_OK    /* test for existence of file */

#endif /* ML_OS_UNIX */


#if defined(ML_OS_NT)

/* include system header files */
#include <io.h>
#include <fcntl.h>
#include <ctype.h>

#define _mlFileOpen        _open
#define _mlFileClose       _close
#define _mlFileRead        _read
#define _mlFileWrite       _write
#define _mlFileSeek        _lseek
#define _mlFileAccess      _access


/* define permission modes for _mlFileOpen() */
#define ML_O_RDONLY   _O_RDONLY
#define ML_O_WRONLY   _O_WRONLY
#define ML_O_RDWR     _O_RDWR
#define ML_O_APPEND   _O_APPEND
#define ML_O_CREATE   _O_CREATE
#define ML_O_TRUNC    _O_TRUNC
#define ML_O_EXCL     _O_EXCL
#define ML_O_NONBLOCK _O_NONBLOCK
#define ML_O_DIRECT   _O_DIRECT

/* define whence modes for _mlFileSeek() */
/*  Note: "origin" modes for _lseek() are the same as IRIX "whence" modes */
#define ML_SEEK_SET   SEEK_SET
#define ML_SEEK_CUR   SEEK_CUR
#define ML_SEEK_END   SEEK_END

/* define access modes for _mlFileAccess() */
#define ML_R_OK       0x04    /* test for read permission */
#define ML_W_OK       0x02    /* test for write permission */
#define ML_X_OK       0x01    /* test for execute permission */
#define ML_F_OK       0x00    /* test for existence of file */

#endif /* ML_OS_NT */

/*** S T A N D A R D   I / O ***/

#include <stdio.h>

/* officially "1.0" */
#define ML_VERSION_MAJOR 1
#define ML_VERSION_MINOR 0
#define ML_VERSION_PRERELEASE 0

#define LOW_MASK_32 0xFFFFFFFF
#define HIGH_MASK_31 ((MLint64)0x7FFFFFFF << 32)

/* Position ML_REF_MASK_DEVICE to/from low order bit
 */
#define DEVICE_MASK_SHIFT 12

/* Floating point tolerance used to test for nearness of user value to
 * the lattice defined by ML_PARAM_INCREMENT and ML_PARAM_MINS.
 */
#define DI_LATTICE_VALIDATE_DELTA 0.001

/* Entry called from the NT mlSDK's dllMain(), or from the IRIX rld
   init
 */
extern void mlDIGenesis( void );

/* Tear-down called from the NT mlSDK's dllMain(); on Unix, called
 * from the "atexit()" function
 */
extern void mlDITearDown( void );

/* Duplicate (malloc and copy) a message
 */
extern MLpv* MLAPI _mlMsgDup(MLpv* p);

extern MLstatus MLAPI _mlDIArrayAlloc( MLpv* param, MLint32 size );
extern MLstatus MLAPI _mlDIArrayAppend( MLpv* param, char* data,
					MLint32 size );
extern void     MLAPI _mlDIArrayFree( MLpv* param );
extern MLint32  MLAPI _mlDIPvSizeofElem( MLint64 param );

#endif /*_KERNEL*/


/* special type for passing pointers between 32 and 64 bit abi's */
#if	defined( ML_ARCH_MIPS )

    typedef	struct { MLint32 high, low; }	MLaddressparts;

    #if (_ML_SZLONG == 64)
	typedef	struct { void	*p; }		MLvoidstar;
    #else
	typedef	struct { void	*pad, *p; }	MLvoidstar;
    #endif

#elif	defined( ML_ARCH_IA64 )

    typedef	struct { MLint32 low, high; }	MLaddressparts;
    typedef	struct { void	*p; }		MLvoidstar;

#elif	defined( ML_ARCH_IA32 )

    typedef	struct { MLint32 low, high; }	MLaddressparts;
    typedef	struct { void	*p, *pad; }	MLvoidstar;

#endif

typedef union MLpointer_e {
    MLuint64		i;
    MLvoidstar		v;
    MLaddressparts	p;
} MLpointer;

#ifdef _KERNEL
#include <sys/debug.h>
#include <sys/ddi.h>
#else
#include <assert.h>
#endif /* _KERNEL */

#ifdef _KERNEL
#define mlAssert ASSERT
#else
#define mlAssert assert
#endif /* _KERNEL */

/* include prototypes for malloc(), et al */
#if !defined(_KERNEL)
#include <stdlib.h>
#endif /* _KERNEL */

/* include system header files */
#if defined(ML_OS_NT) && !defined(__MWERKS__)
#include <memory.h>
#endif 

#define mlFree         free
#define mlMalloc       malloc
#define mlRealloc      realloc
#define mlCalloc       calloc

/* Defines for functions with other names under Windows / MSVC */
#if defined(ML_OS_NT) && defined(_MSC_VER)
#define snprintf _snprintf
#define strncasecmp _strnicmp
#endif

#ifdef __cplusplus
}
#endif
#endif /* _ML_PRIVATE_H */

