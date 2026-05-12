# Server API Calls

Server api calls is a file that uses rest_api_client to register the device on the server.

It performs 2 api requests
- /auth
- /device/create

These 2 requests after being done will return a device ID that is unique to the device's chip version and mac address. The id can be used to perform communcation on MQTT so that all communication is unique to device ids.

Here is description of the APIs in iotcore.cowlar.com as of 17/1/24

## POST /auth

This endpoint is used for user authentication.

### Parameters

| Name       | Description                                | Type    | Location   | Example                             |
|------------|--------------------------------------------|---------|------------|-------------------------------------|
| email *    | Email to use for login.                    | string  | formData   | device.sim-dispenser@cowlar.com     |
| password * | User's password.                           | string  | formData   | 123456                              |
| remember   | Remember Login (optional).                 | boolean | formData   | false                               |

### Request Example

```bash
curl -X 'POST' \
  'https://api.iotcore.cowlar.com/v1/auth' \
  -H 'accept: application/json' \
  -H 'Content-Type: application/x-www-form-urlencoded' \
  -d 'email=device.sim-dispenser%40cowlar.com&password=123456&remember=false'
```

### Response 

```
{
  "success": true,
  "data": {
    "accessToken": "eyJhbGciOiJSUzI1NiIsImtpZCI6Im5vZGUtam9zZS1vcy0xIn0...",
    "refreshToken": "eyJhbGciOiJSUzI1NiIsImtpZCI6Im5vZGUtam9zZS1vcy0xIn0..."
  }
}
```


## POST /device/create

This endpoint is used to create a new device. It is accessible only to administrators.

### Parameters

| Name         | Description                                         | Type     | Location   | Example                        |
|--------------|-----------------------------------------------------|----------|------------|--------------------------------|
| serial *     | Serial for the device (channel identifier).        | string   | formData   | A4CF129EC8A8C2R100             |
| pin_code *   | Device Pin code.                                   | string   | formData   | 0915                           |
| controller   | Flag to indicate if the request is from a controller. | boolean | formData   | true                           |
| password     | Device password.                                   | string   | formData   | password                       |
| mac          | Mac address.                                       | string   | formData   | mac                            |
| hw_ver       | Device hardware version.                           | string   | formData   | hw_ver                         |
| ssid         | SSID.                                              | string   | formData   | ssid                           |
| device_type *| Device Type (e.g., Fiber aligner = 21).           | number   | formData   | 1                              |

### Request Example

```bash
curl -X 'POST' \
  'https://api.iotcore.cowlar.com/v1/device/create' \
  -H 'accept: application/json' \
  -H 'authorization: eyJhbGciOiJSUzI1NiIsImtpZCI6Im5vZGUtam9zZS1vcy0xIn0...' \
  -H 'Content-Type: application/x-www-form-urlencoded' \
  -d 'serial=A4CF129EC8A8C2R100&pin_code=0915&controller=true&device_type=1'
```

### Response 
```
{
  "success": true,
  "data": {
    "id": 304,
    "serial": "A4CF129EC8A8C2R100",
    "status": 1,
    "owner_id": 5,
    "device_type": 1,
    "live_status": false,
    "fv": "v0.0.1",
    "bill_cleared": false,
    "trial_period": 30,
    "grace_period": 7,
    "enable_bill": false,
    "sims": null,
    "lst": "1703681209.671",
    "Organization_Devices": [
      {
        "id": 303,
        "orgId": 5,
        "device_id": 304,
        "device_name": null,
        "status": 1,
        "share_by": null,
        "can_share": true,
        "remote_id": null,
        "share_verify_token": null,
        "can_change_geo_fence": true,
        "can_change_scheduling": true,
        "createdAt": "2023-11-16T12:31:11.000Z",
        "updatedAt": "2023-11-16T12:31:11.000Z"
      }
    ],
    "Owner": {
      "name": "Test"
    },
    "Group": null,
    "Settings": {
      "settings": {
        "timezone_name": "Asia/Karachi"
      },
      "id": 304,
      "geofence_id": null,
      "schedule_id": null
    }
  }
}

```


A task is started in this file which tries to get the access token first and then utilizies that token in the second request to create the device which returns the device id

The device id is passed to the application in event as an integer.

event id: SYSTEM_IOTCORE_API_GOT_CLIENT_ID

here is the actual post command:
```
post_iotcore_app_event(SYSTEM_IOTCORE_API_GOT_CLIENT_ID,
                                   &obj->valueint, sizeof(int));
```

## Get Device ID
If the you need to recieve the device id as soon as it is recieved. You can get it with

```
void my my_callback_function(void *arg, esp_event_base_t event_base,
                            int32_t event_id, void *event_data) {
  ESP_LOGI("DEVICE_ID", "Device_id is : %d", *(int *)event_data)
}

esp_event_handler_register_with(
      iotcore_app_event_loop, IOTCORE_APP_EVENTS_BASE,
      SYSTEM_IOTCORE_API_GOT_CLIENT_ID, my_callback_function, NULL);
```