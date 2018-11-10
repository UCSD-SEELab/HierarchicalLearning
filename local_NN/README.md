# Neural Network (2 Hidden Layers)

The test results in [another folder](https://github.com/Orienfish/photon/tree/master/mqtt_server) show the time consumtion in reading and computation respectively.

## MQTT Library
The MQTT library refers to contributions in this [repo](https://github.com/hirotakaster/MQTT)

## Prerequisite
To compile, flash, monitor from serial, you need the [Particle CLI](https://docs.particle.io/tutorials/developer-tools/cli/).

## Run
To run the code, you can simply connect your photon and download the code by
```
particle flash --usb nn.bin
```
and see test results through USB serial by
```
particle serial monitor
```
To compile the code, use
```
particle compile p --saveTo nn.bin
```

## Test Result
Suppose N_1 = N_2 = N, BATCH_LEN = 1, the maximum N we can have is 98. Letting N = 99 will crash down. It seems the upper limit of .bss space is 49k.