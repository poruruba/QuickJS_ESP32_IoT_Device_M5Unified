#ifndef _LIB_BASE32_H_
#define _LIB_BASE32_H_

#include <Arduino.h>

unsigned long base32_decode(const char* input, unsigned char* output, unsigned long outputSize);
String base32_encode(const unsigned char* data, unsigned long length);
unsigned long base32_encodedLength(unsigned long inputLength);
unsigned long base32_decodedLength(const char* base32_str);

#endif
