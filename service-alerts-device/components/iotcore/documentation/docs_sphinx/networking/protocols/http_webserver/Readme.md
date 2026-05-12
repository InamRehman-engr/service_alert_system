# HTTP Webserver

This file provides implementation over webserver provided by esp32 to give routes for control and usage of wifi.

It does that by providing routes for /wifi and /scan which can be used to give a known network list to esp32 through webserver. This is useful in the scenario where internet is not available and device needs to be provisioned. 

## /scan

You can use the following cURL command to scan for Wi-Fi SSIDs and their RSSI values:

```bash
curl -X GET \
  '192.168.4.1/scan' \
  --header 'Accept: */*' \
  --header 'User-Agent: Thunder Client (https://www.thunderclient.com)'
```
This command sends a GET request to 192.168.4.1/scan to retrieve a list of available Wi-Fi SSIDs and their RSSI values.

### Response

The response from the server will be in JSON format and may look like this:
```
{
  "response": "success",
  "SSIDS": [
    {
      "ssid": "HUAWEI-BG2a",
      "rssi": -42
    },
    {
      "ssid": "Cowlar Test",
      "rssi": -61
    },
    // ... (other SSIDs and RSSI values)
  ]
}
```

## /wifi

You can use the following cURL command to connect to a Wi-Fi network:

```bash
curl -X POST \
  '192.168.4.1/wifi' \
  --header 'Content-Type: application/x-www-form-urlencoded' \
  --data-urlencode 'ssid=Cowlar_Mission_Control_NG' \
  --data-urlencode 'pass=cowsarecool' \
  --data-urlencode 'pin=0915' \
  --data-urlencode 'user_id=10'
```

This command sends a POST request to 192.168.4.1/wifi with the necessary parameters to connect to the Wi-Fi network. Make sure to replace the values with the actual SSID, password, PIN, and user ID that you want to use.

### Response

The response from the server will be in JSON format and may look like this:

```
{
  "serial": "A4CF129EC8A8C2R100",
  "response": "success"
}
```