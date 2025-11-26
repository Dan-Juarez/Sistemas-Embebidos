#include <SoftwareSerial.h>

// HC-06 en pines 10 (RX), 11 (TX)
SoftwareSerial BT(10, 11);  

const int pinEndstop = 2;      // Endstop NO
const int pinBomba   = 9;      // Transistor 2N3904

// Tiempos de bombeo (ajusta a tu gusto)
unsigned long tiempo10  = 500;    // ms para 10 ml
unsigned long tiempo50  = 2400;   // ms para 50 ml
unsigned long tiempo100 = 4800;   // ms para 100 ml
unsigned long tiempo300 = 14000;  // ms para 300 ml

// Estado
bool vasoPresente = false;

enum Estado { ESPERANDO, DISPENSANDO };
Estado estado = ESPERANDO;

unsigned long inicioDispenso = 0;
unsigned long tiempoObjetivo = 0;

void setup() {
  pinMode(pinEndstop, INPUT_PULLUP);   // LOW = vaso
  pinMode(pinBomba, OUTPUT);
  digitalWrite(pinBomba, LOW);

  Serial.begin(9600);    
  BT.begin(9600);        // Bluetooth HC-06

  Serial.println("Sistema listo.");
}

void loop() {
  // 1) Leer estado del vaso
  bool vasoAhora = (digitalRead(pinEndstop) == LOW);

  // Si cambió el estado, avisar al BT
  if (vasoAhora != vasoPresente) {
    vasoPresente = vasoAhora;
    if (vasoPresente) {
      BT.println("V1");       // Vaso presente
      Serial.println("Vaso presente.");
    } else {
      BT.println("V0");       // Vaso NO presente
      Serial.println("Sin vaso.");
      // Si estaba dispensando, cortar de inmediato
      if (estado == DISPENSANDO) {
        detenerBomba("STOP_VASO");
      }
    }
  }

  // 2) Revisar si llegó algún comando por Bluetooth
  if (BT.available()) {
    char cmd = BT.read();
    Serial.print("Cmd recibido: ");
    Serial.println(cmd);

    manejarComando(cmd);
  }

  // 3) Si estamos dispensando, controlar el tiempo
  if (estado == DISPENSANDO) {
    unsigned long ahora = millis();

    // Si se acabó el tiempo Y el vaso sigue presente
    if (vasoPresente && (ahora - inicioDispenso >= tiempoObjetivo)) {
      detenerBomba("OK_DONE");   // terminó correctamente
    }

    // Si el vaso se quitó ya lo corta la sección de arriba
  }
}

void manejarComando(char cmd) {
  // Solo aceptar comando si hay vaso
  if (!vasoPresente) {
    BT.println("ERR_NO_VASO");
    Serial.println("Comando ignorado: no hay vaso.");
    return;
  }

  // Solo aceptar si está en estado ESPERANDO
  if (estado != ESPERANDO) {
    BT.println("ERR_OCUPADO");
    Serial.println("Comando ignorado: ya está dispensando.");
    return;
  }

  switch (cmd) {
    case 'A': tiempoObjetivo = tiempo10;  break;   // 10 ml
    case 'B': tiempoObjetivo = tiempo50;  break;   // 50 ml
    case 'C': tiempoObjetivo = tiempo100; break;   // 100 ml
    case 'D': tiempoObjetivo = tiempo300; break;   // 300 ml
    default:
      BT.println("ERR_CMD");
      Serial.println("Comando desconocido.");
      return;
  }

  // Iniciar bombeo
  inicioDispenso = millis();
  estado = DISPENSANDO;
  digitalWrite(pinBomba, HIGH);

  BT.println("START");
  Serial.println("Bomba ON.");
}

void detenerBomba(const char* mensaje) {
  digitalWrite(pinBomba, LOW);
  estado = ESPERANDO;
  tiempoObjetivo = 0;

  BT.println(mensaje);
  Serial.print("Bomba OFF: ");
  Serial.println(mensaje);
}
