#include <Servo.h>

// ------------ Servos ------------
Servo servoDisco;   // MG995 (disco)
Servo servoRampa;   // 9g (rampa)

// Pines servos
const int pinServoDisco = 9;   // MG995
const int pinServoRampa = 6;   // 9g

// Angulos del disco (MG995)
const int POS_DISCO_DESCARGA = 0;    // Hoyo hacia la rampa
const int POS_DISCO_SENSOR   = 90;   // Sensor de color
const int POS_DISCO_RECOGER  = 180;  // Recoger dulce

// Angulos de la rampa (9g) según color
const int R_AMARILLO = 0;    // Bandeja 1
const int R_VERDE    = 30;   // Bandeja 2
const int R_ROJO     = 60;   // Bandeja 3
const int R_NARANJA  = 90;   // Bandeja 4
const int R_MORADO   = 140;  // Bandeja 5

// --------- Enumeración de colores ---------
enum ColorDulce {
  COLOR_AMARILLO,
  COLOR_VERDE,
  COLOR_ROJO,
  COLOR_NARANJA,
  COLOR_MORADO,
  COLOR_DESCONOCIDO
};

// --------- Pines TCS3200 (GY-31) ----------
const int S0_PIN   = 2;
const int S1_PIN   = 3;
const int S2_PIN   = 4;
const int S3_PIN   = 5;
const int OUT_PIN  = 7;

// Ángulo actual del disco (para movimiento suave)
int anguloDiscoActual = POS_DISCO_DESCARGA;

// --------- Centroides normalizados rN,gN,bN (con disco negro) ---------
// Calculados promediando tus 10 lecturas por color
struct ColorRef {
  float r;
  float g;
  float b;
  ColorDulce color;
  const char *name;
};

ColorRef refs[] = {
  //                rN       gN       bN
  {0.4188f, 0.3243f, 0.2568f, COLOR_AMARILLO, "AMARILLO"},
  {0.3404f, 0.3440f, 0.3156f, COLOR_VERDE,    "VERDE"},
  {0.4010f, 0.2723f, 0.3267f, COLOR_ROJO,     "ROJO"},
  {0.4523f, 0.2632f, 0.2846f, COLOR_NARANJA,  "NARANJA"},
  {0.3446f, 0.2997f, 0.3557f, COLOR_MORADO,   "MORADO"},
};
const int NUM_COLORS = sizeof(refs) / sizeof(refs[0]);

// --------- Movimiento suave del disco (MG995) ---------
void moverDiscoSuave(int destino) {
  int desde = anguloDiscoActual;

  if (desde < destino) {
    for (int pos = desde; pos <= destino; pos++) {
      servoDisco.write(pos);
      delay(15);   // más grande = más suave pero más lento
    }
  } else {
    for (int pos = desde; pos >= destino; pos--) {
      servoDisco.write(pos);
      delay(15);
    }
  }

  anguloDiscoActual = destino;
}

// --------- Mover rampa según color detectado ---------
void moverRampaPorColor(ColorDulce color) {
  int anguloRampa = R_AMARILLO; // por defecto

  switch (color) {
    case COLOR_AMARILLO: anguloRampa = R_AMARILLO; break;
    case COLOR_VERDE:    anguloRampa = R_VERDE;    break;
    case COLOR_ROJO:     anguloRampa = R_ROJO;     break;
    case COLOR_NARANJA:  anguloRampa = R_NARANJA;  break;
    case COLOR_MORADO:   anguloRampa = R_MORADO;   break;
    case COLOR_DESCONOCIDO:
    default:
      // Podrías mandar desconocidos a una bandeja “basura”
      anguloRampa = R_AMARILLO;
      break;
  }

  servoRampa.write(anguloRampa);
  Serial.print("Rampa movida a: ");
  Serial.print(anguloRampa);
  Serial.println(" grados.");
}

// --------- Leer frecuencia de un "filtro" de color ---------
unsigned long leerFrecuenciaColor(bool s2State, bool s3State) {
  digitalWrite(S2_PIN, s2State);
  digitalWrite(S3_PIN, s3State);

  delay(50);  // dejar estabilizar

  unsigned long duracion = pulseIn(OUT_PIN, LOW, 250000); // timeout 250 ms
  if (duracion == 0) {
    duracion = 250000;
  }
  unsigned long frecuencia = 1000000UL / duracion;
  return frecuencia;
}

// --------- Leer RGB en una sola medición ---------
void leerRGB(unsigned long &r, unsigned long &g, unsigned long &b) {
  // S2 S3:
  // L L -> rojo
  // L H -> azul
  // H H -> verde

  // Rojo
  r = leerFrecuenciaColor(LOW, LOW);

  // Azul (B)
  unsigned long azul = leerFrecuenciaColor(LOW, HIGH);

  // Verde
  g = leerFrecuenciaColor(HIGH, HIGH);

  b = azul;
}

// --------- Leer varias veces y hacer promedio normalizado ---------
void leerRGBPromediado(float &rN, float &gN, float &bN, int muestras = 5) {
  unsigned long sumR = 0, sumG = 0, sumB = 0;

  for (int i = 0; i < muestras; i++) {
    unsigned long R, G, B;
    leerRGB(R, G, B);
    sumR += R;
    sumG += G;
    sumB += B;
  }

  float Ravg = sumR / (float)muestras;
  float Gavg = sumG / (float)muestras;
  float Bavg = sumB / (float)muestras;

  float sum = Ravg + Gavg + Bavg;
  if (sum == 0) sum = 1.0;

  rN = Ravg / sum;
  gN = Gavg / sum;
  bN = Bavg / sum;

  Serial.print("RGB promedio - R: ");
  Serial.print(Ravg);
  Serial.print("  G: ");
  Serial.print(Gavg);
  Serial.print("  B: ");
  Serial.println(Bavg);

  Serial.print("Norm -> rN: ");
  Serial.print(rN, 3);
  Serial.print("  gN: ");
  Serial.print(gN, 3);
  Serial.print("  bN: ");
  Serial.println(bN, 3);
}

// --------- Detectar color por "nearest centroid" ---------
ColorDulce detectarColor() {
  float rN, gN, bN;
  leerRGBPromediado(rN, gN, bN, 5);  // 5 lecturas promediadas

  int bestIndex = 0;
  float bestDist = 999999.0;

  for (int i = 0; i < NUM_COLORS; i++) {
    float dr = rN - refs[i].r;
    float dg = gN - refs[i].g;
    float db = bN - refs[i].b;
    float dist = dr * dr + dg * dg + db * db;

    if (dist < bestDist) {
      bestDist = dist;
      bestIndex = i;
    }
  }

  ColorDulce result = refs[bestIndex].color;
  Serial.print("Color elegido (nearest centroid): ");
  Serial.println(refs[bestIndex].name);

  return result;
}

// ----------------------------------------------------
void setup() {
  Serial.begin(9600);

  servoDisco.attach(pinServoDisco);
  servoRampa.attach(pinServoRampa);

  servoRampa.write(R_AMARILLO);
  anguloDiscoActual = POS_DISCO_DESCARGA;
  servoDisco.write(anguloDiscoActual);
  delay(1000);

  // TCS3200
  pinMode(S0_PIN, OUTPUT);
  pinMode(S1_PIN, OUTPUT);
  pinMode(S2_PIN, OUTPUT);
  pinMode(S3_PIN, OUTPUT);
  pinMode(OUT_PIN, INPUT);

  // Escala 20% (HIGH, LOW)
  digitalWrite(S0_PIN, HIGH);
  digitalWrite(S1_PIN, LOW);

  Serial.println("Sistema de separacion de Skittles (sin IA, disco negro) listo.");
}

// ----------------------------------------------------
void loop() {
  // 1) Disco a recoger
  moverDiscoSuave(POS_DISCO_RECOGER);
  Serial.println("Disco en POS_RECOGER (180°).");
  delay(500);

  // 2) Disco al sensor
  moverDiscoSuave(POS_DISCO_SENSOR);
  Serial.println("Disco en POS_SENSOR (90°). Leyendo color...");
  delay(500);

  // 3) Detectar color (en Arduino)
  ColorDulce color = detectarColor();

  // 4) Mover rampa
  moverRampaPorColor(color);
  delay(300);

  // 5) Disco a descarga
  moverDiscoSuave(POS_DISCO_DESCARGA);
  Serial.println("Disco en POS_DESCARGA (0°). Soltando dulce...");
  delay(700);

  Serial.println("----------------------------");
  delay(1000);
}
