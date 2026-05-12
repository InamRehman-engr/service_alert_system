#include "iotcore_errors_data_model.h"
#include "iotcore_events.h"
#include "string.h"

void extractFilename(const char *path, char **filename) {
  const char *lastSlash = strrchr(path, '/'); // For Unix-based systems
  if (lastSlash == NULL) {
    lastSlash = strrchr(path, '\\'); // For Windows systems
  }

  if (lastSlash != NULL) {
    *filename = (char *)(lastSlash + 1);
  } else {
    *filename = (char *)path; // If no directory separator is found, consider
                              // the whole string as a filename
  }
}
void post_task_create_failed_event(char *file, int line, uint32_t heap) {
  thread_create_failed_t data = {
      .line = line,
      .freeheap = heap,
  };
  char *file_name;
  extractFilename(file, &file_name);
  strncpy(data.file, file_name, strlen(file_name));
  post_iotcore_error_event(TASK_CREATE_FAILED, (void *)&data,
                           sizeof(thread_create_failed_t));
}