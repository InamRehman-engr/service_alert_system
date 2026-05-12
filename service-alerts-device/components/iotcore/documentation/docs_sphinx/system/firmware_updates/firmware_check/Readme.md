# Firmware Check

This file provides firmware checking based on the version numbering used in iotcore. It provides functions that can be used to check version strings with current ones and validate rc tags as well.

## Usage

Here is how it is used with http ota handler
```
cJSON *root;
    cJSON *value;
    root = cJSON_Parse(data);
    value = cJSON_GetObjectItem(root, "url");
    ESP_LOGW("ota", "url: %s", value->valuestring);
    if (validate_hardware_version(cJSON_GetObjectItem(root, "hwv")->valuestring, getDeviceInfo()->hardwareVersion) == HARDWARE_VERSION_OK &&
        validate_firmware_version(cJSON_GetObjectItem(root, "version")->valuestring, getDeviceInfo()->firmwareVersion, cJSON_IsTrue(cJSON_GetObjectItem(root, "rb"))) == FIRMWARE_VERSION_OK)
    {
        char * ota_topic = NULL;
        int extractedTopic;
        sscanf(topic, "d/%d/ota", &extractedTopic);
        asiprintf(&ota_topic, "d/%d/ota", extractedTopic);
        init_ota_event_handler(ota_topic, cJSON_GetObjectItem(root, "version")->valuestring);
        http_ota_handler_init(value->valuestring);
        free(ota_topic);
    }
```