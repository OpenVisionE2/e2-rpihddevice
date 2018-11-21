#
# Makefile for a Enigma2 rpihddevice library
#
# $Id: Makefile 2.18 2013/01/12 13:45:01 kls Exp $

# The official name of this E2 lib.
# By default the main source file also carries this name.

E2LIB = rpihddevice

### The version number of this lib (taken from the main source file):
MAJOR = 1
MINOR = 0.4
VERSION = $(MAJOR).$(MINOR)

### The directory environment:

LIBDIR = /usr/lib
LOCDIR = /usr/share
INCDIR = /usr/include

### The compiler options:

CFLAGS   ?= -g -O3 -Wall
CXXFLAGS ?= -g -O3 -Wall -Werror=overloaded-virtual -Wno-parentheses
CDEFINES  = -D_GNU_SOURCE
CDEFINES += -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE

### The name of the distribution archive:

ARCHIVE = $(E2LIB)-$(VERSION)
PACKAGE = e2-$(ARCHIVE)

### The name of the shared object file:

SOFILE = lib$(E2LIB).so

### Includes and Defines (add further entries here):

DEFINES += -DPLUGIN_NAME_I18N='"$(E2LIB)"'
DEFINES += -DHAVE_LIBOPENMAX=2 -DOMX -DOMX_SKIP64BIT -DUSE_EXTERNAL_OMX -DHAVE_LIBBCM_HOST -DUSE_EXTERNAL_LIBBCM_HOST -DUSE_VCHIQ_ARM
DEFINES += -Wno-psabi -Wno-write-strings -fpermissive
DEFINES += -D__STL_CONFIG_H

CFLAGS   += $(CDEFINES) -fPIC
CXXFLAGS += $(CDEFINES) -D__STDC_CONSTANT_MACROS -fPIC

ILCDIR   =ilclient
VCINCDIR =$(SDKSTAGE)/usr/include
VCLIBDIR =$(SDKSTAGE)/usr/lib
#SIGC2LIBDIR =/usr/include/sigc++-2.0
#SIGC2LIBDIR2 =/usr/lib/arm-linux-gnueabihf/sigc++-2.0/include

INCLUDES += -I$(ILCDIR) -I$(VCINCDIR) -I$(VCINCDIR)/interface/vcos/pthreads
INCLUDES += -I$(VCINCDIR)/interface/vmcs_host/linux 
#INCLUDES += -I$(SIGC2LIBDIR) -I$(SIGC2LIBDIR2)

LDLIBS  += -lbcm_host -lvcos -lvchiq_arm -lopenmaxil -lGLESv2 -lEGL -lpthread -lrt -ljpeg
LDLIBS  += -Wl,--whole-archive $(ILCDIR)/libilclient.a -Wl,--no-whole-archive
LDFLAGS += -L$(VCLIBDIR)
#LDFLAGS += -L$(VCLIBDIR) -lbcm_host -lvcos -lvchiq_arm -lopenmaxil -lGLESv2 -lEGL -lpthread -lrt -lavcodec -lavformat -lswresample

DEBUG ?= 0
ifeq ($(DEBUG), 1)
    DEFINES += -DDEBUG
endif

DEBUG_BUFFERSTAT ?= 0
ifeq ($(DEBUG_BUFFERSTAT), 1)
    DEFINES += -DDEBUG_BUFFERSTAT
endif

DEBUG_BUFFERS ?= 0
ifeq ($(DEBUG_BUFFERS), 1)
    DEFINES += -DDEBUG_BUFFERS
endif

DEBUG_OVGSTAT ?= 0
ifeq ($(DEBUG_OVGSTAT), 1)
    DEFINES += -DDEBUG_OVGSTAT
endif

ENABLE_AAC_LATM ?= 0
ifeq ($(ENABLE_AAC_LATM), 1)
    DEFINES += -DENABLE_AAC_LATM
endif

# ffmpeg/libav configuration
ifdef EXT_LIBAV
	LIBAV_PKGCFG = $(shell PKG_CONFIG_PATH=$(EXT_LIBAV)/lib/pkgconfig pkg-config $(1))
else
	LIBAV_PKGCFG = $(shell pkg-config $(1))
endif

LDLIBS   += $(call LIBAV_PKGCFG,--libs libavcodec) $(call LIBAV_PKGCFG,--libs libavformat)
INCLUDES += $(call LIBAV_PKGCFG,--cflags libavcodec) $(call LIBAV_PKGCFG,--cflags libavformat)

ifeq ($(call LIBAV_PKGCFG,--exists libswresample && echo 1), 1)
	DEFINES  += -DHAVE_LIBSWRESAMPLE
	LDLIBS   += $(call LIBAV_PKGCFG,--libs libswresample)
	INCLUDES += $(call LIBAV_PKGCFG,--cflags libswresample)
else
ifeq ($(call LIBAV_PKGCFG,--exists libavresample && echo 1), 1)
	DEFINES  += -DHAVE_LIBAVRESAMPLE
	LDLIBS   += $(call LIBAV_PKGCFG,--libs libavresample)
	INCLUDES += $(call LIBAV_PKGCFG,--cflags libavresample)
endif
endif

LDLIBS   += $(shell pkg-config --libs freetype2)
INCLUDES += $(shell pkg-config --cflags freetype2)

#LDLIBS   += $(shell pkg-config --libs enigma2)
#INCLUDES += $(shell pkg-config --cflags enigma2)

### The object files (add further files here):

ILCLIENT = $(ILCDIR)/libilclient.a
#OBJS = $(E2LIB).o rpisetup.o omx.o rpiaudio.o omxdecoder.o rpidisplay.o
OBJS = rpisetup.o omx.o rpiaudio.o rpidisplay.o condVar.o tools.o

### The main target:

all: $(SOFILE)

### Implicit rules:

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $(DEFINES) $(INCLUDES) -o $@ $<

### Dependencies:

MAKEDEP = $(CXX) -MM -MG
DEPFILE = .dependencies
$(DEPFILE): Makefile
	@$(MAKEDEP) $(CXXFLAGS) $(DEFINES) $(INCLUDES) $(OBJS:%.o=%.cpp) > $@

-include $(DEPFILE)

### Targets:

$(SOFILE): $(ILCLIENT) $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -shared $(OBJS) $(LDLIBS) -Wl,-soname,$(SOFILE).$(MAJOR) -o $@

$(ILCLIENT):
	$(MAKE) --no-print-directory -C $(ILCDIR) all

install-lib: $(SOFILE)
	install -D $^ $(DESTDIR)$(LIBDIR)/$^.$(VERSION)
	install -D $(ILCLIENT) $(DESTDIR)$(LIBDIR)
	ln -s -r $(DESTDIR)$(LIBDIR)/$^.$(VERSION) $(DESTDIR)$(LIBDIR)/$^
	mkdir $(DESTDIR)$(INCDIR)
	cp *.h $(DESTDIR)$(INCDIR)/
	cp $(ILCDIR)/*.h $(DESTDIR)$(INCDIR)/


install: install-lib

dist: $(I18Npo) clean
	@-rm -rf $(DESTDIR)$(LOCDIR)/$(ARCHIVE)
	@mkdir $(DESTDIR)$(LOCDIR)/$(ARCHIVE)
	@cp -a * $(DESTDIR)$(LOCDIR)/$(ARCHIVE)
	@tar czf $(PACKAGE).tgz -C $(DESTDIR)$(LOCDIR) $(ARCHIVE)
	@-rm -rf $(DESTDIR)$(LOCDIR)/$(ARCHIVE)
	@echo Distribution package created as $(PACKAGE).tgz

clean:
	@-rm -f $(OBJS) $(DEPFILE) *.so *.tgz core* *~
	$(MAKE) --no-print-directory -C $(ILCDIR) clean

.PHONY:	cppcheck
cppcheck:
	@cppcheck --language=c++ --enable=all --suppress=unusedFunction -v -f .
