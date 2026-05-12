# HTTP OTA Handler

This module makes use of two modules

1. http_rest_api
2. native_ota

to provide ota support that can download ota binary files from server and write them to flash. It requires that in the headers returned from the server there be a file size param and also an MD5 checksum available.

The library also requires that the url also support sending out partial file chunks. This is necessary to download ota with resume functionality. At the momemnt this is not optional. 

## Usage

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

This chunk of code assumes that data contains a json which contains information of firmware and the download link. This will not modify the link in any way or form and depends on the link to work with the provided CA cert

That is why it is required that the dundle config be followed strictly in http_rest_api