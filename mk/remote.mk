# Build remote-worker
$(OUT)/worker-%.o: $(SRC)/%.c
	$(VECHO) "  CC\t$@\n"
	$(Q)$(CC) -o $@ $(WORKER_CFLAGS) $(SUB_INCLUDE) -c -MMD -MF $@.d $<

$(OUT)/remote-worker: $(OUT)/remote_worker.o $(WORKER_OBJS) $(SUB_OBJS)
	$(VECHO) "  LD\t$@\n"
	$(Q)$(CC) -o $@ $^ $(LDFLAGS)
