#include <Arduino.h>

static const char* base32Alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

static int base32CharToValue(char c) {
  const char* p = strchr(base32Alphabet, c);
  return p ? (p - base32Alphabet) : -1;
}

unsigned long base32_decode(const char* input, unsigned char* output, unsigned long outputSize) {
  unsigned long buffer = 0;
  int bitsLeft = 0;
  unsigned long count = 0;

  for (unsigned long i = 0; input[i] != '\0'; ++i) {
    if (input[i] == '=' || input[i] == ' ') continue;

    int val = base32CharToValue(toupper(input[i]));
    if (val < 0) continue;

    buffer <<= 5;
    buffer |= val;
    bitsLeft += 5;

    if (bitsLeft >= 8) {
      bitsLeft -= 8;
      if (count < outputSize) {
        output[count++] = (buffer >> bitsLeft) & 0xFF;
      }
    }
  }
  return count;
}

String base32_encode(const unsigned char* data, unsigned long length) {
  String result;
  int buffer = 0;
  int bitsLeft = 0;

  for (unsigned long i = 0; i < length; ++i) {
    buffer <<= 8;
    buffer |= data[i];
    bitsLeft += 8;

    while (bitsLeft >= 5) {
      int index = (buffer >> (bitsLeft - 5)) & 0x1F;
      result += base32Alphabet[index];
      bitsLeft -= 5;
    }
  }

  if (bitsLeft > 0) {
    int index = (buffer << (5 - bitsLeft)) & 0x1F;
    result += base32Alphabet[index];
  }

  // Padding to make length a multiple of 8
  while (result.length() % 8 != 0) {
    result += '=';
  }

  return result;
}

unsigned long base32_encodedLength(unsigned long inputLength) {
  unsigned long bits = inputLength * 8;
  unsigned long base32Chars = (bits + 4) / 5; // ceil(bits / 5)
  unsigned long paddedLength = ((base32Chars + 7) / 8) * 8; // round up to multiple of 8
  return paddedLength;
}

unsigned long base32_decodedLength(const char* base32_str) {
  unsigned long len = strlen(base32_str);
  unsigned long pad = 0;

  // パディング '=' を除外
  for (int i = len - 1; i >= 0 && base32_str[i] == '='; --i) {
    pad++;
  }

  unsigned long clean_len = len - pad;
  return (clean_len * 5) / 8;
}

