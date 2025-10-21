//Actividad P2 A3 Arduino con APP Bluetooth
//Daniel Juárez || Viviana Morin

//variables de las LEDS
const byte Led1 = 12;
const byte Led2 = 13;

//Variable que recibe instruccion
char orden;

void setup() {
  pinMode(Led1 , HIGH);
  pinMode(Led2 , HIGH);

  Serial.begin(9600);
}

void loop() {
  if (Serial.available()) {
    orden = Serial.read();
    }
    if (orden == 'A')
    {
      digitalWrite(Led1, HIGH);
    }
    if (orden == 'B')
    {
      digitalWrite(Led1, LOW);
    }
    if (orden == 'C')
    {
      digitalWrite(Led2, HIGH);
    }
    if (orden == 'D')
    {
      digitalWrite(Led2, LOW);
    }
  } 
