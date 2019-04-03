# The auto-detected path of the OpenCL library
OPENCL_LIB_PATH := $(shell pkg-config --variable=libdir OpenCL 2> /dev/null)
OPENCL_LIB_AVAIL := $(shell test -e $(OPENCL_LIB_PATH)/libOpenCL.so; echo $$?)
UNAME_S := $(shell uname -s)

ifneq ($(OPENCL_LIB_AVAIL),0)
$(error "Please install the OpenCL library correctly.")
endif

CFLAGS += -DENABLE_OPENCL
ifeq ($(UNAME_S),Darwin)
    # macOS
    LDFLAGS += -F$(OPENCL_LIB_PATH) -framework OpenCL
else
    # Default to Linux
    LDFLAGS += -L$(OPENCL_LIB_PATH) -lOpenCL
endif
