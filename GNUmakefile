#! gmake
#*************************************************************************
#
# Makefile - OpenML ML SDK
#
# This is the ISM top level Makefile. It meets the requirements for the
# SGI Build Environment, sgibuild(7).
#
#*************************************************************************

#
# Build setup for Linux in and out of the ProPack environment.
#
ifneq ($(PLATFORM),win32)
 export PLATFORM = linux
 LBSDEFS = $(WORKAREA)/linuxmeister/lbs/lbsdefs
 LBSAREA := $(shell oss/make/linux/testexist $(LBSDEFS))

 ifeq ($(LBSAREA),yes)
  export LBSDEFS
  include $(LBSDEFS)
  export XPROOT = $(WORKAREA)/dmedia/ml
  export BUILDER = Silicon Graphics, Inc. <http://www.sgi.com/>
  export PKG_RELEASE = $(release-number)
 else
  ifdef SGI_INTERNAL_BUILD_ENV
   export XPROOT = $(WORKAREA)/ml
  endif
 endif

 export DIST_ROOT = $(XPROOT)/linux
 export LROOT = $(XPROOT)/linux
 export BOOT_ROOT = $(XPROOT)/oss/make/linux
 export LD_LIBRARY_PATH =:$(XPROOT)/linux/usr/lib
 export PATH = /usr/kerberos/sbin:/usr/kerberos/bin:/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin:/usr/X11R6/bin:/root/bin:($WORKAREA)/ml/linux/usr/bin:/usr/local/bin/ptools
endif

#
# Every Makefile must define ISM_DEPTH to be the path to the top of
# the ISM. In this case we are already at the top. The ismdefs file
# must then be included.
#
ISM_DEPTH=$(XPROOT)/oss/make/$(PLATFORM)
include	$(ISM_DEPTH)/commondefs

#
# List all subdirectories that need to be built (i.e. that have
# a Makefile). Note that the 'build' subdirectory is not listed
# since there is no Makefile in that directory.
#

# (Note the reason we descend from here into the devices directories
#  instead of the next directory down, is that each builds it's own
#  RPM files.)

# (Note: some devices exist only on specific platforms, so break out
# those into separate sub-lists).
ifeq ($(PLATFORM),win32)
PLATFORM_SUBDIRS = \
	oss/devices/winaudio
else
PLATFORM_SUBDIRS = \
	oss/devices/ossaudio \
	oss/devices/v4l
endif

SUBDIRS = \
	oss \
	oss/devices/nullxcode \
	oss/devices/ustsource \
	$(PLATFORM_SUBDIRS) \
	$(NULL)

#
# Typical target for building subdirectories
#
all dist install: headers $(_FORCE)
	$(SUBDIRS_MAKERULE)

headers clean copy: $(_FORCE)
	$(SUBDIRS_MAKERULE)

sources: $(_FORCE)
	$(SUBDIRS_MAKERULE)


TOP_LEVEL_FILES=	\
	BUILD_LINUX \
	GNUmakefile \
	LICENSES \
	VERIFY_LINUX \
	SET_ENV \
	$(NULL)

# Public 'distoss' rule:
#
# This simply invokes a new make process with OPENML_DEBUG=0, to
# ensure the distribution is built from non-debug objects.
#
# The actual dist rule is called _internal_dist_target.  Naturally,
# that should *not* be called directly by the end-user
distoss:
	$(MAKEF) OPENML_DEBUG=0 _internal_dist_target


ifeq ($(PLATFORM),win32)

#
# Windows distribution rules
#

MSINSTALLER_EXE = \
	"C:/Program Files/Microsoft Visual Studio/Common/IDE/IDE98/DEVENV.exe"
MSINSTALLER_SRC = oss/build/win32/ML_SDK_installer/ML_SDK_installer.sln
MSINSTALLER_CACHE = oss/build/win32/ML_SDK_installer/ML_SDK_installer.cache

PACKAGE_FILE = $(XPROOT)/mlSDK.zip

distclean: clean
	-rm -rf win32
	-rm $(PACKAGE_FILE)
	-rm $(MSINSTALLER_CACHE)
	-find . -name \*~ -print | xargs rm

# create distribution:
# - start by a complete clean, including the cleaning of the win32 dir
#   (which contains results of compilations, etc.)
# - build everything and install into win32
#
# These two steps are accomplished by depending on the distclean and install
# targets. Then, with our own rules:
# - clean the source again, to remove all temporary compilation files
#   (so that when we zip up the source, it contains no garbage)
#   Note: can't use a dependency on the clean target for this, because
#   we already used distclean which uses clean -- and make figures that
#   this is up-to-date and does not need to be called again. So we must
#   call it explicitly.
# - zip up the source, place it where the Installer script will find it
# - invoke the MS Installer to build the distribution
# - zip up the resulting files into the final distribution package file
_internal_dist_target: distclean install
	$(MAKEF) clean
	find $(TOP_LEVEL_FILES) oss -path \*/CVS -prune -o \
		-path oss/tools/mldaemon -prune -o \
		-path oss/testing -prune -o -print | \
		zip win32/ML-sdk-source.zip -@
	$(MSINSTALLER_EXE) /make /release $(MSINSTALLER_SRC)
	( cd win32/DISK_1; zip $(PACKAGE_FILE) mlSDK.msi mlSDK01.cab )

else

SOURCE_FILE_NAME =	ml-1.1.1.tgz

ifeq ($(PLATFORM),linux)

ML_DIR_NAME := $(shell pwd | xargs basename)

distclean: clean
	-rm -rf linux
	-rm $(SOURCE_FILE_NAME)
	-find . -name \*~ -print | xargs rm

realclean:
	make clean

clean-sgi:
clean-lbs:
	make clean

build-sgi:
	make dist

build-lbs:
	make dist
	make copy

ifneq ($(ML_DIR_NAME),ml)
_internal_dist_target:
	$(error Directory name must be 'ml' (not '$(ML_DIR_NAME)') to use \
	dist rule)

else

_internal_dist_target: distclean install
	$(MAKEF) clean
	find $(TOP_LEVEL_FILES) linux oss -path \*/CVS -prune -o \
		-path oss/tools/mldaemon -prune -o \
		-path oss/testing -prune -o -not -type d -a \
		-printf "$(ML_DIR_NAME)/%p\n" > TAR-FILES
	(cd .. ; tar zcvf $(SOURCE_FILE_NAME) -T $(ML_DIR_NAME)/TAR-FILES)
	rm TAR-FILES

endif

else

## Platform IRIX

#
# make the source tarball for "oss" distribution
#

SOURCE_DIRECTORY =	sgi/html/oss/download/source
TAR		 =	tar

_internal_dist_target: headers $(_FORCE)
	$(SUBDIRS_MAKERULE)

source_tarball:	$(SOURCE_DIRECTORY)
	rm -f FILES $(SOURCE_DIRECTORY)/$(SOURCE_FILE_NAME)
	p_list -a $(TOP_LEVEL_FILES) oss | sort | \
	    fgrep -vf build/linux/linux_source.exclude >FILES
	$(TAR) -cvzf $(SOURCE_DIRECTORY)/$(SOURCE_FILE_NAME) -T FILES

$(SOURCE_DIRECTORY):
	mkdir -p $(SOURCE_DIRECTORY)

FILES:	$(_FORCE)
	p_list -a $(TOP_LEVEL_FILES) oss | sort | \
	    fgrep -vf build/linux/linux_source.exclude >FILES

clobber: clean distclean
	rm -rf linux FILES
	find . -name mlog | xargs rm  -f

#
# if we "distclean" from the top level then clean out the images directory
#
distclean: imagesclean
	$(SUBDIRS_MAKERULE)

imagesclean:
	rm -rf linux/images
	rm -rf linux/sources
endif
endif

#
# The top level Makefile must include $(ISMCOMMONRULES) anywhere
# after the first target is defined. Non-top level Makefiles
# include $(ISMRULES) instead of $(ISMCOMMONRULES).
#
include $(COMMONRULES)

