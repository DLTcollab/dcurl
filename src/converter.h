#ifndef _CONVERTER_H_
#define _CONVERTER_H_

#include "hash.h"

char long_value(char *const trits, const int offset, const int size);
char charValue(char *const trits, const int offset, const int size);
char *bytes_from_trits(char *const trits, const int offset, const int size);
void getTrits(const char *bytes, int bytelength, char *const trits, int length);
int indexOf(char *values, char find);
char *string_trits_from_string_trytes(const char *trytes, int length);
void copyTrits(char const value,
               char *const destination,
               const int offset,
               const int size);
char *string_trytes_from_string_trits(char *const trits,
                                      const int offset,
                                      const int size);
char tryteValue(char *const trits, const int offset);

#endif
