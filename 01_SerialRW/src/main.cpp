// Converted from Sketch_01.1_SerialRW.ino (Freenove FNK0104B tutorial, chapter 1)
#include <Arduino.h>

String inputString = "";      //a String to hold incoming data
bool stringComplete = false;  // whether the string is complete

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println(String("\nESP32-S3 initialization completed!\n")
                + String("Please input some characters,\n")
                + String("select \"Newline\" below and click send button. \n"));
}

void loop() {
  if (Serial.available()) {         // judge whether data has been received
    char inChar = Serial.read();         // read one character
    inputString += inChar;
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
  if (stringComplete) {
    Serial.printf("InputString: %s", inputString);
    inputString = "";
    stringComplete = false;
  }
}
