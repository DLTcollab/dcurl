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

# libtuv related variables
LIBTUV_PATH = deps/libtuv
LIBTUV_INCLUDE := -I $(LIBTUV_PATH)/include
LIBTUV_PLATFORM := $(UNAME_M)-$(UNAME_S)
# PIC (Position-Independent-Code) library
LIBTUV_LIBRARY := $(LIBTUV_PATH)/build/$(LIBTUV_PLATFORM)/release/lib/libtuv.o

$(LIBTUV_PATH)/include:
	git submodule update --init $(LIBTUV_PATH)

$(LIBTUV_LIBRARY):
	$(MAKE) -C $(LIBTUV_PATH) TUV_BUILD_TYPE=release TUV_CREATE_PIC_LIB=yes

# librabbitmq related variables
LIBRABBITMQ_PATH = deps/rabbitmq-c
LIBRABBITMQ_INCLUDE := -I $(LIBRABBITMQ_PATH)/build/include
LIBRABBITMQ_LIBRARY := $(LIBRABBITMQ_PATH)/build/librabbitmq/librabbitmq.a

$(LIBRABBITMQ_PATH)/build/include:
	git submodule update --init $(LIBRABBITMQ_PATH)
	mkdir $(LIBRABBITMQ_PATH)/build
	cd $(LIBRABBITMQ_PATH)/build && \
         cmake -DCMAKE_INSTALL_PREFIX=. .. && \
         cmake --build . --target install

$(LIBRABBITMQ_LIBRARY): $(LIBRABBITMQ_PATH)/build/include
