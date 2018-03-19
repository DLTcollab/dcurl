SRC ?= ./src
OUT ?= ./build
DISABLE_GPU ?= 1
DISABLE_JNI ?= 1

CFLAGS = -Os -fPIC -g
LDFLAGS = -lpthread

# FIXME: avoid hardcoded architecture flags. We might support advanced SIMD
# instructions for Intel and Arm later.
CFLAGS += -msse2

ifneq ("$(DISABLE_GPU)","1")
include mk/opencl.mk
endif

ifneq ("$(DISABLE_JNI)","1")
include mk/java.mk
endif

TESTS = \
	trinary \
	curl \
	pow_sse \
	multi_pow_cpu

ifneq ("$(DISABLE_GPU)","1")
TESTS += \
	pow_cl \
	multi_pow_gpu
endif

TESTS := $(addprefix $(OUT)/test-, $(TESTS))

LIBS = libdcurl.so
LIBS := $(addprefix $(OUT)/, $(LIBS))

all: $(TESTS) $(LIBS)
.DEFAULT_GOAL := all

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
$(OUT)/test-multi_pow_%: tests/test-multi_pow_%.py $(OUT)/libdcurl.so
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
