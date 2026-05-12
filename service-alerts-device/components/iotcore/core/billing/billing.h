#ifndef __BILLING_H__
#define __BILLING_H__
#include "time.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define HOURS_2_SECONDS(h) (h * 60 * 60)
#define DAYS_2_SECONDS(d) (HOURS_2_SECONDS(d * 24))

#define billing_key "bill"
#define billing_paid_upto_key "billpaid"
#define billing_grace_period_key "billgp"

typedef struct {
  bool billing_Enabled;
  time_t billing_paid_upto; // this timestamp of bill is paid upto devie will
                            // keep working to this data + graceperid
  int32_t billing_grace_period;

} billing_config_t;
extern billing_config_t billing_config;

#define BILLING_ACTIVE(unixtime)                                               \
  ((billing_config.billing_Enabled == false)                                   \
       ? 1                                                                     \
       : ((billing_config.billing_paid_upto +                                  \
           billing_config.billing_grace_period) > unixtime                     \
              ? true                                                           \
              : false))

bool BILLING_IS_ACTIVE_NOW(void);
bool BILLING_IS_ACTIVE(time_t unixtime);
#define IF_BILLING_ACTIVE(fn)                                                  \
  {                                                                            \
    time_t now;                                                                \
    time(&now);                                                                \
    if (BILLING_IS_ACTIVE(now)) {                                              \
      fn;                                                                      \
    }                                                                          \
  }

void billing_restore(void);
void billing_enable(void);
void billing_disble(void);
void billing_update(time_t paidupto, time_t graceperid);

#endif // __BILLING_H__