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

#ifndef _ML_QUEUE_H
#define _ML_QUEUE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <ML/mldefs.h>
#include <ML/mlfifo.h>

typedef struct _MLqueueOptions {
  MLint32 sendSignalCount;
  MLint32 sendMaxCount;
  MLint32 receiveMaxCount;
  MLint32 eventMaxCount;
  MLint32 messagePayloadSize;
  MLint32 ddMessageSize;
  MLint32 ddEventSize;
  MLint32 ddAlignment;
  MLint32 noPipeFds;
  MLint32 future[7];
} MLqueueOptions;

typedef struct _MLqueueRec {
  MLint32 magicNumber;          /* to help catch programming errors */

  MLint32 queueSize;		/* size this memory allocation */
				/* (includes all the fifos as well) */

  MLint32 sendOffset;		/* offset to send fifo */
  MLint32 receiveOffset;	/* offset to receive fifo */
  MLint32 eventOffset;		/* offset to event fifo */
  MLint32 messageOffset;	/* offset to payload fifo */

  MLint32 sendFifoSize;		/* size of send fifo (excluding header) */
  MLint32 receiveFifoSize;	/* size of receive fifo (excluding header) */
  MLint32 eventFifoSize;	/* size of event fifo (excluding header) */
  MLint32 messageFifoSize;	/* size of payload fifo (excluding header) */

  /* macros to find pointers to above fifo's
   */
#define	q_send(q)    (MLfifo*)(((char*)(q))+((MLqueueRec*)(q))->sendOffset)
#define	q_receive(q) (MLfifo*)(((char*)(q))+((MLqueueRec*)(q))->receiveOffset)
#define	q_event(q)   (MLfifo*)(((char*)(q))+((MLqueueRec*)(q))->eventOffset)
#define	q_message(q) (MLfifo*)(((char*)(q))+((MLqueueRec*)(q))->messageOffset)

  /* Constant counts - set at creation time
   */
  MLint32 sendMaxCount;      /* max slots in send fifo */
  MLint32 receiveMaxCount;   /* max slots in receive fifo */
  MLint32 sendSignalCount;   /* num free slots for send waitable to fire */
  MLint32 inTransitMaxCount; /* max user-created messages in transit */
  MLint32 eventMaxCount;     /* number of events allowed on queue */

  /* Variable counts
   */
  MLint32 sendCount;         /* number of filled slots in the send fifo */
  MLint32 receiveReplyCount; /* number of unread reply messages in recv fifo */
  MLint32 receiveEventCount; /* number of unread event messages in recv fifo */
  MLint32 inTransitCount;    /* number of user-created messages in transit */
  MLint32 eventCount;        /* number of events in event queue */
  MLint32 dbgCount;	     /* debug count */


  /* Note - a message is in-transit from the time it is pushed onto
   *        the send fifo until the time it is popped off the receive
   *        fifo.
   */
  
  /* The waitables - these are event handles on NT, file descriptors
   * on UNIX.
   */
  MLwaitable sendWaitable;
  MLwaitable receiveWaitable;
  MLwaitable deviceWaitable;

  /* Indicates that UNIX pipes or NT events are present
   */
  MLint32  pipesPresent;

  /* In Unix, use a pipe as a way to generate file descriptors.
   */
#ifdef ML_OS_UNIX 
  int sendpipe[2];
  int receivepipe[2];
  int devicepipe[2];
#endif

    MLint32  pad;
} MLqueueRec;

enum mlQueueEntryStateEnum
{
  ML_ENTRY_EMPTY = 0,

  ML_ENTRY_NEW,
  ML_ENTRY_NEW_MSG,
  ML_ENTRY_NEW_DD_EVENT,

  ML_ENTRY_SENT_TO_DEVICE,
  ML_ENTRY_MSG_SENT_TO_DEVICE,
  ML_ENTRY_DD_EVENT_SENT_TO_DEVICE,

  ML_ENTRY_READ_BY_DEVICE,
  ML_ENTRY_MSG_READ_BY_DEVICE,
  ML_ENTRY_DD_EVENT_READ_BY_DEVICE,

  ML_ENTRY_QUEUED_AT_DEVICE,
  ML_ENTRY_MSG_QUEUED_AT_DEVICE,
  ML_ENTRY_DD_EVENT_QUEUED_AT_DEVICE,

  ML_ENTRY_PROCESSED_BY_DEVICE,
  ML_ENTRY_MSG_PROCESSED_BY_DEVICE,
  ML_ENTRY_DD_EVENT_PROCESSED_BY_DEVICE,

  ML_ENTRY_NEW_EVENT,
  ML_ENTRY_SENT_TO_APP,
  ML_ENTRY_MSG_SENT_TO_APP,
  ML_ENTRY_EVENT_SENT_TO_APP,

  ML_ENTRY_READ_BY_APP,
  ML_ENTRY_MSG_READ_BY_APP,
  ML_ENTRY_EVENT_READ_BY_APP,

  ML_ENTRY_DESTROYED
};


typedef struct _MLqueueEntry {

  /* The state of this entry, this is manipulated automatically by the
   * queue routines.
   */
  enum mlQueueEntryStateEnum state;

  /* The type of message stored in this queue entry, this is
   * manipulated by your device-dependent routines.
   */
  enum mlMessageTypeEnum messageType; 

  /* A queue entry may have associated payload data on the "Message"
   * payload or the "Event" payload FIFO's.
   *
   * Store location of data in message or event payload fifo as an
   * offset to allow for access in different address spaces.
   */
  MLint32 payloadSize;
  MLint32 payloadDataByteOffset;

  /* The DD may append its own data to an entry.
   */
  MLint32 ddPayloadSize;
  MLint32 ddPayloadDataByteOffset;

} MLqueueEntry;

#ifndef	_KERNEL

/* Initialize the queue and interpret open options
 */
MLstatus MLAPI mlDIQueueSize( MLqueueOptions* pOpt, 
			      MLint32* reqdSize );

/* Initialize the queue and interpret open options
 */
MLstatus MLAPI mlDIQueueCreate( MLbyte* preallocSpace,
				MLint32 preallocSize,
				MLqueueOptions* pOpt,
				MLqueueRec** q );

/* Undo the effects of queue allocation
 */
MLstatus MLAPI mlDIQueueDestroy( MLqueueRec* q );

/* Copy message contents into the payload area, push header onto the
 * send fifo and update counts/waitables
 */
MLstatus MLAPI mlDIQueuePushMessage( MLqueueRec* q, 
				     enum mlMessageTypeEnum messageType,
				     MLpv* message,
				     MLbyte* ddData, 
				     MLint32 ddDataSize, 
				     MLint32 ddDataAlignment );

/* Copy message contents into the payload area, push header onto the
 * send fifo and update counts/waitables
 *
 * Return info on where entry in queue was placed
 */
MLstatus MLAPI mlDIQueuePushMessageRet( MLqueueRec* q, 
					enum mlMessageTypeEnum messageType,
					MLpv* message,
					MLbyte* ddData, 
					MLint32 ddDataSize, 
					MLint32 ddDataAlignment,
					MLqueueEntry **retEntry );

/* Send an event to a device.
 *
 * This event only goes to the device, it is not returned to the
 * application.  Useful for private communication within your
 * device-dependent module.
 */
MLstatus MLAPI mlDIQueueSendDeviceEvent( MLqueueRec* q, 
					 enum mlMessageTypeEnum messageType );

/* Read the oldest unread message from application to device so that
 * we may begin processing it.
 */
MLstatus MLAPI mlDIQueueNextMessage( MLqueueRec* q, 
				     MLqueueEntry** qentry,
				     enum mlMessageTypeEnum *messageType,
				     MLpv** message,
				     MLbyte** ddData,
				     MLint32* ddDataSize );

/* Read oldest message of specified state, optionally advance to next
 * state
 */
MLstatus MLAPI mlDIQueueNextMessageState( MLqueueRec* q, 
					  MLqueueEntry** entry,
					  enum mlMessageTypeEnum *messageType,
					  MLpv** message,
					  MLbyte** ddData,
					  MLint32* ddDataSize,
					  enum mlQueueEntryStateEnum dstate,
					  MLint32 advanceState );

/* Update a message state and type.
 */
MLstatus MLAPI mlDIQueueUpdateMessageState( MLqueueEntry* qentry, 
				         enum mlMessageTypeEnum newMessageType,
					    MLint32 newMessageStateAdvance );

/* Update a message after it has been processed, updating its message
 * type.
 */
MLstatus MLAPI mlDIQueueUpdateMessage( MLqueueEntry* qentry,
				       enum mlMessageTypeEnum newMessageType );

/* Return any processed messages - popping them off the fifo going to
 * the device, and pushing them onto the fifo going back to the
 * application.
 */
MLstatus MLAPI mlDIQueueAdvanceMessages( MLqueueRec* q );

/* Add a event message to the event payload and push the corresponding
 * entry onto the receive queue back to the application.
 */
MLstatus MLAPI mlDIQueueReturnEvent( MLqueueRec* q, 
				     enum mlMessageTypeEnum msgType,
				     MLpv* eventMsg );

/* Abort any messages remaining in the send queue, popping them and
 * pushing the aborted replies onto the receive queue.
 */
MLstatus MLAPI mlDIQueueAbortMessages( MLqueueRec* q );

/* Remove and old (previously read) message and return the oldest
 * unread message on the queue from device back to the application.
 */
MLstatus MLAPI mlDIQueueReceiveMessage( MLqueueRec* pQueue, 
					enum mlMessageTypeEnum *messageType,
					MLpv** message,
					MLbyte** ddData,
					MLint32* ddDataSize );

/* Functions to get queue counts
 */
MLint32 MLAPI mlDIQueueGetSendCount( MLqueueRec* q );
MLint32 MLAPI mlDIQueueGetReceiveCount( MLqueueRec* q );
MLint32 MLAPI mlDIQueueGetReplyCount( MLqueueRec* q );
MLint32 MLAPI mlDIQueueGetEventCount( MLqueueRec* q );

/* Functions to get waitables
 */
MLwaitable MLAPI mlDIQueueGetSendWaitable( MLqueueRec* q );
MLwaitable MLAPI mlDIQueueGetReceiveWaitable( MLqueueRec* q );
MLwaitable MLAPI mlDIQueueGetDeviceWaitable( MLqueueRec* q );

/* Additional queue functions, these may change in future versions and
 * are primarily intended for internal use.
 */
MLstatus MLAPI _mlDIQueueSize( MLqueueOptions* pOpt, 
			       MLqueueRec* pQueue, 
			       MLint32* reqdSize );

MLstatus MLAPI _mlDIQueueCheckMessageFits( MLqueueRec* q, 
					   MLint32 payloadSpace );

MLstatus MLAPI _mlDIQueuePushMessageHeader( MLqueueRec* q, 
					    MLqueueEntry* qentry );

MLstatus MLAPI _mlDIQueuePushMessageHeaderRet( MLqueueRec* q, 
					       MLqueueEntry* qentry,
					       MLqueueEntry** retEntry );

MLstatus MLAPI _mlDIQueueTopMessageHeader( MLqueueRec* q, 
					   MLqueueEntry** qentry );

MLstatus MLAPI _mlDIQueuePopMessageHeader( MLqueueRec* q );

MLstatus MLAPI _mlDIQueueNextMessageHeader( MLqueueRec * q,
					    MLqueueEntry ** qentry );

MLstatus MLAPI _mlDIQueueNextMessageHeaderState( MLqueueRec * q,
						 MLqueueEntry **entry,
					     enum mlQueueEntryStateEnum dstate,
						 MLint32 advanceState );

MLstatus MLAPI _mlDIQueueProcessMessageHeader( MLqueueRec * q,
				       enum mlMessageTypeEnum newMessageType );

MLstatus MLAPI _mlDIQueueAdvanceMessageHeaders( MLqueueRec * q );

MLstatus MLAPI _mlDIQueuePushReplyHeader( MLqueueRec* q, 
					  MLqueueEntry* qentry );

MLstatus MLAPI _mlDIQueueReadReplyHeader( MLqueueRec* q, 
					  MLqueueEntry** qentry );

/* Provide DD/DI service to MLmodules for dealing with event
 * notication pipes
 */
enum mlQueuePipeSvcEnum
{
    Q_PIPE_WRITE = 1,
    Q_PIPE_READ
};
enum mlQueuePipeEnum
{
    Q_PIPE_SEND = 1,
    Q_PIPE_RECEIVE,
    Q_PIPE_DEVICE
};

MLstatus MLAPI _mlDIQueuePipeHandler( MLqueueRec*, enum mlQueuePipeSvcEnum,
				      enum mlQueuePipeEnum );

char* MLAPI _mlDIQueueStateToText( enum mlQueueEntryStateEnum state );
char* MLAPI _mlDIQueueTypeToText( enum mlMessageTypeEnum type );

#endif	/* !_KERNEL */

#ifdef __cplusplus
}
#endif

#endif /* _ML_QUEUE_H */
