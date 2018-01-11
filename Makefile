OPENCL_LIB ?= /usr/local/cuda-9.1/lib64/

curl.o: curl.c
	gcc -fPIC -g -c $<

constants.o: constants.c
	gcc -fPIC -g -c $<

trinary.o: trinary.c
	gcc -fPIC -g -c $<

dcurl.o: dcurl.c
	gcc -fPIC -g -c $<

pow_c.o: pow_c.c
	gcc -Os -fPIC -g -c $<

pow_sse.o: pow_sse.c
	gcc -Os -fPIC -g -c $<

pow_cl.o: pow_cl.c
	gcc -fPIC -g -c $<

clcontext.o: clcontext.c
	gcc -fPIC -g -c $<

test: test.c curl.o constants.o trinary.o dcurl.o pow_c.o pow_sse.o
	gcc -g -o $@ $^ -lpthread

#dcurl.o: dcurl.c
#	gcc -fPIC -c dcurl.c
#
libdcurl.so: dcurl.o curl.o constants.o trinary.o pow_c.o pow_cl.o pow_sse.o clcontext.o
	gcc -msse2 -shared -L/usr/local/lib/ -L$(OPENCL_LIB) -o libdcurl.so $^ -lpthread -lOpenCL

clean:
	rm *.o
