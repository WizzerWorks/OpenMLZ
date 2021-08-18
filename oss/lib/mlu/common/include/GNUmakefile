#! gmake
#*************************************************************************
#
# Makefile - lib/mlu/include
#
# This leaf Makefile illustrates the handling of a header file. The
# file only needs to be installed.
#
#*************************************************************************

#
# Every Makefile must define ISM_DEPTH to be the path to the top of
# the ISM. In this case the top is two directories up. The ismdefs file
# must then be included.
#
ISM_DEPTH = ../../../../build/$(PLATFORM)
include	$(ISM_DEPTH)/ismdefs

COMMON_INSTALL_PATH = ../../common/include
VPATH = ../../common/include
#
# In order to package the source files for the ISM, all files must
# be accounted for. To do this for non-code files, simply list the
# files in the LSOURCES macro.
#
PUBLIC_HFILES= mlu.h mlucapabilities.h mluconvenience.h mlutiming.h mlutimecode.h mluaudio.h

HFILES = $(PUBLIC_HFILES)

ifeq ($(PLATFORM),win32)

#
# WINDOWS-specific
#

# Some Windows-specific source files to add
NT_INSTALL_PATH = ../../win32/include
NT_PUBLIC_HFILES = getopt.h

endif

#
# There is nothing to build so use the $(NOOP) rule.
#
all: 
	$(NOOP)

headers: install

exports:
	$(NOOP)

default: 
	$(NOOP)

#
# This target will install the file. The install target depends on the 'all'
# target as a good habit even though there is nothing to build.
#
# Every path must be prefixed with $(DIST_ROOT) so that the installation
# will be relative to the ISM installation directory defined during the
# package build.
#
install: all
	cd $(COMMON_INSTALL_PATH); $(INSTALL) -d $(DIST_ROOT)/usr/include/ML
	cd $(COMMON_INSTALL_PATH); $(INSTALL) -m 644 $(PUBLIC_HFILES) $(DIST_ROOT)/usr/include/ML
ifeq ($(PLATFORM),win32)
	cd $(NT_INSTALL_PATH); $(INSTALL) -m 644 $(NT_PUBLIC_HFILES) $(DIST_ROOT)/usr/include/ML
endif

include $(ISMRULES)
