# Linear Regression
Implementation of linear regression on photon. Considering memory limitation, the program reads features of one sample each time and computes linear regression of that sample, after which the program packs the inference results together to send over MQTT. 

## MQTT Library
The MQTT library refers to contributions in this [repo](https://github.com/hirotakaster/MQTT)

## Test Result
The space our code will take is as follows:

|text  | data | bss   |
|------|------|-------|
|30140 | 172  | 3908  |

The time consumption is terrible for now. To compute the network above of 200 samples, the time consumed is 32ms while time consumption to compute 400 samples is 63ms!