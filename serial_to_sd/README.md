# Write Files to SD Card
This folder contains code on photon and on Ubuntu Linux to write txt file to the SD card connected with photon. In our testing, this is important because instead of obtaining data from sensors, we read data from existed files in the current stage.

## Prerequisites
I use the Ubuntu 18.04 LTS. Nothing is required except [Particle CLI tools](https://docs.particle.io/tutorials/developer-tools/cli/) to compile code and flash to photon.

## Setup
To start the transmission, the photon needs to be connected to PC through USB port after flashing the execute file into it. After plug the photon in, start the write program on Linux and wait for the transmission to start. During the transmission, you can view what the photon receives through the terminal on Linux, since photon echos back everything it receives. The transmission may result in some errors from time to time, due to the unstableness of serial connection. And it could be very time consuming if the file to transfer is really large.

## Photon Part
Based the [SD card library](https://github.com/mumblepins/sd-card-library), the code on photon basically write every valid byte it receives to the SD card and echos it back to PC for checking. The transmission starts with sending a 's' and ends with a 'f'.

Before the transmission, remeber to modify "filename" on line 25 of wirte_files.ino, making it what you want to name your file on the SD card. Commenting out certain parts in setup() can decide whether to write files or to test the existed file content.

Compile and flash it to photon using the following commands in the "photon" directory:
```
particle compile p --saveTo your_file_name.bin
particle flash --usb your_file_name.bin
```

## Ubuntu Part
Simply open the serial port and transmit the txt files byte by byte. Contain a lot of exception handling, e.g. error in opening port or file.

Change the port name on line 9 of write_data.c. "FILE_NAME" on line 10 of write_data.c is to specify the path of the txt file on Linux. "LINE_CNT" on line 12 configure the maximum lines written to photon SD card, for limiting the transmission time.

Compile and start the program by the following commands in the "Ubuntu" directory:
```
gcc write_data.c -o write
sudo ./write // you need sudo right to access the serial port
```

## More...
Alternatives like transfering over TCP can be considered under stable network connection.
