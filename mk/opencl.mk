# FIXME: perform sanity checks before compilation
OPENCL_LIB ?= /usr/local/cuda-9.1/lib64

CFLAGS += \
        -DENABLE_OPENCL \
        -I$(OPENCL_PATH)/include
LDFLAGS += -L$(OPENCL_LIB) -lOpenCL
