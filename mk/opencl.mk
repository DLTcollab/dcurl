# Default the path to the NVIDIA Run time Library
OPENCL_LIB_PATH ?= /usr/local/cuda/lib64
OPENCL_LIB_DIR := $(shell test -d $(OPENCL_LIB_PATH); echo $$?)

ifneq ($(OPENCL_LIB_DIR),0)
$(error "Please specify the path to OpenCL library.")
endif

OPENCL_LIB_AVAIL := $(shell ls $(OPENCL_LIB_PATH) | grep -oi libOpenCL.so > /dev/null; echo $$?)

ifneq ($(OPENCL_LIB_AVAIL),0)
$(error "Please check installation of OpenCL dynamic library in $(OPENCL_LIB_PATH)")
endif

CFLAGS += -DENABLE_OPENCL
LDFLAGS += -L$(OPENCL_LIB_PATH) -lOpenCL
