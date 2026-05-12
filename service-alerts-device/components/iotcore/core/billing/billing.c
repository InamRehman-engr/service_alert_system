#include "billing.h"
#include "esp_err.h"
#include "nvs_read_write.h"
#include "time.h"

billing_config_t billing_config = {
    .billing_Enabled = true,
    .billing_paid_upto = 0,
    .billing_grace_period = DAYS_2_SECONDS(10),
};

void billing_restore(void) {
  size_t len = sizeof(billing_config);
  if (ESP_OK !=
      readKeyValueInFlash_blob(billing_key, (uint8_t *)&billing_config, &len)) {
    billing_config.billing_Enabled = false;

    time(&billing_config.billing_paid_upto);
    billing_config.billing_paid_upto +=
        DAYS_2_SECONDS(15); // 15 days trial perid
    billing_config.billing_grace_period =
        DAYS_2_SECONDS(10); // 10 days grace period
    saveKeyValueInFlash_blob(billing_key, (uint8_t *)&billing_config,
                             sizeof(billing_config));
  }
}
void billing_enable(void) {
  if (billing_config.billing_Enabled == false) {
    billing_config.billing_Enabled = true;
    saveKeyValueInFlash_blob(billing_key, (uint8_t *)&billing_config,
                             sizeof(billing_config));
  }
}

void billing_disble(void) {
  if (billing_config.billing_Enabled == true) {
    billing_config.billing_Enabled = false;
    saveKeyValueInFlash_blob(billing_key, (uint8_t *)&billing_config,
                             sizeof(billing_config));
  }
}
void billing_update(time_t paidupto, time_t graceperid) {
  if (paidupto != billing_config.billing_paid_upto ||
      graceperid != billing_config.billing_grace_period) {
    billing_config.billing_paid_upto = paidupto;
    billing_config.billing_grace_period = graceperid;
    saveKeyValueInFlash_blob(billing_key, (uint8_t *)&billing_config,
                             sizeof(billing_config));
  }
}

bool BILLING_IS_ACTIVE(time_t unixtime) {
  return ((billing_config.billing_Enabled == false)
              ? 1
              : ((billing_config.billing_paid_upto +
                  billing_config.billing_grace_period) > unixtime
                     ? true
                     : false));
}

bool BILLING_IS_ACTIVE_NOW(void) {
  time_t now;
  time(&now);
  return BILLING_IS_ACTIVE(now);
}