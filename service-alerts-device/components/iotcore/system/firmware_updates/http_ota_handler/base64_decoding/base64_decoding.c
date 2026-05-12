#include "base64_decoding.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const unsigned char base64_table[65] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

unsigned char *base64_decode(const char *src, size_t len, size_t *out_len) {
  unsigned char *out, *pos;
  const char *end, *in;

  size_t olen;
  int block_size = 4, pad = 0;

  unsigned char block[4], tmp;
  int i;

  if (len == 0)
    return NULL;

  if (src[len - 1] == '=')
    pad++;
  if (src[len - 2] == '=')
    pad++;

  olen = len * 3 / 4 - pad;
  out = malloc(olen);
  if (out == NULL)
    return NULL;

  end = src + len;
  in = src;
  pos = out;

  while (in < end) {
    for (i = 0; i < block_size; i++) {
      tmp = in < end ? (unsigned char)*in++ : 0;
      block[i] =
          strchr((const char *)base64_table, tmp) - (const char *)base64_table;
    }

    *pos++ = (block[0] << 2) + ((block[1] & 0x30) >> 4);
    if (block[2] != 64)
      *pos++ = ((block[1] & 0x0f) << 4) + ((block[2] & 0x3c) >> 2);
    if (block[3] != 64)
      *pos++ = ((block[2] & 0x03) << 6) + block[3];
  }

  *out_len = pos - out;
  return out;
}