

#include "url_encoding.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int url_encode(char *src, char *dst, int32_t srclen) {
  int setstringTerminator = 0;
  int dstlen = 0;
  char *hex = "0123456789abcdef";
  if (srclen == 0) {
    srclen = strlen(src);
    setstringTerminator = 1;
  }
  for (int i = 0; i < srclen; i++) {
    char c = src[i];

    if (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') ||
        ('0' <= c && c <= '9')) {
      dst[dstlen++] = c;
    } else {
      dst[dstlen++] = '%';
      dst[dstlen++] = hex[c >> 4];
      dst[dstlen++] = hex[c & 0x0F];
    }
  }
  if (setstringTerminator) {
    dst[dstlen] = 0;
  }
  return dstlen;
}

int url_decode(char *src, char *dst, int32_t srclen) {
  int dstlen = 0;
  int setstringTerminator = 0;
  if (srclen == 0) {
    setstringTerminator = 1;
    srclen = strlen(src);
  }
  for (int i = 0; i < srclen; i++) {
    char c = src[i];
    if (c == '+') {
      dst[dstlen++] = ' ';
    } else if (c == '%') { // get the assci value in hex i.e. %20  is space char
      char ch_asci = ((src[i + 1] & 0x0F) << 4) | (src[i + 2] & 0x0F);
      dst[dstlen++] = ch_asci;
      i += 2;
    } else if (c ==
               '\\') { // get the assci value in hex i.e. %20  is space char

      switch (src[i + 1]) {
      case 't':
        dst[dstlen++] = '\t';
        break;
      case 'v':
        dst[dstlen++] = '\v';
        break;
      case '0':
        dst[dstlen++] = '\0';
        break;
      case 'b':
        dst[dstlen++] = '\b';
        break;
      case 'f':
        dst[dstlen++] = '\f';
        break;
      case 'n':
        dst[dstlen++] = '\n';
        break;
      case 'r':
        dst[dstlen++] = '\r';
        break;
      default:
        dst[dstlen++] = src[i + 1];
        break;
      }
      // printf(">>%c%c<<", src[i],src[i+1] );
      // printf("((%c))", dst[dstlen -1 ] );
      i++;

    }

    else {
      dst[dstlen++] = c;
    }
  }
  if (setstringTerminator) {
    dst[dstlen] = 0;
  }
  return dstlen;
}
