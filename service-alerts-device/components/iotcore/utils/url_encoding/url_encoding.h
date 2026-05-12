#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int url_encode(char *src, char *dst, int32_t srclen);
int url_decode(char *src, char *dst, int32_t srclen);
