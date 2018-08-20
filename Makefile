OUT ?= ./build
SRC := src

CFLAGS = -Wall -fPIC -std=c99
LDFLAGS = \
	-lpthread

UNAME_S := $(shell uname -s)
ifneq ($(UNAME_S),Darwin)
# GNU ld specific options
LDFLAGS += \
	-Wl,--gc-sections \
	-Wl,--as-needed \
	-Wl,--version-script=./mk/libdcurl.version
endif

-include $(OUT)/local.mk

ifeq ("$(BUILD_DEBUG)","1")
CFLAGS += -Og -g
else
# Enable all the optimizations in release build
CFLAGS += -Ofast
endif

# Check specific CPU features available on build host
include mk/cpu-features.mk

ifeq ("$(BUILD_AVX)","1")
CFLAGS += -mavx -mavx2 -DENABLE_AVX
else
BUILD_SSE := $(call cpu_feature,SSE)
ifeq ("$(BUILD_SSE)","1")
CFLAGS += -msse2 -DENABLE_SSE
endif
endif
endif

ifeq ("$(BUILD_GPU)","1")
include mk/opencl.mk
endif

ifeq ("$(BUILD_JNI)","1")
include mk/java.mk
endif

TESTS = \
	trinary \
	curl \
	dcurl \
	multi-pow

ifeq ("$(BUILD_AVX)","1")
TESTS += \
	pow_avx
else
ifeq ("$(BUILD_SSE)","1")
TESTS += \
	pow_sse
else
TESTS += \
	pow_c
endif
endif

ifeq ("$(BUILD_GPU)","1")
TESTS += \
	pow_cl
endif

ifeq ("$(BUILD_COMPAT)", "1")
TESTS += ccurl-multi_pow
endif

ifeq ("$(BUILD_FPGA_ACCEL)","1")
TESTS += pow_fpga_accel
endif

TESTS := $(addprefix $(OUT)/test-, $(TESTS))

LIBS = libdcurl.so
LIBS := $(addprefix $(OUT)/, $(LIBS))

all: config $(TESTS) $(LIBS)
.DEFAULT_GOAL := all

OBJS = \
	curl.o \
	constants.o \
	trinary.o \
	dcurl.o \
	implcontext.o

ifeq ("$(BUILD_AVX)","1")
OBJS += pow_avx.o
else
ifeq ("$(BUILD_SSE)","1")
OBJS += pow_sse.o
else
OBJS += pow_c.o
endif
endif

ifeq ("$(BUILD_GPU)","1")
OBJS += \
	pow_cl.o \
	clcontext.o
endif

ifeq ("$(BUILD_JNI)","1")
OBJS += \
	jni/iri-pearldiver-exlib.o
endif

ifeq ("$(BUILD_COMPAT)", "1")
OBJS += \
	compat-ccurl.o
endif

ifeq ("$(BUILD_FPGA_ACCEL)","1")
OBJS += \
	pow_fpga_accel.o
endif

OBJS := $(addprefix $(OUT)/, $(OBJS))

$(OUT)/test-%.o: tests/test-%.c
	$(VECHO) "  CC\t$@\n"
	$(Q)$(CC) -o $@ $(CFLAGS) -I $(SRC) -c -MMD -MF $@.d $<

$(OUT)/%.o: $(SRC)/%.c
	$(VECHO) "  CC\t$@\n"
	$(Q)$(CC) -o $@ $(CFLAGS) -c -MMD -MF $@.d $<

$(OUT)/test-%: $(OUT)/test-%.o $(OBJS)
	$(VECHO) "  LD\t$@\n"
	$(Q)$(CC) -o $@ $^ $(LDFLAGS)

$(OUT)/libdcurl.so: $(OBJS)
	$(VECHO) "  LD\t$@\n"
	$(Q)$(CC) -shared -o $@ $^ $(LDFLAGS)

$(OUT)/test-%: tests/test-%.py $(OUT)/libdcurl.so
	$(Q)echo "#!$(PYTHON)" > $@
	$(call py_prepare_cmd)
	$(Q)chmod +x $@

include mk/common.mk
include mk/python.mk

-include $(deps)
