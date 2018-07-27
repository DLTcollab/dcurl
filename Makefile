OUT ?= ./build
SRC := src

CFLAGS = -Wall -fPIC
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

SSE_S := $(shell grep -o sse /proc/cpuinfo | head -n 1)

# FIXME: avoid hardcoded architecture flags. We might support advanced SIMD
# instructions for Intel and Arm later.
ifeq ("$(BUILD_AVX)","1")
CFLAGS += -mavx -mavx2 -DENABLE_AVX
else
ifeq ("$(BUILD_SSE)"_$(SSE_S),"1"_sse)
CFLAGS += -msse2 -DENABLE_SSE
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
	curl

ifeq ("$(BUILD_AVX)","1")
TESTS += \
	pow_avx \
	multi_pow_cpu
else
ifeq ("$(BUILD_SSE)"_$(SSE_S),"1"_sse)
TESTS += \
	pow_sse \
	multi_pow_cpu
else
TESTS += \
	pow_c \
	multi_pow_cpu
endif
endif

ifeq ("$(BUILD_GPU)","1")
TESTS += \
	pow_cl \
	multi_pow_gpu
endif

ifeq ("$(BUILD_COMPAT)", "1")
TESTS += ccurl-multi_pow
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
	dcurl.o

ifeq ("$(BUILD_AVX)","1")
OBJS += pow_avx.o
else
ifeq ("$(BUILD_SSE)"_$(SSE_S),"1"_sse)
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
