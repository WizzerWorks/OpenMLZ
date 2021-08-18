#! gmake
#*************************************************************************
#
# Makefile - lib
#
# The job of this Makefile is to descend into subdirectories. 
# and handle building rpm package.
#
#*************************************************************************

#
# The ossaudio device is Linux-only. Make sure we aren't trying to
# compile it on another platform
#
ifneq ($(PLATFORM),linux)
dummy := $(error The ossaudio device is for Linux only. Can not compile on platform $(PLATFORM))
endif

#
# Every Makefile must define ISM_DEPTH to be the path to the top of
# the ISM. In this case the top is one directory up. The ismdefs file
# must then be included.
#
# (OK to hard-code win32 path here, since device is not ported to other
# platforms)
ISM_DEPTH = ./linux
include	$(ISM_DEPTH)/ismdefs

LROOT = $(DIST_ROOT)
#
# In order to package all source code for the ISM, each file
# must be listed in one of the source macros listed in the commondefs
# file (e.g CFILES, CXXFILES, etc.). For files not covered under the
# source code macros use LSOURCES. Note that the Makefile is automatically
# included in the source list and should not be added to LSOURCES.
#
LSOURCES = \
	linux/ismdefs \
	linux/ismrules \
	$(NULL)

#
# List all subdirectories that need to be built (i.e. that have
# a Makefile). Note that the 'build' subdirectory is not listed
# since there is no Makefile in that directory.
#
SUBDIRS = \
	linux/mlmodule \
	$(NULL)

#
# Typical target for building subdirectories
#
all $(COMMONTARGS):
	$(SUBDIRS_MAKERULE)

headers:
	$(NOOP)
#
# The top level Makefile must include $(ISMCOMMONRULES) anywhere
# after the first target is defined. Non-top level Makefiles
# include $(ISMRULES) isntead of $(ISMCOMMONRULES).
#
include $(ISMCOMMONRULES)

