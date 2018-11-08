# Linear Regression (Power and Lookup Table)
Implementation of linear regression on photon. Considering memory limitation, the program reads features of one sample each time and computes linear regression of that sample, after which the program packs the inference results together to send over MQTT. 

The test results in [another folder](https://github.com/Orienfish/photon/tree/master/mqtt_server) show the time consumtion in reading and computation respectively.

## MQTT Library
The MQTT library refers to contributions in this [repo](https://github.com/hirot