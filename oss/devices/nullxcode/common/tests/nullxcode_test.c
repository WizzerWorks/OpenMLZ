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

/* Some simple test for the null transcoder.
 * Compile with:
 *   Irix
 *    CC -I/usr/include/dmedia nullxcode_test.c -lML -lMLU -lpthread
 *   Linux
 *    gcc -I/usr/include/dmedia nullxcode_test.c -lML -lMLU -lpthread
 */

#include <ML/ml.h>
#include <ML/mlu.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <sys/types.h>

#ifndef ML_OS_NT
#include <sys/time.h>
#include <unistd.h>
#include <strings.h>
#endif

/* Define a few convenience routines
 */
MLint32 allocateBuffers(MLopenid xcode,
                        MLint64 whichPipe,
                        void** buffers, 
                        MLint32 maxBuffers,
                        MLint32* returnedImageSize);

MLint32 freeBuffers(void** buffers,
                    MLint32 maxBuffers);

// Make NT friendly
#define NUM_BUFFERS 20
MLint32 gAlignedBufferSize = 0;

int
runTest(int synchronous,
        int useCopy)
{
    /* The following would typically come from a UI or a file,
     * but for this example, we just hard-code them in.
     */
    MLint32 transferred_buffers = 0;
    MLint32 requested_buffers = 500;
    MLint32 maxBuffers = NUM_BUFFERS;

    MLint64 devId = 0;
    MLint64 xcodeId = 0;

    void* srcBuffers[NUM_BUFFERS];
    void* dstBuffers[NUM_BUFFERS];

    MLint32 i;
    MLopenid openXcode = 0;

    MLwaitable waitHandle;

    MLint32 srcImageSize = 0;
    MLint32 dstImageSize = 0;
    MLbyte* zeroBuf = 0;
  
    MLpv msg[50];
    MLint32 status;

    char* deviceName = "nullXcode";
    char* xcodeName = 0;
    if (useCopy) {
        xcodeName = "nullXcodeMemoryToMemoryCopy";
    } else {
        xcodeName = "nullXcodeMemoryClear";
    }
    

    /* Find out about this system
     */
    if (mluFindDeviceByName(ML_SYSTEM_LOCALHOST, deviceName, &devId)) {
        printf("Cannot find device named:%s.\n", deviceName);
        return -1;
    }

    if (mluFindXcodeByName(devId, xcodeName, &xcodeId)) {
        printf("Cannot find transcoder named:%s.\n", xcodeName);
        return -1;
    }

    /* Open a codec -- returns a handle to a codec
     */
    i = 0;
    msg[i].param = ML_OPEN_XCODE_STREAM_INT32;
    msg[i].value.int32 = ML_XCODE_STREAM_SINGLE;
    msg[i].length = 1;
    i++;
    if (synchronous) {
        msg[i].param = ML_OPEN_XCODE_MODE_INT32;
        msg[i].value.int32 = ML_XCODE_MODE_SYNCHRONOUS;
        msg[i].length = 1;
        i++;
    }
    msg[i].param = ML_END;

    status = mlOpen(xcodeId, msg, &openXcode);
    if (status != ML_STATUS_NO_ERROR) {
        printf("mlCodecOpen() Failed\n");
        goto SHUTDOWN;
    }
    printf("xcode opened\n");

    /* Test open and close immediately
     */
    mlClose(openXcode);
    printf("xcode closed\n");
    if (mlOpen(xcodeId, msg, &openXcode)) {
        printf( "Cannot open xcode\n");
        goto SHUTDOWN;
    }
    printf("xcode opened again, getting controls...\n");

    /* Make sure that our initial controls make sense.
     */
    i = 0;
    msg[i].param = ML_SELECT_ID_INT64;
    msg[i++].value.int64 = ML_XCODE_SRC_PIPE;

    msg[i++].param = ML_IMAGE_COLORSPACE_INT32;
    msg[i++].param = ML_IMAGE_COMPRESSION_INT32;
    msg[i++].param = ML_IMAGE_PACKING_INT32;
    msg[i++].param = ML_IMAGE_SAMPLING_INT32;
    msg[i++].param = ML_IMAGE_TEMPORAL_SAMPLING_INT32;
    msg[i++].param = ML_IMAGE_WIDTH_INT32;
    msg[i++].param = ML_IMAGE_HEIGHT_1_INT32;
    msg[i++].param = ML_IMAGE_HEIGHT_2_INT32;
    msg[i++].param = ML_IMAGE_ROW_BYTES_INT32;
    msg[i++].param = ML_IMAGE_SKIP_PIXELS_INT32;
    msg[i++].param = ML_IMAGE_ORIENTATION_INT32;

    msg[i].param = ML_SELECT_ID_INT64;
    msg[i++].value.int64 = ML_XCODE_DST_PIPE;

    msg[i++].param = ML_IMAGE_COLORSPACE_INT32;
    msg[i++].param = ML_IMAGE_COMPRESSION_INT32;
    msg[i++].param = ML_IMAGE_PACKING_INT32;
    msg[i++].param = ML_IMAGE_SAMPLING_INT32;
    msg[i++].param = ML_IMAGE_TEMPORAL_SAMPLING_INT32;
    msg[i++].param = ML_IMAGE_WIDTH_INT32;
    msg[i++].param = ML_IMAGE_HEIGHT_1_INT32;
    msg[i++].param = ML_IMAGE_HEIGHT_2_INT32;
    msg[i++].param = ML_IMAGE_ROW_BYTES_INT32;
    msg[i++].param = ML_IMAGE_SKIP_PIXELS_INT32;
    msg[i++].param = ML_IMAGE_ORIENTATION_INT32;

    msg[i++].param = ML_END;

    if (mlGetControls(openXcode, msg) != ML_STATUS_NO_ERROR) {
        printf( "Couldn't get controls on xcoder\n");
        return -1;
    }
    mluPvPrintMsg(openXcode, msg);
    fflush(stdout);
    
    /* Make sure that we ignore controls not meant for us.
     * We "set" inconsistent controls, which should work
     * if we correctly ignore them.
     */
    i = 0;
    msg[i].param = ML_SELECT_ID_INT64;
    msg[i++].value.int64 = 0x12345;
    msg[i].param = ML_IMAGE_WIDTH_INT32;
    msg[i++].value.int32 = 500;
    msg[i].param = ML_SELECT_ID_INT64;
    msg[i++].value.int64 = 0x54321;
    msg[i].param = ML_IMAGE_WIDTH_INT32;
    msg[i++].value.int32 = 200;
    msg[i++].param = ML_END;

    printf("\nTry to set controls with foreign ML_SELECT_ID_INT64\n");
    mluPvPrintMsg(openXcode, msg);
    fflush(stdout);

    if (mlSetControls(openXcode, msg) == ML_STATUS_NO_ERROR) {
        printf("Set controls with foreign ML_SELECT_ID_INT64 ignored\n");
    } else {
        printf("Set controls with foreign ML_SELECT_ID_INT64 not ignored\n");
        exit(-1);
    }

    /* Make sure that we can set an enumerated control to
     * a legal value.
     */
    i = 0;
    msg[i].param = ML_SELECT_ID_INT64;
    msg[i++].value.int64 = ML_XCODE_SRC_PIPE;
    msg[i].param = ML_IMAGE_COMPRESSION_INT32;
    msg[i++].value.int32 = ML_COMPRESSION_LOSSLESS_JPEG;
    msg[i].param = ML_SELECT_ID_INT64;
    msg[i++].value.int64 = ML_XCODE_DST_PIPE;
    msg[i].param = ML_IMAGE_COMPRESSION_INT32;
    msg[i++].value.int32 = ML_COMPRESSION_LOSSLESS_JPEG;
    msg[i].param = ML_END;
    printf("\nTry to set legal enumerated controls\n");
    mluPvPrintMsg(openXcode, msg);
    fflush(stdout);

    if (mlSetControls(openXcode, msg) == ML_STATUS_NO_ERROR) {
        printf("Set legal enumerated control worked\n");
    } else {
        printf("Set legal enumerated control failed!\n");
        exit(-1);
    }
    
    /* Make sure that we can't set an enumerated control to
     * an illegal value.
     */
    i = 0;
    msg[i].param = ML_SELECT_ID_INT64;
    msg[i++].value.int64 = ML_XCODE_SRC_PIPE;
    msg[i].param = ML_IMAGE_COMPRESSION_INT32;
    msg[i++].value.int32 = ML_COMPRESSION_SLIM;
    msg[i].param = ML_SELECT_ID_INT64;
    msg[i++].value.int64 = ML_XCODE_DST_PIPE;
    msg[i].param = ML_IMAGE_COMPRESSION_INT32;
    msg[i++].value.int32 = ML_COMPRESSION_SLIM;
    msg[i].param = ML_END;
    printf("\nTry to set illegal enumerated controls\n");
    mluPvPrintMsg(openXcode, msg);
    fflush(stdout);

    if (mlSetControls(openXcode, msg) == ML_STATUS_NO_ERROR) {
        printf("Set illegal enumerated control worked!\n");
        exit(-1);
    } else {
        printf("Set illegal enumerated control failed as expected\n");
    }
    
    /* Make sure that we can set consistent controls
     */
    i = 0;
    msg[i].param = ML_SELECT_ID_INT64;
    msg[i++].value.int64 = ML_XCODE_SRC_PIPE;
    msg[i].param = ML_IMAGE_WIDTH_INT32;
    msg[i++].value.int32 = 640;
    msg[i].param = ML_SELECT_ID_INT64;
    msg[i++].value.int64 = ML_XCODE_DST_PIPE;
    msg[i].param = ML_IMAGE_WIDTH_INT32;
    msg[i++].value.int32 = 640;
    msg[i++].param = ML_END;

    printf("\nTry to set consistent controls\n");
    mluPvPrintMsg(openXcode, msg);
    fflush(stdout);

    if (mlSetControls(openXcode, msg) == ML_STATUS_NO_ERROR) {
        printf("Set consistent controls worked\n");
    } else {
        printf("Set consistent controls failed!\n");
        exit(-1);
    }

    /* Make sure that we can't set inconsistent controls
     */
    i = 0;
    msg[i].param = ML_SELECT_ID_INT64;
    msg[i++].value.int64 = ML_XCODE_SRC_PIPE;
    msg[i].param = ML_IMAGE_WIDTH_INT32;
    msg[i++].value.int32 = 640;
    msg[i].param = ML_SELECT_ID_INT64;
    msg[i++].value.int64 = ML_XCODE_DST_PIPE;
    msg[i].param = ML_IMAGE_WIDTH_INT32;
    msg[i++].value.int32 = 800;
    msg[i++].param = ML_END;

    printf("\nTry to set inconsistent controls\n");
    mluPvPrintMsg(openXcode, msg);
    fflush(stdout);

    if (mlSetControls(openXcode, msg) != ML_STATUS_NO_ERROR) {
        printf("Set inconsistent controls failed as expected\n");
    } else {
        printf("Set inconsistent controls worked\n");
        exit(-1);
    }

    /* Allocate buffers.
     */
    if (allocateBuffers(openXcode, ML_XCODE_SRC_PIPE,
                        srcBuffers, maxBuffers, &srcImageSize)) {
        printf("Cannot allocate memory for src buffers\n");
        return -1;
    }
    if (allocateBuffers(openXcode, ML_XCODE_DST_PIPE,
                        dstBuffers, maxBuffers, &dstImageSize)) {
        printf("Cannot allocate memory for dst buffers\n");
        return -1;
    }
    if (!useCopy) {
        zeroBuf = (MLbyte*) malloc(dstImageSize);
        if (!zeroBuf) {
            printf("Cannot allocate memory for zeroBuf\n");
            return -1;
        }
        memset(zeroBuf, 0, dstImageSize);
    }

    /* Fill the source buffers with data.
     */
    for (i = 0; i < maxBuffers; i++) {
        memset(srcBuffers[i], 0xab, srcImageSize);
    }

    /* We can either poll mlReceiveMessage, or allow a wait
     * variable to be signalled.  Polling is undesirable - its
     * much better to wait for an event - that way the OS can
     * swap us out and make full use of the processor until the
     * event occurs.
     */
    if (mlGetReceiveWaitHandle(openXcode, &waitHandle)) {
        printf("Cannot get xcode wait handle\n");
        goto SHUTDOWN;
    }

    /* Preroll - send the buffers to the source and destination pipes
     * in preparation for beginning the transfer.  Send buffers full of data
     * bits to the source pipe and empty buffers to the destination pipe.
     * When we have sent 1/4 of the buffers, begin the transfer.
     */
    for (i = 0; i < maxBuffers; i++) {
        int j = 0;
        msg[j].param = ML_SELECT_ID_INT64;
        msg[j].value.int64 = ML_XCODE_SRC_PIPE;
        j++;
        msg[j].param = ML_IMAGE_BUFFER_POINTER;
        msg[j].value.pByte = (MLbyte*)srcBuffers[i];
        msg[j].length = srcImageSize;
        msg[j].maxLength = srcImageSize;
        j++;
        msg[j].param = ML_VIDEO_MSC_INT64;
        msg[j].value.int64 = i;
        j++;
        msg[j].param = ML_VIDEO_UST_INT64;
        mlGetSystemUST(ML_SYSTEM_LOCALHOST, &msg[j].value.int64);
        j++;
        msg[j].param = ML_SELECT_ID_INT64;
        msg[j].value.int64 = ML_XCODE_DST_PIPE;
        j++;
        msg[j].param = ML_IMAGE_BUFFER_POINTER;
        msg[j].value.pByte = (MLbyte*)dstBuffers[i];
        msg[j].length = dstImageSize;
        msg[j].maxLength = dstImageSize;
        j++;
        msg[j].param = ML_VIDEO_MSC_INT64;
        j++;
        msg[j].param = ML_VIDEO_UST_INT64;
        j++;
        msg[j].param = ML_END;

        if (i == 0) {
            printf("Sample mlSendBuffers message:\n");
            mluPvPrintMsg(openXcode, msg);
            fflush(stdout);
        }

        if (mlSendBuffers(openXcode, msg)) {
	  printf("Error sending buffers to openXcode.\n");
	  goto SHUTDOWN;
	}

        if (i == maxBuffers/4) {
            /* Now start the transfer.
             */
            if (mlBeginTransfer(openXcode)) {
                printf("Error beginning transfer.\n");
                goto SHUTDOWN;
            }
        }
    }

    /* Now the transfer is in progress, look for completion messages.
     * Each time a buffer completes, refill it and send it back to be
     * processed.  
     */
    printf("\nStarting to transform %d buffers\n", requested_buffers);
    fflush(stdout);
    while (transferred_buffers < requested_buffers) {
        MLint32 messageType;
        MLpv *message;

        if (synchronous) {
            if (mlXcodeWork(openXcode) != ML_STATUS_NO_ERROR) {
                printf("mlXcodeWork failed\n");
                return(-1);
            }
        } else {
#ifdef ML_OS_NT
	  int timeoutSecs = 5;
            DWORD dwRet = WaitForSingleObject( waitHandle, timeoutSecs*1000 );
            if( dwRet == WAIT_ABANDONED )
            {
                break;
                continue;
            }
            else if( dwRet == WAIT_TIMEOUT )
            {
                printf("Error WaitForSingleObject timed out after %d seconds\n",
                       timeoutSecs);
                goto SHUTDOWN;
            }
#else
            struct timeval timeout = {5, 0}; /* 5 seconds */
            fd_set fdset;

            FD_ZERO(&fdset);
            FD_SET(waitHandle, &fdset );

            if (select(waitHandle+1, &fdset, NULL, NULL, &timeout) == 0) {
                printf("Error select timed out after %d seconds\n",
                       timeout.tv_sec);
                goto SHUTDOWN;
            }

            /* Read the top message on the receive queue.
             */
            if (!FD_ISSET(waitHandle, &fdset)) {
                continue;
            }
#endif
        }

        if (mlReceiveMessage(openXcode, &messageType, &message)) {
            printf("Error receiving reply message\n");
            goto SHUTDOWN;
        }

        switch (messageType) {
         case ML_BUFFERS_COMPLETE: 
             /* Use knowledge of param order when we sent the message...
              */
             if (message[3].length == -1) {
                 printf("After rcv, buffer length -1\n");
                 return(-1);
             }

             /* Verify the buffers.
              */
             if (useCopy) {
                 if (memcmp(message[1].value.pByte,
                            message[5].value.pByte, message[1].length)) {
                     printf("After rcv, buffer verification failed\n");
                     return(-1);
                 }
             } else {
                 if (memcmp(zeroBuf,
                            message[5].value.pByte, message[1].length)) {
                     printf("After rcv, buffer verification failed\n");
                     return(-1);
                 }
             }
                           
             /* Verify that ust/msc were passed thru...
              */
             if (message[2].value.int64 != message[6].value.int64) {
                 printf("msc not passed thru\n");
                 return(-1);
             }
             if (message[3].value.int64 != message[7].value.int64) {
                 printf("ust not passed thru\n");
                 return(-1);
             }

             /* Update the input ust and feed the buffers right back.
              */
             mlGetSystemUST(ML_SYSTEM_LOCALHOST, &message[3].value.int64);
             mlSendBuffers(openXcode, message);
             transferred_buffers++;
             break;
         default:
             printf("Something went wrong - received code %s\n",
                    mlMessageName(messageType));
             goto SHUTDOWN;
        }
    } 

    printf("successfully transformed %d buffers\n", requested_buffers);
    fflush(stdout);

  SHUTDOWN:
    printf("Shutdown\n");
    printf("Close xcoder\n");
    if (openXcode) {
        mlClose(openXcode);
    }

    if (srcImageSize) {
        printf("free srcBuffers\n");
        freeBuffers(srcBuffers, maxBuffers);
    }
    if (dstImageSize) {
        printf("free dstBuffers\n");
        freeBuffers(dstBuffers, maxBuffers);
    }
    if (zeroBuf) {
        free(zeroBuf);
    }
    return 0;
}


int
main(int argc,
     char **argv)
{
    printf("\nSynchronous copy memory test...\n");
    runTest(1, 1);
    printf("\nSynchronous clear memory test...\n");
    runTest(1, 0);
    printf("\n\nAsynchronous copy memory test...\n");
    runTest(0, 1);
    printf("\n\nAsynchronous clear memory test...\n");
    runTest(0, 0);
}


/* Allocate an array of image buffers with specified alignment and size
 * wrapper around virtualAlloc/memalign
 */
MLint32
allocateBuffers(MLopenid xcode,
                MLint64 whichPipe,
                void** buffers, 
                MLint32 maxBuffers,
                MLint32* returnedImageSize)
{
    int i;
    MLpv* devCap = 0;
    MLpv* cap = 0;
    MLint32 memAlignment = 0;
    MLpv msg[3];

    mlGetCapabilities(xcode, &devCap);
    cap = mlPvFind(devCap, ML_XCODE_BUFFER_ALIGNMENT_INT32);
    if (cap) {
        memAlignment = cap->value.int32;
    }

    msg[0].param = ML_SELECT_ID_INT64;
    msg[0].value.int64 = whichPipe;
    msg[0].length = 1;
    msg[1].param = ML_IMAGE_BUFFER_SIZE_INT32;
    msg[2].param = ML_END;
    if (mlGetControls(xcode, msg) != ML_STATUS_NO_ERROR) {
        printf("Cannot get image buffer size\n");
        return -1;
    }
    *returnedImageSize = msg[1].value.int32;
    printf("allocateBuffers, returned size = %d, alignment %d\n",
            *returnedImageSize, memAlignment);

#ifdef ML_OS_NT
    {
        SIZE_T MinWorkingSetSize;
        SIZE_T MaxWorkingSetSize;
        SIZE_T BufferSetSize = *returnedImageSize * maxBuffers;
        
        if (!GetProcessWorkingSetSize( GetCurrentProcess(), &MinWorkingSetSize, &MaxWorkingSetSize))
        {
            printf("ERROR: GetProcessWorkingSetSize() failed.\n");
            return(-1);
        }
        
        MinWorkingSetSize += BufferSetSize;
        MaxWorkingSetSize += BufferSetSize;

        // Grow the working set size to allow locked buffers, especially for Windows 2000
        if (!SetProcessWorkingSetSize( GetCurrentProcess(), MinWorkingSetSize, MaxWorkingSetSize))
        {
            printf("ERROR: SetProcessWorkingSetSize() failed.\n");
            return(-1);
        }
        
        // Get the aligned size
        if( gAlignedBufferSize )	// This had better be the same every time
            assert( gAlignedBufferSize == ((*returnedImageSize + (memAlignment-1)) & ~(memAlignment-1)) );
        else
            gAlignedBufferSize = ((*returnedImageSize + (memAlignment-1)) & ~(memAlignment-1));

        for (i = 0; i < maxBuffers; i++)
        {
            // Allocate buffers as committed and uncachable.  While only able to be block reads and writes,
            // these buffers are always current.
            buffers[i] = (LPBYTE)VirtualAlloc(	NULL, gAlignedBufferSize, MEM_COMMIT, PAGE_READWRITE | PAGE_NOCACHE );
            // Lock the buffers down in memory so they are unswappable
            if( buffers[i] == NULL || 
                ! VirtualLock(buffers[i], gAlignedBufferSize)	)
            {
                printf("Error - VirtualAlloc/Lock failed \n");
                return -1;
            }
        }
    }

#else	// Unix
    for (i = 0; i < maxBuffers; i++) {
        buffers[i] = memalign(memAlignment, *returnedImageSize);
        if (buffers[i] == 0) {
            printf("Memory allocation failed for buffer %d\n", i);
            return -1;
	}
        /*
         * Here we touch the buffers, forcing the buffer memory to be mapped
         * this avoids the need to map the buffers the first time they're used.
         * We could go the extra step and mpin them, but choose not to here,
         * trying a simpler approach first.
         */
        bzero(buffers[i], *returnedImageSize);
        assert(((MLint64)(buffers[i]) % memAlignment) == 0);
    }
#endif
    return 0;
}


MLint32
freeBuffers(void** buffers,
            MLint32 maxBuffers)
{
    int i;

    for (i = 0; i < maxBuffers; i++) {
        if (buffers[i]) {
            free(buffers[i]);
        }
    }
    return 0;
}


void
dumpMsg(MLpv* msg)
{
    int i = 0;
    while (msg[i].param != ML_END) {
        printf("param[%d] = %" FORMAT_LLX "\n", i, msg[i].param);
    }
}


