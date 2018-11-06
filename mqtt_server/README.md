# MQTT Server on a Linux System and the Time Consumption Test
This folder contains the python subscriber to receive data from photon and several test data folders under various conditions.

## Prerequisites
I use the Ubuntu 18.04 LTS. To start with, you need to install Mosquitto, paho-mqtt and Wondershaper. Mosquitto is a popular MQTT server while paho-mqtt is a convenient Python library for MQTT. Wondershaper is a network traffic shaping tool for Linux. All of the installations are pretty easy.

To install and configure Mosquitto, follow the steps [here](https://www.digitalocean.com/community/tutorials/how-to-install-and-secure-the-mosquitto-mqtt-messaging-broker-on-ubuntu-18-04). You need to configure several files under /etc/mosquitto/ for advanced settings like the log and the user's password.

To install paho-mqtt 1.4.0, see the tutorials [here](https://pypi.org/project/paho-mqtt/).

To install Wondershaper, simply do:
```
sudo apt-get install wondershaper
```

## Subscriber
This Python version program subscribes 4 topics: **"house.hand", "tmp", "parameter", "sync"**.
- **"house.hand"** represents the real data, namely the inference results sent from photon.
- **"tmp"** is the final file containing time test results. In each sampling iteration, the test results include the previous time before an iteration starts (prev), the current time after sending (cur), the sleep time (sleep), len of the sending data in bytes (len), computation time (c), reading from SD card time (r) and the total time of computation and reading (ready). All those data will be writen to /tmp_XX/local_log_$bw$_$sr$.txt, where "XX" denotes the name of a special case, "$bw$" and "$sr$" are the bandwidth and sampling rate settings. The complete log of this test is in /tmp_XX/total_log.txt, which is more intuitive for human to compare.
- **"parameter"** topic is for notifying bandwidth and sampling rate settings in this iteration. Then the 
- **"sync"** is for time synchronization, simply sending the time before an iteration starts (prev). This feature is designed for power test initially and may have no usage in the smart home project. As there's no global clock in photon, the simplest method to synchronize its clock with subscriber's clock is sending the time measurements from photon to subscriber.

## Running the Tests
To run the tests, you need to set up the following things one by one:
1. Start the Mosquitto server.
```
# to simply start the server
mosquitto 
# or start the server under certain configurations, e.g. I use this to enable all the loggings
sudo mosquitto -c /etc/mosquitto/mosquitto.conf
```
2. Creat a folder for all the tmp.txt files in this test and modify the path in subsriber.py (line 6 and line 8).
3. Start the subscriber.
```
python subscriber.py # should be used under this folder
```
4. Start the photon. Let the test begin! You should see the data of 4 topics sending to mosquitto server first and then getting forward to our subscriber.

## Test Results
The following picture is a screenshot on my Ubuntu during my tests. The left terminal is the logging of Mosquitto server and the right terminal is the terminal output of subscriber.py.
<div align=center><img width="800" height="250" src="https://github.com/Orienfish/photon/blob/master/mqtt_server/test.png"/></div>

Until now, I tested 3 different versions of photon code: the local connector which simply read data from SD card and send them using MQTT, the linear regression code using power computation in the math library and the linear regression code using lookup table. These code can be found in other folders parallel to mqtt_server.
- The log results show that the local connector is the fastest one with the time consumed in reading from SD card ranges from 300ms to 700ms, depending on the sampling rate. Thus reading from SD card is responsible for most of the time consumption.
- Computation time using the lookup table ranges from 40ms to 90ms while the power computation version consumes a period between 240ms and 530ms.
- Replacing power computation with lookup table does save a lot of time. Each iteration whose sampling rate is larger than 320 in the power computation version ends up in time exceeding (a "N" which means no sleep time left), while the lookup table version could safely transfer the data under all sampling rates.
- Both the power computaiton and the lookup table version suffer from occasionally time exceeding, which possibly results from the relative high time consumption in computation and reading and the limited bandwidth (maybe the network conjests after several iterations).