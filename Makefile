VERSION = 0.6.0

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
        include mk/dynamic-analysis.mk
    endif
else
    # Enable all the valid optimizations for standard programs in release build
    CFLAGS += -O3
endif

# Static code analysis
include mk/static-analysis.mk

# Check specific CPU features available on build host
include mk/cpu-features.mk

# Handle git submodule
include mk/submodule.mk

# Board specific compiler flags
include mk/board.mk

# Generate document
include mk/docgen.mk

# Assign the hardware to CPU if no hardware is specified
PLATFORMS := $(BUILD_AVX) $(BUILD_SSE) $(BUILD_GENERIC) $(BUILD_GPU) $(BUILD_FPGA)
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

ifeq ("$(BUILD_FPGA)","1")
CFLAGS += -DENABLE_FPGA
endif

ifeq ("$(BUILD_REMOTE)","1")
CFLAGS += -DENABLE_REMOTE
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
	pow \
	multi-pow

TESTS := $(addprefix $(OUT)/test-, $(TESTS))

LIBS = libdcurl.so
LIBS := $(addprefix $(OUT)/, $(LIBS))

JARS := dcurljni-$(VERSION).jar
JARS := $(addprefix $(OUT)/, $(JARS))

PREQ := $(SUBS) config $(TESTS) $(LIBS)
ifeq ("$(BUILD_JNI)","1")
PREQ += $(JARS)
endif
ifeq ("$(BUILD_REMOTE)", "1")
PREQ += $(OUT)/remote-worker
endif

all: $(PREQ)
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
	jni/iri_pearldiver_exlib.o
endif

ifeq ("$(BUILD_COMPAT)", "1")
OBJS += \
	compat_ccurl.o
endif

ifeq ("$(BUILD_FPGA)","1")
OBJS += \
	pow_fpga.o
endif

ifeq ("$(BUILD_REMOTE)", "1")
OBJS += \
	remote_common.o \
	remote_interface.o

WORKER_EXCLUDE_OBJS := remote_interface.o
ifeq ("$(BUILD_JNI)", "1")
WORKER_EXCLUDE_OBJS += jni/iri_pearldiver_exlib.o
endif
WORKER_OBJS := $(addprefix $(OUT)/worker-,$(filter-out $(WORKER_EXCLUDE_OBJS), $(OBJS)))
WORKER_CFLAGS := $(filter-out -DENABLE_REMOTE, $(CFLAGS))
endif

OBJS := $(addprefix $(OUT)/, $(OBJS))

$(OUT)/test-%.o: tests/test-%.c
	$(VECHO) "  CC\t$@\n"
	$(Q)$(CC) -o $@ $(CFLAGS) -I $(SRC) $(SUB_INCLUDE) -c -MMD -MF $@.d $<

$(OUT)/%.o: $(SRC)/%.c $(SUB_OBJS)
	$(VECHO) "  CC\t$@\n"
	$(Q)$(CC) -o $@ $(CFLAGS) $(SUB_INCLUDE) -c -MMD -MF $@.d $<

$(OUT)/test-%: $(OUT)/test-%.o $(OBJS) $(SUB_OBJS)
	$(VECHO) "  LD\t$@\n"
	$(Q)$(CC) -o $@ $^ $(LDFLAGS)

$(OUT)/libdcurl.so: $(OBJS) $(SUB_OBJS)
	$(VECHO) "  LD\t$@\n"
	$(Q)$(CC) -shared -o $@ $^ $(LDFLAGS)

ifeq ("$(BUILD_REMOTE)", "1")
include mk/remote.mk
endif

include mk/common.mk

-include $(deps)
