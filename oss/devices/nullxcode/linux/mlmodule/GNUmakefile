#! gmake
#*************************************************************************
#
# Makefile - nullxcode/mlmodule
#
# The job of this Makefile is to descend into subdirectories. 
#
#*************************************************************************

#
# Every Makefile must define ISM_DEPTH to be the path to the top of
# the ISM. In this case the top is one directory up. The ismdefs file
# must then be included.
#
ISM_DEPTH = ..
include	$(ISM_DEPTH)/ismdefs

#
# All subdirectories that need to be built
#
SUBDIRS = \
	src \
	$(NULL)

#
# All targets descend into the subdirectories listed in $(SUBDIRS).
#
all: default $(_FORCE)

default $(COMMONTARGS): $(_FORCE)
	$(SUBDIRS_MAKERULE) 

#
# Always include ismrules using the $(ISMRULES) macro anywhere after
# the first target is defined.
#
include $(ISMRULES)
