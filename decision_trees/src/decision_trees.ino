/*
 * Project decision_trees
 * Description:
 * Author:
 * Date:
 */
#define LED D7

int state = 0;

SYSTEM_MODE(MANUAL);
// setup() runs once, when the device is first turned on.
void setup() {
  // Put initialization like pinMode and begin functions here.
  pinMode(LED, OUTPUT);
  Serial.begin();
}

// loop() runs over and over again, as quickly as it can execute.
void loop() {
  // The core of your code will likely live here.
  digitalWrite(LED, (state) ? HIGH : LOW);
  state = !state;
  Serial.println("hi");
  delay(1000);

}
