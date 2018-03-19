# Control the build verbosity
ifeq ("$(VERBOSE)","1")
    Q :=
    VECHO = @true
else
    Q := @
    VECHO = @printf
endif

PASS_COLOR = \e[32;01m
NO_COLOR = \e[0m

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
	PRINTF = printf
else
	PRINTF = env printf
endif

config: $(OUT)/config-timestamp

$(OUT)/config-timestamp: $(OUT)/local.mk
	$(Q)touch $@
$(OUT)/local.mk:
	$(Q)cp -f mk/defs.mk $@
