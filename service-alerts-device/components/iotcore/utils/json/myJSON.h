

#ifndef _MYJSON_H_
#define _MYJSON_H_

#include "cJSON.h"
#include <stdio.h>
#include <string.h>

CJSON_PUBLIC(cJSON *)
myJSON_AddRawToObject(cJSON *const object, const char *const name,
                      const double number, int decimalpoint);
CJSON_PUBLIC(void)
myJSON_AddRawfloatArray(cJSON *const object, const float number,
                        int decimalpoint);

#endif // _MYJSON_H_