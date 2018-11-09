/*
 * Project decision_trees
 * Description:
 * Author:
 * Date:
 */
#include "inference.h"
#define LED D7
#define CS_PIN SS

int state = 0;
SdFat SD;
File file;
float a[9] = {6.4983678,  0.88696843, 0.59811729, -0.45599312, 62240.84, 13285.165, 53057.996, 8.9070396, 8.7032156};

SYSTEM_MODE(MANUAL);
// setup() runs once, when the device is first turned on.
void setup() {
  // Put initialization like pinMode and begin functions here.
  pinMode(LED, OUTPUT);
  calcProd();


  Serial.begin();

  while (!Serial) {
    SysCall::yield();
  }
  Serial.println("Type any character to start");
  while (Serial.read() <= 0) {
    SysCall::yield();
  }
  // Initialize the SD.
  if (!SD.begin(CS_PIN)) {
    errorHalt("begin failed");
  }

  file = SD.open("feat.txt", O_READ);
  if (!file) errorHalt("open failed");

  Serial.println("Starting read ...");
  processData(file);
  Serial.println("done");

  file.close();

}

// loop() runs over and over again, as quickly as it can execute.
void loop() {
  // The core of your code will likely live here.
  digitalWrite(LED, HIGH);
  //digitalWrite(LED, (state) ? HIGH : LOW);
  //state = !state;
  //Serial.print("Test 1: ");
  //Serial.println(accInterval(a));
  /*Serial.print("7: ");
  Serial.println(whichInterval(a, 4, 7));
  Serial.print("11: ");
  Serial.println(whichInterval(a, 4, 11));
  Serial.print("-7: ");
  Serial.println(whichInterval(a, 4, -7));*/
  //delay(1000);

}
