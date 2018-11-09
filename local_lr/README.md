# Linear Regression
Implementation of linear regression on photon with data sent through MQTT. Time measurements are took in the program. 

## MQTT Library
The MQTT library refers to contributions in this [repo](https://github.com/hirotakaster/MQTT)

## Prerequisite
To compile, flash, monitor from serial, you need the [Particle CLI](https://docs.particle.io/tutorials/developer-tools/cli/).

## Run
To run the code, you can simply connect your photon and download the code by
```
particle flash --usb lr.bin
```
and see test results through USB serial by
```
particle serial monitor
```
To compile the code, use
```
particle compile p --saveTo lr.bin
```