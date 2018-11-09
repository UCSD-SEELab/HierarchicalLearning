# Local Connector
Local connector version on photon. Read features from SD card and pack them and send through MQTT. Test results in [another folder](https://github.com/Orienfish/photon/tree/master/mqtt_server) show that most time in a iteration are consumed in reading from SD card.

## MQTT Library
The MQTT library refers to contributions in this [repo](https://github.com/hirotakaster/MQTT).

## Test Result
The space our code will take is as follows:

|text  | data | bss   |
|------|------|-------|
|28076 | 172  | 21704 |

Note one float is 4 bytes, so it really takes a long time to send those data.