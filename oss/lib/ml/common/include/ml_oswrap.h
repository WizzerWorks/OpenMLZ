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

#ifndef _ML_OSWRAP_H
#define _ML_OSWRAP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <ML/ml.h>


/* These are wrappers to hide os-dependencies.  If your
 * device-dependent module is only intended for a single operating
 * system, then its likely you won't need these.
 */

#ifdef ML_OS_NT
#include <process.h>
typedef HANDLE _mlOSThread;
typedef HANDLE _mlOSSema;

/* This *should* always be a DWORD, but PROCESS.H appears broken in
 * Metrowerks CodeWarrior 8.0, and it defines it as 'unsigned'. Or
 * maybe Metrowerks isn't so broken after all, it seems MinGW does the
 * same...
 */
#if defined(__MWERKS__) || defined(COMPILER_GCC)
typedef unsigned _mlOSThreadRetValue;
#else
typedef DWORD _mlOSThreadRetValue;
#endif
#endif

#ifdef ML_OS_IRIX
#include <sys/mman.h> /* PROT_READ, PROT_WRITE */
#include <sys/prctl.h>
#include <pthread.h>
#include <semaphore.h>
typedef struct _mlOSThread_s {
    pthread_t pthread;
    pid_t pid;
    sem_t join_sema;
} _mlOSThread;
typedef sem_t _mlOSSema;
typedef void* _mlOSThreadRetValue;
#endif

#ifdef ML_OS_LINUX
#define oserror() errno
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <pthread.h>
#include <semaphore.h>
typedef struct _mlOSThread_s {
    pthread_t pthread;
} _mlOSThread;
typedef sem_t _mlOSSema;
typedef void* _mlOSThreadRetValue;
#endif

MLstatus _mlOSThreadCreate(_mlOSThread *thread, 
                           _mlOSThreadRetValue (*entry)(void *), void *arg);
MLstatus _mlOSThreadPrepare(_mlOSThread *thread);
MLstatus _mlOSThreadJoin(_mlOSThread *thread, MLint32 *exitState);
MLstatus _mlOSThreadReap(void);
void _mlOSThreadExit(_mlOSThread *thread, MLstatus exitState);

MLstatus _mlOSSemaNew(_mlOSSema *pSema, MLint32 value);
MLstatus _mlOSSemaFree(_mlOSSema *pSema);
MLstatus _mlOSSemaWait(_mlOSSema *pSema);
MLstatus _mlOSSemaTryWait(_mlOSSema *pSema);
MLstatus _mlOSSemaPost(_mlOSSema *pSema);


/* Simple, user mode, single process, lightweight locks intended to
 * avoid collision of errant application threads.
 */

#ifdef ML_OS_IRIX
#include <semaphore.h>
typedef sem_t _mlOSLockableLite;
#endif
#ifdef ML_OS_LINUX
#include <semaphore.h>
typedef sem_t _mlOSLockableLite;
#endif
#ifdef ML_OS_NT
typedef CRITICAL_SECTION _mlOSLockableLite;
#endif

MLstatus _mlOSNewLockLite(_mlOSLockableLite *pLock);
MLstatus _mlOSFreeLockLite(_mlOSLockableLite *pLock);
MLstatus _mlOSLockLite(_mlOSLockableLite *pLock);
MLstatus _mlOSUnlockLite(_mlOSLockableLite *pLock);


/* Until there is a system-wide UST clock on all platforms, we need to
 * provide our own.
 *
 * NOTE: this function is deprecated, and should never be called
 * directly anymore.  Apps should always use mlGetSystemUST() and
 * modules should obtain a func pointer using mlDIGetUSTSource()
 */
extern MLint32 _mlOSGetUST(MLint64* UST);

#ifdef __cplusplus 
}
#endif

#endif /* ML_OSWRAP_H */


