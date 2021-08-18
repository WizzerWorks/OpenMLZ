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
 * Additional Notice Provisions:
 * License Applicability. Except to the extent portions of this file are 
 * made subject to an alternative license as permitted in the Khronos 
 * Free Software License Version 1.0 (the "License"), the contents of 
 * this file are subject only to the provisions of the License. You may 
 * not use this file except in compliance with the License. You may obtain 
 * a copy of the License at 
 * 
 * The Khronos Group Inc.:  PO Box 1019, Clearlake Park CA 95424 USA or at
 *  
 * http://www.Khronos.org/licenses/KhronosOpenSourceLicense1_0.pdf
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

#define START_CHILD_IMMEDIATELY

#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/soundcard.h>
#include <sys/time.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <values.h>
#include <assert.h>

#include <ML/ml_private.h>
#include <ML/ml_didd.h>
#include <ML/ml_oswrap.h>

#include "generic.h"

/* the current design selects on the audio device filedesc to detect over-
 * and underflows
 */

#define SELECT_ON_DEVICE

/* many mixers report that they have SOUND_MIXER_SPEAKER available, but
 * setting the volume on this channel rarely works, so we work around this
 * by setting both SOUND_MIXER_VOLUME and SOUND_MIXER_PCM
 */

#define NO_SPEAKER_CHANNEL

/* OSS gain levels are not specified to be either linear or decibel
 */

#define OSS_GAIN_LINEAR

#ifdef DEBUG
#include <stdlib.h>
static int debugLevel = 0;
/* error printouts */
#define DEBG1(block) if (debugLevel >= 1) { block; fflush(stdout); }
/* basic function printouts */
#define DEBG2(block) if (debugLevel >= 2) { block; fflush(stdout); }
/* detail printouts */
#define DEBG3(block) if (debugLevel >= 3) { block; fflush(stdout); }
/* extreme debug printouts */
#define DEBG4(block) if (debugLevel >= 4) { block; fflush(stdout); }
#else
#define DEBG1(block)
#define DEBG2(block)
#define DEBG3(block)
#define DEBG4(block)
#endif

#define PIPE_READ 0
#define PIPE_WRITE 1

/* bits for requested events
 */

#define NO_EVENTS		0x0
#define SEQUENCE_LOST_EVENT	0x1
#define RATE_CHANGED_EVENT	0x2

#define MAX_DEVICES 32
#define MAX_CHANNELS 16

#define LJACK 1
#define LPATH 2

#define DEV_INPUT 0
#define DEV_OUTPUT 1

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/* set fragment size and count large enough to avoid drop-outs but small
 * enough to avoid large latencies
 */

#define FRAGMENT_COUNT 16
#define FRAGMENT_SIZE 1024


#define GAIN_UNSET -999999.0

typedef struct _audioChannelDetails {
  int ossIndex;
  char name[32];
  int isInput;
} audioChannelDetails;

typedef struct _audioDeviceDetails {
  char name[64];
  int  version;
  char location[128];
  int  index;
  audioChannelDetails channels[MAX_CHANNELS];
  int numChannels;
} audioDeviceDetails;

/* this stores the persistant state of the audio hardware
 */

typedef struct _physicalDevice {
  char reference;		/* who has reference to this device */
  int mode;			/* O_RDONLY or O_WRONLY */
  int fd;
  int mixer_fd;		        /* for associated mixer */
  int format, channels, rate;	/* cached parameters */
  int volume;			/* OSS param value */
  int oldVolume;		/* OSS param value cached from input
				 * oldVolume is only used when
				 * recording.  It is a hack. */
  MLreal64 gainIndB[2];	        /* user-specified gain value (if set) */
  unsigned stereoDevMask;	/* channels which support stereo levels */
  int fragsize, nfrags;	        /* OSS buffer parameters */
  float framesize;		/* precomputed for speed */
  MLint64 frameCount;		/* for MSC */
  struct {
    MLreal64 rateRanges[2][2];
    int nRatePairs;
    int defaultRate;	        /* when device opened */
    char hasExclusiveInput;	/* only supports one mixer input at a time */
    char hasDuplex;		/* supports R/W simultaneously */
    char hasRealTime;           /* supports real time buffer pointer location*/
    int format;
  } caps;
} physicalDevice;

/* this gets handed to open paths
 */

typedef struct _audioDevice {
  int devIndex;		/* 0, 1, etc. for /dev/dsp, /dev/dsp1 */
  int channel;		/* index for "mike", "line", ... */
  int  mode;		/* O_RDONLY or O_WRONLY */
} audioDevice;

typedef struct _audioPath {
  int state;			/* I/O state */
  int eventsWanted;		/* which events to listen for */
  _mlOSThread child;		/* child thread */
  int childRunning;
  int pipe_fds[2];		/* for IPC */
  MLqueueRec *pQueue;	        /* message queue */
  pthread_mutex_t mutex;	/* for mutex */
  physicalDevice *pdev;	        /* ptr to shared device */
  MLopenid openId;              /* Used in mlDI* calls */
  audioDevice device;
} audioPath;

char *device_names[] = SOUND_DEVICE_NAMES;

/* Address of UST function. This is filled-in the first time
 * 'ddConnect' is called
 */
MLint32 (*USTSourceFunc)( MLint64* ) = 0;

/* first set are inputs, second are outputs
 */
physicalDevice globalPhysicalDevices[MAX_DEVICES][2];


/* ----------------------------------------------------------------deviceParams
 */
MLDDint32paramDetails sendCount[] =
{{
	ML_QUEUE_SEND_COUNT_INT32,
	"ML_QUEUE_SEND_COUNT_INT32",
	ML_TYPE_INT32,
	ML_ACCESS_RI,
	NULL,
	NULL, 0,
	NULL, 0,
	NULL, 0,
	NULL, 0
}};

MLDDint32paramDetails receiveCount[] =
{{
	ML_QUEUE_RECEIVE_COUNT_INT32,
	"ML_QUEUE_RECEIVE_COUNT_INT32",
	ML_TYPE_INT32,
	ML_ACCESS_RI,
	NULL,
	NULL, 0,
	NULL, 0,
	NULL, 0,
	NULL, 0
}};

MLDDint32paramDetails sendWaitable[] =
{{
	ML_QUEUE_SEND_WAITABLE_INT64,
	"ML_QUEUE_SEND_WAITABLE_INT64",
	ML_TYPE_INT64,
	ML_ACCESS_RI,
	NULL,
	NULL, 0,
	NULL, 0,
	NULL, 0,
	NULL, 0
}};

MLDDint32paramDetails receiveWaitable[] =
{{
	ML_QUEUE_RECEIVE_WAITABLE_INT64,
	"ML_QUEUE_RECEIVE_WAITABLE_INT64",
	ML_TYPE_INT64,
	ML_ACCESS_RI,
	NULL,
	NULL, 0,
	NULL, 0,
	NULL, 0,
	NULL, 0
}};

MLint32 stateEnums[] =
{
	ML_DEVICE_STATE_TRANSFERRING,
	ML_DEVICE_STATE_WAITING,
	ML_DEVICE_STATE_ABORTING,
	ML_DEVICE_STATE_FINISHING,
	ML_DEVICE_STATE_READY
};

char stateNames[] =
	"ML_DEVICE_STATE_TRANSFERRING\0"
	"ML_DEVICE_STATE_WAITING\0"
	"ML_DEVICE_STATE_ABORTING\0"
	"ML_DEVICE_STATE_FINISHING\0"
	"ML_DEVICE_STATE_READY\0";

MLDDint32paramDetails deviceState[] =
{{
	ML_DEVICE_STATE_INT32,
	"ML_DEVICE_STATE_INT32",
	ML_TYPE_INT32,
	ML_ACCESS_RWI | ML_ACCESS_DURING_TRANSFER,
	NULL,
	NULL, 0,
	NULL, 0,
	stateEnums, sizeof (stateEnums) / sizeof (MLint32),
	stateNames, sizeof (stateNames)
}};

MLint32 channelEnums[] =
{
	ML_CHANNELS_MONO,
	ML_CHANNELS_STEREO
};

char channelNames[] =
	"ML_CHANNELS_MONO\0"
	"ML_CHANNELS_STEREO\0";

MLDDint32paramDetails audioChannels[] =
{{
	ML_AUDIO_CHANNELS_INT32,
	"ML_AUDIO_CHANNELS_INT32",
	ML_TYPE_INT32,
	ML_ACCESS_RWI | ML_ACCESS_QUEUED,
	channelEnums,
	NULL, 0,
	NULL, 0,
	channelEnums, sizeof (channelEnums) / sizeof (MLint32),
	channelNames, sizeof (channelNames)
}};

MLDDint32paramDetails audioSampleRate[] =
{{
	ML_AUDIO_SAMPLE_RATE_REAL64,
	"ML_AUDIO_SAMPLE_RATE_REAL64",
	ML_TYPE_REAL64,
	ML_ACCESS_RWI | ML_ACCESS_QUEUED,
	NULL,
	NULL, 0,
	NULL, 0,
	NULL, 0,
	NULL, 0,
}};

MLint32 formatEnums[] =
{
	ML_AUDIO_FORMAT_S16,
	ML_AUDIO_FORMAT_U8
};

char formatNames[] =
	"ML_AUDIO_FORMAT_S16\0"
	"ML_AUDIO_FORMAT_U8\0";

MLDDint32paramDetails audioFormat[] =
{{
	ML_AUDIO_FORMAT_INT32,
	"ML_AUDIO_FORMAT_INT32",
	ML_TYPE_INT32,
	ML_ACCESS_RWI | ML_ACCESS_QUEUED,
	formatEnums,
	NULL, 0,
	NULL, 0,
	formatEnums, sizeof (formatEnums) / sizeof (MLint32),
	formatNames, sizeof (formatNames)
}};

MLint32 compressionEnums[] =
{
  ML_COMPRESSION_UNCOMPRESSED
};

char compressionNames[] =
	"ML_COMPRESSION_UNCOMPRESSED";

MLDDint32paramDetails audioCompression[] =
{{
	ML_AUDIO_COMPRESSION_INT32,
	"ML_AUDIO_COMPRESSION_INT32",
	ML_TYPE_INT32,
	ML_ACCESS_RWI | ML_ACCESS_QUEUED,
	compressionEnums,
	NULL, 0,
	NULL, 0,
	compressionEnums, sizeof (compressionEnums) / sizeof (MLint32),
	compressionNames, sizeof (compressionNames)
}};

MLDDint32paramDetails audioFrameSize[] =
{{
  ML_AUDIO_FRAME_SIZE_INT32,
  "ML_AUDIO_FRAME_SIZE_INT32",
  ML_TYPE_INT32,
  ML_ACCESS_RI | ML_ACCESS_QUEUED | ML_ACCESS_DURING_TRANSFER,
  NULL,
  NULL, 0,
  NULL, 0,
  NULL, 0,
  NULL, 0,
}};

MLDDint32paramDetails audioGains[] =
{{
  ML_AUDIO_GAINS_REAL64_ARRAY,
  "ML_AUDIO_GAINS_REAL64_ARRAY",
  ML_TYPE_REAL64_ARRAY,
  ML_ACCESS_RWI | ML_ACCESS_QUEUED | ML_ACCESS_DURING_TRANSFER,
  NULL,
  NULL, 0,
  NULL, 0,
  NULL, 0,
  NULL, 0,
}};

/* ----------------------------------------------------------------Audio params
 */
MLDDint32paramDetails audioBuffer[] =
{{
  ML_AUDIO_BUFFER_POINTER,
  "ML_AUDIO_BUFFER_POINTER",
  ML_TYPE_BYTE_POINTER,
  ML_ACCESS_RWBT,
  NULL,
  NULL, 0,
  NULL, 0,
  NULL, 0,
  NULL, 0
}};

MLDDint32paramDetails audioUST[] =
{{
  ML_AUDIO_UST_INT64,
  "ML_AUDIO_UST_INT64",
  ML_TYPE_INT64,
  ML_ACCESS_RBT,
  NULL,
  NULL, 0,
  NULL, 0,
  NULL, 0,
  NULL, 0
}};

MLDDint32paramDetails audioMSC[] =
{{
  ML_AUDIO_MSC_INT64,
  "ML_AUDIO_MSC_INT64",
  ML_TYPE_INT64,
  ML_ACCESS_RBT,
  NULL,
  NULL, 0,
  NULL, 0,
  NULL, 0,
  NULL, 0
}};

MLDDint32paramDetails waitForAudioUST[] =
{{
  ML_WAIT_FOR_AUDIO_UST_INT64,
  "ML_WAIT_FOR_AUDIO_UST_INT64",
  ML_TYPE_INT64,
  ML_ACCESS_RWBT,
  NULL,
  NULL, 0,
  NULL, 0,
  NULL, 0,
  NULL, 0
}};

MLint32 eventEnums[] = 
{
  ML_EVENT_AUDIO_SEQUENCE_LOST
};

char eventNames[] =
{
  "ML_EVENT_AUDIO_SEQUENCE_LOST\0"
};

MLDDint32paramDetails deviceEvent[] =
{{
  ML_DEVICE_EVENTS_INT32_ARRAY,
  "ML_DEVICE_EVENTS_INT32_ARRAY",
  ML_TYPE_INT32_ARRAY,
  ML_ACCESS_RWI,
  NULL,
  NULL, 0,
  NULL, 0,
  eventEnums, sizeof (eventEnums) / sizeof (MLint32),
  eventNames, sizeof (eventNames)
}};

MLDDint32paramDetails* pathParamDetails[] =
{
  deviceState,
  sendCount,
  receiveCount,
  sendWaitable,
  receiveWaitable,
  audioCompression,
  audioFrameSize,
  audioGains,
  audioChannels,
  audioSampleRate,
  audioFormat,
  audioBuffer,
  audioUST,
  audioMSC,
  waitForAudioUST,
  deviceEvent
};

MLstatus openPhysicalDevice(audioDevice *d, int logicalDevice,
			    physicalDevice **ppd);
MLstatus closePhysicalDevice(audioDevice *d, int logicalDevice);
MLstatus reOpenPhysicalDevice(audioDevice *d);
MLstatus createSemaphore(audioPath *p);
void destroySemaphore(audioPath *p);
MLstatus startUpChild(audioPath* p);
MLstatus shutDownChild(audioPath *p, int state);
MLstatus wakeUp(audioPath *p);
MLstatus goToSleep(audioPath *p);
float getFramesize(int fmt, int channels);
MLstatus updateSizes(physicalDevice *p, int mode);
MLstatus setDeviceSampleRate(physicalDevice *pd, MLint32 rate);
int ossFormatFromParams (MLint32 compression, MLint32 format);
void paramsFromOSSFormat (int ossFmt, MLint32 *compression, MLint32 *format);
MLint32 getSampleWidthFromFormat (MLint32 mlFormat, MLboolean expand3to4);
int channelToIndex (const char *channelName);
double gainFromOSSVolume (unsigned char volume);
unsigned char ossVolumeFromGain (double gain);


/* ----------------------------------------------------------------------ddOpen
 */
MLstatus ddOpen( MLbyte *ddDevicePriv, MLint64 staticObjectId,
		 MLopenid openObjectId, MLpv *openOptions, MLbyte **retddPriv )
{
  MLqueueOptions qOpt;
  MLopenOptions oOpt;
  MLint32 pathIndex;
  MLstatus status = ML_STATUS_NO_ERROR;
  audioDeviceDetails *d = (audioDeviceDetails*) ddDevicePriv;
  audioPath *p = NULL;

  if (mlDIextractIdType(staticObjectId) != ML_REF_TYPE_PATH)
    return ML_STATUS_INVALID_ID; /* this module only supports opening paths */

  pathIndex = mlDIextractPathIndex(staticObjectId);
  if( pathIndex >= d->numChannels ) {
    return ML_STATUS_INVALID_ID;
  }

  /* Parse open options
   */
  if (mlDIparseOpenOptions(openObjectId, openOptions, &oOpt
			   ) != ML_STATUS_NO_ERROR) {
    return ML_STATUS_INVALID_PARAMETER;
  }

  if ((p = malloc(sizeof (audioPath))) == NULL)
    return ML_STATUS_OUT_OF_MEMORY;

  /* Setup a single queue between us and the DI layer.
   * Get queue options from open options.
   */
  memset(&qOpt, 0, sizeof (qOpt));
  qOpt.sendSignalCount = oOpt.sendSignalCount;
  qOpt.sendMaxCount = oOpt.sendQueueCount;
  qOpt.receiveMaxCount = oOpt.receiveQueueCount;
  qOpt.eventMaxCount = oOpt.eventPayloadCount;
  qOpt.messagePayloadSize = oOpt.messagePayloadSize;
  qOpt.ddMessageSize = 0;
  qOpt.ddEventSize = sizeof (MLpv) * 4;
  qOpt.ddAlignment = 4;

  /* Now we can make a queue.
   */
  if ((status = mlDIQueueCreate(0, 0, &qOpt, &(p->pQueue))))
    {
      return status;
    }

  p->state = ML_DEVICE_STATE_READY;
  p->eventsWanted = NO_EVENTS;
  p->pipe_fds[PIPE_READ] = p->pipe_fds[PIPE_WRITE] = -1;
  p->pdev = NULL;
  p->childRunning = FALSE;
  p->device.devIndex = d->index;
  p->device.channel = d->channels[pathIndex].ossIndex;
  p->device.mode = (d->channels[pathIndex].isInput)? O_RDONLY : O_WRONLY;

  /* Save the MLopenid, it may be used by the child thread
   */
  p->openId = openObjectId;

  DEBG4(printf("*mp*: in ddOpen, isInput is %d\n",
	       d->channels[pathIndex].isInput));

  DEBG2(printf("in mlAudioPathOpen\n"));
  DEBG3(printf("    p->state set to %d\n", p->state));

  if((status = openPhysicalDevice(&(p->device), LPATH, &p->pdev)))
    {
      free(p);
      return status;
    }

  if ((status = createSemaphore(p)))
    {
      free(p);
      return status;
    }

  pthread_mutex_init(&(p->mutex), NULL);

#ifdef START_CHILD_IMMEDIATELY
  status = startUpChild(p);
#endif

  *retddPriv = (MLbyte*)p;
  return status;
}


/* ---------------------------------------------------------------------ddClose
 */
MLstatus ddClose(MLbyte* ddPriv,
		 MLopenid openObjectId)
{
  MLstatus status;
  audioPath *p = (audioPath *)ddPriv;

  if( p == NULL )
    return ML_STATUS_INTERNAL_ERROR;

  status = shutDownChild(p, ML_DEVICE_STATE_FINISHING);

  mlDIQueueAbortMessages(p->pQueue);

  closePhysicalDevice(&(p->device), LPATH);

  destroySemaphore(p);
  pthread_mutex_destroy(&(p->mutex));

  mlDIQueueDestroy(p->pQueue);
  free(p);

  return status;
}


/*--------------------------------------------------------------------zeroMixer
 */
void zeroMixer(audioDevice *d)
{
    int dirIndex = (d->mode == O_RDONLY) ? DEV_INPUT : DEV_OUTPUT;
    physicalDevice *pd = &globalPhysicalDevices[d->devIndex][dirIndex];
    int ioval = 0, inputMask = 0, chan;

    DEBG4(printf("in zeroMixer()\n"));

    if (ioctl(pd->mixer_fd, SOUND_MIXER_READ_RECMASK, &inputMask) < 0) {
      DEBG2(printf("ioctl SOUND_MIXER_READ_RECMASK failed: %s\n",
		   strerror(errno)));
      inputMask = SOUND_MASK_MIC;
    }

    for (chan = 0; chan < SOUND_MIXER_NRDEVICES; chan++)
    {
        ioval = 0;

        /* dont reset non-input channels or the current channel
	 */
	if (chan == d->channel) {
	  DEBG4(printf("Not resetting current channel %d\n",chan));
	  continue;
	}
        if ((inputMask & (1 << chan))==0) {
	  DEBG4(printf("Not resetting non-input channel %d\n",chan));
	  continue;
	}

	/* dont reset output or master levels
	 */
	if (chan == SOUND_MIXER_VOLUME || chan == SOUND_MIXER_PCM ||
	    chan == SOUND_MIXER_SPEAKER ||
	    chan == SOUND_MIXER_IGAIN || chan == SOUND_MIXER_OGAIN ||
	    chan == SOUND_MIXER_RECLEV) {
	  DEBG4(printf("Not resetting special channel %d\n",chan));
	  continue;
	}

	DEBG4(printf("zeroing channel %d\n",chan));
        if (ioctl(pd->mixer_fd, MIXER_WRITE(chan), &ioval) < 0) {
	  DEBG2(printf("ioctl MIXER_WRITE channel %d failed: %s\n",
		       chan, strerror(errno)));
	}
    }
}


/* ----------------------------------------------------------openPhysicalDevice
 */
MLstatus
openPhysicalDevice(audioDevice *d, int logicalDevice, physicalDevice **ppd)
{
    MLstatus status = ML_STATUS_NO_ERROR;
    int dirIndex = (d->mode == O_RDONLY) ? DEV_INPUT : DEV_OUTPUT;

    /* look up this device in device table
     */
    physicalDevice *pd = &globalPhysicalDevices[d->devIndex][dirIndex];

    DEBG3(printf("in openPhysicalDevice()\n"));

    /* check to see if it has already been opened by this logical device
     * in the same mode
     */
    if (pd->fd > 0) {
        if (pd->reference & logicalDevice) {
            DEBG1(printf("In use by another device\n"));
	    status = ML_STATUS_INSUFFICIENT_RESOURCES;
	    goto error;
        }
    }
    else {
        char devname[12], mixerDev[16];
	int flags;
	int ioretval, ossQueueMask, fragSize, fragCount;
	double L10_2 = log10(2.0);
	
	assert(pd->reference == 0);
	assert(pd->mixer_fd <= 0);
	
	if (d->devIndex == 0)
	    strcpy(devname, "/dev/dsp");
	else
	    sprintf(devname, "/dev/dsp%d", d->devIndex);

	/* open the audio device
	 */
	pd->mode = d->mode;
	pd->fd = open(devname, d->mode | O_NONBLOCK);

	if (pd->fd < 0) {
	    DEBG1(printf("Open: device open for '%s' failed: %s\n",
	                 devname, strerror(errno)));
            switch (errno) {
	    case EBUSY:		/* we are using it */
	    case EACCES:	/* someone else is using it */
        	status = ML_STATUS_INSUFFICIENT_RESOURCES;
		break;
	    default:
        	status = ML_STATUS_INTERNAL_ERROR;
	    }
	    goto error;
	}
	if (fcntl(pd->fd, F_GETFL, &flags) < 0) {
	    DEBG1(printf("Open: fcntl for '%s' failed: %s\n",
	                 devname, strerror(errno)));
	    close(pd->fd);
	    status = ML_STATUS_INTERNAL_ERROR;
	    goto error;
	}
	flags &= ~O_NONBLOCK;	/* clear this bit */

	if (fcntl(pd->fd, F_SETFL, flags) < 0) {
	    DEBG1(printf("Open: fcntl for '%s' failed: %s\n",
	                 devname, strerror(errno)));
	    close(pd->fd);
	    status = ML_STATUS_INTERNAL_ERROR;
	    goto error;
	}

	DEBG4(printf("device %s opened with fd = %d\n", devname, pd->fd));
	
	fragSize = FRAGMENT_SIZE;
	fragCount = FRAGMENT_COUNT;
        {
	char *env = getenv("FRAG_COUNT");
	if (env) fragCount = atoi(env);
	env = getenv("FRAG_SIZE");
	if (env) fragSize = atoi(env);
	}
	ossQueueMask = (int) (log10(fragSize) / L10_2);
	ossQueueMask |= (fragCount << 16);
        DEBG2(printf("Open: Requesting --> %d frags, size %d bytes\n",
	             fragCount, (int) pow(2.0, (ossQueueMask & 0xff))));
        if (ioctl(pd->fd, SNDCTL_DSP_SETFRAGMENT, &ossQueueMask) < 0) {
	    DEBG1(perror("Open: ioctl(SNDCTL_DSP_SETFRAGMENT) failed"));
	    status = ML_STATUS_INVALID_VALUE;
	    goto error;
	}

	pd->format = ossFormatFromParams(*(audioCompression->deflt),
		*(audioFormat->deflt));
		
	pd->channels = 1;		/* Start with MONO by default */
	if ( ioctl( pd->fd, SNDCTL_DSP_CHANNELS, &pd->channels ) != 0 ) {
	    DEBG1(perror("Open: ioctl(SNDCTL_DSP_CHANNELS) failed"));
	    status = ML_STATUS_INTERNAL_ERROR;
	    goto error;
	}
	
	status = updateSizes(pd, d->mode);
	if (status != ML_STATUS_NO_ERROR)
	    goto error;

	if (ioctl(pd->fd, SOUND_PCM_READ_RATE, &ioretval) == 0)
	{
            pd->rate = ioretval;
	}
	else
	{
            DEBG3(printf("Open: Device does not support "
			 "SOUND_PCM_READ_RATE\n"));
	    pd->rate = 8000;	/* FIXME: ALWAYS TRUE? */
	}
	
	pd->frameCount = 0;	/* RESET MSC */

	/* GAIN_UNSET indicates that user has not set it, so not cached
	 */
	pd->gainIndB[0] = pd->gainIndB[1] = GAIN_UNSET;
	
	/* attempt to open mixer device (not fatal if not present)
	 */
	if (d->devIndex == 0)
            strcpy(mixerDev, "/dev/mixer");
	else
            sprintf(mixerDev, "/dev/mixer%d", d->devIndex);

	pd->mixer_fd = open(mixerDev, O_RDWR);

	if (pd->mixer_fd < 0) {
	    DEBG1(printf("Open: mixer open (%s) failed: %s\n",
	                 mixerDev, strerror(errno)));
	    pd->volume = 100 | (100 << 8);	/* max gain, default */
	}
	else {
	    if (ioctl(pd->mixer_fd, SOUND_MIXER_READ_STEREODEVS,
		      &pd->stereoDevMask))
	    {
	        DEBG1(perror("Open: ioctl(SOUND_MIXER_READ_STEREODEVS) "
			     "failed"));
		pd->stereoDevMask = 0;
	    }

            if (d->mode == O_RDONLY)
	    {
		int mask = 1 << d->channel;
		int ret;
		int volumeFull = (100 << 8) | (100);
		int volumeZero = 0;

		DEBG2(printf("    setting recsrc mask to 0x%x\n", mask));
		ret = ioctl(pd->mixer_fd, SOUND_MIXER_WRITE_RECSRC, &mask);
		if (ret < 0) {
	            DEBG1(perror("Open: SOUND_MIXER_WRITE_RECSRC failed"));
                    status = ML_STATUS_INTERNAL_ERROR;
		    goto error;
		}

		/* OSS docs recommend we check the recsrc to make sure
		 * our request was honoured
		 */
		{
		  int actualMask;
		  ret = ioctl(pd->mixer_fd, SOUND_MIXER_READ_RECSRC,
			      &actualMask);
		  if (ret < 0) {
		    DEBG1(perror("Open: SOUND_MIXER_READ_RECSRC failed"));
		    /* Not an error, we can recover -- but we can't
		     * check the recsrc
		     */
		  } else {
		    if (mask != actualMask) {
		      DEBG1(printf("RECSRC not set properly: "
				   "new settings 0x%x\n",actualMask));
		      /* Is this an un-recoverable error? Nah...
		       */
		    }
		  }
		}

		ret = ioctl(pd->mixer_fd, MIXER_WRITE(d->channel),
			    &volumeFull);
		DEBG2(printf("    setting channel volume to 0x%x\n",
			     volumeFull));
		if (ret < 0)
		{
	            DEBG1(perror("Open: MIXER_WRITE failed"));
                    status = ML_STATUS_INTERNAL_ERROR;
		    goto error;
		}

		ret = ioctl(pd->mixer_fd, SOUND_MIXER_WRITE_RECLEV,
			    &volumeFull);
		DEBG2(printf("    setting reclevel to 0x%x\n", volumeFull));
		if (ret < 0)
		{
	            DEBG1(perror("Open: SOUND_MIXER_WRITE_RECLEV failed"));
		    /* Some sounds cards do not support this mixer
		     * channel -- this is not an error.
		     */
		}

		/* cache the volume and restore it later
		 */
		ioctl(pd->mixer_fd, SOUND_MIXER_READ_VOLUME, &pd->oldVolume);

		DEBG2(printf("    setting gain on VOLUME to zero\n"));
		if (ioctl(pd->mixer_fd, SOUND_MIXER_WRITE_VOLUME,
			  &volumeZero) < 0)
		{
			DEBG1(perror("ioctl(MIXER_WRITE) failed"));
			status = ML_STATUS_INVALID_VALUE;
			goto error;
		}
	    }
	    else
	    {
	      /* mp: the below is not true on low-quality hardware...
	       * no need to set channel on output
	       */
	    }

	    /* retrieve and cache gain
	     */
	    if (ioctl(pd->mixer_fd, MIXER_READ(d->channel), &pd->volume))
	    {
        	DEBG1(perror("Open: ioctl(MIXER_READ) failed"));
		status = ML_STATUS_INTERNAL_ERROR;
		goto error;
	    }
	    DEBG3(printf("Initial OSS volume: %d %d\n",
	                 pd->volume & 0xff, pd->volume >> 8));
	    /* now zero out all channel levels except this one
	     */
	    zeroMixer(d);
	}
    }
    pd->reference |= logicalDevice;	/* set access bit */
    *ppd = pd;				/* assign to external pointer */
    goto done;

error:
    closePhysicalDevice(d, LPATH);
done:
    return status;
}


/* ---------------------------------------------------------closePhysicalDevice
 */
MLstatus closePhysicalDevice (audioDevice *d, int logicalDevice)
{
    MLstatus status = ML_STATUS_NO_ERROR;
    int dirIndex = (d->mode == O_RDONLY) ? DEV_INPUT : DEV_OUTPUT;

    /* look up this device in device table
     */
    physicalDevice *pd = &globalPhysicalDevices[d->devIndex][dirIndex];

    DEBG3(printf("in closePhysicalDevice()\n"));

    /* We killed the volume in opening; restore it.
     */
    if (dirIndex == DEV_INPUT)
    {
       ioctl(pd->mixer_fd, SOUND_MIXER_WRITE_VOLUME, &pd->oldVolume);
    }

    /* close fdescs if not current in use by other logical device
     */
    if (pd->reference == logicalDevice) {
        if (pd->mixer_fd > 0)
            close(pd->mixer_fd);
	pd->mixer_fd = -1;
        close(pd->fd);
	pd->fd = -1;
    }
    pd->reference &= ~logicalDevice;	/* clear access bit */
    return status;
}


/* --------------------------------------------------------reOpenPhysicalDevice
 */
MLstatus
reOpenPhysicalDevice(audioDevice *d)
{
    int dirIndex = (d->mode == O_RDONLY) ? DEV_INPUT : DEV_OUTPUT;
    physicalDevice *pd = &globalPhysicalDevices[d->devIndex][dirIndex];
    char devname[16];
    int flags;

    DEBG3(printf("in reOpenPhysicalDevice()\n"));

    close(pd->fd);
    if (d->devIndex == 0)
	strcpy(devname, "/dev/dsp");
    else
	sprintf(devname, "/dev/dsp%d", d->devIndex);

    /* reopen the audio device
     */
    pd->fd = open(devname, d->mode | O_NONBLOCK);

    if (pd->fd < 0) {
	DEBG1(printf("reOpenPhysicalDevice: device open for '%s' failed: %s\n",
	             devname, strerror(errno)));
        switch (errno) {
	case EBUSY:
            return ML_STATUS_INSUFFICIENT_RESOURCES;
	default:
            return ML_STATUS_INTERNAL_ERROR;
	}
    }
    if (fcntl(pd->fd, F_GETFL, &flags) < 0) {
	DEBG1(printf("reOpenPhysicalDevice: fcntl for '%s' failed: %s\n",
		     devname, strerror(errno)));
	close(pd->fd);
	return ML_STATUS_INTERNAL_ERROR;
    }
    flags &= ~O_NONBLOCK;	/* clear this bit */

    if (fcntl(pd->fd, F_SETFL, flags) < 0) {
	DEBG1(printf("reOpenPhysicalDevice: fcntl for '%s' failed: %s\n",
		     devname, strerror(errno)));
	close(pd->fd);
	return ML_STATUS_INTERNAL_ERROR;
    }
    /* enable duplex if RDWR requested
     */

    if ((d->mode & O_RDWR) == O_RDWR)
        ioctl(pd->fd, SNDCTL_DSP_SETDUPLEX, 0);

    return ML_STATUS_NO_ERROR;
}


/* --------------------------------------------------------audioSetPathControls
 *
 * this is called by ddSetControls, below
 */
MLstatus
audioSetPathControls(audioPath *p, MLpv *msg)
{
    MLstatus status = ML_STATUS_NO_ERROR;
    physicalDevice *pd = p->pdev;
    int n, ret, redoFormat = 0;
    int newrate = 0;
    int volume = -1, idx;
    int fmtIndex = -1, channelsIndex = -1, queueIndex = -1;
    int rateIndex = -1, gainIndex = -1;
    int compressionIndex = -1;
    int newcompression, newformat, newchannels = 0, newqueue = 0;
    int ossFormat = 0, ossChannels = 0;
    MLint32 oldCompressionParam, oldFormatParam;
    MLreal64 userGains[2];
    int oldOSSFormat, oldOSSChannels;

    /* store previous values
     */
    oldOSSFormat = pd->format;
    oldOSSChannels = pd->channels;
    paramsFromOSSFormat(oldOSSFormat, &oldCompressionParam, &oldFormatParam);

    newcompression = oldCompressionParam;
    newformat = oldFormatParam;

    DEBG2(printf("in audioSetPathControls\n"));

    for (n = 0; msg[n].param != ML_END; n++) {
        switch(msg[n].param) {
	case ML_AUDIO_COMPRESSION_INT32:
	    compressionIndex = n;
	    newcompression = msg[n].value.int32;
	    DEBG3(printf("    requesting compression %d\n", newcompression));
	    break;
	case ML_AUDIO_FORMAT_INT32:
	    fmtIndex = n;
	    newformat = msg[n].value.int32;
	    redoFormat =1;
	    DEBG3(printf("    requesting format %d\n", newformat));
	    break;
	case ML_AUDIO_CHANNELS_INT32:
	    channelsIndex = n;
	    newchannels = msg[n].value.int32;
	    break;
	case ML_DEVICE_EVENTS_INT32_ARRAY:
	    {
	        int i;
		MLint32 saved_events = p->eventsWanted;
		p->eventsWanted = NO_EVENTS;	/* reset */
		for (i = 0; i < msg[n].length; i++)
		    switch (msg[n].value.pInt32[i]) {
		    case ML_EVENT_AUDIO_SEQUENCE_LOST:
		        p->eventsWanted |= SEQUENCE_LOST_EVENT;
		        break;
		    default:
		        DEBG1(printf("audioSetPathControls: unsupported "
				     "event: 0x%x\n",
			             msg[n].value.pInt32[i]));
			msg[n].length = -1;	/* mark the invalid param */
			p->eventsWanted = saved_events;	/* restore */
			return ML_STATUS_INVALID_VALUE;
		    }
	    }
	    break;
	case ML_AUDIO_GAINS_REAL64_ARRAY:
	    {
	    int volumes[2];	/* 2 channels is max */
	    gainIndex = n;
	    idx = 0;
	    userGains[idx] = msg[n].value.pReal64[idx];
	    volumes[idx] = ossVolumeFromGain(userGains[idx]);
	    volume = 0;
	    volume |= ((int) volumes[idx] & 0xff);
	    /* OR in right channel if 2 or more gains handed to us
	     */
	    if (msg[n].length > 1)
	        idx = 1;
	    userGains[idx] = msg[n].value.pReal64[idx];
	    volumes[idx] = ossVolumeFromGain(userGains[idx]);
	    volume |= (((int) volumes[idx] & 0xff) << 8);
	    DEBG3(printf("    requesting volume %d, %d\n",
	                 volume & 0xff, volume >> 8));
	    }
	    break;
	case ML_AUDIO_SAMPLE_RATE_REAL64:
	    rateIndex = n;
	    newrate = (int) msg[n].value.real64;
	    DEBG3(printf("    requesting sample rate %d\n", newrate));
	    break;
	default:
	    /* ignore USER class
	     */
	    if (ML_PARAM_GET_CLASS(msg[n].param) == ML_CLASS_USER)
		break;
	    msg[n].length = -1;			/* mark the invalid param */
	    DEBG2(printf("audioSetPathControls: unknown param 0x%"
			 FORMAT_LLX "\n",
			 msg[n].param));
	    return ML_STATUS_INVALID_PARAMETER;
	}
    }

    /* check to see if params have changed.
     */
    if (newcompression != oldCompressionParam || newformat != oldFormatParam)
	redoFormat = TRUE;

    /* zero this if it has not changed
     */
    if (newchannels == pd->channels)
        newchannels = 0;

    if (newqueue) {
	double L10_2 = log10(2.0);
	int ossQueueMask, buffersize;
	int channels = newchannels ? newchannels : pd->channels;
	int fmt = oldOSSFormat;
	
	/* calculate new desired buffersize in bytes from new format info
	 */
	if (newformat)
	    fmt = ossFormatFromParams(newcompression, newformat);
        buffersize = (int) (newqueue * getFramesize(fmt, channels));
	
        status = reOpenPhysicalDevice(&(p->device));
	if (status != ML_STATUS_NO_ERROR)
	    goto error;
	ossQueueMask = (int) (log10(buffersize/4.0) / L10_2);
	ossQueueMask |= (FRAGMENT_COUNT << 16);
        DEBG2(printf("    requesting queuesize %d --> %d frags, "
		     "size %d bytes\n",
	             newqueue, 4, (int) pow(2.0, (ossQueueMask & 0xff))));
        if ((ret = ioctl(pd->fd, SNDCTL_DSP_SETFRAGMENT, &ossQueueMask)) < 0) {
	    DEBG1(perror("ioctl(SNDCTL_DSP_SETFRAGMENT) failed"));
	    if (queueIndex >= 0)
	      msg[queueIndex].length = -1;		/* tag error */
	    status = ML_STATUS_INVALID_VALUE;
	    goto error;
	}
	/* now make sure parameters are reset below
	 */
	redoFormat = TRUE;
	if (!newchannels) newchannels = oldOSSChannels;
	/* we have to reset rate here because it is normally handled
	 * by the jack device
	 */
	status = setDeviceSampleRate(p->pdev, p->pdev->rate);
	if (status != ML_STATUS_NO_ERROR)
	    goto error;
	
    }
    else if (redoFormat || newchannels || newrate) {
        DEBG2(printf("audioSetPathControls: resetting device\n"));
        ioctl(pd->fd, SNDCTL_DSP_RESET);
    }

    if (newchannels) {
        ossChannels = newchannels;
        if (ioctl(pd->fd, SNDCTL_DSP_CHANNELS, &newchannels) < 0) {
            DEBG1(perror("audioSetPathControls: SNDCTL_DSP_CHANNELS failed"));
	    status = ML_STATUS_INTERNAL_ERROR;
	    goto error;
	}
	else {
	  if ( newchannels != ossChannels ) {
	    DEBG1( printf( "audioSetPathControls: SNDCTL_DSP_CHANNELS, "
			   "requested %d channels, got %d instead\n",
			   ossChannels, newchannels ) );
	    if (channelsIndex >= 0) {
	      msg[channelsIndex].length = -1;		/* tag error */
	    }
	    status = ML_STATUS_INVALID_VALUE;
	    goto error;
	  }
	  DEBG3(printf("audioSetPathControls: reset channels to %d\n",
		       newchannels ));
	}
    }
    if (redoFormat) {
        int tryFormat;
	ossFormat = ossFormatFromParams(newcompression, newformat);
	tryFormat = ossFormat;
	DEBG3(printf("    new requested OSS format: 0x%x\n", tryFormat));
	if (tryFormat != -1) {
	    if (ioctl(pd->fd, SNDCTL_DSP_SETFMT, &tryFormat) < 0) {
                DEBG1(printf("audioSetPathControls: SNDCTL_DSP_SETFMT "
			     "failed for format 0x%x: %s\n",
	                     ossFormat, strerror(errno)));
	        status = ML_STATUS_INTERNAL_ERROR;
		goto error;
	    }
	    else if (tryFormat != ossFormat) {
	        DEBG1(printf("audioSetPathControls: actual format (0x%x) "
			     "!= expected (0x%x)\n",
		             tryFormat, ossFormat));
	        if (fmtIndex >= 0)
		  msg[fmtIndex].length = -1;		/* tag error */
	        status = ML_STATUS_INVALID_VALUE;

	        goto error;
	    }
	    else {
                DEBG3(printf("audioSetPathControls: reset format\n"));
	    }
	}
	else {
	    if (fmtIndex >= 0)
	      msg[fmtIndex].length = -1;		/* tag error */
	    status = ML_STATUS_INVALID_VALUE;
	    goto error;
	}
    }

    /* now change the parameters
     */
    if (rateIndex >= 0 && newrate != pd->rate) {
        status = setDeviceSampleRate(pd, newrate);
	if (status != ML_STATUS_NO_ERROR) {
	  msg[rateIndex].length = -1;	/* tag error */
	  goto error;
	}
    }

    if (gainIndex >= 0)
    {
        int ioval = volume;

	/* if we are recording...
	 */
	if (pd->mode == O_RDONLY)
	{
		int volumeZero = 0, volumeFull = (100<<8) | 100;
		DEBG2(printf("    WE ARE RECORDING. MODE IS READ ONLY\n"));

		DEBG2(printf("    setting gain on channel %d to full\n",
			p->device.channel));
		if (ioctl(pd->mixer_fd, MIXER_WRITE(p->device.channel),
			  &volumeFull))
		{
		  msg[gainIndex].length = -1;	/* tag error */
		  DEBG1(perror("ioctl(MIXER_WRITE) failed"));
		  status = ML_STATUS_INVALID_VALUE;
		  goto error;
		}

		DEBG2(printf("    setting gain on RECLEV to 0x%x\n", ioval));
		if (ioctl(pd->mixer_fd, SOUND_MIXER_WRITE_RECLEV, &ioval) < 0)
		{
		  /* Some sound cards do not support this channel, so
		   * don't make this an error
		   */
		  DEBG1(perror("ioctl(MIXER_WRITE) failed"));
		}

		DEBG2(printf("    setting gain on VOLUME to zero\n"));
		if (ioctl(pd->mixer_fd, SOUND_MIXER_WRITE_VOLUME,
			  &volumeZero) < 0)
		{
		  msg[gainIndex].length = -1;	/* tag error */
		  DEBG1(perror("ioctl(MIXER_WRITE) failed"));
		  status = ML_STATUS_INVALID_VALUE;
		  goto error;
		}
	}

	/* if we are playing...
	 */
	if (pd->mode == O_WRONLY)
	{
		DEBG2(printf("    WE ARE PLAYING. MODE IS WRITE ONLY\n"));
	ioval = volume;
	DEBG2(printf("    setting gain on VOLUME to ioval\n"));
	if (ioctl(pd->mixer_fd, SOUND_MIXER_WRITE_VOLUME, &ioval) < 0)
	{
	  msg[gainIndex].length = -1;	/* tag error */
	  DEBG1(perror("ioctl(MIXER_WRITE) failed"));
	  status = ML_STATUS_INVALID_VALUE;
	  goto error;
	}

#ifdef NO_SPEAKER_CHANNEL

        /* set the PCM mixer channel as well -- this way we work on
	 * any driver which supports either VOLUME or PCM for output
	 * gains.
	 *
	 * FIXME: THIS WILL NOT WORK RIGHT FOR DRIVERS WHICH SUPPORT
	 * *BOTH*
	 */
	DEBG2(printf("    HACK: also setting gain on channel %d\n",
	             SOUND_MIXER_PCM));
        ioval = volume;
	ioctl(pd->mixer_fd, SOUND_MIXER_WRITE_PCM, &ioval);
#endif
	}
    }

    /* cache new state
     */
    if (redoFormat)
        pd->format = ossFormat;
    if (newchannels)
        pd->channels = ossChannels;
    if (newrate)
        pd->rate = newrate;
    if (volume >= 0) {
        pd->volume = volume;
	pd->gainIndB[0] = userGains[0];
	if (pd->channels == 2)
	    pd->gainIndB[1] = userGains[1];
    }

    status = updateSizes(pd, p->device.mode);
    if (status != ML_STATUS_NO_ERROR)
        goto error;

    goto done;

error:
    /* undo state changes
     */
    ioctl(pd->fd, SNDCTL_DSP_SETFMT, &pd->format);
    {
      int chnls = pd->channels;
      ioctl(pd->fd, SNDCTL_DSP_CHANNELS, &chnls);
    }
    setDeviceSampleRate(pd, pd->rate);
done:
    return status;
}


/* ---------------------------------------------------------------ddSetControls
 */
MLstatus ddSetControls(MLbyte* ddPriv,
		       MLopenid openObjectId,
		       MLpv *msg)
{
  MLstatus status = ML_STATUS_NO_ERROR;
  audioPath *p = (audioPath *)ddPriv;

  if( p == NULL )
    return ML_STATUS_INTERNAL_ERROR;

    pthread_mutex_lock(&(p->mutex));

    /* Look first to see if this call resulted from a begin- or
     * end-transfer call. If so, do this only, otherwise parse param
     * list
     */

  if (msg[0].param == ML_DEVICE_STATE_INT32) {
        int wasTransferring = (p->state == ML_DEVICE_STATE_TRANSFERRING);
	int newstate = msg[0].value.int32;

        DEBG3(printf("mlAudioSetPathControls: p->state now %d\n", p->state));
	
	/* Should device go to transferring state?
	 */
	if (newstate == ML_DEVICE_STATE_TRANSFERRING) {
	    if (wasTransferring)
	        return ML_STATUS_NO_OPERATION;
	    DEBG2(printf("mlAudioSetPathControls: entering transfer state\n"));
	    p->state = newstate;
#ifndef START_CHILD_IMMEDIATELY
            status = startUpChild(p);
#else
	    status = wakeUp(p);
#endif
	}
	else if (newstate == ML_DEVICE_STATE_ABORTING) {
	    DEBG2(if (wasTransferring)
		  printf("mlAudioSetPathControls: leaving transfer state\n"));

#ifndef START_CHILD_IMMEDIATELY
	    shutDownChild(p, newstate);	/* causes child to exit */
#endif
	    mlDIQueueAbortMessages(p->pQueue);	/* now in parent process */
	}
	else
	    p->state = newstate;
    }
  else {
    /* Before attempting to set the controls, make sure the entire
     * request message is valid at this time.
     */
    MLint32 accessNeeded = ML_ACCESS_WRITE | ML_ACCESS_IMMEDIATE;

    /* If we are currently in the midst of a transfer, make that part
     * of the required access flags
     */
    if ( p->state == ML_DEVICE_STATE_TRANSFERRING ) {
      accessNeeded |= ML_ACCESS_DURING_TRANSFER;
    }

    status = mlDIvalidateMsg( openObjectId, accessNeeded,
			      ML_FALSE /* NOT used in get-controls */, msg );
    if ( status == ML_STATUS_NO_ERROR ) {
      status = audioSetPathControls(p, msg);
    }
  }

  pthread_mutex_unlock(&(p->mutex));

  return status;
}


/* --------------------------------------------------------audioGetPathControls
 *
 * Called for mlGetControls and mlQueryControls
 */
MLstatus
audioGetPathControls( audioPath* p, MLpv *msg )
{
  MLstatus status = ML_STATUS_NO_ERROR;
  physicalDevice *pd = p->pdev;
  int n;
  MLint32 mlCompression, mlFormat, mlChannels, mlSampleWidth;

  DEBG2(printf("in audioGetPathControls() [OSS audio device]\n"));

  /* Retrieve and translate current params
   */
  paramsFromOSSFormat( pd->format, &mlCompression, &mlFormat );
  mlChannels = (pd->channels == 2) ? ML_CHANNELS_STEREO : ML_CHANNELS_MONO;
  mlSampleWidth = getSampleWidthFromFormat( mlFormat, TRUE );

  for ( n = 0; msg[n].param != ML_END; n++ ) {
    switch ( msg[n].param ) {

    case ML_QUEUE_SEND_COUNT_INT32:
      msg[n].value.int32 = mlDIQueueGetSendCount( p->pQueue );
      break;

    case ML_QUEUE_RECEIVE_COUNT_INT32:
      msg[n].value.int32 = mlDIQueueGetReceiveCount( p->pQueue );
      break;

    case ML_QUEUE_SEND_WAITABLE_INT64:
      msg[n].value.int64 = (MLint64)mlDIQueueGetSendWaitable( p->pQueue );
      break;

    case ML_QUEUE_RECEIVE_WAITABLE_INT64:
      msg[n].value.int64 = (MLint64)mlDIQueueGetReceiveWaitable( p->pQueue );
      break;

    case ML_AUDIO_FRAME_SIZE_INT32:
      msg[n].value.int32 = mlChannels * (mlSampleWidth + 7) / 8;
      break;

    case ML_AUDIO_COMPRESSION_INT32:
      msg[n].value.int32 = mlCompression;
      break;

    case ML_AUDIO_FORMAT_INT32:
      msg[n].value.int32 = mlFormat;
      break;

    case ML_AUDIO_CHANNELS_INT32:
      msg[n].value.int32 = (pd->channels == 2) ?
	ML_CHANNELS_STEREO : ML_CHANNELS_MONO;
      break;

    case ML_DEVICE_EVENTS_INT32_ARRAY:
      if ( msg[n].maxLength == 0 ) {
	if ( (p->eventsWanted & SEQUENCE_LOST_EVENT) == SEQUENCE_LOST_EVENT ) {
	  msg[n].maxLength = 1;
	} else {
	  msg[n].maxLength = 0;
	}
	msg[n].length = 0;
	break;
      }

      /* NULL check
       */
      if ( msg[n].value.pInt32 == NULL ) {
	DEBG1( printf( "audioGetPathControls [OSS audio device]: "
		       "NULL array pointer\n" ) );
	msg[n].length = -1;
	return ML_STATUS_INVALID_VALUE;
      }

      msg[n].length = 0;
      if ( p->eventsWanted & SEQUENCE_LOST_EVENT ) {
	msg[n].value.pInt32[0] = ML_EVENT_AUDIO_SEQUENCE_LOST;
	msg[n].length = 1;
      }
      break;

    case ML_AUDIO_SAMPLE_RATE_REAL64:
      msg[n].value.real64 = pd->rate;
      break;

    case ML_AUDIO_GAINS_REAL64_ARRAY:
      if ( msg[n].maxLength == 0 ) {
	msg[n].maxLength = pd->channels;
	break;
      }

      /* NULL check
       */
      if ( msg[n].value.pReal64 == NULL ) {
	DEBG1( printf( "audioGetPathControls [OSS audio device]: "
		       "NULL array pointer\n" ) );
	msg[n].length = -1;
	return ML_STATUS_INVALID_VALUE;
      }

      /* Use user-set cached value if present
       */
      if ( pd->gainIndB[0] != GAIN_UNSET ) {
	msg[n].value.pReal64[0] = pd->gainIndB[0];
      }	else if ( (pd->volume & 0xff) == 0 ) {
	msg[n].value.pReal64[0] = -60;
      }	else {
	msg[n].value.pReal64[0] = gainFromOSSVolume( pd->volume & 0xff );
      }
      msg[n].length = 1;
      /* Return right channel if we are stereo and 2 or more gains
       * handed to us
       */
      if ( (msg[n].maxLength > 1) && (pd->channels == 2) ) {
	/* Use user-set cached value if present
	 */
	if ( pd->gainIndB[1] != GAIN_UNSET ) {
	  msg[n].value.pReal64[1] = pd->gainIndB[1];
	} else if ( ((pd->volume >> 8) & 0xff) == 0 ) {
	  msg[n].value.pReal64[1] = -60;
	} else {
	  msg[n].value.pReal64[1] =
	    gainFromOSSVolume((pd->volume >> 8) & 0xff);
	}
	msg[n].length = 2;
      }
      break;

    default:
      /* Ignore USER class
       */
      if ( ML_PARAM_GET_CLASS( msg[n].param) == ML_CLASS_USER ) {
	break;
      }
      msg[n].length = -1; /* Mark the invalid param */
      DEBG2( printf( "audioGetPathControls [OSS audio device]: returning "
		     "ML_STATUS_INVALID_PARAMETER\n" ) );
      return ML_STATUS_INVALID_PARAMETER;
    } /* switch */
  } /* for n=0.. */
  return status;
}


/* ---------------------------------------------------------------ddGetControls
 */
MLstatus ddGetControls (MLbyte *ddPriv, MLopenid openObjectId, MLpv *msg)
{
  MLstatus status;
  audioPath *p = (audioPath *) ddPriv;
  MLint32 accessNeeded = ML_ACCESS_RI;

  /* If we are currently in the midst of a transfer, make that part of
   * the required access flags
   */
  if ( p->state == ML_DEVICE_STATE_TRANSFERRING ) {
    accessNeeded |= ML_ACCESS_DURING_TRANSFER;
  }

  status = mlDIvalidateMsg( openObjectId, accessNeeded,
			    ML_TRUE /* used in get-controls */, msg );
  if ( status == ML_STATUS_NO_ERROR ) {
    status = audioGetPathControls( p, msg );
  }

  return status;
}


/* --------------------------------------------------------------ddSendControls
 */
MLstatus ddSendControls (MLbyte *ddPriv, MLopenid openObjectId, MLpv *controls)
{
  audioPath *p = (audioPath *) ddPriv;
  DEBG2(printf("in mlAudioSendPathControls()\n"));

  if( p == NULL )
    return ML_STATUS_INTERNAL_ERROR;

  return mlDIQueuePushMessage(p->pQueue,
			      ML_CONTROLS_IN_PROGRESS,
			      controls,
			      0, 0, 0);
}


/* -------------------------------------------------------------ddQueryControls
 */
MLstatus ddQueryControls(MLbyte* ddPriv,
			 MLopenid openObjectId,
			 MLpv *controls)
{
  audioPath *p = (audioPath *) ddPriv;
  MLstatus status = ML_STATUS_NO_ERROR;

  if ( p == NULL ) {
    DEBG1( printf( "mlaudio ddQueryControls: invalid ddPriv\n" ) );
    status = ML_STATUS_INTERNAL_ERROR;

  } else {
    status = mlDIQueuePushMessage( p->pQueue,
				   ML_QUERY_IN_PROGRESS,
				   controls,
				   0, 0, 0 );
  }

  return status;
}


/* ---------------------------------------------------------------ddSendBuffers
 */
MLstatus ddSendBuffers(MLbyte* ddPriv,
		       MLopenid openObjectId,
		       MLpv *buffers)
{
    audioPath *p = (audioPath *) ddPriv;
    DEBG2(printf("in mlAudioSendBuffers()\n"));
#if !defined(SELECT_ON_DEVICE) && defined(START_CHILD_IMMEDIATELY)
    /* we only wake up the xfer thread if we are in xfer mode
     */
    if (p->state == ML_DEVICE_STATE_TRANSFERRING) {
#endif
      wakeUp(p);
#if !defined(SELECT_ON_DEVICE) && defined(START_CHILD_IMMEDIATELY)
    }
#endif
  return mlDIQueuePushMessage(p->pQueue,
			      ML_BUFFERS_IN_PROGRESS,
			      buffers,
			      0, 0, 0);
}


/* ------------------------------------------------------------ddReceiveMessage
 */
MLstatus ddReceiveMessage (MLbyte *ddPriv, MLopenid openObjectId,
	MLint32 *retMsgType, MLpv **retReply)
{
	audioPath *p = (audioPath *) ddPriv;
	if (p == NULL)
		return ML_STATUS_INTERNAL_ERROR;

	/* Just ask our queue to return the next message to the app.
	 */
	return mlDIQueueReceiveMessage(p->pQueue, 
				       (enum mlMessageTypeEnum*)retMsgType,
				       retReply, 0, 0);
}


/* -----------------------------------------------------------------ddXcodeWork
 */
MLstatus ddXcodeWork (MLbyte *ddPriv, MLopenid openObjectId)
{
  return ML_STATUS_INTERNAL_ERROR; /* not applicable for paths */
}


/* ============================================================================
 *
 * END OF STATIC DSO FUNCTIONS
 *
 */


/* ---------------------------------------------------------ossFormatFromParams
 */
int ossFormatFromParams (MLint32 compression, MLint32 format)
{
	switch (compression)
	{
		case ML_COMPRESSION_UNCOMPRESSED:
			break;
		case ML_COMPRESSION_MU_LAW:
			return AFMT_MU_LAW;
		case ML_COMPRESSION_A_LAW:
			return AFMT_A_LAW;
		case ML_COMPRESSION_IMA_ADPCM:
			return AFMT_IMA_ADPCM;
		default:
			assert(FALSE);
	}

	switch (format)
	{
		case ML_AUDIO_FORMAT_S8:
			return AFMT_S8;
		case ML_AUDIO_FORMAT_U8:
			return AFMT_U8;
		case ML_AUDIO_FORMAT_S16:
			return AFMT_S16_LE;
		case ML_AUDIO_FORMAT_U16:
			return AFMT_U16_LE;
		default:
			assert(FALSE);
	}

	return -1;	/*NOTREACHED*/
}


/* ----------------------------------------------------getSampleWidthFromFormat
 */
MLint32 getSampleWidthFromFormat (MLint32 mlFormat, MLboolean expand3to4)
{
	switch (mlFormat)
	{
		case ML_AUDIO_FORMAT_U8:
		case ML_AUDIO_FORMAT_S8:
			return 8;
		case ML_AUDIO_FORMAT_U16:
		case ML_AUDIO_FORMAT_S16:
			return 16;
		case ML_AUDIO_FORMAT_S24in32R:
			if (expand3to4)
				return 32;
			else
				return 24;
		case ML_AUDIO_FORMAT_R32:
			return 32;
		case ML_AUDIO_FORMAT_R64:
			return 64;

		default:
			assert(FALSE);
	}

	return -1;
}


/* ---------------------------------------------------------paramsFromOSSFormat
 */
void paramsFromOSSFormat (int ossFmt, MLint32 *compression, MLint32 *format)
{
	switch (ossFmt)
	{
		case AFMT_MU_LAW:
			*compression = ML_COMPRESSION_MU_LAW;
			*format = ML_AUDIO_FORMAT_S16;
			break;
		case AFMT_A_LAW:
			*compression = ML_COMPRESSION_A_LAW;
			*format = ML_AUDIO_FORMAT_S16;
			break;
		case AFMT_IMA_ADPCM:
			*compression = ML_COMPRESSION_IMA_ADPCM;
			*format = ML_AUDIO_FORMAT_S16;
			break;
		case AFMT_U8:
			*compression = ML_COMPRESSION_UNCOMPRESSED;
			*format = ML_AUDIO_FORMAT_U8;
			break;
		case AFMT_S16_LE:
			*compression = ML_COMPRESSION_UNCOMPRESSED;
			*format = ML_AUDIO_FORMAT_S16;
			break;
		case AFMT_S8:
			*compression = ML_COMPRESSION_UNCOMPRESSED;
			*format = ML_AUDIO_FORMAT_S8;
			break;
		case AFMT_U16_LE:
			*compression = ML_COMPRESSION_UNCOMPRESSED;
			*format = ML_AUDIO_FORMAT_U16;
			break;
		default:
			*compression = -1;
			*format = -1;
			assert(FALSE);
	}
}


/* -----------------------------------------------------------gainFromOSSVolume
 */
double gainFromOSSVolume (unsigned char volume)
{
	double gain;

#ifdef OSS_GAIN_LINEAR
	gain = -60.0 + 60.0 * volume / 100.0;
#else
	gain = 20 * log10(volume) / 100.0;
	DEBG3(printf("    gain linear: %d => dB %g\n", volume, gain));
#endif

	return gain;
}


/* -----------------------------------------------------------ossVolumeFromGain
 */
unsigned char ossVolumeFromGain (double gain)
{
	unsigned char volume;

#ifdef OSS_GAIN_LINEAR
	/* convert -60 -> 0 to 0 -> 100
	 */
	volume = 100.0 * (1.0 + (gain/60.0));
#else
	/* convert from decibels back to linear
	 */
	volume = 100.0 * pow(10.0, gain * 0.05);
	DEBG3(printf("    gain dB: %g => linear: %d\n", gain, volume));
#endif

	return volume;
}


/* --------------------------------------------------------------channelToIndex
 */
int channelToIndex (const char *channelName)
{
    int i;
#ifdef NO_SPEAKER_CHANNEL	/* see note at file top */
    if (!strcmp(channelName, device_names[SOUND_MIXER_SPEAKER]))
        return SOUND_MIXER_VOLUME;
#endif
    for (i = 0; i < SOUND_MIXER_NRDEVICES; i++)
        if (!strcmp(channelName, device_names[i]))
	    return i;
    return -1;
}


/* -------------------------------------------------------------createSemaphore
 */
MLstatus
createSemaphore(audioPath *p)
{
    if (pipe(p->pipe_fds) < 0) {
        DEBG1(printf("createSemaphore:  pipe failed: %s\n", strerror(errno)));
	return ML_STATUS_INSUFFICIENT_RESOURCES;
    }

    return ML_STATUS_NO_ERROR;
}


/* ------------------------------------------------------------destroySemaphore
 */
void
destroySemaphore(audioPath *p)
{
    if (p->pipe_fds[PIPE_READ] > 0) {
        close(p->pipe_fds[PIPE_READ]);
        close(p->pipe_fds[PIPE_WRITE]);
    }
}


/* ----------------------------------------------------handleInputBufferMessage
 */
MLstatus
handleInputBufferMessage(audioPath *c, MLpv* msg)
{
    MLstatus status = ML_STATUS_NO_ERROR;
    audioPath *p = (audioPath *) c;
    MLpv *pvlist = msg;
    int n, bytes = 0;

    DEBG3(printf("---- in handleInputBufferMessage()\n"));

    for (n = 0;
	 pvlist[n].param != ML_END && p->state == ML_DEVICE_STATE_TRANSFERRING;
	 n++)
    {
	switch(pvlist[n].param) {
	case ML_WAIT_FOR_UST_INT64:
	    {
            MLint64 currentUST, currentMSC, waitMSC, deltaMSC;
	    MLint64 targetUST = pvlist[n].value.int64;
	    USTSourceFunc(&currentUST);
	    currentMSC = p->pdev->frameCount;
	    waitMSC = (targetUST - currentUST) * p->pdev->rate * 0.000000001
	      + currentMSC;
	    DEBG3(printf("---- Waiting for UST %" FORMAT_LLD
			 " (delta = %" FORMAT_LLD " frames)\n",
	                 targetUST, waitMSC - currentMSC));
	    if ((deltaMSC = waitMSC - currentMSC) > 0) {
	        int frames;
		short buf[2 * 100];	/* max chans * 100 */
		int bytes = p->pdev->framesize * 100;
		for (frames = 0; frames < deltaMSC; frames += 100) {
		    read(p->pdev->fd, buf, bytes);
		    p->pdev->frameCount += 100;
		}
	    }
	    }
	    break;
	case ML_AUDIO_BUFFER_POINTER:
	    DEBG3(printf("---- reading buffer of %d bytes",
			 pvlist[n].maxLength));
	    DEBG4(printf(" from fd %d", p->pdev->fd));
	    bytes = read(p->pdev->fd, pvlist[n].value.pByte,
			 pvlist[n].maxLength);
	    DEBG3(printf("...got %d\n", bytes));
#ifndef NDEBUG
	    {
	      int i;
	      for(i=0; i<32; i++)
		DEBG4(printf("%d ", (int)pvlist[n].value.pByte[i]));
	      DEBG4(printf("\n"));
	    }
#endif
	    if (bytes < 0) {
	        DEBG1(perror("---- handleInputBufferMessage: read failed"));
		pvlist[n].length = -1;
	        return ML_STATUS_INTERNAL_ERROR;
	    }
	    pvlist[n].length = bytes;
	    /* Increment MSC
	     */
	    p->pdev->frameCount += (int)(bytes / p->pdev->framesize);
	    break;
	case ML_AUDIO_UST_INT64:
	    USTSourceFunc(&pvlist[n].value.int64);
	    break;
	case ML_AUDIO_MSC_INT64:
	    pvlist[n].value.int64 = p->pdev->frameCount;
	    break;
	default:
	    DEBG2(printf("---- got unknown param type: 0x%" FORMAT_LLX "\n",
	                 pvlist[n].param));
	    pvlist[n].length = -1;
	}
    }
    return status;
}


/* -------------------------------------------------------------------zeroValue
 */
/* XXXmpruett: This routine will not work for AFMT_U16. */
unsigned char zeroValue (int ossFormat)
{
	switch (ossFormat)
	{
		case AFMT_MU_LAW:
			return 0xff;
		case AFMT_A_LAW:
			return 0xd5;
		case AFMT_IMA_ADPCM:
		case AFMT_U8:
			return 127;
		case AFMT_S16_LE:
		case AFMT_S16_BE:
		case AFMT_S8:
		default:
			return 0;
	}
}


/* ---------------------------------------------------handleOutputBufferMessage
 *
 * Extract audio data from FIFO and write it to the port.  We always attempt
 * to write the entire buffer at one time;  it is up to the user to ensure
 * that it will not block
 */
MLstatus
handleOutputBufferMessage(audioPath *c, MLpv* msg)
{
    MLstatus status = ML_STATUS_NO_ERROR;
    audioPath *p = (audioPath *) c;
    MLpv *pvlist = msg;
    int n, bytes = 0;

    DEBG3(printf("---- in handleOutputBufferMessage()\n"));

    for (n = 0; pvlist[n].param != ML_END && p->state ==
	   ML_DEVICE_STATE_TRANSFERRING; n++) {
	switch(pvlist[n].param) {
	case ML_WAIT_FOR_UST_INT64:
	    {
            MLint64 currentUST, currentMSC, waitMSC, deltaMSC;
	    MLint64 targetUST = pvlist[n].value.int64;
	    USTSourceFunc(&currentUST);
	    currentMSC = p->pdev->frameCount;
	    waitMSC = (targetUST - currentUST) * p->pdev->rate * 0.000000001
	      + currentMSC;
	    DEBG3(printf("---- Waiting for UST %" FORMAT_LLD
			 " (delta = %" FORMAT_LLD " frames)\n",
	                 targetUST, waitMSC - currentMSC));
	    if ((deltaMSC = waitMSC - currentMSC) > 0) {
	        int frames;
		short buf[2 * 100];	/* max chans * 100 */
		int bytes = p->pdev->framesize * 100;
		memset(buf, zeroValue(p->pdev->format), bytes);
		for (frames = 0; frames < deltaMSC; frames += 100) {
		    write(p->pdev->fd, buf, bytes);
		    p->pdev->frameCount += 100;
		}
	    }
	    }
	    break;
	case ML_AUDIO_BUFFER_POINTER:
	    if (pvlist[n].value.pByte == NULL) {
	        DEBG1(printf("---- handleOutputBufferMessage: "
			     "NULL buffer pointer\n"));
	        pvlist[n].length = -1;
		return ML_STATUS_INVALID_VALUE;
	    }
	    DEBG3(printf("---- writing buffer of %d bytes", pvlist[n].length));
	    DEBG4(printf(" to fd %d", p->pdev->fd));
	    bytes = write(p->pdev->fd, pvlist[n].value.pByte,
			  pvlist[n].length);
	    DEBG3(printf("...wrote %d\n", bytes));
	    if (bytes < 0) {
	        DEBG1(perror("---- handleOutputBufferMessage: write failed"));
		pvlist[n].length = -1;
	        return ML_STATUS_INTERNAL_ERROR;
	    }
	    pvlist[n].length = bytes;
	    /* Increment MSC
	     */
	    p->pdev->frameCount += (int)(bytes / p->pdev->framesize);
	    break;
	case ML_AUDIO_UST_INT64:
	    USTSourceFunc(&pvlist[n].value.int64);
	    break;
	case ML_AUDIO_MSC_INT64:
	    pvlist[n].value.int64 = p->pdev->frameCount;
	    break;
	default:
 	    /* ignore USER class
	     */
	    if (ML_PARAM_GET_CLASS(pvlist[n].param) == ML_CLASS_USER)
	        break;
	    DEBG1(printf("---- handleOutputBufferMessage: "
			 "unknown param type: 0x%" FORMAT_LLX "\n",
	                 pvlist[n].param));
	    pvlist[n].length = -1;
	    return ML_STATUS_INVALID_PARAMETER;
	}
    }
    return status;
}


typedef MLstatus (*BufferMessageFun)(audioPath *, MLpv*);


/* -------------------------------------------------------------------audioLoop
 */
MLstatus
audioLoop(audioPath *c)
{
    audioPath *p = (audioPath *) c;
    fd_set aset, pipeset;
    fd_set *rsetp, *wsetp;
    int nfds = 0;
    int ret, errorReported = 0;
    int reading = (p->device.mode == O_RDONLY);
    char ch;
    MLstatus status = ML_STATUS_NO_ERROR;
    MLqueueEntry* entry = 0;
    MLint32 msgType;
    MLint32 newType;
    MLpv* msg;
    BufferMessageFun bufferMessageFun;

    DEBG3(printf("---- entering audioLoop()\n"));

    if (p->state == ML_DEVICE_STATE_FINISHING) {
        DEBG2(printf("---- audioLoop exiting before main loop reached\n"));
        return ML_STATUS_NO_ERROR;
    }

    FD_ZERO(&aset);
    FD_ZERO(&pipeset);

#ifdef SELECT_ON_DEVICE
    nfds = p->pdev->fd + 1;
#endif
    /* This logic allows a single call to select() to handle both the
     * read and write fd's
     */
    if (reading) {
        rsetp = &aset;	
	wsetp = NULL;
	bufferMessageFun = handleInputBufferMessage;
    }
    else {
	rsetp = &pipeset;
	wsetp = &aset;
	bufferMessageFun = handleOutputBufferMessage;
    }
    if (p->pipe_fds[PIPE_READ] + 1 > nfds)
	nfds = p->pipe_fds[PIPE_READ] + 1;

    while (1) {
        int hungry = 0;
#ifdef SELECT_ON_DEVICE
	FD_SET(p->pdev->fd, &aset);
#endif
	FD_SET(p->pipe_fds[PIPE_READ], rsetp);
        DEBG3(printf("---- audioLoop: just before select()\n"));
	/* wait on pipe's fdesc and audio dev fdesc
	 */
        ret = select(nfds, rsetp, wsetp, NULL, NULL);
	if (ret < 0) {
	    DEBG1(printf("---- audioLoop: select returned %d: %s\n",
	          ret, strerror(errno)));
	    return ML_STATUS_INTERNAL_ERROR;
	}
	DEBG4(printf("---- audioLoop: p->state = '%s' (0x%x)\n",
		     mlDeviceStateName( p->state ), p->state));

	if (FD_ISSET(p->pipe_fds[PIPE_READ], rsetp)) {
	    DEBG3(printf("---- audioLoop: awakened by parent\n"));
	    read(p->pipe_fds[PIPE_READ], &ch, 1);
	}
	if (FD_ISSET(p->pdev->fd, &aset)) {
	    hungry = 1;
	    DEBG3(printf("---- audioLoop: awakened by audio device\n"));
	}

	if (p->state == ML_DEVICE_STATE_FINISHING) {
	    DEBG2(printf("---- audioLoop exiting normally\n"));
	    return ML_STATUS_NO_ERROR;
	}
	else if (p->state == ML_DEVICE_STATE_ABORTING) {
	    DEBG2(printf("---- audioLoop state == ABORTING\n"));
#ifdef START_CHILD_IMMEDIATELY
	    read(p->pipe_fds[PIPE_READ], &ch, 1);
	    DEBG3(printf("---- resetting state\n"));
	    p->state = ML_DEVICE_STATE_READY;
	    DEBG3(printf("---- read from pipe, back to select()\n"));
	    continue;
#else
	    DEBG2(printf("---- audioLoop exiting normally\n"));
	    return ML_STATUS_NO_ERROR;
#endif
	}

	/* At this point, we may or may not be transferring...
	 *
	 * FIXME: if we are not transferring, we should be updating
	 * the MSC anyway
	 *
	 * Now attempt to retrieve a message and process it. But note
	 * that we may have already retrieved a message previously
	 * that we were not able to process -- in that case, that
	 * message is still "pending" and we can't de-queue a new
	 * message.
	 */
	if ( entry == NULL ) {
	  DEBG3( printf( "---- checking message queue\n" ) );
	  status = mlDIQueueNextMessage(p->pQueue, &entry,
					(enum mlMessageTypeEnum*)&msgType,
					&msg, NULL, NULL);

	  assert( (status == ML_STATUS_NO_ERROR) ||
		  (entry == NULL && status == ML_STATUS_RECEIVE_QUEUE_EMPTY) );
	  if ( (status != ML_STATUS_NO_ERROR) &&
	       (status != ML_STATUS_RECEIVE_QUEUE_EMPTY) ) {
	    DEBG1( printf( "[mlaudio] message queue error, status = %s",
			   mlStatusName( status ) ) );
	    break;
	  }
	}

	/* If we are in transfer mode, check for queue underflow, and
	 * get the current sample count.
	 */
	if ( p->state == ML_DEVICE_STATE_TRANSFERRING ) {

	  if (entry == NULL && status == ML_STATUS_RECEIVE_QUEUE_EMPTY) {
	    DEBG3(printf("---- queue empty\n"));
            if (p->pdev->caps.hasRealTime) {
	      count_info info;
	      ioctl(p->pdev->fd,
		    reading ? SNDCTL_DSP_GETIPTR : SNDCTL_DSP_GETOPTR,
		    &info);
	      DEBG3(printf("---- INFO: total bytes: %d  current offset %d\n",
			   info.bytes, info.ptr));
	    }
	    if (hungry) {
	      char buf[FRAGMENT_SIZE];
	      DEBG2(printf("---- Over/Underflow!\n"));
	      /* write or read zero buffer in case of underflow
	       */
	      if (reading) {
		read(p->pdev->fd, buf, FRAGMENT_SIZE);
	      }
	      else {
		memset(buf, zeroValue(p->pdev->format), FRAGMENT_SIZE);
		write(p->pdev->fd, buf, FRAGMENT_SIZE);
	      }
	      p->pdev->frameCount += FRAGMENT_SIZE / p->pdev->framesize;

	      if ((p->eventsWanted & SEQUENCE_LOST_EVENT) &&
		  !errorReported) {
		MLpv evtPV[3];
		evtPV[0].param = ML_AUDIO_UST_INT64;
		USTSourceFunc(&evtPV[0].value.int64);
		evtPV[0].length = 1;
		evtPV[1].param = ML_AUDIO_MSC_INT64;
		evtPV[1].value.int64 = p->pdev->frameCount;
		evtPV[1].length = 1;
		evtPV[2].param = ML_END;
		mlDIQueueReturnEvent(p->pQueue,
				     ML_EVENT_AUDIO_SEQUENCE_LOST,
				     evtPV);
		errorReported = TRUE;
	      }
	    }
	    continue;
	  }

	  if (p->pdev->caps.hasRealTime) {
	    count_info info;
	    ioctl(p->pdev->fd,
		  reading ? SNDCTL_DSP_GETIPTR : SNDCTL_DSP_GETOPTR,
		  &info);
	    DEBG3(printf("---- INFO: total bytes: %d  current offset %d\n",
		         info.bytes, info.ptr));
	  }
	} /* if p->state == ML_DEVICE_STATE_TRANSFERRING */

	/* Now process the message (if possible). Start by
	 * initialising the response message type to "INVALID" -- if
	 * we don't reset it to something valid, that means the
	 * message was not processed, and no response should be sent.
	 */
	if ( entry != NULL ) {
	  newType = ML_MESSAGE_INVALID;

	  switch (msgType)
	    {
	    case ML_CONTROLS_IN_PROGRESS: {
	      /* It is always possible to process a controls message
	       * -- whether or not a transfer is in progress
	       */
	      MLint32 accessNeeded = ML_ACCESS_WRITE | ML_ACCESS_QUEUED;

	      /* Start by validating message
	       */
	      if ( p->state == ML_DEVICE_STATE_TRANSFERRING ) {
		accessNeeded |= ML_ACCESS_DURING_TRANSFER;
	      }
	      status = mlDIvalidateMsg( p->openId, accessNeeded,
					ML_FALSE /* NOT get-controls */, msg );
	      if ( status == ML_STATUS_NO_ERROR ) {
		status = audioSetPathControls(p, msg);
	      }
	      newType = (status == ML_STATUS_NO_ERROR) ?
		ML_CONTROLS_COMPLETE : ML_CONTROLS_FAILED;
	    } break;

	    case ML_BUFFERS_IN_PROGRESS:
	      /* Only process buffers messages if a transfer is in
	       * progress! Otherwise, do NOT process the message at
	       * all, leave it "pending" for the next time around this
	       * loop
	       */
	      if ( p->state == ML_DEVICE_STATE_TRANSFERRING ) {
		status = (*bufferMessageFun)(c, msg);
		if(status == ML_STATUS_NO_ERROR) {
		  newType = ML_BUFFERS_COMPLETE;
		  hungry = 0;
		} else {
		  newType = ML_BUFFERS_FAILED;
		}
		errorReported = FALSE;	/* reset this to allow new message */
	      }
	      break;

	    case ML_QUERY_IN_PROGRESS: {
	      /* It is always possible to process a query message --
	       * whether or not a transfer is in progress
	       */
	      MLint32 accessNeeded = ML_ACCESS_RQ;

	      /* Start by validating message
	       */
	      if ( p->state == ML_DEVICE_STATE_TRANSFERRING ) {
		accessNeeded |= ML_ACCESS_DURING_TRANSFER;
	      }
	      status = mlDIvalidateMsg( p->openId, accessNeeded,
					ML_TRUE /* get-controls */, msg );
	      if ( status == ML_STATUS_NO_ERROR ) {
		status = audioGetPathControls( p, msg );
	      }
	      newType = (status == ML_STATUS_NO_ERROR) ?
		ML_QUERY_CONTROLS_COMPLETE : ML_QUERY_CONTROLS_FAILED;
	    } break;

	    default:
	      DEBG1( printf( "[mlaudio] Invalid message type '%s'\n",
			     mlMessageName( msgType ) ) );
	      assert(0);
	      status = ML_STATUS_INTERNAL_ERROR;
	    }

	  /* If the message was processed, send a reply
	   */
	  if ( newType != ML_MESSAGE_INVALID ) {
	    if (mlDIQueueUpdateMessage(entry, newType) != ML_STATUS_NO_ERROR)
	      {
		DEBG2(printf("UpdateMessage failed\n"));
		return ML_STATUS_INTERNAL_ERROR;
	      }

	    if (mlDIQueueAdvanceMessages(p->pQueue) != ML_STATUS_NO_ERROR)
	      {
		DEBG2(printf("ProcessMessage failed\n"));
		return ML_STATUS_INTERNAL_ERROR;
	      }

	    /* Message was processed, clear it so that it isn't
	     * considered "pending"
	     */
	    entry = NULL;
	  } /* if newType != ML_MESSAGE_INVALID */
	} /* if entry != NULL */
    }
    DEBG2(printf("---- exiting bottom of audioLoop()\n"));
    return status;
}


/* -------------------------------------------------------audioBackgroundThread
 */
void *
audioBackgroundThread(void *args)
{
    MLstatus status;
    audioPath *p = (audioPath *) args;

    p->childRunning = 1;

    DEBG2(printf("---- audioBackgroundThread: started\n"));

    _mlOSThreadPrepare(&p->child);

    status = audioLoop(p);

    _mlOSThreadExit(&p->child, status);
    return 0;	/*NOTREACHED*/
}


/* ----------------------------------------------------------------startUpChild
 */
MLstatus
startUpChild(audioPath* p)
{
    DEBG3(printf("in startUpChild()\n"));
    assert(p->childRunning == 0);
    return _mlOSThreadCreate(&p->child, audioBackgroundThread, p);
}


/* ---------------------------------------------------------------shutDownChild
 */
MLstatus
shutDownChild(audioPath *p, int state)
{
    MLstatus status = ML_STATUS_NO_ERROR;
    MLstatus exitState = ML_STATUS_NO_ERROR;

    DEBG3(printf("in shutDownChild()\n"));

    p->state = state;
    wakeUp(p);
    status = _mlOSThreadJoin(&p->child, &exitState);
    _mlOSThreadReap();

    p->childRunning = 0;

    if (exitState != ML_STATUS_NO_ERROR)
	status = exitState;
    DEBG3(printf("shutDownChild done\n"));
    return status;
}


/* ----------------------------------------------------------------------wakeUp
 */
MLstatus
wakeUp(audioPath *p)
{
    int ret;
    DEBG3(printf("in wakeUp\n"));
    ret = write(p->pipe_fds[PIPE_WRITE], "W", 1);
    if (ret != 1) {
	DEBG1(printf("wakeUp: write to pipe failed: %s\n", strerror(errno)));
	return ML_STATUS_INTERNAL_ERROR;
    }
    return ML_STATUS_NO_ERROR;
}


/* -------------------------------------------------------------------goToSleep
 */
MLstatus
goToSleep(audioPath *p)
{
    char c;
    DEBG3(printf("in goToSleep\n"));
    if (read(p->pipe_fds[PIPE_READ], &c, 1) != 1) {
	DEBG1(printf("goToSleep: read failed: %s\n", strerror(errno)));
	return ML_STATUS_INTERNAL_ERROR;
    }
    return ML_STATUS_NO_ERROR;
}


/* ----------------------------------------------------------------getFrameSize
 */
float
getFramesize(int fmt, int channels)
{
    float size;
    if (fmt == AFMT_S16_LE || fmt == AFMT_U16_LE)
        size = 2.0;
    else if (fmt == AFMT_IMA_ADPCM)
        size = 0.5;
    else
        size = 1.0;
    if (channels == 2)
        size *= 2;
    return size;
}


/* -----------------------------------------------------------------updateSizes
 */
MLstatus
updateSizes(physicalDevice *pd, int mode)
{
    audio_buf_info info;
    pd->framesize = getFramesize(pd->format, pd->channels);

    if (ioctl(pd->fd,
	      (mode == O_RDONLY) ? SNDCTL_DSP_GETISPACE : SNDCTL_DSP_GETOSPACE,
	      &info) < 0)
    {
        DEBG1(printf("Error: ioctl(SNDCTL_DSP_GET*SPACE) failed\n"));
        return ML_STATUS_INTERNAL_ERROR;
    }
    pd->fragsize = info.fragsize;
    pd->nfrags = info.fragstotal;
    DEBG3(printf("updateSizes: fragsize: %d bytes, nfrags: %d\n",
                 pd->fragsize, pd->nfrags));
    return ML_STATUS_NO_ERROR;
}


/* ---------------------------------------------------------setDeviceSampleRate
 */
MLstatus
setDeviceSampleRate(physicalDevice *pd, MLint32 rate)
{
    int tryRate = rate;
    int ret = ioctl(pd->fd, SNDCTL_DSP_SPEED, &tryRate);
    if (ret < 0) {
        DEBG1(perror("setDeviceSamepleRate: ioctl(SNDCTL_DSP_SPEED)"));
	return ML_STATUS_INTERNAL_ERROR;
    }
    /* only allow variation of +-1
     */
    else if (abs(tryRate - rate) > 1) {
        DEBG1(printf("setDeviceSamepleRate: asked for %d but got %d\n",
	             rate, tryRate));
	return ML_STATUS_INVALID_VALUE;
    }
    /* give warning to developers if not exact
     */
    else if (tryRate != rate) {
        DEBG1(printf("setDeviceSamepleRate: asked for %d but got %d\n",
	             rate, tryRate));
    }
    return ML_STATUS_NO_ERROR;
}

#ifdef NOTYET

/* -----------------------------------------------------audioPathIdentifyParams
 */
MLstatus
audioPathIdentifyParams(mlDILogDevCtxt pPath,
			MLint32 direction,
                        int fd)
{
    MLstatus status = ML_STATUS_NO_ERROR;
    MLpv detailParams[9];

    MLint32 legalCompression[10];
    MLbyte legalCompressionNames[256];
    MLbyte *lenPointer = &legalCompressionNames[0];
    int compressionNamesLength = 0;
    MLint32 currentCompression;

    MLint32 legalFormats[10];
    MLbyte legalFormatNames[256];
    MLbyte *lfnPointer = &legalFormatNames[0];
    int formatNamesLength = 0;
    MLint32 currentFormat;

    int eindex = 0, findex = 0;

    int i, ioretval = 0;

    /* all paths share same parameters, so determine them in advance.
     * start by adding uncompressed to head of compression list
     */
    legalCompressionTypes[eindex++] = ML_COMPRESSION_UNCOMPRESSED;
    strcpy(lenPointer, "ML_COMPRESSION_UNCOMPRESSED");
    DEBG3(printf("   added \"%s\" to compression list\n",
		 "ML_COMPRESSION_UNCOMPRESSED"));
    compressionNamesLength += strlen("ML_COMPRESSION_UNCOMPRESSED");
    lenPointer += strlen("ML_COMPRESSION_UNCOMPRESSED");
    *lenPointer++ = '\0';
    compressionNamesLength++;

    for (i = 0; i < 10; i++) {
        MLint32 fmt = 0;
	MLint32 cmp = 0;
	char *fmtstr = NULL;
	char *compstr = NULL;
        if (ioretval & (1 << i)) {
	    switch (1 << i) {
            case AFMT_MU_LAW:
	        cmp = ML_COMPRESSION_MU_LAW;
		compstr = "ML_COMPRESSION_MU_LAW";
		break;
    	    case AFMT_A_LAW:
	        cmp = ML_COMPRESSION_A_LAW;
		compstr = "ML_COMPRESSION_A_LAW";
		break;
            case AFMT_IMA_ADPCM:
	        cmp = ML_COMPRESSION_IMA_ADPCM;
		compstr = "ML_COMPRESSION_IMA_ADPCM";
		break;
            case AFMT_MPEG:
	        cmp = ML_COMPRESSION_MPEG2;
		compstr = "ML_COMPRESSION_MPEG2";
		break;
            case AFMT_U8:
	        fmt = ML_AUDIO_FORMAT_U8;
		fmtstr = "ML_AUDIO_FORMAT_U8";
		break;
            case AFMT_S16_LE:
		fmt = ML_AUDIO_FORMAT_S16;
		fmtstr = "ML_AUDIO_FORMAT_S16";
		break;
            case AFMT_S8:
		fmt = ML_AUDIO_FORMAT_S8;
		fmtstr = "ML_AUDIO_FORMAT_S8";
		break;
            case AFMT_U16_LE:
	        fmt = ML_AUDIO_FORMAT_U16;
		fmtstr = "ML_AUDIO_FORMAT_U16";
		break;
	    default:
	        DEBG3(printf("Unknown or unsupported OSS format type: "
			     "%d (0x%x)\n", 1 << i, 1 << i));
		continue;
	    }
	    if (fmtstr) {
	        legalFormats[findex++] = fmt;
	        strcpy(lfnPointer, fmtstr);
	        DEBG3(printf("   added \"%s\" to format list\n", fmtstr));
	        formatNamesLength += strlen(fmtstr);
	        lfnPointer += strlen(fmtstr);
	        *lfnPointer++ = '\0';
	        formatNamesLength++;
	    }
	    else if (compstr) {
	        legalCompression[eindex++] = cmp;
	        strcpy(lenPointer, compstr);
	        DEBG3(printf("   added \"%s\" to compression list\n",
			     compstr));
	        compressionNamesLength += strlen(compstr);
	        lenPointer += strlen(compstr);
	        *lenPointer++ = '\0';
	        compressionNamesLength++;
	    }
	}
    }

error:
    DEBG1(if (status != ML_STATUS_NO_ERROR)
              printf("audioPathIdentifyParams: "
		     "_mlDINewParam failed with status %d\n", status););
    return status;
}

#endif /* NOTYET */


/* ---------------------------------------------------------ddPvGetCapabilities
 */
MLstatus ddPvGetCapabilities(MLbyte* ddDevicePriv,
			     MLint64 staticObjectId, MLint64 paramId,
			     MLpv** capabilities)
{
  audioDeviceDetails *pDevice = (audioDeviceDetails *) ddDevicePriv;
  MLint32 maxParams = sizeof (pathParamDetails) /
    sizeof (MLDDint32paramDetails *);
  MLint32 p;

  if( mlDIextractIdType(staticObjectId) != ML_REF_TYPE_PATH )
    return ML_STATUS_INVALID_ID;

  if( mlDIextractPathIndex(staticObjectId) >= pDevice->numChannels )
    return ML_STATUS_INVALID_ID;

  for(p=0; p< maxParams; p++)
    if( pathParamDetails[p]->id == paramId )
      return ddIdentifyParam(pathParamDetails[p], staticObjectId,
			     capabilities);
  return ML_STATUS_INVALID_ID;
}


/* -----------------------------------------------------------ddGetCapabilities
 */
MLstatus ddGetCapabilities(MLbyte* ddDevicePriv,
			   MLint64 staticObjectId,
			   MLpv** capabilities)
{
  audioDeviceDetails* pDevice = (audioDeviceDetails*)ddDevicePriv;

  DEBG2(printf("[oss audio device] in getCapabilities\n"));

  switch( mlDIextractIdType(staticObjectId) )
    {
    case ML_REF_TYPE_DEVICE:
      {
	MLDDdeviceDetails device;
	device.name = pDevice->name;
	device.index = pDevice->index;
	device.version = pDevice->version;
	device.location = pDevice->location;
	device.jackLength = pDevice->numChannels;
	device.pathLength = pDevice->numChannels;
	return ddIdentifyPhysicalDevice(&device, staticObjectId, capabilities);
      }
    case ML_REF_TYPE_JACK:
      {
	MLDDjackDetails jack;
	MLint32 jackIndex = mlDIextractJackIndex(staticObjectId);
	if( jackIndex >= pDevice->numChannels )
	  return ML_STATUS_INVALID_ID;
	jack.name = pDevice->channels[jackIndex].name;
	jack.type = ML_JACK_TYPE_ANALOG_AUDIO;
	if (pDevice->channels[jackIndex].isInput)
	  jack.direction = ML_DIRECTION_IN;
	else
	  jack.direction = ML_DIRECTION_OUT;
	jack.pathIndexes = &jackIndex;
	jack.pathLength = 1;

	return ddIdentifyJack(&jack, staticObjectId, capabilities);
      }
    case ML_REF_TYPE_PATH:
      {
	MLDDpathDetails path;
	MLint32 pathIndex = mlDIextractPathIndex(staticObjectId);
	if( pathIndex >= pDevice->numChannels )
	  return ML_STATUS_INVALID_ID;
	path.name = pDevice->channels[pathIndex].name;
	if( pDevice->channels[pathIndex].isInput)
	  {
	    path.type = ML_PATH_TYPE_DEV_TO_MEM;
	    path.srcJackIndex = pathIndex;
	    path.dstJackIndex = -1;
	  }
	else
	  {
	    path.type = ML_PATH_TYPE_MEM_TO_DEV;
	    path.srcJackIndex = -1;
	    path.dstJackIndex = pathIndex;
	  }
	path.pixelLineAlignment = -1; /* not used */
	path.bufferAlignment = 4;
	path.params = pathParamDetails;
	path.paramsLength = sizeof (pathParamDetails) / sizeof (void *);
	return ddIdentifyPath(&path, staticObjectId, capabilities);
      }
    default:
      return ML_STATUS_INVALID_ID;
    }
}

static MLphysicalDeviceOps ddOps =
{
  sizeof (MLphysicalDeviceOps),
  ML_DI_DD_ABI_VERSION,
  ddGetCapabilities,
  ddPvGetCapabilities,
  ddOpen,
  ddSetControls,
  ddGetControls,
  ddSendControls,
  ddQueryControls,
  ddSendBuffers,
  ddReceiveMessage,
  ddXcodeWork,
  ddClose
};


/* ------------------------------------------------------------addDeviceChannel
 */
void addDeviceChannel( audioDeviceDetails* device, int channel,
		       int isInput, int chanMask )
{
  char *dirLabel = isInput ? "input" : "output";

  /* Only include input or output channels, as appropriate
   */
  if (!(chanMask & (1 << channel)))
    {
      DEBG3(printf("skipping non i/o channel %d (%s)\n",
		   channel, device_names[channel]));
      return;
    }

#ifndef BUGGY_OSS_DRIVERS_SUCK
  if (channel < SOUND_MIXER_SYNTH
      || channel == SOUND_MIXER_RECLEV
      || channel == SOUND_MIXER_IGAIN
      || channel == SOUND_MIXER_OGAIN)
    {
      DEBG3(printf("skipping bogus OSS channel %d (%s)\n",
		   channel, device_names[channel]));
      return;
    }
#endif
  sprintf(device->channels[device->numChannels].name,
	  "%s %s", device_names[channel], dirLabel);
  device->channels[device->numChannels].ossIndex = channel;
  device->channels[device->numChannels].isInput = isInput;
  DEBG3(printf("adding channel %d (%s) (%s)\n",
	       channel, device_names[channel], dirLabel));
  device->numChannels++;
}


#define DEVICE_VERSION 1
#define NPDPARAMS 2

/* ------------------------------------------------------audioNewPhysicalDevice
 */
MLstatus audioNewPhysicalDevice(MLsystemContext systemContext,
				MLmoduleContext moduleContext,
				const char* location,
				int idx,
				int fd)
{
  MLstatus status = ML_STATUS_NO_ERROR;
  audioDeviceDetails device;

  int caps = 0;

  sprintf(device.name, "OSS audio device");
  sprintf(device.location, location);
  device.index = idx;
  device.version = DEVICE_VERSION;
  device.numChannels = 0;

  /* Interrogate device to learn the details of potential jacks/paths
   */
  {
    int mixer_fd;
    int deviceMask = 0, inputMask = 0, outputMask = 0;
    int direction;
    char mixerDev[64];

    /* open mixer if present to get listing of inputs and outputs
     */
    if (idx == 0)
        strcpy(mixerDev, "/dev/mixer");
    else
        sprintf(mixerDev, "/dev/mixer%d", idx);

    mixer_fd = open(mixerDev, O_RDWR);

    if (mixer_fd < 0)
      {
        if (errno == ENODEV)
	  {
	    DEBG1(printf("audioIdentifyLogicalDevices: no mixer present\n"));
	  }
	else
	  {
	    DEBG1(printf("audioIdentifyLogicalDevices: couldn't open %s:%s\n",
	                 mixerDev, strerror(errno)));
	  }
      }

    if (ioctl(mixer_fd, SOUND_MIXER_READ_DEVMASK, &deviceMask) < 0)
      {
        DEBG1(printf("audioIdentifyLogicalDevices: ioctl shows no mixer\n"));
	deviceMask = SOUND_MASK_MIC | SOUND_MASK_SPEAKER;
      }

    if (ioctl(mixer_fd, SOUND_MIXER_READ_RECMASK, &inputMask) < 0)
      {
        DEBG1(printf("audioIdentifyLogicalDevices: ioctl "
		     "SOUND_MIXER_READ_RECMASK not supported\n"));
	inputMask = SOUND_MASK_MIC;
      }
    else
      {
        DEBG3(printf("inputMask = 0x%x\n", inputMask));
      }

    /* there is only one output device
     */
    outputMask = SOUND_MASK_SPEAKER;

    for (direction = 0; direction < 2; direction++)
      {
	int isInput = (direction == 0);
	int dirMask = isInput ? inputMask : outputMask;
	int d;

	/* Hack: it is more convenient if the mic input appears first
	 * in the list of input jacks -- that way, a program that uses
	 * the default jack (the sample 'audiotofile', for instance)
	 * will end up recording from the mic -- which is probably the
	 * most common case. But in the OSS scheme, 'line' comes
	 * first. So hard-code the mic jack here, to ensure it ends up
	 * first in *our* list.
	 *
	 * This will be ignored if isInput is 0.
	 */
	addDeviceChannel( &device, SOUND_MIXER_MIC, isInput, dirMask );
	
	for (d = 0; d < SOUND_MIXER_NRDEVICES; d++)
	  {
	    /* Don't add the MIC again -- it was already added above
	     */
	    if ( d != SOUND_MIXER_MIC ) {
	      addDeviceChannel( &device, d, isInput, dirMask );
	    }
	  }
	if (status != ML_STATUS_NO_ERROR)
	  break;
      }

    if (mixer_fd > 0)
      close(mixer_fd);
  }

  {
    int rate, ioretval = 0;
    int defaultRate;

    /* we store some caps in global struct
     */
    physicalDevice *ipd = &globalPhysicalDevices[idx][DEV_INPUT];
    physicalDevice *opd = &globalPhysicalDevices[idx][DEV_OUTPUT];

    /* check the path capabilities
     */
    ioctl(fd, SNDCTL_DSP_GETCAPS, &caps);

    ipd->caps.hasExclusiveInput = caps & SOUND_CAP_EXCL_INPUT;
    ipd->caps.hasDuplex = caps & DSP_CAP_DUPLEX;
    ipd->caps.hasRealTime = caps & DSP_CAP_REALTIME;

    DEBG1(printf("    Exclusive input is %s\n",
                 ipd->caps.hasExclusiveInput ? "required" : "not required"));

    DEBG1(printf("    Duplex is %s\n",
                 ipd->caps.hasDuplex ? "enabled" : "not enabled"));

    DEBG1(printf("    Real Time is %s\n",
                 ipd->caps.hasRealTime ? "enabled" : "not enabled"));

    /* get default sample rate
     */
    if (ioctl(fd, SOUND_PCM_READ_RATE, &ioretval) == 0)
      {
	ipd->caps.defaultRate = ioretval;
	DEBG3(printf("    default rate is %d\n", ipd->caps.defaultRate));
      }
    else
      {
	DEBG3(printf("    device does not support SOUND_PCM_READ_RATE\n"));
	ipd->caps.defaultRate = 8000;	/* ALWAYS TRUE? */
      }

    /* determine minimum and maximum supported sampling rates
     */
    {
      MLint32 newrate;
      int ret;
      rate = 4000;	/* lowest we will try */
      newrate = rate;
      if ((ret = ioctl(fd, SNDCTL_DSP_SPEED, &newrate)) == 0) {
	DEBG4(printf("tried min %d got %d\n", rate, newrate));
	DEBG3(printf("minimum supported rate: %d\n", newrate));
	ipd->caps.rateRanges[0][0] = (MLreal64) newrate;
      }
      else {
        DEBG1(printf("failed test for min sample rate -- using 8K\n"));
	ipd->caps.rateRanges[0][0] = 8000.0;	/* default */
      }

      rate = 48000;	/* expected max */
      newrate = rate;
      if ((ret = ioctl(fd, SNDCTL_DSP_SPEED, &newrate)) == 0) {
	DEBG4(printf("tried max %d got %d\n", rate, newrate));
	DEBG3(printf("maximum supported rate: %d\n", newrate));
	ipd->caps.rateRanges[1][0] = (MLreal64) newrate;

	rate = 96000;	/* highest we will try */
	newrate = rate;
	if ((ret = ioctl(fd, SNDCTL_DSP_SPEED, &newrate)) == 0) {
	  DEBG4(printf("tried 'ultra' max %d got %d\n", rate, newrate));
	  if (newrate > rate) {
	    DEBG3(printf("maximum supported rate: %d\n", newrate));
	    ipd->caps.rateRanges[1][0] = (MLreal64) newrate;
	  }
	}
	/* no error if this fails -- just testing
	 */
      }
      else {
	DEBG1(printf("failed test for max sample rate -- using 8K\n"));
	ipd->caps.rateRanges[1][0] = 8000.0;	/* default */
      }

      ipd->caps.nRatePairs = 1;
    }

    DEBG3(printf("    %d rates supported\n", ipd->caps.nRatePairs));
    defaultRate = ipd->caps.defaultRate;
    /* set the device back to its default
     */
    ioctl(fd, SNDCTL_DSP_SPEED, &defaultRate);

    /* check for all supported formats
     */
    if (ioctl(fd, SNDCTL_DSP_GETFMTS, &ioretval) < 0 || ioretval < 0)
      {
        DEBG1(perror("[oss audio device] ioctl(SNDCTL_DSP_GETFMTS) failed"));
        return ML_STATUS_INTERNAL_ERROR;
      }
    DEBG3(printf("GETFMTS ioctl returned mask 0x%x\n", ioretval));
    ipd->caps.format = ioretval;

    /* copy caps into output device
     */
    memcpy(&opd->caps, &ipd->caps, sizeof (ipd->caps));

  }

  status = mlDINewPhysicalDevice(systemContext,
				 moduleContext,
				 (MLbyte*)(&device),
				 sizeof (audioDeviceDetails));
  return status;
}

static MLstatus registerUSTSource( MLsystemContext systemContext,
				   MLmoduleContext moduleContext );


/* ---------------------------------------------------------------ddInterrogate
 *
 * This is the first device dependent entry point called by the mlSDK.
 * This routine must call NewPhysicalDevice() for every physical
 * device (i.e. board) that it can control. On Un*x this means
 * snuffling around in the hardware graph.
 */
MLstatus ddInterrogate(MLsystemContext systemContext,
		       MLmoduleContext moduleContext)
{
  MLstatus status = ML_STATUS_NO_ERROR;
  int fd, idx;
  char devname[12];	/* enough room for "/dev/dspXX" */

#ifdef DEBUG
  char *e = getenv("ADEBG");
  if (e) debugLevel = atoi(e);
#endif

  /* Attempt to open all /dev/dspXX devices.
   */
  for (idx = 0; idx < MAX_DEVICES; idx++) {
    if (idx == 0)
      strcpy(devname, "/dev/dsp");
    else
      sprintf(devname, "/dev/dsp%d", idx);
    DEBG3(printf("[oss audio]: checking device %s\n", devname));
    if ((fd = open(devname, O_RDONLY | O_NONBLOCK)) > 0) {
      ioctl(fd, SNDCTL_DSP_SETDUPLEX, 0);
      status = audioNewPhysicalDevice(systemContext, moduleContext, devname,
				      idx, fd);
      close(fd);
      if (status != ML_STATUS_NO_ERROR)
	return status;
    }
    else {
      DEBG3(perror("    open failed"));
      /* try for first two
       */
      if (idx > 0)
	break;
    }
  }
  return registerUSTSource( systemContext, moduleContext );
}


/* -------------------------------------------------------------------ddConnect
 */
MLstatus ddConnect(MLbyte *physicalDeviceCookie,
		   MLint64 staticDeviceId,
		   MLphysicalDeviceOps *pOps,
		   MLbyte **ddDevicePriv)
{
  MLstatus status = ML_STATUS_NO_ERROR;

  audioDeviceDetails* pDeviceCookie =
    (audioDeviceDetails*)physicalDeviceCookie;
  audioDeviceDetails *d = NULL;

#ifdef DEBUG
  char *e = getenv("ADEBG");
  if (e) debugLevel = atoi(e);
#endif

  DEBG2(printf("in ddConnect\n"));

  d = (audioDeviceDetails *) malloc(sizeof (audioDeviceDetails));
  if (!d)
    {
      *ddDevicePriv = NULL;
      return ML_STATUS_OUT_OF_MEMORY;
    }

  *d = *pDeviceCookie;
  *pOps = ddOps;
  *ddDevicePriv = (MLbyte*) d;

  /* If the UST source function has not yet been initialised, do that
   * now.
   */
  if ( USTSourceFunc == 0 ) {
    /* Need the "sysId", which is obtained from the staticDeviceId
     *
     * NOTE: this is not technically correct here -- we should be
     * keeping a copy of the USTSourceFunc for each different
     * sysId. For now, the SDK supports a single system (hence a
     * single sysId), so what we're doing is OK, but it isn't
     * future-proof.
     */
    MLint64 sysId = mlDIparentIdOfDeviceId( staticDeviceId );
    status = mlDIGetUSTSource( sysId, &USTSourceFunc );
  }

  return status;
}


/* ============================================================================
 *
 * Alternate UST source
 *
 * This demonstrates how a device might register a UST source. In this
 * case, the UST source is a "dummy" -- it uses the system time, which
 * makes for a very poor UST indeed (and which is no different from
 * the fall-back source built-in to the SDK). This is for demo
 * purposes only; no "real" module should register such a source.
 *
 * This is always compiled, but the UST source is only registered if,
 * at run-time, the env var OSSAUDIO_ENABLE_UST_SOURCE is
 * set. Otherwise, no UST source is registered (which, in general, is
 * what we want, since this module doesn't have a useful source to
 * register anyway).
 */

/* --------------------------------------------------------------------ddGetUST
 *
 * Actual UST source function.
 *
 * Uses the system time, and subtracts a constant offset -- just to
 * distinguish it from the source built-in to the SDK (for testing
 * purposes).
 *
 * The prototype for this function is in ml_didd.h
 */
MLint32 ddGetUST( MLint64* UST )
{
  static MLint64 USTbase = 0;

  struct timeval tv;
  struct timezone tz;
  if ( gettimeofday( &tv, &tz ) ) {
    return -1;
  }
  *UST = ((MLint64)tv.tv_sec)*1e9 + ((MLint64)tv.tv_usec)*1e3;

  if ( USTbase == 0 ) {
    USTbase = *UST;
    *UST = 1; /* Do not return 0, in case someone thinks that's an error */
  } else {
    *UST -= USTbase;
  }

  return 0;
}


/* -----------------------------------------------------------registerUSTSource
 *
 * Register the UST source.
 *
 * Should be called during ddInterrogate
 * Does nothing if env var OSSAUDIO_ENABLE_UST_SOURCE not defined
 */
static MLstatus registerUSTSource( MLsystemContext systemContext,
				   MLmoduleContext moduleContext )
{
  MLstatus status = ML_STATUS_NO_ERROR;
  static const char USTName[] = "ossaudio UST";
  static const char USTDescription[] = "ML SDK sample software UST source "
    "in module ossaudio; for demo purposes only.";

  if ( getenv( "OSSAUDIO_ENABLE_UST_SOURCE" ) ) {
    /* Register the source, using hard-coded (and fictitious) metrics
     * for the source -- we won't bother trying to figure out what the
     * update period or the latency really are. Make them just small
     * enough that they won't be rejected out-of-hand (update period <
     * 2000 and latency variation < 10000)
     *
     * In any case, these can be overriden by env vars
     */
    MLint32 updatePeriod = 1500;
    MLint32 latencyVar = 8000;

    char* e = getenv( "OSSAUDIO_UST_UPDATE_PERIOD" );
    if ( e ) {
      updatePeriod = atoi( e );
    }
    e = getenv( "OSSAUDIO_UST_LATENCY_VAR" );
    if ( e ) {
      latencyVar = atoi( e );
    }

    DEBG1( printf( "[ossaudio]: registering UST source, update period = %d"
		   ", latency var = %d\n", updatePeriod, latencyVar ) );
    status = mlDINewUSTSource( systemContext, moduleContext,
			       updatePeriod, latencyVar,
			       USTName, strlen( USTName ),
			       USTDescription, strlen( USTDescription ) );
  }

  return status;
}
