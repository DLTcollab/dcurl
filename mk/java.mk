UNAME_S := $(shell uname -s)

# if JAVA_HOME is not set, guess it according to system configurations
ifndef JAVA_HOME
ifeq ($(UNAME_S),Darwin)
    # macOS
    JAVA_HOME := $(shell /usr/libexec/java_home)
else
# Default to Linux
    JAVAC := $(shell which javac)
    ifndef JAVAC
    $(error "javac is not available. Please check JDK installation")
    endif
    JAVA_HOME := $(shell readlink -f $(JAVAC) | sed "s:bin/javac::")
endif
endif # JAVA_HOME

JAVAH = javah
ifdef JAVA_HOME
JAVAH = $(JAVA_HOME)/bin/javah
endif
JAVAH := $(shell which $(JAVAH))
ifndef JAVAH
$(error "javah is not available. Please check JDK installation")
endif

CURL := $(shell which curl)
ifndef CURL
$(error "curl is not available.")
endif

SHELL_HACK := $(shell mkdir -p $(addprefix $(OUT)/,jni))
SHELL_HACK := $(shell mkdir -p $(OUT)/com/iota/iri/crypto)

GITHUB_REPO ?= DLTcollab/iri/dev-rebase
PearlDriverSRC := src/main/java/com/iota/iri/crypto/PearlDiver.java

$(OUT)/com/iota/iri/crypto/PearlDiver.java: $(OUT)/com/iota/iri/crypto
	$(VECHO) "  CURL\t$@\n"
	$(Q)$(CURL) -s -o $@ \
	"https://raw.githubusercontent.com/$(GITHUB_REPO)/$(PearlDriverSRC)"

$(OUT)/jni/iri-pearldiver-exlib.h: $(OUT)/com/iota/iri/crypto/PearlDiver.java
	$(VECHO) "  JAVAH\t$@\n"
	$(Q)$(JAVAH) -classpath $(OUT) -o $@ com.iota.iri.crypto.PearlDiver

ifeq ($(UNAME_S),Darwin)
    # macOS
    CFLAGS_JNI := -I$(JAVA_HOME)/include/darwin
else
    # Default to Linux
    CFLAGS_JNI := -I$(JAVA_HOME)/include/linux
endif

CFLAGS_JNI += -I$(JAVA_HOME)/include
CFLAGS_JNI += -I$(OUT)/jni

jni/iri-pearldiver-exlib.c: $(OUT)/jni/iri-pearldiver-exlib.h

$(OUT)/jni/%.o: jni/%.c
	$(VECHO) "  CC\t$@\n"
	$(Q)$(CC) -o $@ $(CFLAGS) $(CFLAGS_JNI) -c -MMD -MF $@.d $<
