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

/* Note: do not change the following define name */
#ifndef __ML_TYPES_H_
#define __ML_TYPES_H_


#ifdef __cplusplus
extern "C" {
#endif

#if defined(WIN32) || defined(WIN64)
#define _WIN32_WINNT	0x0400
#define WIN32_LEAN_AND_MEAN
#include <Windows.h> /* Needed for HANDLE definition */
#endif

#include <stddef.h>   /* for size_t */
#include <sys/types.h>
#ifndef	_KERNEL
#include <float.h>
#endif

#if defined(_WIN64)
#include <basetsd.h>
#endif /* _WIN64 */

/* platform feature selection */
#if defined(WIN32)
	#define ML_OS_NT	1
	#define ML_OS_NT32	1
	#define ML_ARCH_IA32	1
#endif

#if  defined(WIN64)
	#define ML_OS_NT	1
	#define ML_OS_NT64	1
	#define ML_ARCH_IA64	1
#endif

#if defined(__sgi)
	#define ML_OS_IRIX	1
	#define ML_OS_UNIX	1
	#define ML_ARCH_MIPS	1
#endif

#if defined(linux)
	#define ML_OS_LINUX	1
	#define ML_OS_UNIX	1

	#if defined(__ia64)
	#define ML_ARCH_IA64 1
	#endif

	#if defined(__i386)
	#define ML_ARCH_IA32	1
	#endif
#endif

/* Basic types */
typedef signed char		MLbyte;
typedef unsigned char		MLubyte;

typedef signed char		MLint8;
typedef short          		MLint16;
typedef signed int		MLint32;

typedef unsigned char		MLuint8;
typedef unsigned short		MLuint16;
typedef unsigned int		MLuint32;

#if defined(ML_ARCH_IA32)
    #if defined(_WIN32) /* WIN32 lacks native unsigned 64 bit type */
        typedef __int64	            MLuint64; /* FIXME */
    #elif(_WIN64)
        typedef ULONG64             MLuint64;
    #else /* Linux */
        typedef signed long long    MLuint64;
    #endif /* _WIN32 */


/* MIPS 7.3 compilers sensitive about 'long long'
 */
#elif defined(ML_ARCH_MIPS)
    typedef uint64_t		MLuint64;

#else
    typedef unsigned long long	MLuint64;
#endif /* ML_ARCH_IA32 */

#if !defined(_WIN64)
    typedef size_t		MLsize_t;
#else
    typedef SIZE_T		MLsize_t;
#endif /* _WIN64 */

typedef MLint32 MLboolean;

#define ML_FALSE   0
#define ML_TRUE    1

/* set the size of a long so that we can set types correctly */
#if defined(ML_ARCH_MIPS)
    #if (_MIPS_SZLONG == 64)
        #define MLintptr MLint64
	#define _ML_SZLONG 64
    #else /* _MIPS_SZLONG */
        #define MLintptr MLint32
	#define _ML_SZLONG 32
    #endif /* _MIPS_SZLONG */

#else /* !ML_ARCH_MIPS */
    #if defined(ML_ARCH_IA32)
        #define MLintptr MLint32
	#define _ML_SZLONG 32
    #endif

    #if defined(ML_ARCH_IA64)
        #define MLintptr MLint64
	#define _ML_SZLONG 64
    #endif 

#endif /* ML_ARCH_MIPS */

/* MIPS 7.3 compilers sensitive about 'long long'
 */
#ifdef ML_ARCH_MIPS
    typedef int64_t			MLint64;
#else /* PC PLATFORM */
    #ifdef ML_OS_LINUX
	typedef signed long long	MLint64;
    #else /* ML_OS_NT */
	#ifdef ML_OS_NT32
	    typedef __int64         MLint64;
	#else /* ML_OS_NT64 */
	    typedef LONG64          MLint64;
	#endif /* ML_OS_NT */
    #endif /* ML_OS_LINUX */
#endif /* ML_ARCH_MIPS */

typedef float			MLreal32;
typedef double			MLreal64;

typedef MLint64                 MLopenid;
typedef MLint32 MLstatus;

/* Waitable - handle on NT, file descriptor on Irix/Linux
 */
#ifdef	ML_OS_NT
typedef HANDLE MLwaitable;
#else  /* Un*x */
typedef int MLwaitable;
#endif

/* Calling discipline exposed in function prototypes on NT
 */
#ifdef	ML_OS_NT
#define MLAPI __stdcall
#else  /* Un*x */
#define MLAPI 
#endif

/* Helper macros for use in printf's to output MLint64 values
 * FORMAT_LLX outputs hex notation, FORMAT_LLD outputs decimal
 */
#ifdef ML_OS_UNIX
#define FORMAT_LLX "llx"
#define FORMAT_LLD "lld"
#endif
#ifdef ML_OS_NT
#if defined(__MWERKS__) || defined(COMPILER_GCC)
#define FORMAT_LLX "llx"
#define FORMAT_LLD "lld"
#else
/* Assume Microsoft Visual C */
#define FORMAT_LLX "I64x"
#define FORMAT_LLD "I64d"
#endif
#endif

#ifdef __cplusplus 
}
#endif

#endif /* __ML_TYPES_H_ */
