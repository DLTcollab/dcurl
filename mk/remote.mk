# Build remote-worker
$(OUT)/worker-%.o: $(SRC)/%.c $(LIBTUV_PATH)/include $(LIBRABBITMQ_PATH)/build/include
	$(VECHO) "  CC\t$@\n"
	$(Q)$(CC) -o $@ $(WORKER_CFLAGS) $(LIBTUV_INCLUDE) $(LIBRABBITMQ_INCLUDE) -c -MMD -MF $@.d $<

$(OUT)/remote-worker: $(OUT)/remote_worker.o $(WORKER_OBJS) $(LIBTUV_OBJS) $(LIBRABBITMQ_OBJS)
	$(VECHO) "  LD\t$@\n"
	$(Q)$(CC) -o $@ $^ $(LDFLAGS)
