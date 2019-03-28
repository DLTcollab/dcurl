# Copy from the Makefile of libtuv to support different platforms
UNAME_M := $(shell uname -m)
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    UNAME_S := linux
endif
ifeq ($(UNAME_S),Darwin)
    # macOS
    UNAME_S := darwin
endif
# For de10-nano, arrow sockit and raspberry pi 2/3 board
ifeq ($(UNAME_M),armv7l)
    UNAME_M := arm
endif

# libtuv related variables
LIBTUV_PATH = deps/libtuv
LIBTUV_INCLUDE := -I $(LIBTUV_PATH)/include
LIBTUV_PLATFORM := $(UNAME_M)-$(UNAME_S)
LIBTUV_BOARD := $(BUILD_BOARD)
# PIC (Position-Independent-Code) library
LIBTUV_LIBRARY := $(LIBTUV_PATH)/build/$(LIBTUV_PLATFORM)/release/lib/libtuv.o

$(LIBTUV_PATH)/include:
	git submodule update --init $(LIBTUV_PATH)

$(LIBTUV_LIBRARY):
	$(MAKE) -C $(LIBTUV_PATH) TUV_BUILD_TYPE=release TUV_CREATE_PIC_LIB=yes TUV_PLATFORM=$(LIBTUV_PLATFORM) TUV_BOARD=$(LIBTUV_BOARD)
