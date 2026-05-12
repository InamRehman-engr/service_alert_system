#ifndef __md5_decoding_h__
#define __md5_decoding_h__
#include "esp_types.h"

unsigned char *base64_decode(const char *src, size_t len, size_t *out_len);

#endif // __md5_decoding_h__