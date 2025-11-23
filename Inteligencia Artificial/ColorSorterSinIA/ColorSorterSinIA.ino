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

// Para llevar el control del ángulo actual del disco
int anguloDiscoActual = POS_DISCO_DESCARGA;

// --------- Enumeración de colores ---------
enum ColorDulce {
  COLOR_AMARILLO,
  COLOR_VERDE,
  COLOR_ROJO,
  COLOR_NARANJA,
  COLOR_MORADO,
  COLOR_DESCONOCIDO
};

// ------------ Pines TCS3200 (GY-31) ------------
const int S0_PIN   = 2;
const int S1_PIN   = 3;
const int S2_PIN   = 4;
const int S3_PIN   = 5;
const int OUT_PIN  = 7;

// --------- Mover suavemente el disco (MG995) ---------
void moverDiscoSuave(int destino) {
  int desde = anguloDiscoActual;

  if (desde < destino) {
    for (int pos = desde; pos <= destino; pos++) {
      servoDisco.write(pos);
      delay(15);   // Ajusta si quieres más suave/lento
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
  int anguloRampa = R_AMARILLO; // valor por defecto

  switch (color) {
    case COLOR_AMARILLO:
      anguloRampa = R_AMARILLO;
      break;
    case COLOR_VERDE:
      anguloRampa = R_VERDE;
      break;
    case COLOR_ROJO:
      anguloRampa = R_ROJO;
      break;
    case COLOR_NARANJA:
      anguloRampa = R_NARANJA;
      break;
    case COLOR_MORADO:
      anguloRampa = R_MORADO;
      break;
    case COLOR_DESCONOCIDO:
    default:
      // Podrías mandar los desconocidos a alguna bandeja "basura"
      anguloRampa = R_AMARILLO;
      break;
  }

  servoRampa.write(anguloRampa);
  Serial.print("Rampa movida a: ");
  Serial.print(anguloRampa);
  Serial.println(" grados.");
}

// --------- Leer frecuencia de un "filtro" de color ---------
// s2State y s3State seleccionan el filtro (rojo, verde, azul)
unsigned long leerFrecuenciaColor(bool s2State, bool s3State) {
  digitalWrite(S2_PIN, s2State);
  digitalWrite(S3_PIN, s3State);

  delay(100);  // dejar que se estabilice el filtro

  // Medimos el tiempo de un pulso LOW (en microsegundos)
  unsigned long duracion = pulseIn(OUT_PIN, LOW, 250000); // timeout 250 ms

  if (duracion == 0) {
    // Si no se leyó pulso (muy raro), devolvemos algo grande
    duracion = 250000;
  }

  // La frecuencia es inversamente proporcional a la duración
  // freq ≈ 1e6 / duracion, pero podemos usar duracion directamente
  // para comparar entre colores si somos consistentes.
  unsigned long frecuencia = 1000000UL / duracion;

  return frecuencia;
}

// --------- Leer R, G, B del TCS3200 ---------
void leerRGB(unsigned long &r, unsigned long &g, unsigned long &b) {
  // Según el datasheet / estándar:
  // S2 S3
  // L  L  -> rojo
  // L  H  -> azul
  // H  H  -> verde
  // (H,L sería clear, que aquí no usamos)

  // Rojo
  r = leerFrecuenciaColor(LOW, LOW);

  // Azul
  unsigned long azul = leerFrecuenciaColor(LOW, HIGH);

  // Verde
  g = leerFrecuenciaColor(HIGH, HIGH);

  // Guardamos azul en b
  b = azul;
}

// --------- Detectar color del dulce usando TCS3200 ---------
ColorDulce detectarColor() {
  unsigned long r, g, b;
  leerRGB(r, g, b);

  // Debug: imprime valores crudos
  Serial.print("Lectura RGB - R: ");
  Serial.print(r);
  Serial.print("  G: ");
  Serial.print(g);
  Serial.print("  B: ");
  Serial.println(b);

  // Normalización
  float sum = (float)r + (float)g + (float)b;
  if (sum == 0) sum = 1;

  float rN = r / sum;
  float gN = g / sum;
  float bN = b / sum;

  Serial.print("Norm -> rN: ");
  Serial.print(rN, 3);
  Serial.print("  gN: ");
  Serial.print(gN, 3);
  Serial.print("  bN: ");
  Serial.println(bN, 3);

  // ---- Tabla de colores de referencia (centroides) ----
  struct ColorRef {
    float r;
    float g;
    float b;
    ColorDulce color;
  };

  ColorRef refs[] = {
    //               r       g       b
    {0.363, 0.315, 0.321, COLOR_AMARILLO},
    {0.373, 0.295, 0.332, COLOR_NARANJA},
    {0.348, 0.299, 0.353, COLOR_ROJO},
    {0.329, 0.328, 0.343, COLOR_VERDE},
    {0.335, 0.308, 0.356, COLOR_MORADO}
  };

  const int NUM_COLORS = sizeof(refs) / sizeof(refs[0]);

  // ---- Buscar el color más cercano ----
  int bestIndex = 0;
  float bestDist = 999999.0;

  for (int i = 0; i < NUM_COLORS; i++) {
    float dr = rN - refs[i].r;
    float dg = gN - refs[i].g;
    float db = bN - refs[i].b;
    float dist = dr*dr + dg*dg + db*db;  // distancia al cuadrado (no hace falta sqrt)

    if (dist < bestDist) {
      bestDist = dist;
      bestIndex = i;
    }
  }

  ColorDulce result = refs[bestIndex].color;

  Serial.print("Color elegido por distancia: ");
  switch (result) {
    case COLOR_AMARILLO: Serial.println("AMARILLO"); break;
    case COLOR_VERDE:    Serial.println("VERDE"); break;
    case COLOR_ROJO:     Serial.println("ROJO"); break;
    case COLOR_NARANJA:  Serial.println("NARANJA"); break;
    case COLOR_MORADO:   Serial.println("MORADO"); break;
    default:             Serial.println("DESCONOCIDO"); break;
  }

  return result;
}


// ----------------------------------------------------
void setup() {
  Serial.begin(9600);

  // Servos
  servoDisco.attach(pinServoDisco);
  servoRampa.attach(pinServoRampa);

  // Posiciones iniciales
  servoRampa.write(R_AMARILLO);          // Rampa a alguna bandeja inicial
  anguloDiscoActual = POS_DISCO_DESCARGA;
  servoDisco.write(anguloDiscoActual);
  delay(1000);

  // TCS3200 pins
  pinMode(S0_PIN, OUTPUT);
  pinMode(S1_PIN, OUTPUT);
  pinMode(S2_PIN, OUTPUT);
  pinMode(S3_PIN, OUTPUT);
  pinMode(OUT_PIN, INPUT);

  // Escalado de frecuencia: S0 S1
  // HIGH LOW -> 20% (recomendado para no saturar)
  digitalWrite(S0_PIN, HIGH);
  digitalWrite(S1_PIN, LOW);

  Serial.println("Sistema de separacion de Skittles listo.");
}

// ----------------------------------------------------
void loop() {
  // 1) Llevar disco a posición de recoger dulce (180°)
  moverDiscoSuave(POS_DISCO_RECOGER);
  Serial.println("Disco en POS_RECOGER (180°).");
  delay(500);  // tiempo para que el dulce se acomode

  // 2) Llevar disco a posición de sensor (90°)
  moverDiscoSuave(POS_DISCO_SENSOR);
  Serial.println("Disco en POS_SENSOR (90°). Leyendo color...");
  delay(500);  // que se estabilice el dulce frente al sensor

  // 3) Detectar el color del dulce con el TCS3200
  ColorDulce color = detectarColor();

  // 4) Mover la rampa al ángulo correcto
  moverRampaPorColor(color);
  delay(300);  // tiempo para que la rampa llegue a su posición

  // 5) Llevar el disco a la posición de descarga (0°)
  moverDiscoSuave(POS_DISCO_DESCARGA);
  Serial.println("Disco en POS_DESCARGA (0°). Soltando dulce...");
  delay(700);  // tiempo para que el dulce caiga y recorra la rampa

  // 6) Pausa antes del siguiente ciclo
  Serial.println("----------------------------");
  delay(1000);
}
