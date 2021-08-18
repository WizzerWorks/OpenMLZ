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
#include <ML/mlqueue.h>
#include <ML/ml_private.h>

#ifdef ML_OS_UNIX
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <stdlib.h>
#ifdef ML_OS_LINUX
#include <malloc.h>
#endif
#endif

#ifdef ML_OS_NT
#define	memalign(alignment,size)	malloc(size)	/* FIXME */
#endif

#include <string.h>
#include <errno.h>

/* Align to n-byte boundary */
#define ML_ALIGN(x,n) ((x)+(n - (((MLintptr)(x)&0xffff) % n)));


/* ------------------------------------------------------------Debugging macros
 */
#ifdef NDEBUG
#define ML_DEBUG_QUEUE(x)
#else
#define ML_DEBUG_QUEUE(x) fprintf(stderr,"[mlqueue:%04d] %s\n", __LINE__, x );
#endif 

#define ML_Q_MAGIC_NUMBER 0x51005100

#define ML_CHECK_Q( pQueue ) \
  if( pQueue == NULL ) \
  { \
    /* ML_DEBUG_QUEUE("Error, queue pointer is NULL"); */ \
    return ML_STATUS_INVALID_ARGUMENT; \
  } \
  if( pQueue->magicNumber != ML_Q_MAGIC_NUMBER ) \
  { \
    /* ML_DEBUG_QUEUE("Error, corrupt queue pointer"); */\
    return ML_STATUS_INVALID_ARGUMENT; \
  }

#ifndef NDEBUG
#define	QUEUE_DEBUG(s)		if( getenv("ML_DEBUG_QUEUE") ){ s; }
#define	QUEUE_DEBUG_2(s)	{char *c;if((c=getenv("ML_DEBUG_QUEUE"))&&\
				      (atoi(c)>=2)){s;}}
#define	QUEUE_DEBUG_3(s)	{char *c;if((c=getenv("ML_DEBUG_QUEUE"))&&\
				      (atoi(c)>=3)){s;}}
void _mlDIQueueDebugStop(void) {}
#else
#define	QUEUE_DEBUG(s)
#define	QUEUE_DEBUG_2(s)
#define	QUEUE_DEBUG_3(s)
#define _mlDIQueueDebugStop()
#endif


/* -------------------------------------------------------_mlDIQueueStateToText
 */
#define CASE_ENUM_TO_TEXT( value ) case value: return #value;
char* MLAPI _mlDIQueueStateToText( enum mlQueueEntryStateEnum state )
{
  switch ( state ) {
    CASE_ENUM_TO_TEXT( ML_ENTRY_EMPTY )

    CASE_ENUM_TO_TEXT( ML_ENTRY_NEW )
    CASE_ENUM_TO_TEXT( ML_ENTRY_NEW_MSG )
    CASE_ENUM_TO_TEXT( ML_ENTRY_NEW_DD_EVENT )

    CASE_ENUM_TO_TEXT( ML_ENTRY_SENT_TO_DEVICE )
    CASE_ENUM_TO_TEXT( ML_ENTRY_MSG_SENT_TO_DEVICE )
    CASE_ENUM_TO_TEXT( ML_ENTRY_DD_EVENT_SENT_TO_DEVICE )

    CASE_ENUM_TO_TEXT( ML_ENTRY_READ_BY_DEVICE )
    CASE_ENUM_TO_TEXT( ML_ENTRY_MSG_READ_BY_DEVICE )
    CASE_ENUM_TO_TEXT( ML_ENTRY_DD_EVENT_READ_BY_DEVICE )

    CASE_ENUM_TO_TEXT( ML_ENTRY_QUEUED_AT_DEVICE )
    CASE_ENUM_TO_TEXT( ML_ENTRY_MSG_QUEUED_AT_DEVICE )
    CASE_ENUM_TO_TEXT( ML_ENTRY_DD_EVENT_QUEUED_AT_DEVICE )

    CASE_ENUM_TO_TEXT( ML_ENTRY_PROCESSED_BY_DEVICE )
    CASE_ENUM_TO_TEXT( ML_ENTRY_MSG_PROCESSED_BY_DEVICE )
    CASE_ENUM_TO_TEXT( ML_ENTRY_DD_EVENT_PROCESSED_BY_DEVICE )

    CASE_ENUM_TO_TEXT( ML_ENTRY_NEW_EVENT )
    CASE_ENUM_TO_TEXT( ML_ENTRY_SENT_TO_APP )
    CASE_ENUM_TO_TEXT( ML_ENTRY_MSG_SENT_TO_APP )
    CASE_ENUM_TO_TEXT( ML_ENTRY_EVENT_SENT_TO_APP )

    CASE_ENUM_TO_TEXT( ML_ENTRY_READ_BY_APP )
    CASE_ENUM_TO_TEXT( ML_ENTRY_MSG_READ_BY_APP )
    CASE_ENUM_TO_TEXT( ML_ENTRY_EVENT_READ_BY_APP )

    CASE_ENUM_TO_TEXT( ML_ENTRY_DESTROYED )
  }
  return "???";
}


/* --------------------------------------------------------_mlDIQueueTypeToText
 */
char* MLAPI _mlDIQueueTypeToText( enum mlMessageTypeEnum type )
{
  static char static_buffer[100];

  switch ( type ) {
    CASE_ENUM_TO_TEXT( ML_MESSAGE_INVALID )

    CASE_ENUM_TO_TEXT( ML_BUFFERS_COMPLETE )
    CASE_ENUM_TO_TEXT( ML_BUFFERS_ABORTED )
    CASE_ENUM_TO_TEXT( ML_BUFFERS_FAILED )

    CASE_ENUM_TO_TEXT( ML_CONTROLS_COMPLETE )
    CASE_ENUM_TO_TEXT( ML_CONTROLS_ABORTED )
    CASE_ENUM_TO_TEXT( ML_CONTROLS_FAILED )

    CASE_ENUM_TO_TEXT( ML_QUERY_CONTROLS_COMPLETE )
    CASE_ENUM_TO_TEXT( ML_QUERY_CONTROLS_ABORTED )
    CASE_ENUM_TO_TEXT( ML_QUERY_CONTROLS_FAILED )

    CASE_ENUM_TO_TEXT( ML_EVENT_QUEUE_OVERFLOW )

    CASE_ENUM_TO_TEXT( ML_EVENT_DEVICE_INFO )
    CASE_ENUM_TO_TEXT( ML_EVENT_DEVICE_ERROR )
    CASE_ENUM_TO_TEXT( ML_EVENT_DEVICE_UNAVAILABLE )

    CASE_ENUM_TO_TEXT( ML_EVENT_VIDEO_SEQUENCE_LOST )
    CASE_ENUM_TO_TEXT( ML_EVENT_VIDEO_SYNC_GAINED )
    CASE_ENUM_TO_TEXT( ML_EVENT_VIDEO_SYNC_LOST )
    CASE_ENUM_TO_TEXT( ML_EVENT_VIDEO_VERTICAL_RETRACE )
    CASE_ENUM_TO_TEXT( ML_EVENT_VIDEO_SIGNAL_GAINED )
    CASE_ENUM_TO_TEXT( ML_EVENT_VIDEO_SIGNAL_LOST )

    CASE_ENUM_TO_TEXT( ML_EVENT_XCODE_FAILED )

    CASE_ENUM_TO_TEXT( ML_EVENT_AUDIO_SEQUENCE_LOST )
    CASE_ENUM_TO_TEXT( ML_EVENT_AUDIO_SAMPLE_RATE_CHANGED )

    CASE_ENUM_TO_TEXT( ML_REDUNDANT_MESSAGE )
    CASE_ENUM_TO_TEXT( ML_BUFFERS_IN_PROGRESS )
    CASE_ENUM_TO_TEXT( ML_CONTROLS_IN_PROGRESS )
    CASE_ENUM_TO_TEXT( ML_QUERY_IN_PROGRESS )
  }

  if ( ML_DD_EVENT <= type && type < (ML_DD_EVENT+255) ) {
    snprintf( static_buffer, sizeof( static_buffer ),
	      "MLDI_DD_EVENT_TYPE(%d)", type - ML_DD_EVENT );
  } else {
    snprintf( static_buffer, sizeof( static_buffer ),
	      "entry type 0x%x?", type );
  }
  return static_buffer;
}


/* ---------------------------------------------------------Debugging utilities
 */

#ifndef	NDEBUG

static void _mlDIQueuePrintSendRecvFifo( const char* prefix,
					 MLqueueRec * q,
					 MLfifo * fifo )
{
  MLqueueEntry*entry = NULL; /* Need null to kick-start the iterate routine. */

  for ( ;; ) {
    entry = (MLqueueEntry*)
      _mlDIFifoIterate( fifo, (void*) entry, sizeof( MLqueueEntry ) );
    fprintf( stderr, "%s: entry   : 0x%p\n", prefix, entry );

    if ( entry == NULL ) {
      fprintf( stderr, "\n" );
      break;
    }

    fprintf( stderr, "%s: state   : 0x%08x %s\n", prefix, entry->state,
	     _mlDIQueueStateToText( entry->state ) );

    fprintf( stderr, "%s: type    : 0x%08x %s\n", prefix, entry->messageType, 
	     _mlDIQueueTypeToText( entry->messageType ) );

    fprintf( stderr, "%s: di start: 0x%08x (0x%p)\n", prefix,
	     entry->payloadDataByteOffset,
	     (char*) q_message( q ) + ML_FIFO_HEADER_SIZE +
	     entry->payloadDataByteOffset );

    fprintf( stderr, "%s: di size : 0x%08x (%d)\n", prefix,
	     entry->payloadSize, entry->payloadSize );

    fprintf( stderr, "%s: dd start: 0x%08x (0x%p)\n", prefix,
	     entry->ddPayloadDataByteOffset,
	     (char*) q_message(q) + ML_FIFO_HEADER_SIZE +
	     entry->ddPayloadDataByteOffset );

    fprintf( stderr, "%s: dd size : 0x%08x (%d)\n", prefix,
	     entry->ddPayloadSize, entry->ddPayloadSize );
    fprintf( stderr,"\n" );
  }
}

#define	PRINT_QUEUE(msg,q) \
  { char *s; if ( (s = getenv( "ML_DEBUG_QUEUE" )) && (atoi( s ) > 1) ) {\
    _mlDIQueuePrintDebug( msg, __LINE__, q ); }}

MLstatus _mlDIQueuePrintDebug( char* msg, int line, MLqueueRec* q )
{
  fprintf( stderr, "[mlqueue:%04d]   ==== %s ====\n", line, msg );
  _mlDIQueuePrintSendRecvFifo( "[send q]", q, q_send( q ) );
  _mlDIQueuePrintSendRecvFifo( "[recv q]", q, q_receive( q ) );

  _mlDIQueueDebugStop();

  return ML_STATUS_NO_ERROR;
}

#else
#define	PRINT_QUEUE(msg,q)
#endif /* #ifdef NDEBUG ... #else ... */


/* ---------------------------------------------------------------mlDIQueueSize
 *
 * Interpret queue options to compute queue size (in bytes).
 */
MLstatus MLAPI mlDIQueueSize( MLqueueOptions* pOpt, MLint32* reqdSize )
{
  if ( pOpt == NULL ) {
    ML_DEBUG_QUEUE( "Queue options pointer may not be NULL" );
    return ML_STATUS_INVALID_ARGUMENT;
  }

  return _mlDIQueueSize( pOpt, NULL, reqdSize );
}


/* -------------------------------------------------------------mlDIQueueCreate
 *
 * Initialize the queue and interpret open options
 */
MLstatus MLAPI mlDIQueueCreate( MLbyte* preallocSpace,
				MLint32 preallocSize,
				MLqueueOptions* pOpt,
				MLqueueRec** retpQueue )
{
  MLqueueRec newQueue, *pQueue;

#ifndef NDEBUG
  MLint32 zero = 0; /* Use extra variable to quiet compiler warning */
  mlAssert( sizeof( MLqueueRec) % 8 == zero );
  mlAssert( sizeof( MLqueueEntry) % 8 == zero );
  mlAssert( ML_FIFO_HEADER_SIZE % 8 == zero );
#endif

  if ( pOpt == NULL ) {
    ML_DEBUG_QUEUE( " Queue options pointer may not be NULL" );
    return ML_STATUS_INVALID_ARGUMENT;
  }

  if ( retpQueue == NULL ) {
    ML_DEBUG_QUEUE( "Queue pointer may not be NULL" );
    return ML_STATUS_INVALID_ARGUMENT;
  }

  /* Insist on at least 64-bit alignment
   */
  if ( pOpt->ddAlignment < 8 ) {
    pOpt->ddAlignment = 8;
  }

  /* Go get queue sizes
   */
  _mlDIQueueSize( pOpt, &newQueue, NULL );

  /* Have we been handed space to use?
   */
  if ( preallocSpace == NULL ) {
    /* If not, we need to allocate our own space
     */
    preallocSpace = (MLbyte*)memalign( pOpt->ddAlignment, newQueue.queueSize );
    if ( preallocSpace == NULL ) {
      return ML_STATUS_OUT_OF_MEMORY;
    }
  } else {
    /* If os, space is already allocated, just confirm size
     */
    if ( preallocSize < newQueue.queueSize ) {
      ML_DEBUG_QUEUE( "Preallocated queue space is too small" );
      return ML_STATUS_INVALID_ARGUMENT;
    }
    if ( ((MLintptr) preallocSpace % 8) != 0 ) {
      ML_DEBUG_QUEUE( "Preallocated queue is not 8-byte aligned" );
      return ML_STATUS_INVALID_ARGUMENT;
    }
  }

  /* Initialize static counts
   */
  newQueue.sendSignalCount   = pOpt->sendSignalCount;
  newQueue.sendMaxCount      = pOpt->sendMaxCount;
  newQueue.inTransitMaxCount = pOpt->receiveMaxCount;
  newQueue.eventMaxCount     = pOpt->eventMaxCount;
  newQueue.receiveMaxCount   = pOpt->receiveMaxCount + pOpt->eventMaxCount;

  /* Set fifo offsets, taking alignment into account
   */
  {
    MLbyte* currentAddress, *alignedAddress;

    newQueue.sendOffset    = sizeof( MLqueueRec );
    newQueue.receiveOffset = newQueue.sendOffset+(ML_FIFO_HEADER_SIZE+
						  newQueue.sendFifoSize);
    newQueue.eventOffset   = newQueue.receiveOffset + (ML_FIFO_HEADER_SIZE +
						     newQueue.receiveFifoSize);
    /* Correct eventOffset to handle desired alignment
     */
    currentAddress = preallocSpace + newQueue.eventOffset;
    alignedAddress = ML_ALIGN( currentAddress, pOpt->ddAlignment );
    newQueue.eventOffset += (alignedAddress - currentAddress);

    newQueue.messageOffset = newQueue.eventOffset + (ML_FIFO_HEADER_SIZE +
						     newQueue.eventFifoSize);
    /* Correct messageOffset to handle desired alignment
     */
    currentAddress = preallocSpace + newQueue.messageOffset;
    alignedAddress = ML_ALIGN( currentAddress, pOpt->ddAlignment );
    newQueue.messageOffset += (alignedAddress - currentAddress);

    /* Check all our calculations worked out correctly
     */
    mlAssert( newQueue.sendOffset % 8 == 0 );
    mlAssert( newQueue.receiveOffset % 8 == 0 );
    mlAssert( newQueue.eventOffset % 8 == 0 );
    mlAssert( newQueue.messageOffset % 8 == 0 );

    mlAssert( (newQueue.eventOffset+(MLintptr)preallocSpace) %
	      pOpt->ddAlignment == 0 );
    mlAssert( (newQueue.messageOffset+(MLintptr)preallocSpace) %
	      pOpt->ddAlignment == 0 );

#ifndef NDEBUG
    {
      MLint32 endOffset;
      endOffset = newQueue.messageOffset +  (ML_FIFO_HEADER_SIZE +
					     newQueue.messageFifoSize);
      mlAssert( endOffset <= newQueue.queueSize );
    }
#endif
  }

  /* Initialize queue counts
   */
  newQueue.sendCount = 0;
  newQueue.receiveReplyCount = 0;
  newQueue.receiveEventCount = 0;
  newQueue.inTransitCount = 0;
  newQueue.eventCount = 0;
  newQueue.inTransitCount = 0;
  newQueue.dbgCount = 0;
  newQueue.pipesPresent = ( pOpt->noPipeFds == 0 );

  /* Copy our new queue record to the top of the queue space.
   */
  *((MLqueueRec*) preallocSpace) = newQueue;

  /* And set the return queue locaton
   */
  pQueue = (*retpQueue) = ((MLqueueRec*) preallocSpace);

  /* Init fifo headers
   */
  _mlDIFifoOpen( q_send( pQueue ),    pQueue->sendFifoSize );
  _mlDIFifoOpen( q_receive( pQueue ), pQueue->receiveFifoSize );
  _mlDIFifoOpen( q_message( pQueue ), pQueue->messageFifoSize );
  _mlDIFifoOpen( q_event( pQueue ),   pQueue->eventFifoSize );

  pQueue->magicNumber = ML_Q_MAGIC_NUMBER;

  if ( pOpt->noPipeFds == 0 ) {

#ifdef ML_OS_UNIX
    if ( -1 == pipe( pQueue->sendpipe ) || 
	 -1 == pipe( pQueue->receivepipe ) ||
	 -1 == pipe( pQueue->devicepipe ) ) {
      ML_DEBUG_QUEUE( "Error - unable to create pipes" );
      mlDIQueueDestroy( pQueue );
      return ML_STATUS_INSUFFICIENT_RESOURCES;
    }
    pQueue->sendWaitable = pQueue->sendpipe[0];
    pQueue->receiveWaitable = pQueue->receivepipe[0];
    pQueue->deviceWaitable = pQueue->devicepipe[0];

    /* From the manpage (Corresponds to OpenML 1.0 Spec)
     * mlDIQueueGetSendWaitable returns a ML waitable (a file
     * descriptor in Unix implementations).  This will fire whenever
     * there are more than N slots free in the queue between
     * application and device.  It is used by applications which wish
     * to enqueue messages in chunks.  The setting for N is specified
     * at open time via the ML_OPEN_SEND_SIGNAL_COUNT parameter.
     */

    /* Prime the send queue waitable, Write one char for each spot in
     * the queue that the signal should be active (defined as the
     * "SIGNAL COUNT").  Later, we'll read one char each time a slot
     * is filled.  When the pipe is empty, the queue is then full
     * beyond the SIGNAL COUNT and the send signal becomes inactive.
     */
    {
      MLint32 i;
      for ( i = 0; i < pQueue->sendSignalCount; i++ ) {
	ssize_t ret = write( pQueue->sendpipe[1], "S", 1 );
	mlAssert( ret == 1 );
      }
    }
#endif

#ifdef ML_OS_NT
    if ( NULL == (pQueue->sendWaitable =
		  CreateEvent( NULL, TRUE, FALSE, NULL )) ||
	 NULL == (pQueue->receiveWaitable =
		  CreateEvent( NULL, TRUE, FALSE, NULL )) ||
	 NULL == (pQueue->deviceWaitable =
		  CreateEvent( NULL, TRUE, FALSE, NULL )) ) {
      ML_DEBUG_QUEUE( "Error - unable to create pipes" );
      mlDIQueueDestroy( pQueue );
      return ML_STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Prime the send queue waitable.
     */
    if ( pQueue->sendMaxCount - pQueue->sendSignalCount > 0 ) {
      SetEvent( pQueue->sendWaitable );
    }
#endif

  } else {
#ifdef ML_OS_NT
    pQueue->sendWaitable = INVALID_HANDLE_VALUE;
    pQueue->receiveWaitable = INVALID_HANDLE_VALUE;
    pQueue->deviceWaitable = INVALID_HANDLE_VALUE;
#else	/* ML_OS_UNIX */
    pQueue->sendWaitable = -1;
    pQueue->receiveWaitable = -1;
    pQueue->deviceWaitable = -1;
#endif
  }

  return ML_STATUS_NO_ERROR;
}


/* ------------------------------------------------------------mlDIQueueDestroy
 *
 * Undo the effects of queue allocation
 */
MLstatus MLAPI mlDIQueueDestroy( MLqueueRec* q )
{
  ML_CHECK_Q( q );

  if ( q->pipesPresent ) {
#ifdef ML_OS_UNIX
    close( q->sendpipe[0] );
    close( q->sendpipe[1] );
    close( q->receivepipe[0] );
    close( q->receivepipe[1] );
    close( q->devicepipe[0] );
    close( q->devicepipe[1] );
#endif

#ifdef ML_OS_NT
    CloseHandle( q->sendWaitable );
    CloseHandle( q->receiveWaitable );
    CloseHandle( q->deviceWaitable );
#endif
  }

  free( q );
  return ML_STATUS_NO_ERROR;
}


/* --------------------------------------------------------mlDIQueuePushMessage
 *
 * Copy message contents into the payload area, push header onto the
 * send fifo and update counts/waitables
 */
MLstatus MLAPI mlDIQueuePushMessage( MLqueueRec* q, 
				     enum mlMessageTypeEnum messageType,
				     MLpv* message,
				     MLbyte* ddData, 
				     MLint32 ddDataSize, 
				     MLint32 ddDataAlignment )
{
  return mlDIQueuePushMessageRet( q, messageType, message, ddData,
				  ddDataSize, ddDataAlignment, NULL );
}


/* -----------------------------------------------------mlDIQueuePushMessageRet
 *
 * Copy message contents into the payload area, push header onto the
 * send fifo and update counts/waitables
 */
MLstatus MLAPI mlDIQueuePushMessageRet( MLqueueRec* q, 
					enum mlMessageTypeEnum messageType,
					MLpv* message,
					MLbyte* ddData, 
					MLint32 ddDataSize, 
					MLint32 ddDataAlignment,
					MLqueueEntry **retEntry )
{
  MLint32 nBytes;
  MLqueueEntry newEntry, *entryStart;
  MLbyte *msgStart;
  MLstatus status;

  ML_CHECK_Q( q );

  mlAssert( (ddData == NULL && ddDataSize == 0) ||
	    (ddData != NULL && ddDataSize > 0) );

  /* Create a header for the new queue entry
   */
  memset( &newEntry, 0, sizeof( newEntry ) );
  newEntry.state = ML_ENTRY_NEW_MSG;
  newEntry.messageType = messageType;

  /* Conservatively calculate number of bytes required
   */
  mlPvSizes( message, NULL, &nBytes );
  newEntry.payloadSize = nBytes;
  if ( ddData != NULL ) {
    newEntry.payloadSize += (ddDataSize + ddDataAlignment);
  }

  if ( (status = _mlDIQueueCheckMessageFits( q, newEntry.payloadSize )) ) {
    return status;
  }

  /* Reserve space on the message (payload) fifo
   */
  msgStart = _mlDIFifoPush( q_message( q ), NULL, 0, newEntry.payloadSize );
  if ( NULL == msgStart ) {
    return ML_STATUS_SEND_QUEUE_OVERFLOW;
  }

  /* Copy the message into that reserved payload space
   */
  mlPvCopy( message, msgStart, nBytes );
  newEntry.payloadDataByteOffset =
    _mlDIFifoOffsetTo( q_message( q ), msgStart );
  msgStart += nBytes;

  /* Copy dd private data into that reserved space (Or just reserve it
   * if the address is NULL but the size specified)
   */
  if ( ddData != NULL || ddDataSize != 0 ) {
    MLint32 offset = (MLint32) ((MLintptr) msgStart&0xffff) % ddDataAlignment;
    if ( offset ) {
      msgStart += (ddDataAlignment - offset);
    }
    if ( ddData ) {
      memcpy(msgStart, ddData, ddDataSize);
    }
    newEntry.ddPayloadSize = ddDataSize;
    newEntry.ddPayloadDataByteOffset =
      _mlDIFifoOffsetTo( q_message( q ), msgStart );

  } else {
    newEntry.ddPayloadSize = 0;
    newEntry.ddPayloadDataByteOffset = -1;
  }

  /* Message is safely on the queue, so now enqueue the message header
   */
  status = _mlDIQueuePushMessageHeaderRet( q, &newEntry, &entryStart );

  PRINT_QUEUE( "mlDIQueuePushMessageRet exit", q );

  if ( retEntry ) {
    *retEntry = entryStart;
  }
  return status;
}


/* ----------------------------------------------------mlDIQueueSendDeviceEvent
 *
 * Send a device-dependent event message on the queue to the device.
 * push header onto the send fifo and update counts/waitables
 *
 * This message is consumed by the device, and not returned.
 */
MLstatus MLAPI mlDIQueueSendDeviceEvent( MLqueueRec* q, 
					 enum mlMessageTypeEnum messageType )
{
  MLqueueEntry newEntry;
  MLstatus status;

  ML_CHECK_Q( q );

  /* Create a header for the new queue entry
   */
  memset( &newEntry, 0, sizeof( newEntry ) );
  newEntry.state = ML_ENTRY_NEW_DD_EVENT;
  newEntry.messageType = messageType;
  newEntry.payloadDataByteOffset = -1;
  newEntry.payloadSize = 0;
  newEntry.ddPayloadDataByteOffset = -1;
  newEntry.ddPayloadSize = 0;

  status = _mlDIQueuePushMessageHeader( q, &newEntry );

  PRINT_QUEUE( "mlDIQueueSendDeviceEvent exit", q );

  return status;
}


/* ---------------------------------------------------mlDIQueueNextMessageState
 *
 * Read the oldest unread message from application to device so that
 * we may begin processing it.
 */
MLstatus MLAPI mlDIQueueNextMessageState( MLqueueRec* q, 
					  MLqueueEntry** entry,
					  enum mlMessageTypeEnum *messageType,
					  MLpv** message,
					  MLbyte** ddData,
					  MLint32* ddDataSize,
					  enum mlQueueEntryStateEnum dstate,
					  MLint32 advanceState )
{
  MLstatus status;

  PRINT_QUEUE( "mlDIQueueNextMessage entry", q );
  ML_CHECK_Q( q );

  if ( messageType == NULL ) {
    return ML_STATUS_INVALID_ARGUMENT;
  }

  status = _mlDIQueueNextMessageHeaderState( q, entry, dstate, advanceState );
  if ( status ) {
    *messageType = ML_MESSAGE_INVALID;
    if ( message != NULL ) {
      *message = NULL;
    }
    return status;
  }

  *messageType = (*entry)->messageType;

  if ( message != NULL ) {
    if ( (*entry)->payloadSize > 0 ) {
      *message = _mlDIFifoAtOffset( q_message( q ), 
				   (*entry)->payloadDataByteOffset );
    } else {
      *message = NULL;
    }
  }
      
  if ( ddData != NULL ) {
    if ( (*entry)->ddPayloadSize > 0 ) {
      *ddData =  _mlDIFifoAtOffset( q_message( q ), 
				    (*entry)->ddPayloadDataByteOffset );
    } else {
      *ddData =  NULL;
    }
  }

  if ( ddDataSize != NULL ) {
    *ddDataSize = (*entry)->ddPayloadSize;
  }

  PRINT_QUEUE( "mlDIQueueNextMessage exit", q );

  return ML_STATUS_NO_ERROR;
}


/* --------------------------------------------------------mlDIQueueNextMessage
 */
MLstatus MLAPI mlDIQueueNextMessage( MLqueueRec* q, 
				     MLqueueEntry** entry,
				     enum mlMessageTypeEnum *msgType,
				     MLpv** msg,
				     MLbyte** ddData,
				     MLint32* ddDataSize )
{
  return mlDIQueueNextMessageState( q, 
				    entry,
				    msgType,
				    msg,
				    ddData,
				    ddDataSize,
				    ML_ENTRY_READ_BY_DEVICE,
				    1 );
}


/* ------------------------------------------------------mlDIQueueUpdateMessage
 *
 * Update a message after it has been processed, updating its message
 * type.
 */
MLstatus MLAPI mlDIQueueUpdateMessage( MLqueueEntry* qentry, 
				       enum mlMessageTypeEnum newMessageType )
{
  switch ( qentry->state ) {
  case ML_ENTRY_MSG_SENT_TO_DEVICE:
  case ML_ENTRY_MSG_READ_BY_DEVICE:
  case ML_ENTRY_MSG_QUEUED_AT_DEVICE:
    qentry->messageType = newMessageType;
    qentry->state = ML_ENTRY_MSG_PROCESSED_BY_DEVICE;
    break;

  case ML_ENTRY_DD_EVENT_SENT_TO_DEVICE:
  case ML_ENTRY_DD_EVENT_READ_BY_DEVICE:
  case ML_ENTRY_DD_EVENT_QUEUED_AT_DEVICE:
    qentry->messageType = newMessageType;
    qentry->state = ML_ENTRY_DD_EVENT_PROCESSED_BY_DEVICE;
    break;

  default: {
    QUEUE_DEBUG( fprintf( stderr,
			  "[mlqueue:%04d] invalid qentry 0x%p: %s %s\n",
			  __LINE__, qentry,
			  _mlDIQueueStateToText( qentry->state ),
			  _mlDIQueueTypeToText( qentry->messageType ) ) );
    return ML_STATUS_INTERNAL_ERROR;
  }
  } /* switch quentry-state */

  QUEUE_DEBUG( fprintf( stderr,
			"[mlqueue:%04d] update state 0x%p %s %s\n",
			__LINE__, qentry,
			_mlDIQueueStateToText( qentry->state ),
			_mlDIQueueTypeToText( qentry->messageType ) ) );
  return ML_STATUS_NO_ERROR;
}

  
/* ----------------------------------------------------mlDIQueueAdvanceMessages
 *
 * Return any processed messages - popping them off the fifo going to
 * the device, and pushing them onto the fifo going back to the
 * application.
 */
MLstatus MLAPI mlDIQueueAdvanceMessages( MLqueueRec* q )
{
  MLstatus status;

  ML_CHECK_Q( q );

  status = _mlDIQueueAdvanceMessageHeaders( q );

#ifndef NDEBUG
  QUEUE_DEBUG(
  { char *s = getenv( "ML_DEBUG_QUEUE" );
  if ( status != ML_STATUS_NO_ERROR ) {
    fprintf( stderr, "[mlqueue:%04d] advance messages failed: %s\n",
	     __LINE__, mlStatusName( status ) );
  }
  if ( status || atoi( s ) > 1 ) {
    _mlDIQueuePrintDebug( "mlDIQueueAdvanceMessages exit",__LINE__,q);
  }
  });
#endif

  return status;
}


/* --------------------------------------------------------mlDIQueueReturnEvent
 *
 * Add a event message to the event payload and push the corresponding
 * entry onto the receive queue back to the application.
 */
MLstatus MLAPI mlDIQueueReturnEvent( MLqueueRec* q, 
				     enum mlMessageTypeEnum messageType,
				     MLpv* eventMsg )
{
  MLqueueEntry newEntry;
  MLint32 nBytes;
  MLpv emptyMessage;
  void* eventPayload;

  ML_CHECK_Q( q );

  /* Check if there is space for this message on the payload.
   */
  mlPvSizes( eventMsg, NULL, &nBytes );

  /* Create a header for the new queue entry    
   */
  memset( &newEntry, 0, sizeof( newEntry ) );
  newEntry.messageType = messageType;
  newEntry.state = ML_ENTRY_NEW_EVENT;

  if ( q->receiveEventCount >= q->receiveMaxCount ) {
    /* This reply is for an event, but events have been disabled due
     * to queue overlow, so silently ignore the event
     */
    return ML_STATUS_NO_ERROR;
  }

  /* Make sure there is enough space for this new event header Must
   * leave enough free space for every message in-transit.
   */
  if ( q->receiveEventCount >= q->receiveMaxCount - q->inTransitMaxCount ) {
    ML_DEBUG_QUEUE( " Warning - event queue is full, suspending events" );

    /* We're about to run out of space for events.  Advise app we're
     * dropping events from now on.
     */
    emptyMessage.param = ML_END;
    eventMsg = &emptyMessage;
    nBytes = sizeof( emptyMessage );
    newEntry.messageType = ML_EVENT_QUEUE_OVERFLOW;

    /* Artificially inflate the receive event count to prevent any
     * more event messages from being enqueued.
     */
    q->receiveEventCount += q->receiveMaxCount;
  }

  /* Now handle the event payload.
   *
   * Assumption - events do not contain arrays (so a straight copy
   * works, we don't need to mlPVCopy).
   */
  if ( ! _mlDIFifoHasSpace( q_event( q ), nBytes ) ) {
    ML_DEBUG_QUEUE( "Error - event payload is full" );
    ML_DEBUG_QUEUE( "        This shouldn't happen - event payload" );
    ML_DEBUG_QUEUE( "        must have been sized incorrectly." );
    return ML_STATUS_INTERNAL_ERROR;
  }

  eventPayload = _mlDIFifoPush( q_event( q ), eventMsg, nBytes, nBytes );
  if ( eventPayload == NULL ) {
    ML_DEBUG_QUEUE( "Error - unable to place event in event payload." );
    ML_DEBUG_QUEUE( "        This shouldn't happen - are you sending" );
    ML_DEBUG_QUEUE( "        events/replies from more than one thread?" );
    return ML_STATUS_INTERNAL_ERROR;
  }
  newEntry.payloadDataByteOffset =
    _mlDIFifoOffsetTo( q_event( q ),eventPayload );
  newEntry.payloadSize = nBytes;

  /* Push the header onto the Receive queue 
   */
  return _mlDIQueuePushReplyHeader( q, &newEntry );
}


/* ------------------------------------------------------mlDIQueueAbortMessages
 *
 * Abort any messages remaining in the send queue, popping them and
 * pushing the aborted replies onto the receive queue.
 */
MLstatus MLAPI mlDIQueueAbortMessages( MLqueueRec* q )
{
  MLstatus status; 
  MLqueueEntry* entry;
  enum mlMessageTypeEnum messageType;
  MLpv* message;

  ML_CHECK_Q( q );
  QUEUE_DEBUG( fprintf( stderr, "mlDIQueueAbortMessages" ) );

  status = mlDIQueueNextMessageState( q, &entry, &messageType, &message,
				      NULL, NULL,
				      ML_ENTRY_DD_EVENT_QUEUED_AT_DEVICE, 0 );

  while ( status == ML_STATUS_NO_ERROR) {
    QUEUE_DEBUG( if ( entry ) {
      fprintf( stderr, "[mlqueue:%04d] abort msg    0x%p %s %s\n",
	       __LINE__, entry,	_mlDIQueueStateToText( entry->state ),
	       _mlDIQueueTypeToText( entry->messageType ) ); } );

    if ( entry->state == ML_ENTRY_MSG_READ_BY_DEVICE ||
	 entry->state == ML_ENTRY_QUEUED_AT_DEVICE ||
	 entry->state == ML_ENTRY_MSG_SENT_TO_DEVICE ) {

      switch ( messageType ) {
      case ML_BUFFERS_IN_PROGRESS:
	mlDIQueueUpdateMessage( entry, ML_BUFFERS_ABORTED );
	break;
      case ML_CONTROLS_IN_PROGRESS:
	mlDIQueueUpdateMessage( entry, ML_CONTROLS_ABORTED );
	break;
      case ML_QUERY_IN_PROGRESS:
	mlDIQueueUpdateMessage( entry, ML_QUERY_CONTROLS_ABORTED );
	break;
      default: {
	char s[200];
	snprintf( s, sizeof( s ),
		  "[mlDIQueueAbortMessages] Unexpected msg type %s\n",
		  _mlDIQueueTypeToText( messageType ) );
	ML_DEBUG_QUEUE( s );
	return ML_STATUS_INTERNAL_ERROR;
      }
      }

    } else if ( entry->state == ML_ENTRY_DD_EVENT_SENT_TO_DEVICE ) {
      entry->state = ML_ENTRY_DD_EVENT_PROCESSED_BY_DEVICE;

    } else {
      ML_DEBUG_QUEUE( "internal error - illegal qentry state" );
      return ML_STATUS_INTERNAL_ERROR;
    }
    status = mlDIQueueNextMessageState( q, &entry, &messageType, &message, 
					NULL, NULL,
					ML_ENTRY_DD_EVENT_QUEUED_AT_DEVICE, 0);
  }

  return mlDIQueueAdvanceMessages( q );
}


/* -----------------------------------------------------mlDIQueueReceiveMessage
 *
 * Return the oldest message on the receive queue.
 */
MLstatus MLAPI mlDIQueueReceiveMessage( MLqueueRec* q, 
					enum mlMessageTypeEnum *messageType,
					MLpv** message,
					MLbyte** ddData,
					MLint32* ddDataSize )
{
  MLqueueEntry* entry;
  MLstatus status;

  PRINT_QUEUE( "mlDIQueueReceiveMessage entry", q );

  ML_CHECK_Q( q );
  status = _mlDIQueueReadReplyHeader( q, &entry );

  if ( status ) {
    return status;
  }

  *messageType = entry->messageType;
  if ( entry->state == ML_ENTRY_EVENT_READ_BY_APP ) {
    *message =
      _mlDIFifoAtOffset( q_event( q ), entry->payloadDataByteOffset );
    if ( ddData != NULL ) {
      if ( entry->ddPayloadDataByteOffset == -1 ) {
	*ddData = (MLbyte *)-1;
      }	else {
	*ddData =
	  _mlDIFifoAtOffset( q_event( q ), entry->ddPayloadDataByteOffset );
      }
    }

  } else { 
    *message =
      _mlDIFifoAtOffset( q_message( q ), entry->payloadDataByteOffset );
    if ( ddData != NULL ) {
      if ( entry->ddPayloadDataByteOffset == -1 ) {
	*ddData = (MLbyte *) -1;
      }	else {
	*ddData =
	  _mlDIFifoAtOffset( q_message( q ), entry->ddPayloadDataByteOffset );
      }
    }
  }

  if ( ddDataSize != NULL ) {
    *ddDataSize = entry->ddPayloadSize;
  }

  PRINT_QUEUE( "mlDIQueueReceiveMessage exit", q );

  return ML_STATUS_NO_ERROR;
}


/* ------------------------------------------------------------------get counts
 */
MLint32 MLAPI mlDIQueueGetSendCount( MLqueueRec* q )
{
  ML_CHECK_Q( q );
  return q->sendCount;
}

MLint32 MLAPI mlDIQueueGetReceiveCount( MLqueueRec* q )
{
  ML_CHECK_Q( q );
  return q->receiveReplyCount + q->receiveEventCount;
}

MLint32 MLAPI mlDIQueueGetReplyCount( MLqueueRec* q )
{
  ML_CHECK_Q( q );
  return q->receiveReplyCount;
}

MLint32 MLAPI mlDIQueueGetEventCount( MLqueueRec* q )
{
  ML_CHECK_Q( q );
  return q->receiveEventCount;
}


/* ---------------------------------------------------------------get waitables
 */
MLwaitable MLAPI mlDIQueueGetSendWaitable( MLqueueRec* q )
{
  if ( q==NULL ) {
    return 0;
  }
  return q->sendWaitable;
}

MLwaitable MLAPI mlDIQueueGetReceiveWaitable( MLqueueRec* q )
{
  if ( q==NULL ) {
    return 0;
  }
  return q->receiveWaitable;
}

MLwaitable MLAPI mlDIQueueGetDeviceWaitable( MLqueueRec* q )
{
  if ( q==NULL ) {
    return 0;
  }
  return q->deviceWaitable;
}


/* --------------------------------------------------------------_mlDIQueueSize
 *
 * Compute the size of the queues from the queue options.
 */
MLstatus MLAPI _mlDIQueueSize( MLqueueOptions* pOpt,
			       MLqueueRec* pQueue,
			       MLint32* reqdSize )
{
  MLint32 sendFifoSize, receiveFifoSize, eventFifoSize, messageFifoSize;
  MLint32 queueSize;

  sendFifoSize    = pOpt->sendMaxCount * (MLint32) sizeof( MLqueueEntry );
  receiveFifoSize = ( pOpt->receiveMaxCount + pOpt->eventMaxCount ) *
		    (MLint32) sizeof( MLqueueEntry );

  eventFifoSize   = pOpt->eventMaxCount * pOpt->ddEventSize;
  messageFifoSize = pOpt->messagePayloadSize + 
		    pOpt->sendMaxCount * pOpt->ddMessageSize;

  if ( pOpt->ddAlignment < 8 ) {
    pOpt->ddAlignment = 8;
  }

  /* When computing the queue size, add extra space for the queue
   * record, fifo headers and extra space so we have room to meet any
   * alignment constraints.
   */
  queueSize = ( (MLint32) sizeof( MLqueueRec ) +
		sendFifoSize + ML_FIFO_HEADER_SIZE +
		receiveFifoSize + ML_FIFO_HEADER_SIZE +
		eventFifoSize + ML_FIFO_HEADER_SIZE + pOpt->ddAlignment +
		messageFifoSize + ML_FIFO_HEADER_SIZE + pOpt->ddAlignment );

  if ( pQueue ) {
    pQueue->queueSize = queueSize;
    pQueue->sendFifoSize = sendFifoSize;
    pQueue->receiveFifoSize = receiveFifoSize;
    pQueue->eventFifoSize = eventFifoSize;
    pQueue->messageFifoSize = messageFifoSize;
  }
  
  if ( reqdSize ) {
    *reqdSize = queueSize;
  }

  return ML_STATUS_NO_ERROR;
}


/* --------------------------------------------------_mlDIQueueCheckMessageFits
 *
 * Check that there is space for a new message Does not modify the
 * queue.
 */
MLstatus MLAPI _mlDIQueueCheckMessageFits( MLqueueRec* q,
					   MLint32 payloadSpace )
{
  /* Is there space for another entry on the send fifo?
   */
  if ( ! _mlDIFifoHasSpace( q_send( q ), sizeof( MLqueueEntry ) ) ) {
    return ML_STATUS_SEND_QUEUE_OVERFLOW;
  }
  /* Is there space for the message payload?
   */
  if ( ! _mlDIFifoHasSpace( q_message( q ), payloadSpace ) ) {
    return ML_STATUS_SEND_QUEUE_OVERFLOW;
  }
  /* Are we below the maximum number of messages allowed to be
   * in-transit.  (This allows us to guarantee space on the receive
   * queue to hold the reply to this message).
   */
  if ( q->inTransitCount + 1 > q->inTransitMaxCount ) {
    return ML_STATUS_RECEIVE_QUEUE_OVERFLOW;
  }
  return ML_STATUS_NO_ERROR;
}


/* -------------------------------------------------_mlDIQueuePushMessageHeader
 *
 * Push a message header onto the send fifo 
 * and update counts/waitables
 */
MLstatus MLAPI _mlDIQueuePushMessageHeader( MLqueueRec* q,
					    MLqueueEntry * entry )
{
  return _mlDIQueuePushMessageHeaderRet( q, entry, NULL );
}


/* ----------------------------------------------_mlDIQueuePushMessageHeaderRet
 */
MLstatus MLAPI _mlDIQueuePushMessageHeaderRet( MLqueueRec* q,
					       MLqueueEntry *entry,
					       MLqueueEntry **retEntry )
{
  MLqueueEntry *entryStart;

  switch ( entry->state ) {
  case ML_ENTRY_NEW_MSG:
    entry->state = ML_ENTRY_MSG_SENT_TO_DEVICE;
    q->inTransitCount++;
    if ( q->inTransitCount > q->inTransitMaxCount ) {
      q->inTransitCount--;
      return ML_STATUS_RECEIVE_QUEUE_OVERFLOW;
    }
    break;

  case ML_ENTRY_NEW_DD_EVENT:
    entry->state = ML_ENTRY_DD_EVENT_SENT_TO_DEVICE;
    break;

  default:
#ifndef NDEBUG
    if ( getenv( "ML_DEBUG_QUEUE" ) ) {
      fprintf( stderr, "[mlDIQueue debug] _mlDIQueuePushMessageHeader "
	       "invalid entry state: %s\n",
	       _mlDIQueueStateToText( entry->state ) );
    }
#endif
    return ML_STATUS_INTERNAL_ERROR;
  }

  /* Push it onto the send queue.
   */
  entryStart = _mlDIFifoPush( q_send( q ), entry, sizeof( MLqueueEntry ),
			      sizeof( MLqueueEntry ) );
  if ( entryStart == NULL ) {
    q->inTransitCount--;
    return ML_STATUS_SEND_QUEUE_OVERFLOW;
  }
  q->sendCount++;

  /* Do following only if pipes (or NT events) are present
   */
  if ( q->pipesPresent ) {

#ifdef ML_OS_UNIX
    /* There's one fewer free spot in the send queue now - indicate by
     * removing one character from the send pipe.
     */
    {
      MLstatus status;
      fd_set fdset;
      struct timeval timeout;
      timeout.tv_sec = 0;
      timeout.tv_usec = 1000;

      FD_ZERO( &fdset );
      FD_SET( q->sendpipe[0], &fdset );
      if ( select( q->sendpipe[0] + 1, &fdset, NULL, NULL, &timeout ) == 1 ) {
	if ( (status = _mlDIQueuePipeHandler( q, Q_PIPE_READ, Q_PIPE_SEND)) ) {
	  return status;
	}
      }
      if ( (status = _mlDIQueuePipeHandler( q, Q_PIPE_WRITE, Q_PIPE_DEVICE))) {
	return status;
      }

#ifndef NDEBUG
      q->dbgCount++; 
      QUEUE_DEBUG( fprintf( stderr, "[mlqueue:%04d] ++count = %d\n",
			    __LINE__, q->dbgCount ) );
#endif /* NDEBUG */
    }
#endif /* ML_OS_UNIX */

#ifdef ML_OS_NT
    if ( q->sendMaxCount - q->sendCount <= q->sendSignalCount ) {
      if ( ! ResetEvent( q->sendWaitable ) ) {
	return ML_STATUS_INTERNAL_ERROR;
      }
    }
    if ( ! SetEvent( q->deviceWaitable ) ) {
      return ML_STATUS_INTERNAL_ERROR;
    }
#endif /* ML_OS_NT */
  }

  if ( retEntry ) {
    *retEntry = entryStart;
  }
  return ML_STATUS_NO_ERROR;
}


/* --------------------------------------------------_mlDIQueueTopMessageHeader
 *
 * Peek at the top (oldest) message header on the send queue
 */
MLstatus MLAPI _mlDIQueueTopMessageHeader( MLqueueRec* q,
					   MLqueueEntry ** entry )
{
  return _mlDIFifoTop( q_send( q ), (void **) entry );
}


/* --------------------------------------------_mlDIQueueNextMessageHeaderState
 *
 * Get the next unread message header on the send queue
 */
MLstatus MLAPI _mlDIQueueNextMessageHeaderState( MLqueueRec* q,
						 MLqueueEntry **entry,
					     enum mlQueueEntryStateEnum dstate,
						 MLint32 advanceState )
{
  QUEUE_DEBUG( fprintf( stderr,	"[mlqueue:%04d] get next %s (%d) message\n",
			__LINE__,
			_mlDIQueueStateToText( (enum mlQueueEntryStateEnum)
					       dstate ), dstate ) );

  /* Skip over any message headers which are above dstate
   */
  *entry = NULL; /* need null to kick-start the iterate routine. */
  do {
    *entry = (MLqueueEntry*) _mlDIFifoIterate( q_send( q ), (void*) (*entry), 
					       sizeof( MLqueueEntry ) );
    if ( *entry == NULL ) {
      return ML_STATUS_RECEIVE_QUEUE_EMPTY;
    }

    QUEUE_DEBUG_2( fprintf( stderr,
	    "[mlqueue:%04d] next msg     0x%p %s %s\n", __LINE__, *entry,
			    _mlDIQueueStateToText( (*entry)->state ),
			    _mlDIQueueTypeToText( (*entry)->messageType ) ) );

  } while ( (*entry)->state > dstate );

  /* Found an unread message, mark it as read and return it.
   */
  if ( advanceState ) {
    enum mlQueueEntryStateEnum ns;

    switch ( (*entry)->state ) {
    case ML_ENTRY_MSG_SENT_TO_DEVICE:
      ns = ML_ENTRY_MSG_READ_BY_DEVICE;
      break;

    case ML_ENTRY_MSG_READ_BY_DEVICE:
      ns = ML_ENTRY_MSG_PROCESSED_BY_DEVICE;
      break;

    case ML_ENTRY_DD_EVENT_SENT_TO_DEVICE:
      ns = ML_ENTRY_DD_EVENT_READ_BY_DEVICE;
      break;

    case ML_ENTRY_DD_EVENT_READ_BY_DEVICE:
      ns = ML_ENTRY_DD_EVENT_PROCESSED_BY_DEVICE;
      break;

    default:
#ifndef NDEBUG
      if ( getenv( "ML_DEBUG_QUEUE" ) ) {
	fprintf( stderr, "[mlDIQueue debug] _mlDIQueueNextMessageHeaderState "
		 "invalid entry state: %s\n",
		 _mlDIQueueStateToText((*entry)->state));
      }
#endif
      return ML_STATUS_INTERNAL_ERROR;
    }

    /* Update new state
     */
    (*entry)->state = ns;
  }
  return ML_STATUS_NO_ERROR;
}


/* -------------------------------------------------_mlDIQueueNextMessageHeader
 */
MLstatus MLAPI _mlDIQueueNextMessageHeader( MLqueueRec * q,
					    MLqueueEntry ** entry )
{
  /* Skip over any message headers which have already been read
   */
  return _mlDIQueueNextMessageHeaderState( q, entry,
					   ML_ENTRY_READ_BY_DEVICE, 1 );
}


/* -------------------------------------------------------advanceMessageHeaders
 *
 * Advance any processed messages on the send fifo, to the receive
 * fifo.  If there are no processed messages, then simply return (this
 * is not an error).
 */
MLstatus MLAPI _mlDIQueuePopMessageHeaderState( MLqueueRec* q, int dec );
MLstatus MLAPI _mlDIQueueAdvanceMessageHeaders( MLqueueRec* q )
{
  MLstatus status = ML_STATUS_NO_ERROR;
  MLqueueEntry* entry = NULL;

  for ( ;; ) {
    entry = (MLqueueEntry*) _mlDIFifoIterate( q_send( q ), (void*) entry, 
					      sizeof( MLqueueEntry ) );
    if ( entry == NULL ) {
      return ML_STATUS_NO_ERROR;
    }

    QUEUE_DEBUG( fprintf( stderr, "[mlqueue:%04d] advance  msg 0x%p %s\n",
			  __LINE__, entry, entry ?
			  _mlDIQueueStateToText( entry->state ) : "" ) );

    switch ( entry->state ) {
    case ML_ENTRY_MSG_PROCESSED_BY_DEVICE:
      status = _mlDIQueueProcessMessageHeader( q, entry->messageType );
      break;

    case ML_ENTRY_DD_EVENT_QUEUED_AT_DEVICE:
      status = _mlDIQueuePopMessageHeaderState( q, 0 );
      break;

    case ML_ENTRY_DD_EVENT_READ_BY_DEVICE:
    case ML_ENTRY_DD_EVENT_PROCESSED_BY_DEVICE:
      status = _mlDIQueuePopMessageHeaderState( q, 1 );
      break;

    default:
      entry = NULL;
      break;
    }

    PRINT_QUEUE( "_mlDIQueueAdvanceMessageHeaders", q );

    if ( status != ML_STATUS_NO_ERROR ) {
      return status;
    }

    if ( entry == NULL ) {
      return ML_STATUS_NO_ERROR;
    }
  }
}


/*---------------------------------------------_mlDIQueuePopMessageHeaderState
 *
 * Pop the oldest message header off the send fifo and update
 * counts/waitables
 */
MLstatus MLAPI _mlDIQueuePopMessageHeaderState( MLqueueRec* q, int dec )
{
#ifndef ML_OS_UNIX
  /* Pretend the argument is used (it is only really used on UNIX), to
   * avoid compilation warnings
   */
  (void) dec;
#endif

  if ( _mlDIFifoPop( q_send( q ), sizeof( MLqueueEntry ) ) ) {
    return ML_STATUS_INTERNAL_ERROR;
  }
  q->sendCount--;

  if ( q->pipesPresent ) {
#ifdef ML_OS_UNIX
    MLstatus status;

    if ( q->sendMaxCount - q->sendCount > q->sendSignalCount ) {
      /* Signal that there is one more free slot on the queue
       */
      if ( (status = _mlDIQueuePipeHandler( q, Q_PIPE_WRITE, Q_PIPE_SEND )) ) {
	return status;
      }
    }
    if ( dec ) {
      /* Signal that there is one less entry on the queue
       */
      if ( (status = _mlDIQueuePipeHandler( q, Q_PIPE_READ, Q_PIPE_DEVICE ))) {
	return status;
      }
#ifndef NDEBUG
      q->dbgCount--;
      QUEUE_DEBUG( fprintf( stderr, "[mlqueue:%04d] --count = %d\n",
			    __LINE__, q->dbgCount ) );
#endif
    }
#endif

#ifdef ML_OS_NT
    if ( q->sendMaxCount - q->sendCount > q->sendSignalCount ) {
      if ( ! SetEvent( q->sendWaitable ) ) {
	return ML_STATUS_INTERNAL_ERROR;
      }
    }
    if ( q->sendCount == 0 ) {
      if( ! ResetEvent( q->deviceWaitable ) ) {
	return ML_STATUS_INTERNAL_ERROR;
      }
    }
#endif
  }

  return ML_STATUS_NO_ERROR;
}


/* --------------------------------------------------_mlDIQueuePopMessageHeader
 */
MLstatus MLAPI _mlDIQueuePopMessageHeader( MLqueueRec* q )
{
  return _mlDIQueuePopMessageHeaderState( q, 1 );
}


/* ---------------------------------------------------_mlDIQueuePushReplyHeader
 *
 * Push a reply header onto the receive queue
 * and update counts/waitables
 */
MLstatus MLAPI _mlDIQueuePushReplyHeader( MLqueueRec* q,
					  MLqueueEntry* entry )
{
  if ( entry->state == ML_ENTRY_NEW_EVENT ) {
    q->receiveEventCount++;
    entry->state = ML_ENTRY_EVENT_SENT_TO_APP;

  } else {
    q->receiveReplyCount++;
    entry->state = ML_ENTRY_MSG_SENT_TO_APP;
  }

  if ( _mlDIFifoPush( q_receive( q ), entry, sizeof( MLqueueEntry ),
		      sizeof( MLqueueEntry ) ) == NULL ) {
    ML_DEBUG_QUEUE( "Error - unable to return message" );
    ML_DEBUG_QUEUE( "        This shouldn't happen - are you sending" );
    ML_DEBUG_QUEUE( "        events/replies from more than one thread?" );
    if ( entry->state == ML_ENTRY_EVENT_SENT_TO_APP ) {
      q->receiveEventCount--;
    } else {
      q->receiveReplyCount--;
    }
    return ML_STATUS_INTERNAL_ERROR;
  }

  if ( q->pipesPresent ) {
#ifdef ML_OS_UNIX
    MLstatus status;

    /* Signal that there is one more entry on the queue
     */
    if ( (status = _mlDIQueuePipeHandler( q, Q_PIPE_WRITE, Q_PIPE_RECEIVE ))) {
      return status;
    }
#endif

#ifdef ML_OS_NT
    if ( ! SetEvent( q->receiveWaitable ) ) {
      return ML_STATUS_INTERNAL_ERROR;
    }
#endif
  }

  return ML_STATUS_NO_ERROR;
}


/* ----------------------------------------------_mlDIQueueProcessMessageHeader
 *
 * Process a message and place it on the queue back to the applicaion.
 * i.e. Take a message header from the send fifo, modify the message type
 * and push the result onto the receive fifo.
 */
MLstatus MLAPI
_mlDIQueueProcessMessageHeader( MLqueueRec* q,
				enum mlMessageTypeEnum newMessageType )
{
  MLqueueEntry* entry;
  MLstatus status;

  /* Examine the oldest message sent to us
   */
  status = _mlDIQueueTopMessageHeader( q, &entry );
  mlAssert( status == ML_STATUS_NO_ERROR ||
	    (entry == NULL && status == ML_STATUS_RECEIVE_QUEUE_EMPTY) );

  if ( status == ML_STATUS_RECEIVE_QUEUE_EMPTY ) {
    return status;
  }

  /* Update the message type
   */
  entry->messageType = newMessageType;

  /* And push it onto the queue back to the application
   */
  status = _mlDIQueuePushReplyHeader( q, entry );
  mlAssert( status == ML_STATUS_NO_ERROR );

  /* Now, we've finished with the message, remove the original from
   * the send queue.
   */
  status = _mlDIQueuePopMessageHeader( q );
  mlAssert( status == ML_STATUS_NO_ERROR );

  return ML_STATUS_NO_ERROR;
}


/* ---------------------------------------------------_mlDIQueueReadReplyHeader
 *
 * Get the top (oldest) message header on the receive queue, at the
 * same time, pop any previously-read header, delete any old entry
 * from the payload area, and update counts/waitables
 */
MLstatus MLAPI _mlDIQueueReadReplyHeader( MLqueueRec* q,
					  MLqueueEntry** entry )
{
  MLstatus status;
  MLint32 inTransitDelta = 0;

  status = _mlDIFifoTop( q_receive( q ), (void **) entry );

  mlAssert( status == ML_STATUS_NO_ERROR ||
	    (*entry == NULL && status == ML_STATUS_RECEIVE_QUEUE_EMPTY) );

  QUEUE_DEBUG( fprintf( stderr,	"[mlqueue:%04d] read reply hdr 0x%p %s %s\n",
			__LINE__, *entry, *entry ?
			_mlDIQueueStateToText( (*entry)->state ) : "",
			*entry ?
			_mlDIQueueTypeToText( (*entry)->messageType ) : "" ) );

  if ( status != ML_STATUS_NO_ERROR ) {
    return status;
  }

  /* Check for, and free any previously read entry
   */
  if ( (*entry)->state == ML_ENTRY_MSG_READ_BY_APP ||
       (*entry)->state == ML_ENTRY_EVENT_READ_BY_APP ) {
    if ( (*entry)->payloadSize > 0 ) {
      if ( (*entry)->state == ML_ENTRY_MSG_READ_BY_APP ) {
	status = _mlDIFifoPop( q_message( q ), (*entry)->payloadSize );
      } else {
	status = _mlDIFifoPop( q_event( q ), (*entry)->payloadSize );
      }
      mlAssert( status == ML_STATUS_NO_ERROR );
    }

    if ( (*entry)->state == ML_ENTRY_MSG_READ_BY_APP ) {
      /* Compute the change to the in-transit count - only messages
       * are counted.
       */
      inTransitDelta = -1;
    }
    
    /* Remove the entry from the queue
     */
    (*entry)->state = ML_ENTRY_DESTROYED;
    status = _mlDIFifoPop( q_receive( q ), sizeof( MLqueueEntry ) );
    mlAssert( status == ML_STATUS_NO_ERROR );
    if ( status != ML_STATUS_NO_ERROR ) {
      return ML_STATUS_INTERNAL_ERROR;
    }

    /* Now the entry has been popped, update the in-transit count.
     */
    q->inTransitCount += inTransitDelta;

    status = _mlDIFifoTop( q_receive( q ), (void **) entry );

    mlAssert( status == ML_STATUS_NO_ERROR ||
	      (*entry == NULL && status == ML_STATUS_RECEIVE_QUEUE_EMPTY) );

    if ( status != ML_STATUS_NO_ERROR ) {
      return status;
    }
  }

  if ( (*entry)->state == ML_ENTRY_EVENT_SENT_TO_APP ) {
    (*entry)->state = ML_ENTRY_EVENT_READ_BY_APP;
    q->receiveEventCount--;
  } else {
    (*entry)->state = ML_ENTRY_MSG_READ_BY_APP;
    q->receiveReplyCount--;
  }

  if ( q->pipesPresent ) {
#ifdef ML_OS_UNIX
    /* Signal that there is one less entry on the queue
     */
    if ( (status = _mlDIQueuePipeHandler( q, Q_PIPE_READ, Q_PIPE_RECEIVE)) ) {
      return status;
    }
#endif

#ifdef ML_OS_NT
    if ( q->receiveReplyCount == 0 ) {
      if( ! ResetEvent( q->receiveWaitable ) ) {
	return ML_STATUS_INTERNAL_ERROR;
      }
    }
#endif
  }

  QUEUE_DEBUG( fprintf( stderr, "[mlqueue:%04d] new  reply hdr 0x%p %s %s\n",
			__LINE__, *entry,
			_mlDIQueueStateToText( (*entry)->state ),
			_mlDIQueueTypeToText( (*entry)->messageType ) ) );

  return ML_STATUS_NO_ERROR;
}


/* ---------------------------------------------------_mlDIQueueReadReplyHeader
 */
#ifndef	ML_OS_NT
MLstatus MLAPI _mlDIQueuePipeHandler( MLqueueRec* q,
				      enum mlQueuePipeSvcEnum svc,
				      enum mlQueuePipeEnum pipe )
{
  int *pipefds;
  size_t ret;
  char c, ch;

  switch ( pipe ) {
  case Q_PIPE_SEND:    pipefds = q->sendpipe; 	 c = 'S'; break;
  case Q_PIPE_RECEIVE: pipefds = q->receivepipe; c = 'R'; break;
  case Q_PIPE_DEVICE:  pipefds = q->devicepipe;  c = 'D'; break;
  default: return ML_STATUS_INTERNAL_ERROR;
  }

  if ( svc == Q_PIPE_WRITE ) {
    ret = write( pipefds[ 1 ], &c, 1 );

  } else if ( svc == Q_PIPE_READ ) {
    ret = read( pipefds[ 0 ], &ch, 1 );

  } else {
    return ML_STATUS_INTERNAL_ERROR;
  }

  if ( ret != 1 ) {
#ifndef NDEBUG
    fprintf( stderr, "_mlDIQueuePipeHandler %s %s pipe %d error %s\n",
	     svc == Q_PIPE_WRITE ? "write" :
	     svc == Q_PIPE_READ ?  "read" : "?",
	     pipe == Q_PIPE_SEND ? "send" :
	     pipe == Q_PIPE_RECEIVE ? "receive" :
	     pipe == Q_PIPE_DEVICE ? "device" : "?",
	     pipefds[ 0 ], strerror(oserror()));
#endif
    return ML_STATUS_INTERNAL_ERROR;
  }

  QUEUE_DEBUG( char s[4];
	       if ( svc == Q_PIPE_READ && c != ch ) {
		 s[0] = c; s[1] = ' '; s[2] = ch; s[3] = '\0';
		 fprintf( stderr, "unexpected pipe character sb/is: %s\n", s );
	       } );

  return ML_STATUS_NO_ERROR;
}
#endif	/* !ML_OS_NT */


