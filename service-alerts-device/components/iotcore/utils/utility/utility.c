#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *encode_string(char *data, int len) {
  int i = 0;
  int ind_data = 0;
  int bufsize = len * 1.5;

  char *buffer = malloc(bufsize * 1.5);
  char *realloc_buff = NULL;

  if (buffer) {
    while (i < len) {
      if (data[i] != '\n') {
        if ((ind_data + 1) > bufsize) {
          bufsize = bufsize * 1.5;
          realloc_buff = realloc(buffer, bufsize);
          if (realloc_buff) {
            buffer = realloc_buff;
          } else {
            break;
          }
        }
        if (data[i] == '\"') {
          buffer[ind_data] = '\\';
          ind_data++;
        }
        buffer[ind_data] = data[i];
        ind_data++;
      }
      i++;
    }
    buffer[ind_data] = '\0';
  }
  return buffer;
}