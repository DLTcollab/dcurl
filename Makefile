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

clean:
	rm *.o
