# Local Connector
Local connector version on photon. Read features from SD card and pack them and send through MQTT.

## MQTT Library
The MQTT library refers to contributions in this [repo](https://github.com/hirotakaster/MQTT).

## Test Result
The space our code will take is as follows:

|text  | data | bss   |
|------|------|-------|
|28076 | 172  | 21704 |

Note one float is 4 bytes, so it really takes a long time to send those data. Read from SD card costs around 500ms for 200 samples.