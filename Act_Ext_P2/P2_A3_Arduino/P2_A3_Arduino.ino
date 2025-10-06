//P2 A3 Caja contadora de moneras con Arduino -- Contador de monedas
//Daniel Eduardo Juárez Bañuelos || Viviana Jaqueline Morin Garcia

//Declaracion de variables
//Sensores
const byte sensorM01 = 2;
const byte sensorM02 = 3;
const byte sensorM05 = 4;
const byte sensorM10 = 5;

//Buzzer 
const byte buzzer = 6;

//Variables donde se almacenan las lecturas
bool lecturaM01 = 0;
bool lecturaM02 = 0;
bool lecturaM05 = 0;
bool lecturaM10 = 0;

//Conteo de dinero :)
int contador = 0;

//Funcion de buzzer
void Beep(){
  digitalWrite(buzzer, HIGH);
  delay(300);
  digitalWrite(buzzer, LOW);
}

void setup() {
  //Comunicación serial
  Serial.begin(9600);
  
  //Declaramos pines de sensor como entrada para leer los datos
  pinMode(sensorM01, INPUT);
  pinMode(sensorM02, INPUT);
  pinMode(sensorM05, INPUT);
  pinMode(sensorM10, INPUT);

  //Declaracion de buzzer
  pinMode(buzzer, OUTPUT);

}

void loop() {
  lecturaM01 = digitalRead(sensorM01);
  lecturaM02 = digitalRead(sensorM02);
  lecturaM05 = digitalRead(sensorM05);
  lecturaM10 = digitalRead(sensorM10);

  if (lecturaM01 == 1){
    contador = contador + 1;
    Beep();
  }
  else if (lecturaM02 == 1){
    contador = contador + 2;
    Beep();
  }
  else if (lecturaM05 == 1){
    contador = contador + 5;
    Beep();
  }
  else if (lecturaM10 == 1){
    contador = contador + 10;
    Beep();
  }
  else{
    
  }

  Serial.print("Monedas: ");
  Serial.println(contador);

  delay(500);

}
