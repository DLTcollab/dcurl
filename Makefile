OPENCL_LIB ?= /usr/local/cuda-9.1/lib64
OPENJDK_PATH ?= /usr/lib/jvm/java-1.8.0-openjdk-amd64
SRC ?= ./src
OUT ?= ./build

CFLAGS = -Os -fPIC -g
LDFLAGS = -L$(OPENCL_LIB) -lpthread -lOpenCL

TESTS = \
	trinary \
	curl \
	pow_sse \
	pow_cl
TESTS := $(addprefix $(OUT)/test_, $(TESTS))

LIBS = libdcurl.so
LIBS := $(addprefix $(OUT)/, $(LIBS))

all: $(TESTS) $(LIBS)

OBJS = \
	hash/curl.o \
	constants.o \
	trinary/trinary.o \
	dcurl.o \
	pow_sse.o \
	pow_cl.o \
	clcontext.o
OBJS := $(addprefix $(OUT)/, $(OBJS))
deps := $(OBJS:%.o=%.o.d)

# Control the build verbosity
ifeq ("$(VERBOSE)","1")
    Q :=
    VECHO = @true
else
    Q := @
    VECHO = @printf
endif

SUBDIRS = \
	hash \
	trinary
SHELL_HACK := $(shell mkdir -p $(OUT))
SHELL_HACK := $(shell mkdir -p $(addprefix $(OUT)/,$(SUBDIRS)))

$(OUT)/test_%.o: test/test_%.c
	$(VECHO) "  CC\t$@\n"
	$(Q)$(CC) -o $@ $(CFLAGS) -c -MMD -MF $@.d $<

$(OUT)/trinary/%.o: $(SRC)/trinary/%.c
$(OUT)/hash/%.o: $(SRC)/hash/%.c
$(OUT)/%.o: $(SRC)/%.c
	$(VECHO) "  CC\t$@\n"
	$(Q)$(CC) -o $@ $(CFLAGS) -c -MMD -MF $@.d $<

$(OUT)/test_%: $(OUT)/test_%.o $(OBJS)
	$(VECHO) "  LD\t$@\n"
	$(Q) gcc -I$(OPENCL_PATH)/include -o $@ $^ $(LDFLAGS)

$(OUT)/libdcurl.so: ./jni/iri-pearldiver-exlib.c $(OBJS)
	gcc -fPIC -msse2 -shared \
	    -I$(OPENJDK_PATH)/include -I$(OPENJDK_PATH)/include/linux \
	    -I$(OPENCL_PATH)/include -o $@ $^ $(LDFLAGS)

PASS_COLOR = \e[32;01m
NO_COLOR = \e[0m

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
	PRINTF = printf
else
	PRINTF = env printf
endif

$(OUT)/test_%.done: $(OUT)/test_%
	$(Q)./$< && $(PRINTF) "*** $< *** $(PASS_COLOR)[ Verified ]$(NO_COLOR)\n"
check: $(addsuffix .done, $(TESTS))

clean:
	$(RM) -r $(OUT)

-include $(deps)
