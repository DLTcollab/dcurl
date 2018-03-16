OPENCL_LIB ?= /usr/local/cuda-9.1/lib64
OPENJDK_PATH ?= /usr/lib/jvm/java-1.8.0-openjdk-amd64
SRC ?= ./src
OUT ?= ./build

CFLAGS = -Os -fPIC -g
LDFLAGS = -lpthread

# FIXME: avoid hardcoded architecture flags. We might support advanced SIMD
# instructions for Intel and Arm later.
CFLAGS += -msse2

ifneq ("$(DISABLE_GPU)","1")
CFLAGS += \
	-DBUILD_OPENCL \
	-I$(OPENCL_PATH)/include
LDFLAGS += -L$(OPENCL_LIB) -lOpenCL
endif

ifneq ("$(DISABLE_JNI)","1")
CFLAGS += \
	-I$(OPENJDK_PATH)/include \
	-I$(OPENJDK_PATH)/include/linux
endif

TESTS = \
	trinary \
	curl \
	pow_sse

ifneq ("$(DISABLE_GPU)","1")
TESTS += \
	pow_cl
endif

TESTS := $(addprefix $(OUT)/test_, $(TESTS))

LIBS = libdcurl.so
LIBS := $(addprefix $(OUT)/, $(LIBS))

all: $(TESTS) $(LIBS)

OBJS = \
	hash/curl.o \
	constants.o \
	trinary/trinary.o \
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

SUBDIRS = \
	hash \
	trinary \
	jni
SHELL_HACK := $(shell mkdir -p $(OUT))
SHELL_HACK := $(shell mkdir -p $(addprefix $(OUT)/,$(SUBDIRS)))

$(OUT)/test_%.o: test/test_%.c
	$(VECHO) "  CC\t$@\n"
	$(Q)$(CC) -o $@ $(CFLAGS) -c -MMD -MF $@.d $<

$(OUT)/jni/%.o: jni/%.c
$(OUT)/trinary/%.o: $(SRC)/trinary/%.c
$(OUT)/hash/%.o: $(SRC)/hash/%.c
$(OUT)/%.o: $(SRC)/%.c
	$(VECHO) "  CC\t$@\n"
	$(Q)$(CC) -o $@ $(CFLAGS) -c -MMD -MF $@.d $<

$(OUT)/test_%: $(OUT)/test_%.o $(OBJS)
	$(VECHO) "  LD\t$@\n"
	$(Q)$(CC) -o $@ $^ $(LDFLAGS)

$(OUT)/libdcurl.so: $(OBJS)
	$(VECHO) "  LD\t$@\n"
	$(Q)$(CC) -shared -o $@ $^ $(LDFLAGS)

$(OUT)/test_%.done: $(OUT)/test_%
	$(Q)$(PRINTF) "*** Validating $< ***\n"
	$(Q)./$< && $(PRINTF) "\t$(PASS_COLOR)[ Verified ]$(NO_COLOR)\n"
check: $(addsuffix .done, $(TESTS))

clean:
	$(RM) -r $(OUT)

include mk/common.mk
-include $(deps)
