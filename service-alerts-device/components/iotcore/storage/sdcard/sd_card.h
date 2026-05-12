#ifndef _SD_CARD_H_
#define _SD_CARD_H_

#include "stdint.h"

void app_sdcard_task(void);
#ifdef CONFIG_UNITTEST_ENABLE_ALL
esp_err_t unittest_sdcard();
#endif
void appendFile(char *filename, uint8_t *data, size_t size);

#endif //_SD_CARD_H_