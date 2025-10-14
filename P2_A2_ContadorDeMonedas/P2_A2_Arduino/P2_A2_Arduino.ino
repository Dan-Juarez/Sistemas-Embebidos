// Caja contadora de monedas con Arduino + LCD I2C
// Daniel Eduardo Juárez Bañuelos || Viviana Jaqueline Morín García

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// --- Configurar LCD (dirección 0x27 o 0x3F según módulo) ---
LiquidCrystal_I2C lcd(0x27, 16, 2);

// --- Pines de sensores y buzzer ---
const byte sensorM01 = 2;
const byte sensorM02 = 3;
const byte sensorM05 = 4;
const byte sensorM10 = 5;
const byte buzzer = 6;

// --- Variables ---
int total = 0;
int c01 = 0, c02 = 0, c05 = 0, c10 = 0;
bool lastM01 = 0, lastM02 = 0, lastM05 = 0, lastM10 = 0;

// --- Función para beep ---
void Beep(unsigned long duracion = 100) {
  digitalWrite(buzzer, HIGH);
  delay(duracion);
  digitalWrite(buzzer, LOW);
}

void setup() {
  Serial.begin(9600);

  pinMode(sensorM01, INPUT);
  pinMode(sensorM02, INPUT);
  pinMode(sensorM05, INPUT);
  pinMode(sensorM10, INPUT);
  pinMode(buzzer, OUTPUT);

  // --- Inicializar LCD ---
  lcd.init();       // Inicia el LCD
  lcd.backlight();  // Enciende la luz de fondo
  lcd.setCursor(0, 0);
  lcd.print("Contador listo!");
  delay(1000);
  lcd.clear();

  Serial.println("Contador de monedas iniciado!");
}

void loop() {
  // Leer sensores
  bool lecturaM01 = digitalRead(sensorM01);
  bool lecturaM02 = digitalRead(sensorM02);
  bool lecturaM05 = digitalRead(sensorM05);
  bool lecturaM10 = digitalRead(sensorM10);

  // --- Detectar flanco ascendente ---
  if (lecturaM01 && !lastM01) {
    c01++;
    total += 1;
    Beep();
  }
  if (lecturaM02 && !lastM02) {
    c02++;
    total += 2;
    Beep();
  }
  if (lecturaM05 && !lastM05) {
    c05++;
    total += 5;
    Beep();
  }
  if (lecturaM10 && !lastM10) {
    c10++;
    total += 10;
    Beep();
  }

  // Guardar estado anterior
  lastM01 = lecturaM01;
  lastM02 = lecturaM02;
  lastM05 = lecturaM05;
  lastM10 = lecturaM10;

  // --- Mostrar en Serial ---
  Serial.print("Total: $");
  Serial.print(total);
  Serial.print(" | M1:");
  Serial.print(c01);
  Serial.print(" M2:");
  Serial.print(c02);
  Serial.print(" M5:");
  Serial.print(c05);
  Serial.print(" M10:");
  Serial.println(c10);

  // --- Mostrar en LCD ---
  lcd.setCursor(0, 0);
  lcd.print("Total:$");
  lcd.print(total);
  lcd.print("     ");  // Limpia residuos si el número baja de dígitos

  lcd.setCursor(0, 1);
  lcd.print("1:");
  lcd.print(c01);
  lcd.print(" 2:");
  lcd.print(c02);
  lcd.print(" 5:");
  lcd.print(c05);
  lcd.print(" 10:");
  lcd.print(c10);

  delay(50);
}
