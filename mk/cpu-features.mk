UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
# macOS
FEATURES := sysctl machdep.cpu.features
else
# Linux
FEATURES := cat /proc/cpuinfo
endif

cpu_feature = $(shell $(FEATURES) | grep -oi $1 >/dev/null && echo 1 || echo 0)
