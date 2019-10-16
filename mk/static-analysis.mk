static-analysis:
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
