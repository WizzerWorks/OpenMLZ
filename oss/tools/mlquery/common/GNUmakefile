#! gmake
#*************************************************************************
#
# Makefile - mlquery
#
# This Makefile builds the mlquery program executable. 
#
#*************************************************************************

#
# Every Makefile must define ISM_DEPTH to be the path to the top of
# the ISM. In this case the top is two directories up. The ismdefs file
# must then be included.
#
ISM_DEPTH = ../../../build/$(PLATFORM)
include $(ISM_DEPTH)/ismdefs

#
# The TARGETS macro must be set to the name of the executable we
# are building.
#
TARGETS = mlquery$(EXE_SUFFIX)

#
# The program consists of a single C file so it is listed using the
# CFILE macro. See the /usr/include/make/commondefs file for a list
# of the source macros.
#
CXXFILES=mlquery.cxx

ifeq ($(PLATFORM),win32)

#
# Windows
#

LLDLIBPATHS = $(DIST_ROOT)/usr/lib
LLDLIBS = ML10.lib MLU10.lib

INSTALL_BIN_DEST = /usr/bin

else

#
# Unix / Linux
#

#
# Tell the compiler where our ISM header files are located.
#
LCXXINCS = -I$(DIST_ROOT)/usr/include

#
# Tell the linker where our ISM libraries are located.
#
LLDOPTS = -L$(DIST_ROOT)/usr/lib

#
# Tell the linker which libraries to link with the executable.
#
LLDLIBS = -lML -lMLU -lstdc++

INSTALL_BIN_DEST = /usr/bin

endif

#
# The default target, 'all', depends on the building of the target
# executable(s).
#
all: default $(_FORCE)

headers:
	$(NOOP)

exports:
	$(NOOP)

default: $(TARGETS) 

#
# The executable depends on the object files. An implicit rule
# builds the object files from the source files.
#
## Note: this rule is already present in commonrules. Why the need to
## repeat it here?
## Leaving it in for Unix until I have time to figure out if it is really
## needed
## --FJaubert
ifneq ($(PLATFORM),win32)
$(TARGETS): $(OBJECTS)
	$(CCF) $(OBJECTS) $(LDFLAGS) -o $@
endif

#
# The install target depends on the 'all' target to ensure
# that the executable(s) is built before attempting to install it.
#
# Important points:
#
#	1. Every path must be prefixed with $(DIST_ROOT) so that
#	   the installation will be relative to the ISM installation
#	   directory defined during the package build.
#
#	2. Always create the installation directory regardless of
#	   whether it normally exists relative to '/'. This is because
#	   the ISM relative installation start with no directory
#	   hierarchy.
#
#	3. Use $(INSTALL_BIN) when installing compiled executables.
#	   Use $(INSTALL) when installing scripts, non-executable
#	   files and directories. The $(INSTALL_BIN) macro will
#	   automatically strip the executables when installing them
#	   unless the environment variable NOSTRIP is set.
#
install: all
	$(INSTALL) -d $(DIST_ROOT)$(INSTALL_BIN_DEST)
	$(INSTALL_BIN) -m 755 $(TARGETS) $(DIST_ROOT)$(INSTALL_BIN_DEST)

include $(ISMRULES)
