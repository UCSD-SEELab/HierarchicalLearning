/*
 * Project: Establish MQTT connection with RPi and send local data
 * Suppose necessary files are alreay written on the SD card:
 *     data.txt (extracted_subject1.txt)
 * Author: Xiaofan Yu
 * Date: 10/13/2018
 */
#define NUM_OF_SR 11 // number of sameple rate choices
#define NUM_OF_BW 3 // number of bandwidth choices
#define FEATURE_NUM 12 // number of floats read from each line
#define MAX_BATCH_LENGTH 9600 // maximum length of sending batch
#define MAX_INFO_LENGTH 100 // maximum length of sending info, e.g. bw and sample rate
#include "application.h"
#include "MQTT.h"
#include "sd-card-library-photon-compat.h"

// #define DEBUG_LOCAL_CONNECTOR // DEBUG mode
#ifdef DEBUG_LOCAL_CONNECTOR
 #define DEBUG_PRINTLN  Serial.println
 #define DEBUG_PRINT Serial.print
 #define DEBUG_PRINTF Serial.printf
#else
 #define DEBUG_PRINTLN
 #define DEBUG_PRINT
 #define DEBUG_PRINTF
#endif

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
MQTT client(server, 1883, 60, callback, MAX_BATCH_LENGTH+10); // ip, port, keepalive, callback, maxpacketsize=

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
uint32_t position = 0; // used to remember the last reading position when open filename
// specify the file recording sleep time, tmp.txt
const char tmpname[] = "tmp.txt";
char write_buffer[MAX_INFO_LENGTH];

// string and float to hold float input
String inString = "";
float inFloat;
uint8_t batch_data[MAX_BATCH_LENGTH+10]; // send batch, can not use string cuz it is too long
uint32_t len_of_batch = 0; // current length of batch_data
char send_info[MAX_INFO_LENGTH];
unsigned long prev_time, cur_time; // to store measured time
unsigned long read_start, read_time; // to record the start and time of reading
unsigned long ready_time; // total time of reading
long sleep_time; // to store time, should be signed

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

	Serial.println("done");
}

/*
 * batch_append - append a new float data to the sending batch
 * Return - 0 fail, exceed maximum length
 *			1 success
 */
inline uint8_t batch_append(float data) {
	if (len_of_batch >= MAX_BATCH_LENGTH-2) // no placy to append, return failure
		return 0;
	uint8_t *data_byte = (uint8_t *)&data; // convert the pointer
	batch_data[len_of_batch++] = *(data_byte); // append lower byte to send batch
	batch_data[len_of_batch++] = *(data_byte + 1); // append higher byte to send batch
	return 1;
}

/*
 * read_line - read one line from myFile, each line read first readcnt floats in 36 floats
 */
inline void read_line(unsigned int readcnt) {
	inString = ""; // clear string
	// read readcnt float from one line
	for (int i = 0; i < readcnt && myFile.available(); ++i) {
		// read until finish reading one float
		char byte = myFile.read();
		while (byte != ' ' && byte != '\n') {
			inString += byte;
			byte = myFile.read();
		}
		if (byte == '\n') // accidentally meet the end of line, return
			return;
		// finish reading one float, convert it
		float inFloat = inString.toFloat();
		DEBUG_PRINTF("%f ", inFloat);
		batch_append(inFloat); // append to the sending batch
		inString = ""; // clear inString
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
 * read_file - read sample_rate lines from myFile and append the data to sending batch
 * parameter - cur_sr current sample rate
 * Return - 0 not end
 *			1 meet end of file (EOF)
 */
uint8_t read_file(uint16_t cur_sr) {
	myFile = SD.open(filename); // open the file
	myFile.seek(position); // locate the last time of reading
	if (myFile) {
		for (int j = 0; j < cur_sr && myFile.available(); ++j) { // read sample_rate lines from myFile
			read_start = millis();
			read_line(FEATURE_NUM);
			read_time += millis() - read_start;
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
		// final pub
		if (client.isConnected()) {
			client.publish(tmp_topic, batch_data, len_of_batch, false, client.QOS2, false, NULL);
			while (!client.loop_QoS2()); // block and wait for pub done
			Serial.println("publish all parts of tmp.txt successfully"); // note success in publishing
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
			myFile = SD.open(tmpname, O_WRITE | O_TRUNC);
			if (myFile)
				myFile.close();
			else
				Serial.println("error opening file");

			// start reading!
			prev_time = millis();
			// 
		Serial.printf("prev_time %ld\n", prev_time);
			while (1) {
				len_of_batch = 0; // clear sending batch
				read_time = 0; // clear and ready for cumulative add
				// read sample_rate lines, file is opened in this function
				uint8_t end = read_file(sample_rate[sample_index]);
				ready_time = millis() - prev_time;
					
				// publish
				if (client.isConnected()) {
					// topic, payload, plength, retain, qos, dup, messageid
					client.publish(data_topic, batch_data, len_of_batch, false, client.QOS2, false, NULL);
					while (!client.loop_QoS2()); // block and wait for pub done
					Serial.printf("%d %d %d publish successfully\r\n", bw[bw_index], sample_rate[sample_index], len_of_batch);
				}
				
				cur_time = millis();
				// Serial.printf("cur_time %ld\r\n", cur_time);
				sleep_time = 1000 - (cur_time - prev_time);
				if (sleep_time > 0) {
					delay(sleep_time); // delay sleep_time milliseconds
					sprintf(write_buffer, "S\tprev:%ld\tcur:%ld\tsleep:%ld\tlen:%d\tr:%ld\tready:%ld\r\n", 
						prev_time, cur_time, sleep_time, len_of_batch, read_time, ready_time);
				}
				else {
					sprintf(write_buffer, "N\tprev:%ld\tcur:%ld\tsleep:%ld\tlen:%d\tr:%ld\tready:%ld\r\n", 
						prev_time, cur_time, sleep_time, len_of_batch, read_time, ready_time);
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
