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

MANUNAL_TESTS = \
	remote-new-task \
	remote-get-result

MANUNAL_TESTS := $(addprefix $(OUT)/manunal-test-, $(MANUNAL_TESTS))

LIBS = libdcurl.so
LIBS := $(addprefix $(OUT)/, $(LIBS))

REMOTEDCURL := $(OUT)/remotedcurl

all: config $(TESTS) $(MANUNAL_TESTS) $(LIBS) $(REMOTEDCURL)
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

# Add the libtuv PIC(position independent code) library into the object files
# if the specified hardware is CPU
CPU_PLATFORMS := $(BUILD_AVX) $(BUILD_SSE) $(BUILD_GENERIC)
ENABLE_CPU_PLATFORMS := $(findstring 1,$(CPU_PLATFORMS))
ifeq ("$(ENABLE_CPU_PLATFORMS)","1")
    OBJS += $(LIBTUV_LIBRARY)
endif

# Link the librabbitmq library
LDFLAGS += -L$(LIBRABBITMQ_DIR) -lrabbitmq

$(OUT)/test-%.o: tests/test-%.c $(LIBTUV_PATH)/include
	$(VECHO) "  CC\t$@\n"
	$(Q)$(CC) -o $@ $(CFLAGS) -I $(SRC) $(LIBTUV_INCLUDE) -c -MMD -MF $@.d $<

$(OUT)/%.o: $(SRC)/%.c $(LIBTUV_PATH)/include
	$(VECHO) "  CC\t$@\n"
	$(Q)$(CC) -o $@ $(CFLAGS) $(LIBTUV_INCLUDE) -c -MMD -MF $@.d $<

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

$(OUT)/manunal-test-%.o: tests/manunal-test-%.c $(LIBRABBITMQ_PATH)/build/include
	$(VECHO) "  CC\t$@\n"
	$(Q)$(CC) -o $@ $(CFLAGS) -I $(SRC) $(LIBRABBITMQ_INCLUDE) -c $<

$(OUT)/manunal-test-%: $(OUT)/manunal-test-%.o $(OBJS)
	$(VECHO) "  LD\t$@\n"
	$(Q)$(CC) -o $@ $^ $(LDFLAGS)

$(OUT)/%.o: remotedcurl/%.c
	$(VECHO) "  CC\t$@\n"
	$(Q)$(CC) -o $@ $(CFLAGS) -I $(SRC) $(LIBRABBITMQ_INCLUDE) -c -MMD -MF $@.d $<

$(OUT)/remotedcurl: $(OUT)/remotedcurl.o $(OUT)/libdcurl.so
	$(VECHO) "  CC\t$@\n"
	$(Q)$(CC) -o $@ $^ $(LDFLAGS)

include mk/common.mk

-include $(deps)
