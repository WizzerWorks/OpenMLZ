#! gmake
#*************************************************************************
#
# Makefile - lib/ml/include
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

COMMON_INCLUDE_PATH = ../../common/include
VPATH = ../../common/include
#
# In order to package the source files for the ISM, all files must
# be accounted for. To do this for non-code files, simply list the
# files in the LSOURCES macro.
#
PUBLIC_HFILES= mltypes.h mldefs.h mljack.h mlparam.h mlpath.h ml.h \
	mlxcode.h mluser.h mlimage.h mlaudio.h mlvideo.h mlqueue.h mlfifo.h \
	mlancdata.h mlcompression.h

PRIVATE_HFILES = ml_didd.h ml_oswrap.h ml_private.h

HFILES = $(PUBLIC_HFILES) $(PRIVATE_HFILES)

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
	cd $(COMMON_INCLUDE_PATH); $(INSTALL) -d $(DIST_ROOT)/usr/include/ML
	cd $(COMMON_INCLUDE_PATH); $(INSTALL) -m 644 $(PUBLIC_HFILES) $(DIST_ROOT)/usr/include/ML
	cd $(COMMON_INCLUDE_PATH); $(INSTALL) -m 644 $(PRIVATE_HFILES) $(DIST_ROOT)/usr/include/ML

include $(ISMRULES)
