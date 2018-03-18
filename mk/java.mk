JDK_PATH ?= $(shell readlink -f /usr/bin/javac | sed "s:bin/javac::")

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
