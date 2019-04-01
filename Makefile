PREFIX := kss
LIBS :=

OS := $(shell uname -s)
ifeq ($(OS),Linux)
    LIBS := -luuid
endif

include BuildSystem/common.mk
