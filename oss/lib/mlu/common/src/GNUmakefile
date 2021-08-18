#! gmake
#*************************************************************************
#
# Makefile - libMLU
#
# This Makefile provides an example of how to build a Dynamically Shared
# Object (DSO), also known as a shared library.
#
#*************************************************************************

ISM_DEPTH = ../../../../build/$(PLATFORM)
include $(ISM_DEPTH)/ismdefs

#
# The following three macros must be defined.
#
# DSO_TARGET_BASENAME	- Base name of the shared libarary without any
#			  version numbers, prefix or suffix
#                         (e.g. DSO_TARGET_BASENAME = foo).
#                         This will be expanded to libfoo.so or fooxy.dll
#                         according to the platform. (On NT, 'xy' is the
#                         major, minor version: versioning must be built
#                         in to the base name)
#                         Set this macro instead of the TARGETS macro.
#                         The 'all' target should depend upon $(DSO_TARGET)
#                         intead of $(TARGETS)
#                         (Note: depend on DSO_TARGET, NOT DSO_TARGET_BASENAME)
#
# DSO_MAJOR_VERSION     - Major version number (e.g. DSO_MAJOR_VERSION = 1).
#
# DSO_MINOR_VERSION     - Minor version number (e.g. DSO_MINOR_VERSION = 0)
#
DSO_TARGET_BASENAME = MLU
DSO_MAJOR_VERSION = 1
DSO_MINOR_VERSION = 0

#
# The source files comprising the library.
#
CFILES = \
	mluaudio.c\
	mlubuffersize.c\
	mlucapabilities.c\
	mluconvenience.c\
	mlupixelsize.c\
	mlutimecode.c\
	mlutiming.c\
	$(NULL)

LSOURCES = \
	mlutimings.txt \
	$(NULL)


ifeq ($(PLATFORM),win32)

#
# WINDOWS-specific
#

# Some Windows-specific source files to add
VPATH = ../../win32/src
CFILES += \
	dllmain.c \
	getopt.c \
	$(NULL)

LLDLIBPATHS = $(DIST_ROOT)/usr/lib
LLDLIBS = ML10.lib

NT_DEF_FILE = ../../win32/mlu.def

else
#
# UNIX / LINUX
#

LCINCS = -I$(DIST_ROOT)/usr/include
endif


#
# Note that 'all' depends on $(DSO_TARGET) instead of $(TARGETS).
#
all: headers exports default $(_FORCE)

headers:
	$(NOOP)

exports: $(DSO_TARGET) 
	$(INSTALL_DSO)

default: 
	$(NOOP)

#
# The install target simply invokes the $(INSTALL_DSO) macro. This
# macro does all the work of installing the DSO and creating the
# necessary links to keep the dynamic loader happy.
#
install: all
	$(INSTALL_DSO)

include $(ISMRULES)
