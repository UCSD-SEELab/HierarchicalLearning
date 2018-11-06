/*
* Project: Write certain files to SD card through USB serial.
* Author: Xiaofan Yu
* Date: 10/13/2018
*/
#include "application.h"
#include "sd-card-library-photon-compat.h"

/* global variables for SD card */
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

// specify the file to write
const char filename[] = "boost.txt";

/*
 * testCard - check cardinfo: type, volume, lists of file 
 */
void testCard()
{
	Serial.print("\nInitializing SD card...");

	// we'll use the initialization code from the utility libraries
	// since we're just testing if the card is working!
	// Initialize HARDWARE SPI with user defined chipSelect
	if (!card.init(SPI_HALF_SPEED, chipSelect)) {
		Serial.println("initialization failed!");
		return;
	}
	// Initialize SOFTWARE SPI (uncomment and comment out above line to use)
	/* if (!card.init(mosiPin, misoPin, clockPin, chipSelect)) {
		Serial.println("initialization failed. Things to check:");
		Serial.println("* is a card is inserted?");
		Serial.println("* Is your wiring correct?");
		Serial.println("* did you change the chipSelect pin to match your shield or module?");
		return;
	}
	else {
		Serial.println("Wiring is correct and a card is present.");
	} */

	// print the type of card
	Serial.print("\nCard type: ");
	switch (card.type()) {
	case SD_CARD_TYPE_SD1:
		Serial.println("SD1");
		break;
	case SD_CARD_TYPE_SD2:
		Serial.println("SD2");
		break;
	case SD_CARD_TYPE_SDHC:
		Serial.println("SDHC");
		break;
	default:
		Serial.println("Unknown");
	}

	// Now we will try to open the 'volume'/'partition' - it should be FAT16 or FAT32
	if (!volume.init(card)) {
		Serial.println("Could not find FAT16/FAT32 partition.\nMake sure you've formatted the card");
		return;
	}

	// print the type and size of the first FAT-type volume
	uint32_t volumesize;
	Serial.print("\nVolume type is FAT");
	Serial.println(volume.fatType(), DEC);
	Serial.println();

	volumesize = volume.blocksPerCluster();    // clusters are collections of blocks
	volumesize *= volume.clusterCount();       // we'll have a lot of clusters
	volumesize *= 512;                         // SD card blocks are always 512 bytes
	Serial.print("Volume size (bytes): ");
	Serial.println(volumesize);
	Serial.print("Volume size (Kbytes): ");
	volumesize /= 1024;
	Serial.println(volumesize);
	Serial.print("Volume size (Mbytes): ");
	volumesize /= 1024;
	Serial.println(volumesize);

	Serial.println("\nFiles found on the card (name, date and size in bytes): ");
	root.openRoot(volume);

	// list all files in the card with date and size
	root.ls(LS_R | LS_DATE | LS_SIZE);
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
	Serial.println("SD init done");
}

/*
 * check the validation of read byte, to get rid of interfere
 */
inline bool check_validation(char byte) {
	if (byte >= '0' && byte <= '9')
		return 1;
	if (byte == '.')
		return 1;
	if (byte == '<')
		return 1;
	if (byte == '-')
		return 1;
	if (byte == '\n' || byte == '\r' || byte == ' ')
		return 1;
	return 0;
}

/*
 * write_file - write file to SD card and read it back if necessary
 */
void write_file()
{
	// open the file. note that only one file can be open at a time,
	myFile = SD.open(filename, O_WRITE | O_CREAT | O_TRUNC);

	// if the file opened okay, write to it:
	bool flag = 0;
	if (myFile) {
		while (1) {
			if (Serial.available()) {
				// receive one byte each time from USB serial
				char byte = Serial.read();
				Serial.write(byte); // echo back
				// add 's' as start sign, 'f' as stop sign, to secure the message
				if (byte == 's') { // start writing
					flag = 1;
					continue;
				}
				else if (byte == 'f') // finish writing
					break;
				if (flag && check_validation(byte))
					myFile.write(byte);
			}
		}
		// close the file:
		myFile.close();
	}
	else {
		// if the file didn't open, print an error:
		Serial.println("error opening txt");
	}
}

/*
* check-content - check the content of the file just written, for debug use
*/
void check_content() {
	myFile = SD.open(filename);
	if (myFile) {
		// read from the file until there's nothing else in it:
		Serial.println("Content for file");
		while (myFile.available()) {
			char byte = myFile.read();
			Serial.write(byte);
		}
		// close the file:
		myFile.close();
	}
	else {
		// if the file didn't open, print an error:
		Serial.println("error opening file");
	}
}

void setup() {
	// make it simple, open and write
	Serial.begin(9600);
	
	// The followings are for writing files
	// init_card(); // init first
	// write_file();

	// The followings are for testing file content
	testCard();	
	init_card();
	check_content();
}

void loop() {
}
