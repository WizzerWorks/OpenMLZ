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
#ifndef __INC_MLDEFS_H__
#define __INC_MLDEFS_H__

#ifdef __cplusplus
extern "C" {
#endif

/**********************************************************************
 *
 * Error return values
 *
 **********************************************************************/

enum mlStatusReturnEnum
{
  ML_STATUS_NO_ERROR               = 0,
  ML_STATUS_NO_OPERATION           = 1,
  ML_STATUS_OUT_OF_MEMORY          = 2,
  ML_STATUS_INVALID_ID	           = 3,
  ML_STATUS_INVALID_ARGUMENT       = 4,
  ML_STATUS_INVALID_VALUE          = 5,
  ML_STATUS_INVALID_PARAMETER      = 6,
  ML_STATUS_RECEIVE_QUEUE_EMPTY    = 7,
  ML_STATUS_SEND_QUEUE_OVERFLOW    = 8,
  ML_STATUS_RECEIVE_QUEUE_OVERFLOW = 9,
  ML_STATUS_INSUFFICIENT_RESOURCES = 10,
  ML_STATUS_DEVICE_UNAVAILABLE     = 11,
  ML_STATUS_ACCESS_DENIED          = 12,
  ML_STATUS_DEVICE_ERROR           = 13,
  ML_STATUS_DEVICE_BUSY            = 14,
  ML_STATUS_INVALID_CONFIGURATION  = 15,
  ML_STATUS_GENLOCK_NO_SIGNAL           = 16,
  ML_STATUS_GENLOCK_UNKNOWN_SIGNAL      = 17,
  ML_STATUS_GENLOCK_ILLEGAL_COMBINATION = 18,
  ML_STATUS_GENLOCK_TIMING_MISMATCH     = 19,
  ML_STATUS_INTERNAL_ERROR         = 1000

  /* If additional functions are added to the API, there may be a need
   * to add additional return status values.
   */
};


/**********************************************************************
 *
 * Message classes and types.
 *
 **********************************************************************/

enum mlMessageClassEnum
{
  ML_BUFFERS_MESSAGE      = 0x010000,
  ML_CONTROLS_MESSAGE     = 0x020000,
  ML_EVENT_MESSAGE        = 0x040000,
  ML_QUEUE_EVENT          = 0x040100,
  ML_GENERIC_DEVICE_EVENT = 0x040200,
  ML_VIDEO_DEVICE_EVENT   = 0x040300,
  ML_XCODE_DEVICE_EVENT   = 0x040400,
  ML_AUDIO_DEVICE_EVENT   = 0x040500,

  ML_CUSTOM_DEVICE_EVENT  = 0x04f000,

  /* Following are internal and not intended for use by user
   * applications.
   */

  ML_DD_EVENT            = 0x080000,
  ML_MESSAGE_CLASS_MASK  = 0x0f0000, 
  ML_MESSAGE             = 0x100000 
};

enum mlMessageTypeEnum
{
  ML_MESSAGE_INVALID = 0,

  /* The following repy messages are generated in reponse to send
   * calls from the application.  They are not maskable.
   */

  /* send buffers replies */
  ML_BUFFERS_COMPLETE            = ML_BUFFERS_MESSAGE,
  ML_BUFFERS_ABORTED,
  ML_BUFFERS_FAILED,

  /* send controls replies */
  ML_CONTROLS_COMPLETE           = ML_CONTROLS_MESSAGE,
  ML_CONTROLS_ABORTED,
  ML_CONTROLS_FAILED,

  /* query controls replies */
  ML_QUERY_CONTROLS_COMPLETE,
  ML_QUERY_CONTROLS_ABORTED,
  ML_QUERY_CONTROLS_FAILED,

  /* The following event is generated if the event queue overflows.
   * This can only happen if the application i) enables events and ii)
   * does not read them fast enough.  It is not maskable (except
   * indirectly, by not enabling any events).
   */

  ML_EVENT_QUEUE_OVERFLOW        = ML_QUEUE_EVENT,

  /* The following event messages are generated spontaneously by
   * devices.  They are only generated if the application explicitly
   * asks for them.  Most devices will only support a subset of all
   * the possible events.  To select/query events use the
   * DEVICE_EVENTS parameter.
   */

  ML_EVENT_DEVICE_INFO           = ML_GENERIC_DEVICE_EVENT,
  ML_EVENT_DEVICE_ERROR,
  ML_EVENT_DEVICE_UNAVAILABLE,

  ML_EVENT_VIDEO_SEQUENCE_LOST   = ML_VIDEO_DEVICE_EVENT,
  ML_EVENT_VIDEO_SYNC_GAINED,
  ML_EVENT_VIDEO_SYNC_LOST,
  ML_EVENT_VIDEO_VERTICAL_RETRACE,
  ML_EVENT_VIDEO_SIGNAL_GAINED,
  ML_EVENT_VIDEO_SIGNAL_LOST,

  ML_EVENT_XCODE_FAILED          = ML_XCODE_DEVICE_EVENT,

  ML_EVENT_AUDIO_SEQUENCE_LOST   = ML_AUDIO_DEVICE_EVENT,
  ML_EVENT_AUDIO_SAMPLE_RATE_CHANGED,

  /* Following are internal and not intended for use by user
   * applications.
   */

  ML_REDUNDANT_MESSAGE		 = ML_MESSAGE,
  ML_BUFFERS_IN_PROGRESS        = ML_MESSAGE | ML_BUFFERS_MESSAGE,
  ML_CONTROLS_IN_PROGRESS       = ML_MESSAGE | ML_CONTROLS_MESSAGE,
  ML_QUERY_IN_PROGRESS          = ML_MESSAGE | ML_QUERY_CONTROLS_COMPLETE
};


/* For backwards compatibility
 */
#define ML_EVENT_VIDEO_SIGNAL_PRESENT ML_EVENT_VIDEO_SIGNAL_GAINED

#ifdef __cplusplus 
}
#endif

#endif /* ! __INC_MLDEFS_H__  */

