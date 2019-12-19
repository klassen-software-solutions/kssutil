PREFIX := kss
PROJECT_NAME := KSSUtil
PROJECT_TITLE := C++ general utility library
LIBS :=
TESTLIBS := -lksstest

OS := $(shell uname -s)
ifeq ($(OS),Linux)
    LIBS := -luuid
endif

include BuildSystem/common.mk
