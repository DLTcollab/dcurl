# FIXME: perform sanity checks before compilation
OPENCL_LIB ?= /usr/local/cuda/lib64

CFLAGS += \
        -DENABLE_OPENCL \
        -I$(OPENCL_PATH)/include
LDFLAGS += -L$(OPENCL_LIB) -lOpenCL
