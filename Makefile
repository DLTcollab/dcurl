curl.o: curl.c
	gcc -g -c $<

constants.o: constants.c
	gcc -g -c $<

trinary.o: trinary.c
	gcc -g -c $<

dcurl.o: dcurl.c
	gcc -g -c $<

pow_c.o: pow_c.c
	gcc -g -c $<

test: test.c curl.o constants.o trinary.o dcurl.o pow_c.o
	gcc -g -o $@ $^ -lpthread

#dcurl.o: dcurl.c
#	gcc -fPIC -c dcurl.c
#
#libdcurl.so: dcurl.o curl.o constants.o trinary.o pow_c.o
#	gcc -shared -o libdcurl.so $^ -lpthread
