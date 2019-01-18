# This is the common portion of the make system. You should not have to make any
# modifications to this file.

PROJECTDIR := $(shell pwd)
PACKAGEBASENAME := $(shell basename `pwd` | tr '[:upper:]' '[:lower:]')
PACKAGEBASENAME := $(shell echo $(PACKAGEBASENAME) | sed 's/^$(PREFIX)//')
BUILDSYSTEMDIR := $(PROJECTDIR)/BuildSystem
VERSION := $(shell $(BUILDSYSTEMDIR)/revision.sh)
VERSIONMACRO := $(shell echo $(PREFIX)$(PACKAGEBASENAME) | tr '[:lower:]' '[:upper:]')_VERSION
TARGETDIR := /usr/local

OS := $(shell uname -s)
ARCH := $(OS)-$(shell uname -m)
BUILDDIR := .build/$(ARCH)
PREREQSDIR := .prereqs/$(ARCH)
HEADERDIR := $(BUILDDIR)/include/$(PREFIX)/$(PACKAGEBASENAME)
LIBDIR := $(BUILDDIR)/lib
LIBS := $(LIBS) $($(ARCH)_LIBS)

ifeq ($(OS),Darwin)
	SOEXT := .$(VERSION).dylib
	SOEXT_SHORT := .dylib
	LDPATHEXPR := DYLD_LIBRARY_PATH="$(LIBDIR)"
	CC := clang
	CXX := clang++
else
	SOEXT := .so.$(VERSION)
	SOEXT_SHORT := .so
	LIBS := $(LIBS) -lpthread
	LDPATHEXPR := LD_LIBRARY_PATH="$(LIBDIR)"
	CC := gcc
	CXX := g++
	CFLAGS := $(CFLAGS) -fPIC 
endif

CFLAGS := $(CFLAGS) -Wall
OPTIMIZED_FLAGS := -O2 -DNDEBUG
DEBUGGING_FLAGS := -g
ifeq ($(DEBUG),true)
	CFLAGS := $(CFLAGS) $(DEBUGGING_FLAGS)
else
	CFLAGS := $(CFLAGS) $(OPTIMIZED_FLAGS)
endif

CXXFLAGS := $(CXXFLAGS) $(CFLAGS)
LDFLAGS := $(LDFLAGS) -L$(LIBDIR)
ifneq ("$(wildcard $(PREREQSDIR)/lib)","")
    LDFLAGS := $(LDFLAGS) -L$(PREREQSDIR)/lib
endif

-include $(PROJECTDIR)/config.local
-include $(PROJECTDIR)/config.defs

CFLAGS := $(CFLAGS) -I$(BUILDDIR)/include
CXXFLAGS := $(CXXFLAGS) -I$(BUILDDIR)/include -std=c++14 -Wno-unknown-pragmas
ifneq ("$(wildcard $(PREREQSDIR)/include)","")
    CFLAGS := $(CFLAGS) -I$(PREREQSDIR)/include
    CXXFLAGS := $(CXXFLAGS) -I$(PREREQSDIR)/include
endif

.PHONY: build library install check clean cleanall directory-checks hello prep docs help prereqs

LIBNAME := $(PREFIX)$(PACKAGEBASENAME)
LIBFILE := lib$(LIBNAME)$(SOEXT)
LIBFILELINK := lib$(LIBNAME)$(SOEXT_SHORT)
LIBPATH := $(LIBDIR)/$(LIBFILE)

TESTDIR := $(BUILDDIR)/tests
TESTPATH := $(TESTDIR)/unittest


# Turn off the building of the tests if there is no Tests directory.
ifeq ($(wildcard Tests/.*),)
	TESTPATH :=
endif

build: library

library: $(LIBPATH)


# Use "make help" to give some instructions on how the build system works.

help:
	@cat BuildSystem/makefile_help.txt

# Use "make hello" to test that the names are what you would expect.

hello:
	@echo "Hello from $(PACKAGEBASENAME)"
	@echo "  PROJECTDIR=$(PROJECTDIR)"
	@echo "  TARGETDIR=$(TARGETDIR)"
	@echo "  VERSION=$(VERSION)"
	@echo "  HEADERDIR=$(HEADERDIR)"
	@echo "  LIBNAME=$(LIBNAME)"
	@echo "  LIBPATH=$(LIBPATH)"
	@echo "  CC=$(CC)"
	@echo "  CFLAGS=$(CFLAGS)"
	@echo "  CXX=$(CXX)"
	@echo "  CXXFLAGS=$(CXXFLAGS)"
ifneq ($(wildcard Tests/.*),)
	@echo "  TESTPATH=$(TESTPATH)"
endif

prereqs:
	BuildSystem/update_prereqs.py

# Build the library

C_SRCS := $(wildcard Sources/*.c)
CPP_SRCS := $(wildcard Sources/*.cpp)
SRCS := $(C_SRCS) $(CPP_SRCS)
SRCS := $(filter-out Sources/main.cpp, $(SRCS))
HDRS := $(wildcard Sources/*.h) $(wildcard Sources/*.hpp)
HDRS := $(filter-out Sources/all.h, $(HDRS))
INSTALLHDRS := $(HDRS) Sources/all.h
INSTALLHDRS := $(filter-out $(wildcard Sources/_*), $(INSTALLHDRS))
OBJS := $(patsubst Sources/%.cpp,$(BUILDDIR)/%.o,$(CPP_SRCS)) $(patsubst Sources/%.c,$(BUILDDIR)/%.o,$(C_SRCS))

PREPS := Sources/_version_internal.h Sources/_license_internal.h Sources/all.h $(HEADERDIR)

prep: $(PREPS)

$(LIBPATH): $(PREPS) $(LIBDIR) $(OBJS)
	$(CXX) $(LDFLAGS) -shared $(OBJS) $(LIBS) -o $@
	(cd $(LIBDIR) ; rm -f $(LIBFILELINK) ; ln -s $(LIBFILE) $(LIBFILELINK))

$(BUILDDIR)/%.o: Sources/%.cpp $(HDRS)
	$(CXX) -c $< $(CXXFLAGS) -o $@

$(BUILDDIR)/%.o: Sources/%.c $(HDRS)
	$(CC) -c $< $(CFLAGS) -o $@

$(BUILDDIR):
	-mkdir -p $@

$(LIBDIR):
	-mkdir -p $@

Sources/_version_internal.h: REVISION
	echo "Rebuild $@"
	@echo "/* This file is auto-generated and should not be edited. */" > $@
	@echo "namespace {" >> $@
	@echo "    constexpr const char* versionText = \"$(VERSION)\";" >> $@
	@echo "}" >> $@
	@echo "" >> $@

Sources/_license_internal.h: LICENSE
	echo "Rebuild $@"
	@echo "/* This file is auto-generated and should not be edited. */" > $@
	@echo "namespace {" >> $@
	@echo "    constexpr const char* licenseText = R\"TXT(" >> $@
	cat LICENSE >> $@
	@echo ")TXT\";" >> $@
	@echo "}" >> $@
	@echo "" >> $@

Sources/all.h: $(HDRS)
	(cd Sources ; PREFIX=$(LIBNAME) $(BUILDSYSTEMDIR)/generate_all_h.sh) > $@

$(HEADERDIR):
	echo build $(HEADERDIR)
	-mkdir -p $(BUILDDIR)/include/$(PREFIX)
	-rm $(BUILDDIR)/include/$(PREFIX)/$(PACKAGEBASENAME)
	-ln -s `pwd`/Sources $(BUILDDIR)/include/$(PREFIX)/$(PACKAGEBASENAME)


# Build and run the unit tests.

TESTSRCS := $(wildcard Tests/*.cpp)
ifneq ($(wildcard Sources/main.cpp),)
	TESTSRCS := $(TESTSRCS) $(SRCS)
endif
TESTSRCS := $(filter-out Sources/main.cpp, $(TESTSRCS))
TESTOBJS := $(patsubst Tests/%.cpp,$(TESTDIR)/%.o,$(TESTSRCS))
TESTHDRS := $(wildcard Tests/*.h) $(wildcard Tests/*.hpp)

check: library $(TESTPATH)
ifneq ($(wildcard $(EXEPATH)),)
	$(LDPATHEXPR) $(EXEPATH) --version
endif
	$(LDPATHEXPR) $(TESTPATH)

$(TESTPATH): $(LIBPATH) $(TESTDIR) $(TESTOBJS)
	$(CXX) $(LDFLAGS) -L$(BUILDDIR) $(TESTOBJS) -l $(LIBNAME) $(LIBS) -o $@

$(TESTDIR):
	-mkdir -p $@

$(TESTDIR)/%.o: Tests/%.cpp $(TESTHDRS)
	$(CXX) -c $< $(CXXFLAGS) -I. -o $@


# Build the documentation.
docs:
	-rm -rf docs
	doxygen Doxyfile

# Perform the install.
install: $(TARGETDIR)/include/$(PREFIX) $(TARGETDIR)/lib
	-rm -rf $(TARGETDIR)/include/$(PREFIX)/$(PACKAGEBASENAME)
	-mkdir -p $(TARGETDIR)/include/$(PREFIX)/$(PACKAGEBASENAME)
	cp $(INSTALLHDRS) $(TARGETDIR)/include/$(PREFIX)/$(PACKAGEBASENAME)
	chmod 644 $(TARGETDIR)/include/$(PREFIX)/$(PACKAGEBASENAME)/*
	cp $(LIBPATH) $(TARGETDIR)/lib
	(cd $(TARGETDIR)/lib ; rm -f $(LIBFILELINK) ; ln -s $(LIBFILE) $(LIBFILELINK))

$(TARGETDIR)/include/$(PREFIX):
	-mkdir -p $@

$(TARGETDIR)/lib:
	-mkdir -p $@

# Clean the build.
clean:
	rm -rf $(BUILDDIR) REVISION Sources/_version_internal.h \
		Sources/_license_internal.h Sources/all.h

cleanall: clean
	rm -rf .build config.defs config.target.defs docs .prereqs
