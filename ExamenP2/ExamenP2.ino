//Examen 2° Parcial 
// Daniel Eduardo Juárez Bañuelos || Viviana Jaqueline Morin Garcia
// Sistema de seguridad (PIR + Teclado + LCD I2C) con alarma


#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

// ---------- PINES ----------
const byte PIN_PIR       = 11;
const byte PIN_LED_ROJO  = 12;
const byte PIN_LED_VERDE = 13;
const byte PIN_BUZZER    = 10;

// ---------- LCD ----------
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ---------- TECLADO 4x4 ----------
const byte FILAS = 4, COLS = 4;
char teclas[FILAS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte pinesFilas[COLS] = {2, 3, 4, 5};
byte pinesCols[FILAS]   = {6, 7, 8, 9};
Keypad keypad = Keypad(makeKeymap(teclas), pinesFilas, pinesCols, FILAS, COLS);

// ---------- LÓGICA ----------
const String PASS = "123A";   // CONTRASEÑA SELECCIONADA
String ingreso = "";          //Lectura de contraseña ingresada por el usuario

bool alarmaArmada    = true;   // sistema armado/desarmado
bool alarmaDisparada = false;  // sirena activa

unsigned long tBlink=0, tBeep=0, tSerial=0;
const unsigned long BLINK_MS=500, BEEP_MS=200, SERIAL_MS=500;
bool estadoBlink=false, estadoBeep=false;

//Funcion que activa el buzzer
void beepOn(bool on){ digitalWrite(PIN_BUZZER, on ? HIGH : LOW); }
//Funcion de control de Leds
void ledsOff(){ digitalWrite(PIN_LED_ROJO, LOW); digitalWrite(PIN_LED_VERDE, LOW); }

// ---------- LCD ----------
void lcdHeader() {
  lcd.setCursor(0,0);
  if (!alarmaArmada) {
    lcd.print("DESARMADA       ");   
  } else if (alarmaDisparada) {
    lcd.print("ARMADA  ALERTA  ");   
  } else {
    lcd.print("ARMADA          ");
  }
}

void mostrarIngreso() {
  lcd.setCursor(5,1);
  for (uint8_t i=0;i<4;i++) lcd.print(i < ingreso.length() ? '*' : ' ');
}

void lcdEstadoBase() {
  lcd.clear();
  lcdHeader();
  lcd.setCursor(0,1); lcd.print("PIN: ");
  mostrarIngreso();
}

// ---------- TECLADO ----------
void manejarTeclado() {
  char k = keypad.getKey();
  if (!k) return;

  // Debug de teclas
  Serial.print(F("[KEY] ")); Serial.println(k);

  if (k == '*') {            // limpiar
    ingreso = "";
    mostrarIngreso();
    return;
  }

  if (k == '#') {            // confirmar
    if (ingreso == PASS) {
      if (alarmaDisparada) {
        // Si estaba sonando, la apagamos y DESARMAMOS
        alarmaDisparada = false;
        alarmaArmada = false;
        beepOn(false); ledsOff();
        Serial.println(F("[ALARM] PIN OK -> Restablecida y DESARMADA"));
      } else {
        // Alterna armado/desarmado cuando no está disparada
        alarmaArmada = !alarmaArmada;
        Serial.print(F("[SYS] Estado -> "));
        Serial.println(alarmaArmada ? F("ARMADA") : F("DESARMADA"));
      }
      
      lcdHeader();
      lcd.setCursor(0,1); lcd.print("PIN: ");
      lcd.setCursor(5,1); lcd.print("OK  ");
      delay(800);
    } else {
      Serial.println(F("[PIN] Incorrecto"));
      lcdHeader();
      lcd.setCursor(0,1); lcd.print("PIN: ");
      lcd.setCursor(5,1); lcd.print("ERR ");
      // pequeño beep
      beepOn(true); delay(180); beepOn(false);
      delay(500);
    }
    ingreso = "";
    lcdEstadoBase();         
    return;
  }

  // Tecla normal (0-9 / A-D)
  if (ingreso.length() < 4) {
    ingreso += k;
    mostrarIngreso();
  }
}

// ---------- SALIDAS ----------
void actualizarSalidas() {
  if (!alarmaArmada) {
    digitalWrite(PIN_LED_VERDE, HIGH);
    digitalWrite(PIN_LED_ROJO, LOW);
    beepOn(false);
    return;
  }

  if (alarmaDisparada) {
    // LATCH: rojo fijo + beep intermitente
    digitalWrite(PIN_LED_ROJO, HIGH);
    digitalWrite(PIN_LED_VERDE, LOW);
    if (millis() - tBeep >= BEEP_MS) {
      tBeep = millis();
      estadoBeep = !estadoBeep;
      beepOn(estadoBeep);
    }
  } else {
    // Armado en reposo: rojo parpadea
    digitalWrite(PIN_LED_VERDE, LOW);
    beepOn(false);
    if (millis() - tBlink >= BLINK_MS) {
      tBlink = millis();
      estadoBlink = !estadoBlink;
      digitalWrite(PIN_LED_ROJO, estadoBlink ? HIGH : LOW);
    }
  }
}

void setup() {
  pinMode(PIN_PIR, INPUT);        
  pinMode(PIN_LED_ROJO, OUTPUT);
  pinMode(PIN_LED_VERDE, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  beepOn(false); ledsOff();

  Serial.begin(9600);
  Serial.println(F("\n[BOOT] Sistema de seguridad listo"));
  Serial.println(F("[INFO] Teclas -> [KEY] <caracter>"));

  lcd.init(); lcd.backlight();
  lcd.clear();
  lcd.setCursor(0,0); lcd.print("Sistema Seguridad");
  lcd.setCursor(0,1); lcd.print("Inicializando...");
  delay(800);
  lcdEstadoBase();
}

void loop() {
  // 1) Lectura PIR: si ARMADA y detecta -> DISPARA
  int pir = digitalRead(PIN_PIR);
  if (alarmaArmada && pir == HIGH && !alarmaDisparada) {
    alarmaDisparada = true;
    Serial.println(F("[ALARM] Movimiento -> DISPARADA (latch)"));
    
    lcdHeader();                 
    lcd.setCursor(0,1); lcd.print("PIN: ");
    mostrarIngreso();
  }

  
  manejarTeclado();
  actualizarSalidas();

  
  if (millis() - tSerial >= SERIAL_MS) {
    tSerial = millis();
    Serial.print(F("[PIR] raw=")); Serial.print(pir);
    Serial.print(F(" | armada=")); Serial.print(alarmaArmada);
    Serial.print(F(" | disparada=")); Serial.println(alarmaDisparada);
  }
}
