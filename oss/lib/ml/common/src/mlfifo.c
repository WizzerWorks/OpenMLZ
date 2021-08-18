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

#include <ML/ml.h>
#include <ML/mlfifo.h>
#include <ML/ml_private.h>

#ifdef ML_OS_UNIX
#include "mldebug.h"
#endif

/* Include system header files */
#ifdef ML_OS_IRIX
#ifdef _KERNEL
#include <sys/types.h>
#include <sys/ddi.h>
#include <sys/immu.h>
#else
#include <stdlib.h>
#include <unistd.h>
#endif
#endif

#ifdef ML_OS_LINUX
#include <unistd.h>
#include <stdlib.h>
#include <string.h> 
#endif


/* --------------------------------------------------------------------- macros
 */
#define ML_FIFO_CLEAR_FLAGS(fifo) \
    ((fifo)->flags = 0x00000000)

#define ML_FIFO_SET_ALLOCED(fifo) \
    (((fifo)->flags) = (((fifo)->flags) & ~ML_FIFO_STATUS_ALLOC_MASK) | \
                      ML_FIFO_STATUS_ALLOCED)
#define ML_FIFO_SET_FREE(fifo) \
    (((fifo)->flags) = (((fifo)->flags) & ~ML_FIFO_STATUS_ALLOC_MASK) | \
                      ML_FIFO_STATUS_FREE)
#define ML_FIFO_IS_ALLOCED(fifo) \
    ((((fifo)->flags) & ML_FIFO_STATUS_ALLOC_MASK) == ML_FIFO_STATUS_ALLOCED)
#define ML_FIFO_IS_FREE(fifo) \
    ((((fifo)->flags) & ML_FIFO_STATUS_ALLOC_MASK) == ML_FIFO_STATUS_FREE)

#define ML_FIFO_SET_OPEN(fifo) \
    (((fifo)->flags) = (((fifo)->flags) & ~ML_FIFO_STATUS_OPEN_MASK) | \
                      ML_FIFO_STATUS_OPENED)
#define ML_FIFO_SET_CLOSE(fifo) \
    (((fifo)->flags) = (((fifo)->flags) & ~ML_FIFO_STATUS_OPEN_MASK) | \
                      ML_FIFO_STATUS_CLOSED)
#define ML_FIFO_IS_OPEN(fifo) \
    ((((fifo)->flags) & ML_FIFO_STATUS_OPEN_MASK) == ML_FIFO_STATUS_OPENED)
#define ML_FIFO_IS_CLOSED(fifo) \
    ((((fifo)->flags) & ML_FIFO_STATUS_OPEN_MASK) == ML_FIFO_STATUS_CLOSED)

#define ML_FIFO_SET_LOCK(fifo) \
    (((fifo)->flags) = (((fifo)->flags) & ~ML_FIFO_STATUS_LOCK_MASK) | \
                      ML_FIFO_STATUS_LOCKED)
#define ML_FIFO_SET_UNLOCK(fifo) \
    (((fifo)->flags) = (((fifo)->flags) & ~ML_FIFO_STATUS_LOCK_MASK) | \
                      ML_FIFO_STATUS_UNLOCKED)
#define ML_FIFO_IS_LOCKED(fifo) \
    ((((fifo)->flags) & ML_FIFO_STATUS_LOCK_MASK) == ML_FIFO_STATUS_LOCKED)
#define ML_FIFO_IS_UNLOCKED(fifo) \
    ((((fifo)->flags) & ML_FIFO_STATUS_LOCK_MASK) == ML_FIFO_STATUS_UNLOCKED)


#ifndef NDEBUG
#define	FIFO_DEBUG(s)	if( getenv("ML_DEBUG_FIFO") ){ s; }
#else
#define	FIFO_DEBUG(s)
#endif


/* ---------------------------------------------------------------_mlDIFifoOpen
 *
 * Open a fifo using a pre-exisiting MLfifo struct by initializing it.
 */
MLstatus MLAPI _mlDIFifoOpen( MLfifo* rb, MLint32 size )
{
#ifndef NDEBUG
  MLint32 zero = 0; /* use extra variable to quiet compiler warning */
  mlAssert( ML_FIFO_HEADER_SIZE % 8 == zero ); /* verify int64 alignment */
  mlAssert( rb != NULL );
  mlAssert( size > 0 );
#endif

  rb->hardEnd = rb->softEnd = size;
  rb->readOffset = rb->writeOffset = 0;
  ML_FIFO_SET_OPEN( rb );

  return ML_STATUS_NO_ERROR;
}


/* --------------------------------------------------------------_mlDIFifoClose
 *
 * Clear out the fifo structure to indicate closure.
 */
MLstatus MLAPI _mlDIFifoClose( MLfifo* rb )
{
  mlAssert( rb != NULL );
  mlAssert( ML_FIFO_IS_UNLOCKED( rb ) );

  /* Assume we can't close unless the queue is empty */
  mlAssert( rb->writeOffset == rb->readOffset );

  /* To assist in debugging zero out the structure */
  rb->writeOffset = rb->readOffset = 0;
  rb->softEnd = rb->hardEnd = 0;
  ML_FIFO_SET_CLOSE( rb );

  return ML_STATUS_NO_ERROR;
}


/* -----------------------------------------------------------_mlDIFifoPreWrite
 *
 * PreWrite guarantees that there are maxBytes contiguous space in the
 * FIFO.
 *
 * If read <= write at the start of this function, then it may modify
 * the soft end marker to perform a wrap.
 *
 * Returns a pointer to the data area or NULL on error. 
 */
void* MLAPI _mlDIFifoPreWrite( MLfifo* rb, MLint32 maxSize )
{
  MLbyte *pStart;
  MLint32 localRead, localWrite;

  mlAssert( rb != NULL );

  pStart = ((MLbyte *) rb) + ML_FIFO_HEADER_SIZE;

  mlAssert( (((MLintptr) rb) % 8) == 0 );
  mlAssert( (((MLintptr) pStart) % 8) == 0 );
  mlAssert( maxSize > 0 );

  /* Insure size requested respects our in64 alignment requirements */
  maxSize = ( maxSize + 7 ) & ~7;

  localWrite = rb->writeOffset;
  localRead  = rb->readOffset; /* assume this is atomic */

  if ( localRead <= localWrite ) {
    /* Not yet wrapped - see if there's enough space */
    if ( maxSize >= rb->hardEnd - localWrite ) {
      if ( maxSize >= localRead ) {
	return NULL;
      }

      /* there is enough space, but only if we wrap */
      rb->softEnd = localWrite;
      localWrite = 0;

      /* Queue is now in a wrapped state */
    }
  } else {
    /* we're already wrapped - see if there's enough space */
    if ( maxSize >= localRead - localWrite ) {
      return NULL;
    }
  }

  mlAssert( localWrite % 8 == 0 ); /* maintain int64 alignment */
  mlAssert( localRead >= 0 );
  mlAssert( localRead <= rb->softEnd );

  return pStart + localWrite;
}


/* ----------------------------------------------------------_mlDIFifoPostWrite
 *
 * PostWrite may modify the write offset just before returning.  pEnd
 * is the address of the next free byte in the fifo - it is one more
 * than the address of the last byte written.
 *
 * If pEnd == (the pointer returned by PreWrite), then 0 bytes were
 * written.
 *
 * Returns success/failure.
 */
MLstatus MLAPI _mlDIFifoPostWrite( MLfifo* rb, void* pEnd )
{
  MLbyte *pStart;
  MLint32 size;

  mlAssert( rb != NULL );
  pStart = ( (MLbyte *) rb ) + ML_FIFO_HEADER_SIZE;

  /* Insure size respects our in64 alignment requirements */
  size = (MLint32) ( (MLbyte*) pEnd - pStart );
  size = ( size + 7 ) & ~7;

  mlAssert( size <= rb->softEnd );	/* check space */
  rb->writeOffset = size;		/* assume this is atomic */

  mlAssert( rb->writeOffset % 8 == 0 ); /* insure int64 alignment */

  return ML_STATUS_NO_ERROR;
}


/* ---------------------------------------------------------------_mlDIFifoPush
 *
 * Push may modify the write offset just before returning.
 *
 * If read <= write at the start of this function, then it may modify
 * the soft end marker to perform a wrap.  Allocate space for maxSize
 * bytes, but only copy size bytes.
 *
 * Returns a pointer to the copied data or NULL on error. 
 */

#include <stdio.h>

void* MLAPI _mlDIFifoPush( MLfifo* rb, void* data, MLint32 size,
			   MLint32 maxSize )
{
  void* pWrite;

  mlAssert( rb != NULL );
  mlAssert( size >= 0 );
  mlAssert( maxSize > 0 );
  mlAssert( size <= maxSize );
  mlAssert( (data == NULL && size == 0) || (data != NULL && size != 0) );

  /* Check for space, handle the wrap, and find where to write */
  if ( (pWrite = _mlDIFifoPreWrite( rb, maxSize )) == NULL ) {
    return NULL;
  }

  /* if a copy is requested... */
  if ( size ) {
    memcpy( pWrite, data, size );
  }

  /* Now update the write pointer and make sure all is well */
  if ( _mlDIFifoPostWrite( rb, (char*) pWrite+maxSize ) ) {
    return NULL;
  }

  return pWrite;
}


/* -----------------------------------------------------------_mlDIFifoHasSpace
 *
 * HasSpace makes no changes
 */
MLint32 MLAPI _mlDIFifoHasSpace( MLfifo* rb, MLint32 size )
{
  MLint32 localRead, localWrite;

  mlAssert( rb != NULL );
  mlAssert( size > 0 );

  /* Insure size respects our int64 alignment requirement */
  size = ( size + 7 ) & ~7;

  localWrite = rb->writeOffset;
  localRead  = rb->readOffset;

  if ( localRead <= localWrite ) { /* Have we wrapped yet? */
    /* Not yet wrapped */
    if ( size >= rb->hardEnd - localWrite && size >= localRead ) {
      return 0;
    }
  } else {
    /* Wrapped */
    if ( size >= localRead - localWrite ) {
      return 0;
    }
  }
  return 1;
}


/* ----------------------------------------------------------------_mlDIFifoTop
 *
 * Top doesn't modify the ringBuffer pointers in any way
 *
 * ASSUMPTION - both top and pop are run from a single thread.
 */
MLstatus MLAPI _mlDIFifoTop( MLfifo* rb, void** data )
{
  MLbyte *pStart;

  mlAssert( rb != NULL );

  pStart = ((MLbyte *) rb) + ML_FIFO_HEADER_SIZE;

  if ( rb->readOffset == rb->writeOffset ) {
    *data = NULL;
    FIFO_DEBUG( fprintf( stderr, "[_mlDIFifoTop] recv queue empty\n" ) )
      return ML_STATUS_RECEIVE_QUEUE_EMPTY;
  }

  if ( rb->readOffset == rb->softEnd ) {
    *data = pStart; /* wrap around to the beginning */
  } else {
    *data = pStart + rb->readOffset;
  }

#if !defined(_LP64) && !defined(ML_OS_LINUX) && !defined(COMPILER_GCC)
  FIFO_DEBUG( fprintf( stderr, "[_mlDIFifoTop] 0x%lx\n", *data ) )
#else
  FIFO_DEBUG( fprintf( stderr, "[_mlDIFifoTop] 0x%p\n", *data ) )
#endif

  return ML_STATUS_NO_ERROR;
}


/* ------------------------------------------------------------_mlDIFifoIterate
 *
 * Iterate doesn't modify the ringBuffer pointers in any way Given a
 * pointer to, and size of, an item in the fifo, it returns a pointer
 * to the following entry (or NULL if there is none).
 */
void* MLAPI _mlDIFifoIterate( MLfifo* rb, void* data, MLint32 entrySize )
{
  MLbyte *pStart;
  MLint32 nextReadOffset;

#if !defined(_LP64) && !defined(ML_OS_LINUX) && !defined(COMPILER_GCC)
  FIFO_DEBUG( fprintf( stderr, "[_mlDIFifoIterate] rb 0x%lx\n", rb ) )
#else
  FIFO_DEBUG( fprintf( stderr, "[_mlDIFifoIterate] rb 0x%p\n", rb ) )
#endif

  mlAssert( rb != NULL );
  mlAssert( entrySize > 0 );

  pStart = ((MLbyte *) rb) + ML_FIFO_HEADER_SIZE;

  if ( rb->readOffset == rb->writeOffset ) {
    /* queue is empty */
    return NULL;
  }

  if ( data == NULL ) {
    void* ret;
    _mlDIFifoTop( rb, &ret );
    return ret;
  }

  nextReadOffset = (MLint32) (((MLbyte*) data) - pStart) + entrySize;
  if ( nextReadOffset == rb->writeOffset ) {
    /* at end of fifo */
    return NULL;
  }

  if ( nextReadOffset == rb->softEnd ) {
    nextReadOffset = 0; /* wrap around to the beginning */
  }

#if !defined(_LP64) && !defined(ML_OS_LINUX) && !defined(COMPILER_GCC)
  FIFO_DEBUG( fprintf( stderr, "[_mlDIFifoIterate] return 0x%lx\n",
		       pStart + nextReadOffset ) )
#else
  FIFO_DEBUG( fprintf( stderr, "[_mlDIFifoIterate] return 0x%p\n",
		       pStart + nextReadOffset ) )
#endif
  return pStart + nextReadOffset;
}


/* -----------------------------------------------------------_mlDIFifoOffsetTo
 *
 * Given a pointer to some datum in the FIFO, return its offset
 * relative to the start of the FIFO. This allows us to record the
 * location of an item without regard to its user/kernel address
 * mapping.
 */
MLint32 MLAPI _mlDIFifoOffsetTo( MLfifo* rb, void* data )
{
  MLbyte *pStart;

  mlAssert( rb != NULL );
  mlAssert( data != NULL );

  pStart = ((MLbyte *) rb) + ML_FIFO_HEADER_SIZE;
  mlAssert( (pStart <= (MLbyte*)data) &&
	    ((MLbyte*)data < pStart + rb->softEnd) );

  return (MLint32) (((MLbyte*) data) - pStart);
}


/* -----------------------------------------------------------_mlDIFifoAtOffset
 *
 * Given a byte offset to some datum in the FIFO, return its address.
 * This allows us to retrieve the location of an item without regard
 * to its user/kernel address mapping.
 */
void* MLAPI _mlDIFifoAtOffset( MLfifo* rb, MLint32 offset )
{
  MLbyte *pStart;

  mlAssert( rb != NULL );
  mlAssert( (0 <= offset) && (offset < rb->softEnd) );
  pStart = ((MLbyte *) rb) + ML_FIFO_HEADER_SIZE;
  return (void *) (pStart + offset);
}


/* ----------------------------------------------------------------_mlDIFifoPop
 *
 * Pop may modify the read Offset just before returning.  If read >
 * write at the start of this function then it may additionally modify
 * the soft end marker to remove a wrap Important - we always pop data
 * in the same sizes in which it was pushed
 */
MLstatus MLAPI _mlDIFifoPop( MLfifo* rb, MLint32 size )
{
  MLint32 localRead  = rb->readOffset;

  mlAssert( rb != NULL );
  mlAssert( size > 0 );
  mlAssert( localRead >= 0 );
  mlAssert( localRead != rb->writeOffset );
  mlAssert( localRead <= rb->softEnd );

  /* The following must match the alignment contraints imposed by the
   * mlDIFifoPostWrite routine when the entry was pushed
   */
  size = ( size + 7 ) & ~7;

  if ( localRead == rb->softEnd ) {
    /* Wrap the read Offset from the soft end back to the start */
    localRead = 0;
    rb->softEnd = rb->hardEnd;
    /* Queue is now back to linear state */
  }

  localRead += size;

  rb->readOffset = localRead; /* assume this is atomic */

  return ML_STATUS_NO_ERROR;
}


/* ---------------------------------------------------------------_mlDIFifoLock
 *
 * Pin or lock the fifo in memory. The size argument is the size of
 * the fifo.
 */
MLstatus MLAPI _mlDIFifoLock( MLfifo* rb, MLint32 size )
{
  MLbyte *pStart;

  mlAssert( rb != NULL );
  mlAssert( size > 0 );

  pStart = ((MLbyte *) rb) + ML_FIFO_HEADER_SIZE;

#ifdef ML_OS_NT
  VirtualLock( pStart, size );
#endif
#ifdef ML_OS_IRIX
#ifdef _KERNEL
#else
  mpin( pStart, size );
#endif /* _KERNEL */
#endif
#ifdef ML_OS_LINUX
	/* XXX Linux mpin? */
  mlDebug( "_mlDIFifoLock missing mpin()!\n" );
  mlAssert( 0 );
#endif

  ML_FIFO_SET_LOCK( rb );

  return ML_STATUS_NO_ERROR;
}


/* -------------------------------------------------------------_mlDIFifoUnlock
 *
 * Unpin or unlock the fifo in memory. The size argument is the size
 * of the fifo.
 */
MLstatus MLAPI _mlDIFifoUnlock( MLfifo* rb, MLint32 size )
{
  MLbyte *pStart;
  mlAssert(size > 0);

  mlAssert( rb != NULL );

  pStart = ((MLbyte *) rb) + ML_FIFO_HEADER_SIZE;

#ifdef ML_OS_NT
  VirtualUnlock( pStart, size );
#endif
#ifdef ML_OS_IRIX
#ifdef _KERNEL
#else
  munpin( pStart, size );
#endif /* _KERNEL */
#endif
#ifdef ML_OS_LINUX
	/* XXX Linux munpin? */
  mlDebug( "_mlDIFifoUnlock missing munpin()!\n" );
  mlAssert( 0 );
#endif

  ML_FIFO_SET_UNLOCK( rb );

  return ML_STATUS_NO_ERROR;
}
