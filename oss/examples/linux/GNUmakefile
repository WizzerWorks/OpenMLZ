#! gmake
#*************************************************************************
#
# Makefile - mlSDK examples
#
#*************************************************************************

#
# Every Makefile must define ISM_DEPTH to be the path to the top of
# the ISM. In this case the top is two directories up. The ismdefs file
# must then be included.
#
ISM_DEPTH = ../../build/$(PLATFORM)
include $(ISM_DEPTH)/ismdefs

LSOURCES = GNUmakefile.user

#
# The default target, 'all', depends on the building of the target
# executable(s).
#
all: default $(_FORCE)

#
# Include $(ISMRULES) after the first target is defined.
#
include $(ISMRULES)

default: $(TARGETS) $(OBJECTS)

#
# The install target depends on the 'all' target to ensure
# that the executable(s) is built before attempting to install it.
#
# Important points:
#
#       1. Every path must be prefixed with $(DIST_ROOT) so that
#          the installation will be relative to the ISM installation
#          directory defined during the package build.
#
#       2. Always create the installation directory regardless of
#          whether it normally exists relative to '/'. This is because
#          the ISM relative installation start with no directory
#          hierarchy.
#
#       3. Use $(INSTALL_BIN) when installing compiled executables.
#          Use $(INSTALL) when installing scripts, non-executable
#          files and directories. The $(INSTALL_BIN) macro will
#          automatically strip the executables when installing them
#          unless the environment variable NOSTRIP is set.
#

EXAMPLES_DIR = $(DIST_ROOT)/usr/src/ml/examples

INSTALL_PLATFORM_FILES = \
	$(INSTALL) -m 644 GNUmakefile.user $(EXAMPLES_DIR)/GNUmakefile

install:
	$(INSTALL) -d $(EXAMPLES_DIR)/
	$(INSTALL_PLATFORM_FILES)

headers:
	$(NOOP)

dist:
	$(NOOP)
