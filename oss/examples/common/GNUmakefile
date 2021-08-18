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

TARGETS = \
	audiotomem$(EXE_SUFFIX) \
	memtoaudio$(EXE_SUFFIX) \
	vidtomem$(EXE_SUFFIX) \
	memtovid$(EXE_SUFFIX) \
	filetoaudio$(EXE_SUFFIX) \
	audiotofile$(EXE_SUFFIX) \
	controls$(EXE_SUFFIX) \
	$(NULL)

CFILES = audiotomem.c memtoaudio.c \
	vidtomem.c memtovid.c filetoaudio.c audiotofile.c controls.c \
	utils.c audioUtils.c

HFILES = utils.h audioUtils.h

ifeq ($(PLATFORM),win32)
TARGETS += mlAudioCapture.exe

CFILES += mlAudioCapture.c mlOperations.c
RESFILES = mlAudioCapture.res
VPATH = ../win32/mlAudioCapture

else
TARGETS += vidtogfx

CFILES += vidtogfx.c
endif

ifeq ($(PLATFORM),win32)

#
# Windows
#

LLDLIBPATHS = $(DIST_ROOT)/usr/lib ../win32
LLDLIBS = ML10.lib MLU10.lib audiofile.lib
LCINCS = ../win32

NO_DEFAULT_TARGETS_RULE = 1

else

#
# Linux / Unix
#

#
# Tell the compiler where our ISM header files are located.
#
LCINCS = \
	-I$(DIST_ROOT)/usr/include/

#
# Tell the linker where our ISM libraries are located.
#
LLDOPTS = -L$(DIST_ROOT)/usr/lib/ -L/usr/local/lib -L/usr/X11/lib \
	-L/usr/X11R6/lib

#
# Tell the linker which libraries to link with the executable.
#
LLDLIBS =

LSOURCES = README.audio README.video

# Turn on debugging
OPTIMIZE = -g

endif


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


ifeq ($(PLATFORM),win32)

_LINK_RULE = $(_LINKER) $^ $(LD_DASH_O)$@ $(LDFLAGS)

audiotomem.exe: audiotomem.obj utils.obj audioUtils.obj

memtoaudio.exe: memtoaudio.obj utils.obj audioUtils.obj

filetoaudio.exe: filetoaudio.obj utils.obj audioUtils.obj

audiotofile.exe: audiotofile.obj utils.obj audioUtils.obj

%.exe: %.$(OBJ_SUFFIX) utils.obj
	$(_LINK_RULE)

mlAudioCapture.exe: mlAudioCapture.obj mlOperations.obj mlAudioCapture.res
	$(_LINK_RULE) user32.lib gdi32.lib comdlg32.lib

mlAudioCapture.res: mlAudioCapture.rc
	rc.exe /l 0x409 /fo$@ $^


clean: cleanResFiles

.PHONY: cleanResFiles
cleanResFiles:
	rm -f -r $(RESFILES)

install: mlAudioCapture.exe

else

_LINK_RULE = $(CC) $(CFLAGS) -o $@ $^ $(LDOPTS) -lML -lMLU

audiotomem: audiotomem.o utils.o audioUtils.o
	$(_LINK_RULE) -l audiofile

memtoaudio: memtoaudio.o utils.o audioUtils.o
	$(_LINK_RULE) -l audiofile

audiotofile: audiotofile.o utils.o audioUtils.o
	$(_LINK_RULE) -l audiofile

filetoaudio: filetoaudio.o utils.o audioUtils.o
	$(_LINK_RULE) -l audiofile

vidtomem: vidtomem.o utils.o
	$(_LINK_RULE)

memtovid: memtovid.o utils.o
	$(_LINK_RULE)

vidtogfx: vidtogfx.o utils.o
	$(_LINK_RULE) -lGLU -lGL -lXaw -lXmu -lXext -lX11

controls: controls.o utils.o
	$(_LINK_RULE)

endif

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

ifeq ($(PLATFORM),win32)
EXAMPLES_DIR = $(DIST_ROOT)/usr/examples/ml
BIN_DIR = $(DIST_ROOT)/usr/bin/

INSTALL_PLATFORM_FILES = \
	$(INSTALL) -m 644 ../win32/audiofile.h $(EXAMPLES_DIR)/; \
	$(INSTALL) -m 644 ../win32/aupvlist.h $(EXAMPLES_DIR)/; \
	$(INSTALL) -m 644 ../win32/audiofile.lib $(EXAMPLES_DIR)/; \
	$(INSTALL) -m 755 ../win32/audiofile.dll $(EXAMPLES_DIR)/; \
	$(INSTALL) -m 644 ../win32/ML_examples.dsw $(EXAMPLES_DIR)/; \
	$(INSTALL) -m 644 ../win32/audiotomem.dsp $(EXAMPLES_DIR)/; \
	$(INSTALL) -m 644 ../win32/memtoaudio.dsp $(EXAMPLES_DIR)/; \
	$(INSTALL) -m 644 ../win32/audiotofile.dsp $(EXAMPLES_DIR)/; \
	$(INSTALL) -m 644 ../win32/filetoaudio.dsp $(EXAMPLES_DIR)/; \
	$(INSTALL) -m 644 ../win32/vidtomem.dsp $(EXAMPLES_DIR)/; \
	$(INSTALL) -m 644 ../win32/memtovid.dsp $(EXAMPLES_DIR)/; \
	$(INSTALL) -m 644 ../win32/controls.dsp $(EXAMPLES_DIR)/; \
	$(INSTALL) -d $(EXAMPLES_DIR)/mlAudioCapture/; \
	$(INSTALL) -m 644 ../win32/mlAudioCapture/mlAudioCapture.dsw $(EXAMPLES_DIR)/mlAudioCapture/; \
	$(INSTALL) -m 644 ../win32/mlAudioCapture/mlAudioCapture.dsp $(EXAMPLES_DIR)/mlAudioCapture/; \
	$(INSTALL) -m 644 ../win32/mlAudioCapture/mlAudioCapture.rc $(EXAMPLES_DIR)/mlAudioCapture/; \
	$(INSTALL) -m 644 ../win32/mlAudioCapture/mlAudioCapture.h $(EXAMPLES_DIR)/mlAudioCapture/; \
	$(INSTALL) -m 644 ../win32/mlAudioCapture/mlAudioCapture.c $(EXAMPLES_DIR)/mlAudioCapture/; \
	$(INSTALL) -m 644 ../win32/mlAudioCapture/mlOperations.h $(EXAMPLES_DIR)/mlAudioCapture/; \
	$(INSTALL) -m 644 ../win32/mlAudioCapture/mlOperations.c $(EXAMPLES_DIR)/mlAudioCapture/; \
	$(INSTALL) -m 644 ../win32/mlAudioCapture/resource.h $(EXAMPLES_DIR)/mlAudioCapture/;\
	$(INSTALL) -m 644 ../win32/mlAudioCapture/icon1.ico $(EXAMPLES_DIR)/mlaudiocapture/;\
	$(INSTALL) -m 755 mlAudioCapture.exe $(BIN_DIR)/


else
EXAMPLES_DIR = $(DIST_ROOT)/usr/src/ml/examples

INSTALL_PLATFORM_FILES = \
	$(INSTALL) -m 644 vidtogfx.c $(EXAMPLES_DIR)

endif

install:
	$(INSTALL) -d $(EXAMPLES_DIR)/
	$(INSTALL) -m 644 utils.h $(EXAMPLES_DIR)/
	$(INSTALL) -m 644 utils.c $(EXAMPLES_DIR)/
	$(INSTALL) -m 644 audioUtils.h $(EXAMPLES_DIR)/
	$(INSTALL) -m 644 audioUtils.c $(EXAMPLES_DIR)/
	$(INSTALL) -m 644 audiotomem.c $(EXAMPLES_DIR)/
	$(INSTALL) -m 644 memtoaudio.c $(EXAMPLES_DIR)/
	$(INSTALL) -m 644 audiotofile.c $(EXAMPLES_DIR)/
	$(INSTALL) -m 644 filetoaudio.c $(EXAMPLES_DIR)/
	$(INSTALL) -m 644 vidtomem.c $(EXAMPLES_DIR)/
	$(INSTALL) -m 644 memtovid.c $(EXAMPLES_DIR)/
	$(INSTALL) -m 644 controls.c $(EXAMPLES_DIR)/
	$(INSTALL) -m 644 README.audio $(EXAMPLES_DIR)/
	$(INSTALL) -m 644 README.video $(EXAMPLES_DIR)/
	$(INSTALL_PLATFORM_FILES)

headers:
	$(NOOP)

dist:
	$(NOOP)
