#include "lib_hmac.h"
#include <mbedtls/md.h>

long hmac_calculate(unsigned char type, const unsigned char *p_key, int key_len,const unsigned char *p_input, int input_len, unsigned char *p_hmac)
{
  int output_len;
  mbedtls_md_type_t md_type;
  
  if( type == HMAC_TYPE_SHA1 ){
    output_len = 20;
    md_type = MBEDTLS_MD_SHA1;
  }else if( type == HMAC_TYPE_MD5 ){
    output_len = 16;
    md_type = MBEDTLS_MD_MD5;
  }else if( type == HMAC_TYPE_SHA256 ){
    output_len = 32;
    md_type = MBEDTLS_MD_SHA256;
  }else{
    return -1;
  }

  mbedtls_md_context_t context;
  
  mbedtls_md_init(&context);
  mbedtls_md_setup(&context, mbedtls_md_info_from_type(md_type), 1);
  mbedtls_md_hmac_starts(&context, (const unsigned char*)p_key, key_len);
  mbedtls_md_hmac_update(&context, (const unsigned char*)p_input, input_len);
  mbedtls_md_hmac_finish(&context, p_hmac); // output_len bytes
  mbedtls_md_free(&context);

  return 0;
}