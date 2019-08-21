ifeq ($(BOARD),de10nano)
    CFLAGS += -mcpu=cortex-a9 -mtune=cortex-a9 -mfloat-abi=hard -mfpu=neon
endif
