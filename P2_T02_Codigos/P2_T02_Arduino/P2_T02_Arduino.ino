#include <Stepper.h>

const int stepsPerRevolution = 200;  // Ajusta según tu motor
Stepper myStepper(stepsPerRevolution, 8, 9, 10, 11);

String inputString = "";
boolean stringComplete = false;
int velocidad = 60; // default

void setup() {
  Serial.begin(9600);
  myStepper.setSpeed(velocidad);
}

void loop() {
  if (stringComplete) {
    if (inputString.startsWith("P")) {
      int pasos = inputString.substring(1).toInt();
      myStepper.step(pasos);
    } else if (inputString == "H") {
      myStepper.step(stepsPerRevolution);
    } else if (inputString == "A") {
      myStepper.step(-stepsPerRevolution);
    } else if (inputString == "1") {
      velocidad = 30;
      myStepper.setSpeed(velocidad);
    } else if (inputString == "2") {
      velocidad = 60;
      myStepper.setSpeed(velocidad);
    } else if (inputString == "3") {
      velocidad = 120;
      myStepper.setSpeed(velocidad);
    }
    inputString = "";
    stringComplete = false;
  }
}

void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    if (inChar == '\n') {
      stringComplete = true;
    } else {
      inputString += inChar;
    }
  }
}
