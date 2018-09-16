# Always ensure directory build exist
SHELL_HACK := $(shell mkdir -p $(OUT))

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    PRINTF = printf
else
    PRINTF = env printf
endif

# Control the build verbosity
ifeq ("$(VERBOSE)","1")
    Q :=
    VECHO = @true
else
    Q := @
    VECHO = @$(PRINTF)
endif

# dependency of source files
deps := $(OBJS:%.o=%.o.d)

# Test suite
PASS_COLOR = \e[32;01m
NO_COLOR = \e[0m
$(OUT)/test-%.done: $(OUT)/test-%
	$(Q)$(PRINTF) "*** Validating $< ***\n"
	$(Q)./$< && $(PRINTF) "\t$(PASS_COLOR)[ Verified ]$(NO_COLOR)\n"
check: $(addsuffix .done, $(TESTS))

config: $(OUT)/config-timestamp

$(OUT)/config-timestamp: $(OUT)/local.mk
	$(Q)touch $@
$(OUT)/local.mk:
	$(Q)cp -f mk/defs.mk $@

clean:
	$(RM) $(deps) $(TESTS) $(LIBS) $(OBJS)
distclean:
	$(RM) -r $(OUT)
