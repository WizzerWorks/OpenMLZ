#! gmake
#*************************************************************************
#
# The job of this Makefile is to descend into subdirectories. 
# and handle building rpm package.
#
#*************************************************************************

#
# Every Makefile must define ISM_DEPTH to be the path to the top of
# the ISM. In this case the top is one directory up. The ismdefs file
# must then be included.
#
ISM_DEPTH = ./build/$(PLATFORM)
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
	build/$(PLATFORM)/ismdefs \
	build/$(PLATFORM)/ismrules \
	$(NULL)

#
# List all subdirectories that need to be built (i.e. that have
# a Makefile). Note that the 'build' subdirectory is not listed
# since there is no Makefile in that directory.
#

ifeq ($(PLATFORM),linux)
PLATFORM_SUBDIRS = \
	examples/linux
endif

SUBDIRS = \
	lib/ml/common \
	lib/mlu/common \
	man/man3dm/common \
	tools/mlquery/common \
	examples/common \
	$(PLATFORM_SUBDIRS) \
	$(NULL)


# Typical target for building subdirectories
#
all headers $(COMMONTARGS):
	$(SUBDIRS_MAKERULE)

#
# The top level Makefile must include $(ISMCOMMONRULES) anywhere
# after the first target is defined. Non-top level Makefiles
# include $(ISMRULES) isntead of $(ISMCOMMONRULES).
#
include $(ISMCOMMONRULES)

