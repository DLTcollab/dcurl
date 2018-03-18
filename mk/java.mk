JDK_PATH ?= $(shell readlink -f /usr/bin/javac | sed "s:bin/javac::")

JAVAH := $(shell which javah)
ifndef JAVAH
$(error "javah is not available. Please check JDK installation")
endif

CURL := $(shell which curl)
ifndef CURL
$(error "curl is not available.")
endif

SHELL_HACK := $(shell mkdir -p $(OUT)/com/iota/iri/hash)

GITHUB_REPO ?= chenwei-tw/iri/feat/new_pow_interface
PearlDriverSRC := src/main/java/com/iota/iri/hash/PearlDiver.java

$(OUT)/com/iota/iri/hash/PearlDiver.java: $(OUT)/com/iota/iri/hash
	$(VECHO) "  CURL\t$@\n"
	$(Q)$(CURL) -s -o $@ \
	"https://raw.githubusercontent.com/$(GITHUB_REPO)/$(PearlDriverSRC)"

$(OUT)/jni/iri-pearldiver-exlib.h: $(OUT)/com/iota/iri/hash/PearlDiver.java
	$(VECHO) "  JAVAH\t$@\n"
	$(Q)$(JAVAH) -classpath $(OUT) -o $@ com.iota.iri.hash.PearlDiver

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
JAVA_HOME := $(shell /usr/libexec/java_home)
CFLAGS_JNI = \
	-I$(JAVA_HOME)/include \
	-I$(JAVA_HOME)/include/darwin
else
# Default to Linux
CFLAGS_JNI = \
	-I$(JDK_PATH)/include \
	-I$(JDK_PATH)/include/linux
endif

CFLAGS_JNI += -I$(OUT)/jni

jni/iri-pearldiver-exlib.c: $(OUT)/jni/iri-pearldiver-exlib.h

$(OUT)/jni/%.o: jni/%.c
	$(VECHO) "  CC\t$@\n"
	$(Q)$(CC) -o $@ $(CFLAGS) $(CFLAGS_JNI) -c -MMD -MF $@.d $<
