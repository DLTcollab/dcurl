# Copy from mk/java.mk for Infer static code analysis,
# since Infer needs the environment variable JAVA_HOME to be set
# If JAVA_HOME is not set, guess it according to system configurations
ifndef JAVA_HOME
    JAVAC := $(shell which javac)
    ifeq ($(UNAME_S),Darwin)
        # macOS
        JAVA_HOME := $(shell /usr/libexec/java_home)
    else
        # Linux
        JAVA_HOME := $(shell readlink -f $(JAVAC) | sed "s:/bin/javac::")
    endif
endif # JAVA_HOME

static-analysis:
	# CppCheck
	cppcheck \
	  --enable=all \
	  --error-exitcode=1 \
	  --force \
	  -I $(SRC) \
	  $(LIBTUV_INCLUDE) \
	  $(LIBRABBITMQ_INCLUDE) \
	  $(SSE2NEON_INCLUDE) \
	  --quiet \
	  --suppressions-list=cppcheck_suppress \
	  -UERANGE \
	  $(SRC) tests
	# Infer
	$(MAKE) distclean
	$(MAKE) BUILD_JNI=1  # Workaround: For enabling Infer to check the build option BUILD_JNI=1
	$(MAKE) distclean
	JAVA_HOME=$(JAVA_HOME) infer run \
	  --fail-on-issue \
	  --skip-analysis-in-path +deps/rabbitmq-c/librabbitmq/amqp* \
	  --skip-analysis-in-path +deps/rabbitmq-c/tests/test* \
	  -- \
	  $(MAKE) \
	    BUILD_AVX=1 \
	    BUILD_GPU=1 \
	    BUILD_FPGA_ACCEL=1 \
	    BUILD_REMOTE=1 \
	    BUILD_JNI=1 \
	    BUILD_STAT=1
	$(MAKE) distclean
	infer run --fail-on-issue -- $(MAKE) \
	  BUILD_SSE=1
	$(MAKE) distclean
	infer run --fail-on-issue -- $(MAKE) \
	  BUILD_GENERIC=1
	$(MAKE) distclean
