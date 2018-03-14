OPENCL_LIB ?= /usr/local/cuda-9.1/lib64
OPENJDK_PATH ?= /usr/lib/jvm/java-1.8.0-openjdk-amd64
SRC ?= ./src
TEST ?= ./test
OUT ?= ./build

$(OUT)/curl.o: $(SRC)/hash/curl.c
	gcc -Os -fPIC -g -o $@ -c $<

$(OUT)/constants.o: $(SRC)/constants.c
	gcc -fPIC -g -o $@ -c $<

$(OUT)/trinary.o: $(SRC)/trinary/trinary.c
	gcc -Os -fPIC -g -o $@ -c $<

$(OUT)/dcurl.o: $(SRC)/dcurl.c
	gcc -Os -fPIC -g -o $@ -c $<

$(OUT)/pow_c.o: $(SRC)/pow_c.c
	gcc -Os -fPIC -o $@ -g -c $<

$(OUT)/pow_sse.o: $(SRC)/pow_sse.c
	gcc -Os -fPIC -o $@ -g -c $<

$(OUT)/pow_cl.o: $(SRC)/pow_cl.c
	gcc -fPIC -g -o $@ -c $<

$(OUT)/clcontext.o: $(SRC)/clcontext.c
	gcc -fPIC -g -o $@ -c $<

test_trinary: $(TEST)/test_trinary.c $(OUT)/curl.o $(OUT)/trinary.o
	gcc -Wall -g -o $@ $^
	./$@

test_curl: $(TEST)/test_curl.c $(OUT)/curl.o $(OUT)/trinary.o
	gcc -Wall -g -o $@ $^
	./$@

test: test_trinary \
	  test_curl

libdcurl.so: ./jni/iri-pearldiver-exlib.c \
	     $(OUT)/curl.o $(OUT)/constants.o \
             $(OUT)/trinary.o $(OUT)/dcurl.o \
             $(OUT)/pow_sse.o $(OUT)/pow_cl.o \
             $(OUT)/clcontext.o
	gcc -fPIC -msse2 -shared -I$(OPENJDK_PATH)/include -I$(OPENJDK_PATH)/include/linux \
	-I$(OPENCL_PATH)/include -L/usr/local/lib/ -L$(OPENCL_LIB) -o $@ $^ \
	-lpthread -lOpenCL

clean:
	rm $(OUT)/* test_trinary test_curl
