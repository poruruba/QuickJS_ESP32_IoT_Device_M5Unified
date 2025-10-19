#ifndef _LIB_HMAC_H_
#define _LIB_HMAC_H_

#define HMAC_TYPE_SHA1    1
#define HMAC_TYPE_MD5     5
#define HMAC_TYPE_SHA256  256

long hmac_calculate(unsigned char type, const unsigned char *p_key, int key_len,const unsigned char *p_input, int input_len, unsigned char *p_hmac);

#endif
