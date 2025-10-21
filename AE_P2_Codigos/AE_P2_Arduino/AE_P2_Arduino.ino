//P2_Actividad Extracurricular - Sistemas Embebidos con Arduino y Python
//Daniel Eduardo Juárez Bañuelos || Viviana Jaqueline Morin Garcia

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ------------------- Pines -------------------
//Sensor Ultrasonico
const byte PIN_TRIG = 6;
const byte PIN_ECHO = 7;

//Entradas del driver del motor a pasos 
const byte PIN_IN1 = 8;
const byte PIN_IN2 = 9;
const byte PIN_IN3 = 10;
const byte PIN_IN4 = 11;

// ------------------- LCD ---------------------
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ------------------- Parámetros ----------------
const float MIN_CM = 5.0;
const float MAX_CM = 20.0;

const float CERCA = 10.0;   // Muy cerca  -> Sentido Horario
const float LEJOS  = 15.0;   // Muy lejos  -> Sentido Antihorario
const float TOLERANCIA    = 0.5;

const unsigned long ECHO_TIMEOUT_US = 20000UL; 
const unsigned long READ_PERIOD_MS  = 60;      // periodo de medición
const unsigned long FAILSAFE_MS     = 300;     // sin datos válidos -> stop
const unsigned long STEP_INTERVAL_MS = 20;      // velocidad del paso

// ------------------- Estado -------------------
enum RunState { IDLE, RUN };
RunState state = IDLE;

enum Dir { STOP, CW, CCW };
Dir stepDir = STOP;

unsigned long lastReadMs = 0;
unsigned long lastValidMs = 0;
unsigned long lastStepMs = 0;

// Secuencia half-step para 28BYJ-48 (8 estados)
const byte STEPS[8][4] = {
  {1,0,0,0},
  {1,1,0,0},
  {0,1,0,0},
  {0,1,1,0},
  {0,0,1,0},
  {0,0,1,1},
  {0,0,0,1},
  {1,0,0,1}
};
int stepIndex = 0;

// Buffer para mediana
const byte MED_N = 5;
float medBuf[MED_N];
byte medCount = 0;

// ------------------- Utilidades -------------------
void setCoils(int a, int b, int c, int d) {
  digitalWrite(PIN_IN1, a);
  digitalWrite(PIN_IN2, b);
  digitalWrite(PIN_IN3, c);
  digitalWrite(PIN_IN4, d);
}

void stepOnce(Dir d) {
  if (d == STOP) { setCoils(0,0,0,0); return; }
  if (d == CW)   stepIndex++;
  else           stepIndex--;
  if (stepIndex > 7) stepIndex = 0;
  if (stepIndex < 0) stepIndex = 7;
  setCoils(STEPS[stepIndex][0], STEPS[stepIndex][1], STEPS[stepIndex][2], STEPS[stepIndex][3]);
}

float readDistanceOnce() {
  // Trigger de 10us
  digitalWrite(PIN_TRIG, LOW); delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  unsigned long dur = pulseIn(PIN_ECHO, HIGH, ECHO_TIMEOUT_US);
  if (dur == 0) return NAN; 

  // Conversión a cm: (vel sonido ~ 340 m/s) => cm = (dur(us) / 58.0)
  float cm = dur / 58.0;
  if (cm < MIN_CM || cm > MAX_CM) return NAN; // fuera de rango útil
  return cm;
}

float median5(float a[], byte n) {
  // copia local para no alterar buffer
  float t[MED_N];
  for (byte i=0;i<n;i++) t[i]=a[i];
  // ordenamiento simple (n=5)
  for (byte i=0;i<n;i++)
    for (byte j=i+1;j<n;j++)
      if (t[j] < t[i]) { float tmp=t[i]; t[i]=t[j]; t[j]=tmp; }
  return t[n/2];
}

// ------------------- Serie -------------------
void processSerial() {
  static String line;
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      if (line.length()) {
        line.trim();
        if (line.equalsIgnoreCase("START")) {
          state = RUN;
          Serial.println("STATE:RUN");
        } else if (line.equalsIgnoreCase("STOP")) {
          state = IDLE;
          stepDir = STOP;
          Serial.println("STATE:IDLE");
        }
        line = "";
      }
    } else {
      line += c;
    }
  }
}

// ------------------- LCD -------------------
void printLCD(const char* linea1, const char* linea2) {
  lcd.setCursor(0,0);
  lcd.print("                ");
  lcd.setCursor(0,0);
  lcd.print(linea1);

  lcd.setCursor(0,1);
  lcd.print("                ");
  lcd.setCursor(0,1);
  lcd.print(linea2);
}

// ------------------- Setup -------------------
void setup() {
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);

  pinMode(PIN_IN1, OUTPUT);
  pinMode(PIN_IN2, OUTPUT);
  pinMode(PIN_IN3, OUTPUT);
  pinMode(PIN_IN4, OUTPUT);
  setCoils(0,0,0,0);

  Serial.begin(115200);

  lcd.init();
  lcd.backlight();
  printLCD("Sistema listo", "En espera (STOP)");
}

// ------------------- Loop -------------------
void loop() {
  processSerial();

  unsigned long now = millis();

  // Lectura periódica con filtro de mediana(5)
  static float lastDist = NAN;

  if (now - lastReadMs >= READ_PERIOD_MS) {
    lastReadMs = now;

    float d = readDistanceOnce();
    if (!isnan(d)) {
      // Llenado del buffer (rotate)
      if (medCount < MED_N) {
        medBuf[medCount++] = d;
      } else {
        // shift izquierda y agrega al final
        for (byte i=1;i<MED_N;i++) medBuf[i-1]=medBuf[i];
        medBuf[MED_N-1] = d;
      }

      if (medCount == MED_N) {
        lastDist = median5(medBuf, MED_N);
        lastValidMs = now;

        // Emite por serie
        Serial.print("DIST:");
        Serial.println(lastDist, 2);

        // LCD: línea 1 distancia, línea 2 estado
        char l1[17], l2[17];
        snprintf(l1, sizeof(l1), "Dist: %5.2f cm", lastDist);
        if (state == RUN) snprintf(l2, sizeof(l2), "Motor: ACTIVO   ");
        else              snprintf(l2, sizeof(l2), "Motor: DETENIDO ");
        printLCD(l1, l2);

        // Lógica de dirección solo si RUN
        if (state == RUN) {
          if (stepDir == CW) {
            
            if (lastDist > (CERCA + TOLERANCIA) && lastDist < (LEJOS - TOLERANCIA)) stepDir = STOP;
            else if (lastDist > (LEJOS + TOLERANCIA)) stepDir = CCW;
          } else if (stepDir == CCW) {
            
            if (lastDist < (LEJOS - TOLERANCIA) && lastDist > (CERCA + TOLERANCIA)) stepDir = STOP;
            else if (lastDist < (CERCA - TOLERANCIA)) stepDir = CW;
          } else { // STOP
            if (lastDist < (CERCA - TOLERANCIA)) stepDir = CW;
            else if (lastDist > (LEJOS + TOLERANCIA)) stepDir = CCW;
            else stepDir = STOP;
          }
        } else {
          stepDir = STOP;
        }
      }
    } else {
      // lectura inválida, no se añade al filtro
      // Si llevamos mucho sin válidas -> failsafe
      if (now - lastValidMs > FAILSAFE_MS) {
        stepDir = STOP;
        Serial.println("DIST:NaN");
        printLCD("Dist: ---       ", "SIN DATOS       ");
      }
    }
  }

  // Avance del paso no bloqueante
  if (stepDir != STOP && (now - lastStepMs >= STEP_INTERVAL_MS)) {
    lastStepMs = now;
    stepOnce(stepDir);
  }

  // Si estamos parados, liberar bobinas
  if (stepDir == STOP) {
    setCoils(0,0,0,0);
  }
}
