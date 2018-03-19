PYTHON = python3
PYTHON := $(shell which $(PYTHON))
ifndef PYTHON
$(error "python3 is required.")
endif

# check "iota" module in Python installation
PY_CHECK_MOD_IOTA := $(shell $(PYTHON) -c "import iota" 2>/dev/null && \
                       echo 1 || echo 0)
ifeq ("$(PY_CHECK_MOD_IOTA)","1")
    py_prepare_cmd = $(Q)cat $< >> $@
else 
    py_prepare_cmd = $(warning "skip $@ because PyIOTA is not installed.")
endif
