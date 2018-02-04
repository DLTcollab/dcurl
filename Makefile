OPENCL_LIB ?= /usr/local/cuda-9.1/lib64/
SRC ?= ./src
TEST ?= ./test
BUILD ?= ./build

$(BUILD)/curl.o: $(SRC)/curl.c
	gcc -fPIC -g -o $@ -c $<

$(BUILD)/constants.o: $(SRC)/constants.c
	gcc -fPIC -g -o $@ -c $<

$(BUILD)/trinary.o: $(SRC)/trinary.c
	gcc -fPIC -g -o $@ -c $<

$(BUILD)/dcurl.o: $(SRC)/dcurl.c
	gcc -fPIC -g -o $@ -c $<

$(BUILD)/pow_c.o: $(SRC)/pow_c.c
	gcc -Os -fPIC -o $@ -g -c $<

$(BUILD)/pow_sse.o: $(SRC)/pow_sse.c
	gcc -Os -fPIC -o $@ -g -c $<

$(BUILD)/pow_cl.o: $(SRC)/pow_cl.c
	gcc -fPIC -g -o $@ -c $<

$(BUILD)/clcontext.o: $(SRC)/clcontext.c
	gcc -fPIC -g -o $@ -c $<

./test_dcurl: $(TEST)/test.c $(BUILD)/curl.o $(BUILD)/constants.o $(BUILD)/trinary.o $(BUILD)/dcurl.o $(BUILD)/pow_c.o \
	  $(BUILD)/pow_sse.o $(BUILD)/pow_cl.o $(BUILD)/clcontext.o
	gcc -g -L/usr/local/lib/ -L$(OPENCL_LIB) -o $@ $^ -lpthread -lOpenCL

./libdcurl.so: $(BUILD)/curl.o $(BUILD)/constants.o $(BUILD)/trinary.o $(BUILD)/dcurl.o \
	         $(BUILD)/pow_sse.o $(BUILD)/pow_cl.o $(BUILD)/clcontext.o
	gcc -msse2 -shared -L/usr/local/lib/ -L$(OPENCL_LIB) -o libdcurl.so $^ -lpthread -lOpenCL

$(BUILD)/new_trinary.o: $(SRC)/trinary/trinary.c
	gcc -Wall -g -o $@ -c $<

$(BUILD)/new_curl.o: $(SRC)/hash/curl.c
	gcc -Wall -g -o $@ -c $<

test_trinary: $(TEST)/test_trinary.c $(BUILD)/new_trinary.o
	gcc -Wall -g -o $@ $^
	./$@

test_curl: $(TEST)/test_curl.c $(BUILD)/new_curl.o $(BUILD)/new_trinary.o
	gcc -Wall -g -o $@ $^
	./$@

test_pow_sse: $(TEST)/test_pow.c $(BUILD)/new_trinary.o $(BUILD)/pow_sse.o \
	   	      $(BUILD)/new_curl.o $(BUILD)/constants.o
	gcc -Wall -msse2 -DPOW_SSE -g -o $@ $^ -lpthread
	./$@

test_pow_cl: $(TEST)/test_pow.c $(BUILD)/new_trinary.o $(BUILD)/pow_cl.o \
	         $(BUILD)/clcontext.o $(BUILD)/new_curl.o $(BUILD)/constants.o
	gcc -Wall -DPOW_CL -L/usr/local/lib/ -L$(OPENCL_LIB) -g -o $@ $^ -lpthread -lOpenCL
	./$@

clean:
	rm *.o
