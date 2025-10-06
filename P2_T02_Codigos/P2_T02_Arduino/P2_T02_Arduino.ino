#include <Stepper.h>

const int stepsPerRevolution = 2048; // motor 28BYJ-48
// Pines del ULN2003 conectados al Arduino
Stepper motor(stepsPerRevolution, 8, 10, 9, 11);

void setup() {
  Serial.begin(9600);
  motor.setSpeed(10);  // velocidad inicial
}

void loop() {
  if (Serial.available()) {
    char comando = Serial.read();

    if (comando == 'H') {
      motor.step(stepsPerRevolution / 10);  // giro horario
    }
    else if (comando == 'A') {
      motor.step(-stepsPerRevolution / 10); // giro antihorario
    }
    else if (comando == '1') {
      motor.setSpeed(5);  // lento
    }
    else if (comando == '2') {
      motor.setSpeed(10); // medio
    }
    else if (comando == '3') {
      motor.setSpeed(15); // rápido
    }
  }
}
