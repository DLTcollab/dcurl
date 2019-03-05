SHELL = /bin/bash

# Check whether the GCC version is higher than the minimum required version including Sanitizers
# TODO: The ARM platform should be handled as well
SANITIZER_MIN_GCC_VER = 4.9
GCC_VER := $(shell $(CC) -dumpfullversion)
# Get the lower version of the current GCC version and the minimum required version
LOWER_VER := $(firstword $(shell echo -e "$(SANITIZER_MIN_GCC_VER)\n$(GCC_VER)" | sort -V))

# Check whether the specified Sanitizer is existed or not
SANITIZER_LIST = address undefined thread
# If it is existed, return the name of the Sanitizer
# Otherwise, return empty string
SANITIZER := $(filter $(SANITIZER_LIST),$(BUILD_DEBUG))

ifeq ("$(LOWER_VER)","$(SANITIZER_MIN_GCC_VER)")
    ifneq ("$(SANITIZER)","")
        CFLAGS += -fsanitize=$(SANITIZER)
        LDFLAGS += -fsanitize=$(SANITIZER)
    endif
endif
