dcurl.o: dcurl.c
	gcc -fPIC -c dcurl.c

libdcurl.so: dcurl.o
	gcc -shared -o libdcurl.so dcurl.o -lpthread
