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

#ifndef _ML_FIFO_H_
#define _ML_FIFO_H_

/*
 * mlfifo.h
 *
 * Defines a first-in-first-out (fifo) structure for the queues in mlSDK
 * Any single piece of data inserted into the fifo is guaranteed to reside 
 * in contiguous memory.
 *
 * A fifo is always in one of three states:
 *
 * EMPTY:  (read == write)
 *
 *   Start          Read and Write                    SoftEnd and HardEnd
 *     |--------------|---------------------------------|
 *  
 *
 * LINEAR:  (read < write)
 *
 *   Start          Read           Write              SoftEnd and HardEnd
 *     |--------------|11111122222222|------------------|
 * 
 *
 * WRAPPED:  (write > read)
 *
 *   Start      Write      Read              SoftEnd  HardEnd
 *     |4444444444|---------|2222222233333333333|ooooooo|
 *
 *
 * - indicates free space.
 * 1,2,3,4 examples of single items written to the buffer
 * o indicates space which was left blank so that item 4 could
 *   be in contiguous memory.
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

/* Include header files
 */

#include <ML/mltypes.h> /* For type definitions */
#include <ML/mldefs.h> /* For status definitions */


/* Declare structures
 */

typedef struct 
{
  MLint32 flags;
  MLint32 softEnd;
  MLint32 hardEnd;
  MLint32 readOffset;
  MLint32 writeOffset;
  MLint32 pad; /* structure must be multiple of 8 bytes in length! */
} MLfifo;


/* Size of the MLfifo structure in bytes
 */
/* #define ML_FIFO_HEADER_SIZE sizeof(MLfifo) */
#define ML_FIFO_HEADER_SIZE 256


/* Enumerate flags
 */

enum mliFifoStatusEnum {
  ML_FIFO_STATUS_ALLOCED    = 0x00000001,
  ML_FIFO_STATUS_FREE       = 0x00000000,
  ML_FIFO_STATUS_ALLOC_MASK = 0x00000001,
  ML_FIFO_STATUS_OPENED     = 0x00000002,
  ML_FIFO_STATUS_CLOSED     = 0x00000000,
  ML_FIFO_STATUS_OPEN_MASK  = 0x00000002,
  ML_FIFO_STATUS_LOCKED     = 0x00000004,
  ML_FIFO_STATUS_UNLOCKED   = 0x00000000,
  ML_FIFO_STATUS_LOCK_MASK  = 0x00000004,
  ML_FIFO_STATUS_MAPPED     = 0x00000008,
  ML_FIFO_STATUS_UNMAPPED   = 0x00000000,
  ML_FIFO_STATUS_MAP_MASK   = 0x00000008
};


/* Prototype internal API
 */

#ifndef	_KERNEL
MLstatus MLAPI _mlDIFifoCreate( MLfifo** rb,MLint32 size );
MLstatus MLAPI _mlDIFifoOpen( MLfifo* rb,MLint32 size );
MLstatus MLAPI _mlDIFifoDestroy( MLfifo* rb );
MLstatus MLAPI _mlDIFifoClose( MLfifo* rb );
MLint32  MLAPI _mlDIFifoHasSpace( MLfifo* rb,MLint32 size );
void*    MLAPI _mlDIFifoPreWrite( MLfifo* rb, MLint32 maxSize );
MLstatus MLAPI _mlDIFifoPostWrite( MLfifo* rb, void* pWrite );
void*    MLAPI _mlDIFifoPush( MLfifo* rb, void* data, MLint32 size,
			      MLint32 maxSize );
MLstatus MLAPI _mlDIFifoTop( MLfifo* rb,void** data );
MLint32  MLAPI _mlDIFifoOffsetTo( MLfifo* rb, void* data );
void*	 MLAPI _mlDIFifoAtOffset( MLfifo* rb, MLint32 offset );
MLstatus MLAPI _mlDIFifoPop( MLfifo* rb,MLint32 size ); 
void*    MLAPI _mlDIFifoIterate( MLfifo* rb,void* previousEntry,
				 MLint32 entrySize );
MLstatus MLAPI _mlDIFifoLock( MLfifo* rb,MLint32 size );
MLstatus MLAPI _mlDIFifoUnlock( MLfifo* rb,MLint32 size );

#define  _mlDIFifoIsEmpty(rb) ((rb)->readOffset == (rb)->writeOffset)
#define  _mlDIFifoTotalSize(rb) ((rb)->hardEnd)
#endif	/* _KERNEL */
 

#ifdef __cplusplus 
}
#endif

#endif /* _ML_FIFO_H_ */
