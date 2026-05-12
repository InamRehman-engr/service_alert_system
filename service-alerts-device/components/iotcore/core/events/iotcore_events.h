#ifndef _IOTCORE_EVENTS_
#define _IOTCORE_EVENTS_

#include "iotcore_app_events.h"
#include "iotcore_data_events.h"
#include "iotcore_error_events.h"
// //Some events maybe string or direct integers. no need to make data models
// for them. just cast them to their type #include "system_errors_data_model.h"
// #include "system_info_data_model.h"
// #include "mqtt_data_model.h"

void start_iotcore_event_loop();

void delete_iotcore_event_loop();

#endif
