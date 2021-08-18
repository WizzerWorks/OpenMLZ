#! gmake
#
#

ISM_DEPTH = ../../../build/$(PLATFORM)
include $(ISM_DEPTH)/ismdefs

LDIRT =

# Path to which the man pages get installed, relative to DIST_ROOT
MAN_PATH=usr/man/man3

# Path in which the man pages (which are "common" to all platforms)
# reside
COMMON_MAN_PATH = ../common
VPATH = $(COMMON_MAN_PATH)

COMMON_MANPAGES = mlIntro.3dm \
		ddConnect.3dm \
		ddInterrogate.3dm \
		mlDIIntro.3dm \
		mlDINew.3dm \
		mlDIQueue.3dm \
		mlDIid.3dm \
		mlPvCopy.3dm \
		mlPvFind.3dm \
		mlGetVersion.3dm \
		mlParameters.3dm \
		mlGetCapabilities.3dm \
		mlPvGetCapabilities.3dm \
		mlFreeCapabilities.3dm \
		mlOpen.3dm \
		mlClose.3dm \
		mlGetControls.3dm \
		mlSetControls.3dm \
		mlSendControls.3dm \
		mlSendBuffers.3dm \
		mlReceiveMessage.3dm \
		mlGetWaitHandle.3dm \
		mlGetMessageCount.3dm \
		mlBeginTransfer.3dm \
		mlEndTransfer.3dm \
		mlXcodeGetOpenPipe.3dm \
		mlXcodeWork.3dm \
		mlStatusName.3dm \
		mlMessageName.3dm \
		mlPvString.3dm \
		mlGetSystemUST.3dm \
		mlVideoParameters.3dm \
		mlImageParameters.3dm \
		mlPixel.3dm \
		mlAudioParameters.3dm \
		mlSynchronization.3dm 

UTIL_MANPAGES =	mluCapabilities.3dm \
		mluDefaults.3dm \
		mluImageBufferSize.3dm \
		mluPv.3dm \
		mluSizes.3dm \
		mluTCAddTC.3dm \
		mluTCFramesBetween.3dm \
		mluTCFramesPerDay.3dm \
		mluTCToSeconds.3dm \
		mluTCToString.3dm

LSOURCES = $(COMMON_MANPAGES) $(UTIL_MANPAGES)

default:
	$(NOOP)

all headers: 	
	$(NOOP)

install:	all
	$(INSTALL) -d $(DIST_ROOT)/usr/man
	$(INSTALL) -d $(DIST_ROOT)/usr/man/man3
	cd $(COMMON_MAN_PATH); $(INSTALL) -m 644 $(COMMON_MANPAGES) $(DIST_ROOT)/$(MAN_PATH); $(INSTALL) -m 644 $(UTIL_MANPAGES) $(DIST_ROOT)/$(MAN_PATH)

include $(ISMRULES)
