

#include "myJSON.h"
#include "cJSON.h"
#include "string.h"

CJSON_PUBLIC(cJSON *)
myJSON_AddRawToObject(cJSON *const object, const char *const name,
                      const double number, int decimalpoint) {
  char format[10];
  char mynumber[100];
  if ((number * 0) != 0) {
    sprintf(mynumber, "%d", 0);
  } else {
    if (decimalpoint) {
      sprintf(format, "%%1.%df", decimalpoint);
    } else {
      sprintf(format, "%%1.%dg", 15);
    }
    sprintf(mynumber, format, number);
    // if the "g" converte this into scientific notation format convert it into
    // "f"
    if (strstr(mynumber, "e") != NULL) {
      if (decimalpoint) {
        sprintf(format, "%%1.%df", decimalpoint);
      } else {
        sprintf(format, "%%1.%df", 15);
      }
      sprintf(mynumber, format, number);
    }
  }
  return cJSON_AddRawToObject(object, name, mynumber);
}
CJSON_PUBLIC(void)
myJSON_AddRawfloatArray(cJSON *const object, const float number,
                        int decimalpoint) {
  char mynumber[100];
  char format[10];
  if ((number * 0) != 0) {
    sprintf(mynumber, "%d", 0);
  } else {
    if (decimalpoint) {
      sprintf(format, "%%1.%df", decimalpoint);
    } else {
      sprintf(format, "%%1.%dg", 15);
    }
    sprintf(mynumber, format, number);
    // if the "g" converte this into scientific notation format convert it into
    // "f"
    if (strstr(mynumber, "e") != NULL) {
      if (decimalpoint) {
        sprintf(format, "%%1.%df", decimalpoint);
      } else {
        sprintf(format, "%%1.%df", 15);
      }
      sprintf(mynumber, format, number);
    }
  }

  cJSON *raw_item = cJSON_CreateRaw(mynumber);
  cJSON_AddItemToArray(object, raw_item);
}