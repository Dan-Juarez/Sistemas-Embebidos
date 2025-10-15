// Actividad Sensor con Bluetooth
// Dan Juárez || Viviana Morin

#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>

#define DHTPIN 2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// RX, TX para Bluetooth (ej. HC-05 / HC-06)
SoftwareSerial BT(10, 11);

void setup() {
  Serial.begin(9600);
  BT.begin(9600);

  dht.begin();
  lcd.init();
  lcd.backlight();
  lcd.print("Sensor activo...");
  delay(2000);
  lcd.clear();
}

void loop() {
  // Lectura del sensor
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    lcd.clear();
    lcd.print("Error sensor");
    delay(1000);
    return;
  }

  // Mostrar en LCD
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(t);
  lcd.print(" C   ");
  lcd.setCursor(0, 1);
  lcd.print("Hum:  ");
  lcd.print(h);
  lcd.print(" %   ");

  // ===== Enviar por Bluetooth =====
  // Formato CSV: "t,h\n
  BT.print(t, 1);    // 1 decimal
  BT.print(',');
  BT.print(h, 1);
  BT.write(10);      // '\n' = 10 (DelimiterByte en App Inventor)

  
  Serial.print(t, 1); Serial.print(','); Serial.println(h, 1);

  delay(2000);
}
