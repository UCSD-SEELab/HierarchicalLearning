/*
 * Project: Establish MQTT connection with RPi and send data after run linear regression
 * Suppose necessary files are alreay written on the SD card:
 *     data.txt (extracted_subject1.txt)
 * Author: Xiaofan Yu
 * Date: 11/2/2018
 */
// put it here to avoid "was not declared error"
#define NUM_OF_SR 11 // number of sameple rate choices
#define NUM_OF_BW 3 // number of bandwidth choices
#define MAX_INFO_LENGTH 100 // maximum length of sending info, e.g. bw and sample rate
#define MAX_RAND_NUM 1000000 // maximum number in random()
#define FEATURE_NUM 12 // number of features, linear regression input
#define CLASSES_NUM 18 // number of classes, linear regression output
#define MAX_SAMPLE_RATE 400 // maximum sample rate
#define HALF_LOOKUP_LENGTH 25 // half of lookup table's length
#define LOOKUP_LENGTH 50 // lookup table is [0, 50]
#define HALF_LOOKUP_RANGE 5 // half of the numerical range 

#include "application.h"
#include "MQTT.h"
#include "sd-card-library-photon-compat.h"
#include "math.h"

// const variables in experiments
const int sample_rate[] = {200, 220, 240, 260, 280, 300, 320, 340, 360, 380, 400};
const int bw[] = {200, 700, 5000};

// define all categories of topics
const char data_topic[] = "house.hand"; // for sending data
const char tmp_topic[] = "tmp"; // for sending tmp.txt
const char parameter_topic[] = "parameter"; // for sending bandwidth and sample rate
const char sync_topic[] = "sync"; // for sync, send current time

/* global variables for MQTT */
void callback(char* topic, byte* payload, unsigned int length);

byte server[] = { 137,110,160,230 }; // specify the ip address of RPi
MQTT client(server, 1883, 60, callback, MAX_SAMPLE_RATE+10); // ip, port, keepalive, callback, maxpacketsize=

/* glabal variables for SD card */
File myFile;
// set up variables using the SD utility library functions:
Sd2Card card;
SdVolume volume;
SdFile root;

// SOFTWARE SPI pin configuration - modify as required
// The default pins are the same as HARDWARE SPI
const uint8_t chipSelect = A2;    // Also used for HARDWARE SPI setup
const uint8_t mosiPin = A5;
const uint8_t misoPin = A4;
const uint8_t clockPin = A3;

// specify the file storing data, extracted_subject1.txt
const char filename[] = "data.txt";
unsigned long position = 0; // used to remember the last reading position when open filename
// specify the file recording sleep time, tmp.txt
const char tmpname[] = "tmp.txt";
char write_buffer[MAX_INFO_LENGTH]; // to store the info sent to RPi
// specify the file containing linear regression parameters
// const char boostname[] = "lr.txt";

// global array to store float data
float lr_para[FEATURE_NUM][CLASSES_NUM]; // parameters for linear regression
float readLine[FEATURE_NUM]; // to store a line
uint8_t batch_data[MAX_SAMPLE_RATE+10]; // send batch
unsigned int len_of_batch = 0; // len of send batch
// global array to store assistant info
char send_info[MAX_INFO_LENGTH];
unsigned long prev_time, cur_time; // to store measured time
unsigned long comp_start, comp_time; // to record the start and time of computation
unsigned long read_start, read_time; // to record the start and time of reading
unsigned long ready_time; // total time of computation and reading
long sleep_time; // to store sleep time, should be signed

/*************************************************************************
 * lookup table settings and functions
 ************************************************************************/
const float RATIO = HALF_LOOKUP_LENGTH/HALF_LOOKUP_RANGE;
float lookup_table[LOOKUP_LENGTH+1];

/*
 * init_lookup_table
 */
void init_lookup_table() {
	float e = 2.718281828;
	for (uint8_t i = 0; i < LOOKUP_LENGTH+1; ++i) {
		//i=0->e^(5) ~ i=LOOKUP_LENGTH->e^(-5)
		float exp = (float)(-i)/RATIO+HALF_LOOKUP_RANGE;
		lookup_table[i] = pow(e, exp);
		// Serial.printf("%d %f %f\r\n", i, exp, lookup_table[i]); // print out for check
	}
}

/*
 * sigmoid_lookup - use lookup table to calculate e^(-x)
 */
inline float sigmoid_lookup(float x) {
	if (x > HALF_LOOKUP_RANGE) // X > 5, return 0
		return 0;
	if (-x > HALF_LOOKUP_RANGE) // x < -5, return e^(-5)
		return lookup_table[LOOKUP_LENGTH];
	uint8_t index = floor((-1)*(x-HALF_LOOKUP_RANGE)*RATIO);
	return lookup_table[index];
}

/*
 * sigmoid - return e^(-x)
 * Could be replaced by a look-up table in the future
 */
inline float sigmoid(float x) {
	float e = 2.718281828;
	return pow(e, -x);
}

/*
 * init_lr_parameter
 */
void init_lr_parameter(float para_array[FEATURE_NUM][CLASSES_NUM]) {
	long randNum;
	for (uint8_t i = 0; i < FEATURE_NUM; ++i)
		for (uint8_t j = 0; j < CLASSES_NUM; ++j) {
			randNum = random(0, MAX_RAND_NUM);
			lr_para[i][j] = ((float)randNum) / MAX_RAND_NUM;
			// Serial.printf("%f ", para_num[i][j]);
		}
}

/*
 * softmax_index, matrix multiply, return argmax-index
 * Parameter - readLine: one line of data, its size is 1*features
 *			   para_array: theta, its size is features*classes
 *			   features, classes: size of matrixes
 * Return - max_index: the index with the max value
 */
uint8_t softmax_index(float *readLine, float para_array[FEATURE_NUM][CLASSES_NUM], 
	unsigned int features, unsigned int classes) {
	float max_result = 0, tmpRes; // place to store temporary multiply and sum result
	uint8_t max_index = 0;
	for (uint8_t j = 0; j < classes; ++j) { // j is current class
		tmpRes = 0; // clear result for each class
		for (uint8_t k = 0; k < features; ++k) { // k is current feature
			tmpRes += readLine[k] * lr_para[k][j];
			// Serial.printf("%f %f %f\r\n", tmpRes, readLine[k], lr_para[k][j]);
		}
		tmpRes = sigmoid(tmpRes); // use lookup table here
		// Serial.printf("tmpRes: %f\r\n", tmpRes);
		if (tmpRes > max_result) {
			max_result = tmpRes;
			max_index = j;
		}
	}
	return max_index;
}

/*
 * softmax, matrix multiply, return max index and softmax array
 * Parameter - readLine: one line of data, its size is 1*features
 *			   para_array: theta, its size is features*classes
 *			   features, classes: size of matrixes
 *			   softmax: returned softmax array, its size is 1*classes
 * Return - max_index: the index with the max value
 */
uint8_t softmax_array(float *readLine, float para_array[FEATURE_NUM][CLASSES_NUM], 
	unsigned int features, unsigned int classes, float *softmax) {
	float max_result = 0, tmpRes; // place to store temporary multiply and sum result
	uint8_t max_index = 0;
	float sum = 0; // regularize for softmax regression
	for (uint8_t j = 0; j < classes; ++j) { // j is current class
		tmpRes = 0; // clear result for each class
		for (uint8_t k = 0; k < features; ++k) { // k is current feature
			tmpRes += readLine[k] * lr_para[k][j];
			// Serial.printf("%f %f %f\r\n", tmpRes, readLine[k], lr_para[k][j]);
		}
		softmax[j] = tmpRes = sigmoid(tmpRes); // use lookup table here
		// Serial.printf("%f\r\n", tmpRes);
		if (tmpRes > max_result) {
			max_result = tmpRes;
			max_index = j;
		}
		sum += tmpRes;
	}
	// each element divide by sum. regularize to 1.
	for (uint8_t j = 0; j < classes; ++j)
		softmax[j] /= sum;
	return max_index;
}

/*************************************************************************
 * Other settings for local connector
 ************************************************************************/
/*
 * Receive Message - Not used here
 */
void callback(char* topic, byte* payload, unsigned int length) {
}

/*
 * init_card - init SD card, used most often in read and write
 */
void init_card() {
	// Initialize HARDWARE SPI with user defined chipSelect
	if (!SD.begin(chipSelect)) {
		Serial.println("error");
		return;
	}

	Serial.println("SD card init done");
}


/*
 * read_line - read the first readcnt floats in a line from myFile
 * Parameter - Line: a line to store the floats
 *			 - readcnt: number of floats to read from this line
 */
void read_line(float *Line, unsigned int readcnt) {
	String readString = ""; // clear string
	// read FEATURE_NUM float from one line, note readcnt should less than max-of-uint16_t
	for (int i = 0; i < readcnt && myFile.available(); ++i) {
		// read until finish reading one float
		char byte = myFile.read();
		while (byte != ' ' && byte != '\n') {
			readString += byte;
			byte = myFile.read();
		}
		if (byte == '\n') // accidentally meet the end of line, return
			return;
		// finish reading one float, convert it
		float readFloat = readString.toFloat();
		Line[i] = readFloat; // add the float to readLine
		readString = ""; // clear inString
	}
	// if read until end of line, move on
	while (myFile.available()) {
		char byte = myFile.read();
		if (byte == '\n')
			break;
	}
	return;
}

/*
 * read_data_file - read sample_rate lines from myFile and append the data to sending batch
 * parameter - readFile: the 2-D array to store reading floats
 *			   cur_sr： current sample rate, number of lines to read
 *			   readcnt: number of features to read in one line
 * Return - 0 not end
 *			1 meet end of file (EOF)
 * Set sample_num! A global variable
 */
uint8_t read_data_file(unsigned int cur_sample_rate) {
	myFile = SD.open(filename); // open the file
	myFile.seek(position); // locate the last time of reading
	if (myFile) {
		// read sample_rate lines from myFile, note cur_sample_rate should be less than max-of-uint16_t
		for (int i = 0; i < cur_sample_rate && myFile.available(); ++i) {
			read_start = millis();
			// read FEATURE floats in one line and store them in readLine
			read_line(readLine, FEATURE_NUM);
			comp_start = millis();
			read_time += comp_start - read_start; // cumulative add
			// compute argmax-index data for this line
			uint8_t max_index = softmax_index(readLine, lr_para, FEATURE_NUM, CLASSES_NUM);
			// pack the result into send batch
			batch_data[len_of_batch++] = max_index;
			comp_time += millis() - comp_start; // cumulative add
		}
		
		// update position
		if (myFile.available()) {
			position = myFile.position(); // remember the position of last open time
			myFile.close();
			return 0;
		}
		// finish one sample rate!
		position = 0;
		myFile.close();
		return 1;
	}
	// open error!
	Serial.println("error opening file");
	return 1;
}

/*
 * write_sleep_time - write current time and sleep time to tmp.txt
 */
void write_sleep_time(char *buf) {
	// open the file. note that only one file can be open at a time,
	myFile = SD.open(tmpname, O_WRITE | O_APPEND);
	if (myFile) {
		myFile.write((const uint8_t *)buf, (size_t)strlen(buf));
		myFile.close();
	}
	else {
		Serial.println("error opening file");
	}
}

/*
 * send_sleep_time - send tmp.txt for this round of bandwidth and sample rate
 */
void send_sleep_time() {
	myFile = SD.open(tmpname); // open to read
	if (myFile) {
		len_of_batch = 0; // clear the sending batch
		while (myFile.available()) {
			char byte = myFile.read();
			// Serial.write(byte); // for reference
			batch_data[len_of_batch++] = (uint8_t)byte; // append lower byte to send batch
		}
		// publish tmp.txt
		if (client.isConnected()) {
			client.publish(tmp_topic, batch_data, len_of_batch, false, client.QOS2, false, NULL);
			while (!client.loop_QoS2()); // block and wait for pub done
			// Serial.println("publish all parts of tmp.txt successfully"); // note success in publishing
		}
		myFile.close();
	}
	else {
		Serial.println("error opening file");
	}
}

/*
 * setup
 */
void setup() {
	Serial.begin(115200);
	init_card(); // init sd card
	init_lr_parameter(lr_para); // init linear regression parameters
	init_lookup_table();

	// connect to the RPi
	// client_id, user, passwd, willTopic, willQoS, willRetain, willMessage, cleanSession, version?
	client.connect("photon", "xiaofan", "0808", 0, client.QOS2, 0, 0, true, client.MQTT_V311);
	Serial.println("Connect!");

	for (uint8_t bw_index = 0; bw_index < NUM_OF_BW; bw_index++) {
		for (uint8_t sample_index = 0; sample_index < NUM_OF_SR; sample_index++) {
			// set bandwidth and sample rate
			sprintf(send_info, "%d %d", bw[bw_index], sample_rate[sample_index]);
			if (client.isConnected()) {
				client.publish(parameter_topic, (uint8_t *)send_info, strlen(send_info), false, client.QOS2, false, NULL);
				while (!client.loop_QoS2()); // block and wait for pub done
			}
			// sync time
			sprintf(send_info, "%ld", millis());
			if (client.isConnected()) {
				client.publish(sync_topic, (uint8_t *)send_info, strlen(send_info), false, client.QOS2, false, NULL);
				while (!client.loop_QoS2()); // block and wait for pub done
			}

			// clear tmp.txt
			myFile = SD.open(tmpname, O_WRITE | O_CREAT | O_TRUNC);
			if (myFile)
				myFile.close();
			else
				Serial.println("error opening file");

			// start reading!
			prev_time = millis();
			// Serial.printf("prev_time %ld\n", prev_time);
			while (1) {
				len_of_batch = 0;
				read_time = comp_time = 0; // clear and ready for cumulative add
				// read sample_rate lines and do linear regression after reading each line
				uint8_t end = read_data_file(sample_rate[sample_index]);
				ready_time = millis() - prev_time; // total time in read and comp
					
				// publish
				if (client.isConnected()) {
					// topic, payload, plength, retain, qos, dup, messageid
					client.publish(data_topic, batch_data, len_of_batch, false, client.QOS2, false, NULL);
					while (!client.loop_QoS2()); // block and wait for pub done
					Serial.printf("%d %d %d publish successfully\r\n", bw[bw_index], 
						sample_rate[sample_index], len_of_batch);
				}
				
				cur_time = millis();
				// Serial.printf("cur_time %ld\r\n", cur_time);
				sleep_time = 1000 - (cur_time - prev_time);
				if (sleep_time > 0) {
					delay(sleep_time); // delay sleep_time milliseconds
					sprintf(write_buffer, "S\tprev:%ld\tcur:%ld\tsleep:%ld\tlen:%d\tc:%ld\tr:%ld\tready:%ld\r\n", 
						prev_time, cur_time, sleep_time, len_of_batch, comp_time, read_time, ready_time);
				}
				else {
					sprintf(write_buffer, "N\tprev:%ld\tcur:%ld\tsleep:%ld\tlen:%d\tc:%ld\tr:%ld\tready:%ld\r\n", 
						prev_time, cur_time, sleep_time, len_of_batch, comp_time, read_time, ready_time);
					Serial.println("N");
				}
				write_sleep_time(write_buffer);
				Serial.print(write_buffer);

				prev_time = millis(); // update prev_time
				// Serial.printf("prev_time %ld\r\n", prev_time);
				
				if (end) // read until the end of file, ending this round of bandwidth and sample rate
					break;
			}
			send_sleep_time(); // send tmp.txt for this round
		}
	}

	client.disconnect();
}

/*
 * loop
 */
void loop() {
}
