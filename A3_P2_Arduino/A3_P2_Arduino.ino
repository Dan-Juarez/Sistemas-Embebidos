int velocidad = 300; // valor inicial del delay en ms

void setup() {
  Serial.begin(9600);
  for (int i = 2; i <= 5; i++) { 
    pinMode(i, OUTPUT);
  }
}

void loop() {
  if (Serial.available()) {
    char opcion = Serial.read();
    switch (opcion) {
      case '1': secuenciaIzqDer(); break;
      case '2': secuenciaDerIzq(); break;
      case '3': todosParpadean(); break;
      case '4': secuenciaAleatoria(); break;   // Nueva secuencia (Reto)
      case '5': cambiarVelocidad(); break;     // Cambiar velocidad (Reto)
    }
  }
}

void secuenciaIzqDer() {
  for (int i = 2; i <= 9; i++) {
    digitalWrite(i, HIGH);
    delay(velocidad);
    digitalWrite(i, LOW);
  }
}

void secuenciaDerIzq() {
  for (int i = 5; i >= 2; i--) {
    digitalWrite(i, HIGH);
    delay(velocidad);
    digitalWrite(i, LOW);
  }
}

void todosParpadean() {
  for (int i = 2; i <= 5; i++) digitalWrite(i, HIGH);
  delay(velocidad);
  for (int i = 2; i <= 5; i++) digitalWrite(i, LOW);
  delay(velocidad);
}

//NUEVA SECUENCIA ALEATORIA
void secuenciaAleatoria() {
  for (int i = 0; i < 20; i++) { // repite 20 encendidos aleatorios
    int pin = random(2, 6);     // elige un pin aleatorio entre 2 y 5
    digitalWrite(pin, HIGH);
    delay(velocidad);
    digitalWrite(pin, LOW);
  }
}

//CAMBIAR VELOCIDAD (rápido/lento)
void cambiarVelocidad() {
  if (velocidad == 300) {
    velocidad = 100; // más rápido
  } else {
    velocidad = 300; // normal
  }
}
