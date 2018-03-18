OPENCL_LIB ?= /usr/local/cuda-9.1/lib64
OPENJDK_PATH ?= $(shell readlink -f /usr/bin/javac | sed "s:bin/javac::")
SRC ?= ./src
OUT ?= ./build
DISABLE_GPU ?= 1
DISABLE_JNI ?= 1

CFLAGS = -Os -fPIC -g
LDFLAGS = -lpthread
PYFLAGS = --gpu_enable

# FIXME: avoid hardcoded architecture flags. We might support advanced SIMD
# instructions for Intel and Arm later.
CFLAGS += -msse2

ifneq ("$(DISABLE_GPU)","1")
CFLAGS += \
	-DENABLE_OPENCL \
	-I$(OPENCL_PATH)/include
LDFLAGS += -L$(OPENCL_LIB) -lOpenCL
PYFLAGS += 1
else
PYFLAGS += 0
endif

ifneq ("$(DISABLE_JNI)","1")
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
JAVA_HOME := $(shell /usr/libexec/java_home)
CFLAGS_JNI = \
	-I$(JAVA_HOME)/include \
	-I$(JAVA_HOME)/include/darwin
else
# Default to Linux
CFLAGS_JNI = \
	-I$(OPENJDK_PATH)/include \
	-I$(OPENJDK_PATH)/include/linux
endif
endif

TESTS = \
	trinary \
	curl \
	pow_sse \
	multi_pow

ifneq ("$(DISABLE_GPU)","1")
TESTS += \
	pow_cl
endif

TESTS := $(addprefix $(OUT)/test-, $(TESTS))

LIBS = libdcurl.so
LIBS := $(addprefix $(OUT)/, $(LIBS))

all: $(TESTS) $(LIBS)

OBJS = \
	curl.o \
	constants.o \
	trinary.o \
	dcurl.o \
	pow_sse.o

ifneq ("$(DISABLE_GPU)","1")
OBJS += \
	pow_cl.o \
	clcontext.o
endif

ifneq ("$(DISABLE_JNI)","1")
OBJS += \
	jni/iri-pearldiver-exlib.o
endif

OBJS := $(addprefix $(OUT)/, $(OBJS))
deps := $(OBJS:%.o=%.o.d)

SHELL_HACK := $(shell mkdir -p $(OUT))
SHELL_HACK := $(shell mkdir -p $(addprefix $(OUT)/,jni))

$(OUT)/test-%.o: tests/test-%.c
	$(VECHO) "  CC\t$@\n"
	$(Q)$(CC) -o $@ $(CFLAGS) -I src -c -MMD -MF $@.d $<

$(OUT)/jni/%.o: jni/%.c
	$(VECHO) "  CC\t$@\n"
	$(Q)$(CC) -o $@ $(CFLAGS) $(CFLAGS_JNI) -c -MMD -MF $@.d $<

$(OUT)/%.o: $(SRC)/%.c
	$(VECHO) "  CC\t$@\n"
	$(Q)$(CC) -o $@ $(CFLAGS) -c -MMD -MF $@.d $<

$(OUT)/test-%: $(OUT)/test-%.o $(OBJS)
	$(VECHO) "  LD\t$@\n"
	$(Q)$(CC) -o $@ $^ $(LDFLAGS)

$(OUT)/libdcurl.so: $(OBJS)
	$(VECHO) "  LD\t$@\n"
	$(Q)$(CC) -shared -o $@ $^ $(LDFLAGS)

# FIXME: script "tests/test-multi_pow.py" depends on PyIOTA package, and we
# have to check in advance, otherwise python3 would complain as following:
#     ModuleNotFoundError: No module named 'iota'
$(OUT)/test-multi_pow: tests/test-multi_pow.py $(OUT)/libdcurl.so
	@echo "#!/usr/bin/env python3" > $@
	@cat $< >> $@
	@chmod +x $@

$(OUT)/test-%.done: $(OUT)/test-%
	$(Q)$(PRINTF) "*** Validating $< ***\n"
	$(Q)./$< && $(PRINTF) "\t$(PASS_COLOR)[ Verified ]$(NO_COLOR)\n"
check: $(addsuffix .done, $(TESTS))

clean:
	$(RM) -r $(OUT)

include mk/common.mk
-include $(deps)
