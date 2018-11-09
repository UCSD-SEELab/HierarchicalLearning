# Neural Network (2 Hidden Layers)

The test results in [another folder](https://github.com/Orienfish/photon/tree/master/mqtt_server) show the time consumtion in reading and computation respectively.

## MQTT Library
The MQTT library refers to contributions in this [repo](https://github.com/hirotakaster/MQTT)

## Test Result
Suppose N_1 = N_2 = N, BATCH_LEN = 1, the maximum N we can have is 90. The space our code will take is as follows:

|text  | data | bss   |
|------|------|-------|
|30652 | 172  | 46964 |

Letting N = 95 will crash down.

The time consumption is terrible for now. To compute the network above of 200 samples, the time consumed is 1848ms!