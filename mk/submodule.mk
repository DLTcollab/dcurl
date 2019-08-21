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
LIBTUV_BOARD := $(BOARD)
# PIC (Position-Independent-Code) object file
LIBTUV_OBJS := $(LIBTUV_PATH)/build/$(LIBTUV_PLATFORM)/release/lib/libtuv.o

$(LIBTUV_PATH)/include:
	git submodule update --init $(LIBTUV_PATH)

$(LIBTUV_OBJS):
	$(MAKE) -C $(LIBTUV_PATH) TUV_BUILD_TYPE=release TUV_CREATE_PIC_LIB=yes TUV_PLATFORM=$(LIBTUV_PLATFORM) TUV_BOARD=$(LIBTUV_BOARD)

# librabbitmq related variables
LIBRABBITMQ_PATH = deps/rabbitmq-c
LIBRABBITMQ_INCLUDE := -I $(LIBRABBITMQ_PATH)/build/include
LIBRABBITMQ_OBJS := $(LIBRABBITMQ_PATH)/build/librabbitmq/CMakeFiles/rabbitmq.dir/*.o
ifeq ($(UNAME_S),darwin)
    OPENSSL_PATH := /usr/local/opt/openssl
    LDFLAGS += -L$(OPENSSL_PATH)/lib -lcrypto -lssl
endif

$(LIBRABBITMQ_PATH)/librabbitmq:
	git submodule update --init $(LIBRABBITMQ_PATH)

$(LIBRABBITMQ_OBJS):
	mkdir -p $(LIBRABBITMQ_PATH)/build
ifeq ($(UNAME_S),darwin)
	# macOS
	cd $(LIBRABBITMQ_PATH)/build && \
         cmake -DOPENSSL_ROOT_DIR=$(OPENSSL_PATH) -DCMAKE_INSTALL_PREFIX=. .. && \
         cmake --build . --target install
else
	cd $(LIBRABBITMQ_PATH)/build && \
         cmake -DCMAKE_INSTALL_PREFIX=. .. && \
         cmake --build . --target install
endif
	cd $(LIBRABBITMQ_PATH)/build && \
         cmake --build .

# Submodules
SUBS := $(LIBTUV_PATH)/include
ifeq ($(BUILD_REMOTE),1)
    SUBS += $(LIBRABBITMQ_PATH)/librabbitmq
endif
# Submodule related objects
SUB_OBJS := $(LIBTUV_OBJS)
ifeq ($(BUILD_REMOTE),1)
    SUB_OBJS += $(LIBRABBITMQ_OBJS)
endif
# Submodule C flags for including header files
SUB_INCLUDE := $(LIBTUV_INCLUDE)
ifeq ($(BUILD_REMOTE),1)
    SUB_INCLUDE += $(LIBRABBITMQ_INCLUDE)
endif
