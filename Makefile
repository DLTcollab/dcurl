OUT ?= ./build
SRC := src

CFLAGS = -Wall -fPIC -std=gnu99
LDFLAGS = \
	-lm \
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

ifneq ("$(BUILD_DEBUG)","0")
    CFLAGS += -Og -g3 -DENABLE_DEBUG
    ifneq ("$(BUILD_DEBUG)","1")
        include mk/sanitizers.mk
    endif
else
    # Enable all the valid optimizations for standard programs in release build
    CFLAGS += -O3
endif

# Check specific CPU features available on build host
include mk/cpu-features.mk

# Handle git submodule
include mk/submodule.mk

# Assign the hardware to CPU if no hardware is specified
PLATFORMS := $(BUILD_AVX) $(BUILD_SSE) $(BUILD_GENERIC) $(BUILD_GPU) $(BUILD_FPGA_ACCEL)
ENABLE_PLATFORMS := $(findstring 1,$(PLATFORMS))
ifneq ("$(ENABLE_PLATFORMS)","1")
    ifeq ("$(call cpu_feature,AVX)","1")
        BUILD_AVX = 1
    else ifeq ("$(call cpu_feature,SSE)","1")
        BUILD_SSE = 1
    else
        BUILD_GENERIC = 1
    endif
endif


ifeq ("$(BUILD_AVX)","1")
CFLAGS += -mavx -DENABLE_AVX
ifeq ("$(call cpu_feature,AVX2)","1")
CFLAGS += -mavx2
endif
else ifeq ("$(BUILD_SSE)","1")
CFLAGS += -msse2 -DENABLE_SSE
else ifeq ("$(BUILD_GENERIC)","1")
CFLAGS += -DENABLE_GENERIC
endif

ifeq ("$(call cpu_feature,SSE4_2)","1")
    CFLAGS += -msse4.2
endif

ifeq ("$(BUILD_GPU)","1")
include mk/opencl.mk
endif

ifeq ("$(BUILD_FPGA_ACCEL)","1")
CFLAGS += -DENABLE_FPGA_ACCEL
endif

ifeq ("$(BUILD_JNI)","1")
include mk/java.mk
endif

ifeq ("$(BUILD_STAT)","1")
CFLAGS += -DENABLE_STAT
endif

TESTS = \
	trinary \
	curl \
	dcurl \
	pow

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
	implcontext.o \
	common.o

ifeq ("$(BUILD_AVX)","1")
OBJS += pow_avx.o
else ifeq ("$(BUILD_SSE)","1")
OBJS += pow_sse.o
else ifeq ("$(BUILD_GENERIC)","1")
OBJS += pow_c.o
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

$(OUT)/test-%.o: tests/test-%.c $(LIBTUV_PATH)/include
	$(VECHO) "  CC\t$@\n"
	$(Q)$(CC) -o $@ $(CFLAGS) -I $(SRC) $(LIBTUV_INCLUDE) -c -MMD -MF $@.d $<

$(OUT)/%.o: $(SRC)/%.c $(LIBTUV_PATH)/include
	$(VECHO) "  CC\t$@\n"
	$(Q)$(CC) -o $@ $(CFLAGS) $(LIBTUV_INCLUDE) -c -MMD -MF $@.d $<

$(OUT)/test-%: $(OUT)/test-%.o $(OBJS) $(LIBTUV_LIBRARY)
	$(VECHO) "  LD\t$@\n"
	$(Q)$(CC) -o $@ $^ $(LDFLAGS)

$(OUT)/libdcurl.so: $(OBJS) $(LIBTUV_LIBRARY)
	$(VECHO) "  LD\t$@\n"
	$(Q)$(CC) -shared -o $@ $^ $(LDFLAGS)

include mk/common.mk

-include $(deps)
