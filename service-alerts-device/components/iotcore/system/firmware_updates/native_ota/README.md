# Native OTA

This is the file that handles the processing native ota. 

It provides a functionality to write data to flash and reboot into the newly flashed firmware

It is build on top of the esp's provided api for usage of native ota and provides crc checksum matching of ota.
Current support includes 2 types of checksums

1. SHA256
2. MD5

## Usage

Native ota allows only usage of a single structure to make sure only one ota instance is running at a time.

User is requested to call init with paramters and pass the data in a queue

```
    // This is usage with md5 checksum
    native_ota.init_native_ota(ota_total_size, NATIVE_OTA_CHECKSUM_MD5, decodedMd5);


    // This needs to be called whenever data is available to write it to flash
    ota_done_size += evt->data_len;
    ota_done_size == evt->data_len ? update_http_ota_state(HTTP_OTA_DOWNLOAD_STARTED) : 0;
    ota_done_size == ota_total_size ? update_http_ota_state(HTTP_OTA_DOWNLOAD_SUCCESSFUL) : 0;
    ota_packet_t ota_data = {
        .last_packet = ota_total_size == ota_done_size,
        .packet_length = evt->data_len,
        .packet = malloc(evt->data_len * sizeof(char))
    };
    memcpy(ota_data.packet, evt->data, evt->data_len);
    xQueueSend(*native_ota.dataqueue, &ota_data, portMAX_DELAY);

```

The library currently does not support writing non sequentially and does not match crc if the data is not written sequentially.