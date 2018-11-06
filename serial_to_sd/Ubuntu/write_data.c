// simple program for writing certain files to photon through usb serial
#include <stdio.h>
#include <errno.h>
#include <fcntl.h> 
#include <string.h>
#include <termios.h>
#include <unistd.h>

const char DEV_NAME[] = "/dev/ttyACM0";
const char FILE_NAME[] = "/home/xiaofan/Desktop/RaspberryPi_Measurements/data/extracted_subject1.txt";
const int MAX_BUFFER_SIZE = 1024;
const int LINE_CNT = 1000; // the lines that write to sd card
const speed_t SPEED = B9600;

int set_interface_attribs (int fd, int speed, int parity);
void set_blocking (int fd, int should_block);
void receive_info(int fd);
void receive_echo(int fd);
void delay(int n);
int open_and_wait(speed_t speed);
void write_file(int fd, const char *filename);

int main() {
	// open port and wait the photon to be ready
    int fd = open_and_wait(SPEED);
    if (fd < 0) // open or wait error
		return -1;

	write_file(fd, FILE_NAME);
	close(fd);

	return 0;
}

/*
 * set attributes of serial port fd
 */
int set_interface_attribs (int fd, int speed, int parity)
{
    struct termios tty;
    memset (&tty, 0, sizeof tty);
    if (tcgetattr (fd, &tty) != 0)
    {
        fprintf(stderr, "error %d from tcgetattr", errno);
        return -1;
    }

    cfsetospeed (&tty, speed);
    cfsetispeed (&tty, speed);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
    // disable IGNBRK for mismatched speed tests; otherwise receive break
    // as \000 chars
    tty.c_iflag &= ~IGNBRK;         // disable break processing
    tty.c_lflag = 0;                // no signaling chars, no echo,
                                    // no canonical processing
    tty.c_oflag = 0;                // no remapping, no delays
    tty.c_cc[VMIN]  = 0;            // read doesn't block
    tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl
    tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
					                // enable reading
    tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
    tty.c_cflag |= parity;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    if (tcsetattr (fd, TCSANOW, &tty) != 0)
    {
        fprintf(stderr, "error %d from tcsetattr", errno);
        return -1;
    }
    return 0;
}

/*
 * set blocking of serial port fd
 */
void set_blocking (int fd, int should_block)
{
    struct termios tty;
    memset (&tty, 0, sizeof tty);
    if (tcgetattr (fd, &tty) != 0)
    {
        fprintf(stderr, "error %d from tggetattr", errno);
        return;
    }

    tty.c_cc[VMIN]  = should_block ? 1 : 0;
    tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

    if (tcsetattr (fd, TCSANOW, &tty) != 0)
        fprintf(stderr, "error %d setting term attributes", errno);
    return;
}

/*
 * receive info from photon and print it in terminal here
 * the exact received info may be longer than MAX_BUFFER_SIZE, 
 * we just take the first MAX_BUFFER_SIZE byte and show them
 */
void receive_info(int fd)
{
	// wait for the photon to be ready
	int ret;
	char recv[MAX_BUFFER_SIZE];
	while (!(ret = read(fd, recv, MAX_BUFFER_SIZE))); // wait until receive sth
	if (ret < 0) // if error
	{
		fprintf(stderr, "read from photon error!\n");
		return;
	}
	printf("Received from photon: (%d) %s\r\n", (int)strlen(recv), recv); // receive usage info from photon
	fflush(stdout);
	return;
}

/*
 * receive echo back from photon
 * The echo is never right. This step is just for synchronization.
 */
void receive_echo(int fd)
{
	// wait for the photon to be ready
	int ret;
	char ret_val;
	while (!(ret = read(fd, &ret_val, 1))); // wait until receive sth
	if (ret < 0) // if error
	{
		fprintf(stderr, "read from photon error!\r\n");
		return;
	}
	printf("%c", ret_val); // receive usage info from photon
	fflush(stdout);
	// printf("%c\r\n", ret_val); // immediately print to terminal
	return;
}

// delay for some time, waiting for photon to connect
void delay(int n)
{
	for (int j = 0; j < n; ++j)
		for (int i = 0; i < 1000; ++i);
}

// open port and wait the photon to be ready
int open_and_wait(speed_t speed)
{
	// open port
	int fd;
	while ((fd = open (DEV_NAME, O_RDWR | O_NOCTTY | O_SYNC)) < 0) {
		printf("remember using sudo. wait for photon.\n");
		printf("please check if this lasts too long.\n");
		delay(10000);
	}

	set_interface_attribs (fd, speed, 0);  // set speed to 115,200 bps, 8n1 (no parity)
	set_blocking (fd, 0);                // set no blocking
	
	printf("open serial port successfully!\r\n");

	receive_info(fd); // receive info to sync
	return fd;
}

// write file to photon
void write_file(int fd, const char *filename)
{
	int file, ret;
	char readByte;
	file = open(filename, O_RDONLY);
	if (file < 0)
	{
		fprintf(stderr, "open file error!");
		return;
	}

	// send 's' as a sign of start
	readByte = 's';
	if (write(fd, &readByte, 1) < 0) // try to write this line to serial port
	{
		fprintf(stderr, "write buffer to serial port error!");
		close(file);
		return;
	}
	receive_echo(fd); // receive the echo
	// read one byte at each time
	int lines = 0; // # of lines that read
	while (ret = read(file, &readByte, 1)) { // looping until the EOF
		if (ret < 0) // read error
		{
			fprintf(stderr, "read file error!");
			close(file);
			return;
		}

		if (write(fd, &readByte, 1) < 0) // try to write this line to serial port
		{
			fprintf(stderr, "write buffer to serial port error!");
			close(file);
			return;
		}
		receive_echo(fd); // receive the echo
		// compute current reading lines and break out if read LINE_CNT lines
		if (readByte == '\n')
			lines++;
		if (lines >= LINE_CNT)
			break;
	}
	// send 'f' as a sign of finish
	readByte = 'f';
	if (write(fd, &readByte, 1) < 0) // try to write this line to serial port
	{
		fprintf(stderr, "write buffer to serial port error!");
		close(file);
		return;
	}
	receive_echo(fd); // receive the echo
	close(file);
	printf("\r\n");
	printf("read and write successfully!\r\n");
	return;
}
