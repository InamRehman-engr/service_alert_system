#ifndef _system_errors_h_
#define _system_errors_h_
#include "esp_types.h"

typedef struct task_fail {
  char file[100];
  int line;
  int freeheap;
} thread_create_failed_t;

void post_task_create_failed_event(char *file, int line, uint32_t heap);

#endif