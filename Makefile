BUILDDIR	:=	./build
SDK_BASE	:=	../
TARGET		:=	LCDxpl
SRC_DIR		:= src

SOURCES = \
	lcdxpl.cpp serial.cpp datarefs.cpp commands.cpp

INCLUDES = \
	-I$(SDK_BASE)/SDK/CHeaders/XPLM \
	-I$(SDK_BASE)/SDK/CHeaders/Widgets

DEFINES = -DXPLM200

XPLUGDIR = "/opt/X-Plane 10/Resources/plugins/"

############################################################################

CFLAGS := $(DEFINES) -DAPL=0 -DIBM=0 -DLIN=1 $(INCLUDES) -fPIC -fvisibility=hidden
CFLAGSW := $(DEFINES) -DAPL=0 -DIBM=1 -DLIN=0 $(INCLUDES)
LIBSW32 = $(SDK_BASE)/SDK/Libraries/Win/XPLM.lib
LIBSW64 = $(SDK_BASE)/SDK/Libraries/Win/XPLM_64.lib


VPATH = $(SRC_DIR):$(SDK_BASE)

CSOURCES	:= $(filter %.c, $(SOURCES))
CXXSOURCES	:= $(filter %.cpp, $(SOURCES))


CDEPS		:= $(patsubst %.c, $(BUILDDIR)/obj32/lin/%.cdep, $(CSOURCES))
CXXDEPS		:= $(patsubst %.cpp, $(BUILDDIR)/obj32/lin/%.cppdep, $(CXXSOURCES))
COBJECTS	:= $(patsubst %.c, $(BUILDDIR)/obj32/lin/%.o, $(CSOURCES))
CXXOBJECTS	:= $(patsubst %.cpp, $(BUILDDIR)/obj32/lin/%.o, $(CXXSOURCES))
ALL_DEPS	:= $(sort $(CDEPS) $(CXXDEPS))
ALL_OBJECTS	:= $(sort $(COBJECTS) $(CXXOBJECTS))

CDEPS64			:= $(patsubst %.c, $(BUILDDIR)/obj64/lin/%.cdep, $(CSOURCES))
CXXDEPS64		:= $(patsubst %.cpp, $(BUILDDIR)/obj64/lin/%.cppdep, $(CXXSOURCES))
COBJECTS64		:= $(patsubst %.c, $(BUILDDIR)/obj64/lin/%.o, $(CSOURCES))
CXXOBJECTS64	:= $(patsubst %.cpp, $(BUILDDIR)/obj64/lin/%.o, $(CXXSOURCES))
ALL_DEPS64		:= $(sort $(CDEPS64) $(CXXDEPS64))
ALL_OBJECTS64	:= $(sort $(COBJECTS64) $(CXXOBJECTS64))


CDEPSW32			:= $(patsubst %.c, $(BUILDDIR)/obj32/win/%.cdep, $(CSOURCES))
CXXDEPSW32		:= $(patsubst %.cpp, $(BUILDDIR)/obj32/win/%.cppdep, $(CXXSOURCES))
COBJECTSW32		:= $(patsubst %.c, $(BUILDDIR)/obj32/win/%.o, $(CSOURCES))
CXXOBJECTSW32	:= $(patsubst %.cpp, $(BUILDDIR)/obj32/win/%.o, $(CXXSOURCES))
ALL_DEPSW32		:= $(sort $(CDEPSW32) $(CXXDEPSW32))
ALL_OBJECTSW32	:= $(sort $(COBJECTSW32) $(CXXOBJECTSW32))

CDEPSW64			:= $(patsubst %.c, $(BUILDDIR)/obj64/win/%.cdep, $(CSOURCES))
CXXDEPSW64		:= $(patsubst %.cpp, $(BUILDDIR)/obj64/win/%.cppdep, $(CXXSOURCES))
COBJECTSW64		:= $(patsubst %.c, $(BUILDDIR)/obj64/win/%.o, $(CSOURCES))
CXXOBJECTSW64	:= $(patsubst %.cpp, $(BUILDDIR)/obj64/win/%.o, $(CXXSOURCES))
ALL_DEPSW64		:= $(sort $(CDEPSW64) $(CXXDEPSW64))
ALL_OBJECTSW64	:= $(sort $(COBJECTSW64) $(CXXOBJECTSW64))


# Phony directive tells make that these are "virtual" targets, even if a file named "clean" exists.
.PHONY: all clean $(TARGET)
# Secondary tells make that the .o files are to be kept - they are secondary derivatives, not just
# temporary build products.
.SECONDARY: $(ALL_OBJECTS) $(ALL_OBJECTS64) $(ALL_OBJECTSW32) $(ALL_OBJECTSW64) $(ALL_DEPS)



# Target rules - these just induce the right .xpl files.

$(TARGET): LIN32 LIN64 WIN32 WIN64

LIN32: $(BUILDDIR)/$(TARGET)/32/lin.xpl 
LIN64: $(BUILDDIR)/$(TARGET)/64/lin.xpl
WIN32: $(BUILDDIR)/$(TARGET)/32/win.xpl
WIN64: $(BUILDDIR)/$(TARGET)/64/win.xpl

$(BUILDDIR)/$(TARGET)/32/lin.xpl: $(ALL_OBJECTS)
	@echo Linking $@
	mkdir -p $(dir $@)
	g++ -m32 -static-libgcc -shared -Wl,--version-script=exports.txt -o $@ $(ALL_OBJECTS) $(LIBS)

$(BUILDDIR)/$(TARGET)/64/lin.xpl: $(ALL_OBJECTS64)
	@echo Linking $@
	mkdir -p $(dir $@)
	g++ -m64 -static-libgcc -shared -Wl,--version-script=exports.txt -o $@ $(ALL_OBJECTS64) $(LIBS)

$(BUILDDIR)/$(TARGET)/32/win.xpl: $(ALL_OBJECTSW32)
	@echo Linking $@
	mkdir -p $(dir $@)
	i686-w64-mingw32-g++ -m32 -static-libgcc -static-libgcc -static-libstdc++ -shared -Wl,--version-script=exports.txt -o $@ $(ALL_OBJECTSW32) $(LIBSW32)

$(BUILDDIR)/$(TARGET)/64/win.xpl: $(ALL_OBJECTSW64)
	@echo Linking $@
	mkdir -p $(dir $@)
	x86_64-w64-mingw32-g++ -m64 -static-libgcc -static-libgcc -static-libstdc++ -shared -Wl,--version-script=exports.txt -o $@ $(ALL_OBJECTSW64) $(LIBSW64)

# Compiler rules

# What does this do?  It creates a dependency file where the affected
# files are BOTH the .o itself and the cdep we will output.  The result
# goes in the cdep.  Thus:
# - if the .c itself is touched, we remake the .o and the cdep, as expected.
# - If any header file listed in the cdep turd is changed, rebuild the .o.

$(BUILDDIR)/obj32/lin/%.o : %.c
	mkdir -p $(dir $@)
	g++ $(CFLAGS) -m32 -c $< -o $@
	g++ $(CFLAGS) -MM -MT $@ -o $(@:.o=.cdep) $<

$(BUILDDIR)/obj32/lin/%.o : %.cpp
	mkdir -p $(dir $@)
	g++ $(CFLAGS) -m32 -c $< -o $@
	g++ $(CFLAGS) -MM -MT $@ -o $(@:.o=.cppdep) $<

$(BUILDDIR)/obj64/lin/%.o : %.c
	mkdir -p $(dir $@)
	g++ $(CFLAGS) -m64 -c $< -o $@
	g++ $(CFLAGS) -MM -MT $@ -o $(@:.o=.cdep) $<

$(BUILDDIR)/obj64/lin/%.o : %.cpp
	mkdir -p $(dir $@)
	g++ $(CFLAGS) -m64 -c $< -o $@
	g++ $(CFLAGS) -MM -MT $@ -o $(@:.o=.cppdep) $<


#  apt-get install g++-mingw-w64

$(BUILDDIR)/obj32/win/%.o : %.c
	mkdir -p $(dir $@)
	i686-w64-mingw32-g++ $(CFLAGSW) -m32 -c $< -o $@
	i686-w64-mingw32-g++ $(CFLAGSW) -MM -MT $@ -o $(@:.o=.cdep) $<

$(BUILDDIR)/obj32/win/%.o : %.cpp
	mkdir -p $(dir $@)
	i686-w64-mingw32-g++ $(CFLAGSW) -m32 -c $< -o $@
	i686-w64-mingw32-g++ $(CFLAGSW) -MM -MT $@ -o $(@:.o=.cppdep) $<

$(BUILDDIR)/obj64/win/%.o : %.c
	mkdir -p $(dir $@)
	x86_64-w64-mingw32-g++ $(CFLAGSW) -m64 -c $< -o $@
	x86_64-w64-mingw32-g++ $(CFLAGSW) -MM -MT $@ -o $(@:.o=.cdep) $<

$(BUILDDIR)/obj64/win/%.o : %.cpp
	mkdir -p $(dir $@)
	x86_64-w64-mingw32-g++ $(CFLAGSW) -m64 -c $< -o $@
	x86_64-w64-mingw32-g++ $(CFLAGSW) -MM -MT $@ -o $(@:.o=.cppdep) $<


install:
	cp -r $(BUILDDIR)/$(TARGET) $(XPLUGDIR)

sertest: src/serial.cpp src/sertest.cpp
	g++ src/serial.cpp src/sertest.cpp -o sertest -DLIN=1
	i686-w64-mingw32-g++ src/serial.cpp src/sertest.cpp -o sertest.exe -DIBM=1 -static-libgcc -static-libstdc++

clean:
	@echo Cleaning out everything.
	rm -rf $(BUILDDIR)
	rm -rf sertest
	rm -rf sertest.exe

# Include any dependency turds, but don't error out if they don't exist.
# On the first build, every .c is dirty anyway.  On future builds, if the
# .c changes, it is rebuilt (as is its dep) so who cares if dependencies
# are stale.  If the .c is the same but a header has changed, this 
# declares the header to be changed.  If a primary header includes a 
# utility header and the primary header is changed, the dependency
# needs a rebuild because EVERY header is included.  And if the secondary
# header is changed, the primary header had it before (and is unchanged)
# so that is in the dependency file too.
-include $(ALL_DEPS)
-include $(ALL_DEPS64)
-include $(ALL_DEPSW32)
-include $(ALL_DEPSW64)


