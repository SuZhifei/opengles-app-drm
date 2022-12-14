ifeq "$(X11BUILD)" "1"

WS_LIBS = -L$(X11ROOT)/lib -Wl,--rpath-link,$(X11ROOT)/lib -lX11 -lXau -ldl
WS_INC  = $(X11ROOT)/include
WS       = X11
else

ifeq "$(EWSBUILD)" "1"

PLAT_CFLAGS += -DEWS
WS_LIBS = -lews
WS_INC =
WS=EWS

else

ifeq "$(DRMBUILD)" "1"

WS_LIBS =
WS_INC =
WS=Drm
DRMROOT=$(SDKDIR)/Builds/Linux/armv7omap/libdrm

else

WS_LIBS =
WS_INC  =
WS = NullWS

endif

endif

endif

PLAT_LINK += $(WS_LIBS)
PLAT_INC  += $(WS_INC)
