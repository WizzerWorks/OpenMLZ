#! gmake
#*************************************************************************
#
# Makefile - winaudio
#
# This Makefile provides an example of how to build a Dynamically Shared
# Object (DSO), also known as a shared library.
#
#*************************************************************************

#
# The winaudio device is Windows-only. Make sure we aren't trying to
# compile it on another platform
#
ifneq ($(PLATFORM),win32)
dummy := $(error The winaudio device is for Windows only. Can not compile on platform $(PLATFORM))
endif

ISM_DEPTH = ../..
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
#			  Set this macro instead of the TARGETS macro.
#			  The 'all' target should depend upon $(DSO_TARGET)
#			  intead of $(TARGETS)
#                         (Note: depend on DSO_TARGET, NOT DSO_TARGET_BASENAME)
#
# DSO_MAJOR_VERSION     - Major version number (e.g. DSO_MAJOR_VERSION = 1).
#
# DSO_MINOR_VERSION     - Minor version number (e.g. DSO_MINOR_VERSION = 0)
#
DSO_TARGET_BASENAME = winaudio
DSO_MAJOR_VERSION = 1
DSO_MINOR_VERSION = 0

#
# The source files comprising the library.
#
CFILES = \
	generic.c \
	winaudio.c \
	audioParams.c \
	audioLoop.c \
	devOps.c \
	dllmain.c
       $(NULL)

HFILES = \
	generic.h \
	$(NULL)

LLDLIBPATHS = $(DIST_ROOT)/usr/lib
LLDLIBS = ML10.lib MLU10.lib

ifeq ($(USE_COMPILER),gnu)
LLDLIBS	+= winmm
else
LLDLIBS += winmm.lib
endif

ifeq ($(USE_COMPILER),msvc)
LLDLIBS += user32.lib
endif

NT_DEF_FILE = winaudio.def

#
#
# Note that 'all' depends on $(DSO_TARGET) instead of $(TARGETS).
#
all: default $(_FORCE)

default: $(DSO_TARGET)
	$(INSTALL_MLMODULE)

#
# The install target simply invokes the $(INSTALL_DSO) macro. This
# macro does all the work of installing the DSO and creating the
# necessary links to keep the dynamic loader happy.
#
install: all
	$(INSTALL_MLMODULE)

include $(ISMRULES)


