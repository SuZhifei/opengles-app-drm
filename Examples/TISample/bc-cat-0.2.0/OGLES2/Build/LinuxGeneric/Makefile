#
# File			Makefile
# Title			Example makefile
#
# Copyright		Copyright (C) Imagination Technologies Limited.
#

.PHONY: clean

SDKDIR  = ../../../../../..
OUTNAME = OGLES2StreamCube
APIS    = OGLES2

OBJECTS +=	\
			common.o \
			drv_util.o \
			streamcube.o

LINK +=	\
       -L../..	\
       -lpthread -lrt

VPATH +=	\
         ../../Content

PLAT_CFLAGS += -DGLES_20

include $(SDKDIR)/Builds/Linux/make_example.mak

$(PLAT_OBJPATH)/%.o: ../../Content/%.cpp ../../%.c
	mkdir -p $(PLAT_OBJPATH)
	$(PLAT_CPP) $(PLAT_CFLAGS) $(INCLUDES) -MF"$(@:.o=.d)" -MG -MM -MP -MT"$(@:.o=.d)" -MT"$@" "$<"
	$(PLAT_CPP) -c $(PLAT_CFLAGS) $(INCLUDES) $< -o$@


