#include <Adafruit_BMP280.h>
#include <Adafruit_Sensor.h>
#include <dht.h>
#include <Wire.h>
#include <JeeLib.h>

//Radio
#define myNodeID 20
#define network 255
#define freq RF12_433MHZ
//Senzor Temperatura-Umiditate
#define DHT11_PIN 3
//Senzor de presiune
#define BMP_SCK 4 //13
#define BMP_MISO 5 //12
#define BMP_MOSI 6 //11
#define BMP_CS 8 //10

//Calculul bateriei
const int voltmetrupin = 0;
volatile int val=0; // valoare achizitionata
// tensiunea citita vout = (val * 5.0) / 1024.0;
volatile float vout = 0.0;
const float R1 = 220.0; // valori rezistente divizor tensiune
const float R2 = 220.0;
// tensiune baterie vin = vout / (R2/(R1+R2))
volatile float vin = 0.0;
// valoarea maxima a tensiunii bateriei
volatile float initialvin = 8.4;
//intesitate curent in mA i = (vin * 1000) / (R1+R2)
volatile float i = 0.0;
//procent dupa care consideram ca bateria e descarcata 60%
const float lb = 0.6;
volatile float sumvin = 0.0; // suma tensiunilor bateriei
// capacitate consumata cap = (sumvin * 10) / (R1+R2)
volatile float cap = 0.0;
volatile int sec = 0;
volatile float timp = 0; // timp in minute (sec/60)

Adafruit_BMP280 bmp(BMP_CS, BMP_MOSI, BMP_MISO, BMP_SCK);
dht DHT;

const int tmp102Address = 0x48;
float temperatura;
float humidity;
float presiune;
double roua;
double nivel_baterie;
const int emonTx_NodeID=10;
typedef struct { float temp; float umid; float pres; double punctRoua; double nivelBaterie;} Cerere;
Cerere Baza;

void setup() 
{
  pinMode(9, OUTPUT);
  pinMode(10,OUTPUT);
  TIMSK1 = (1 << TOIE1); // activare timer overflow
  TCCR1A = 0;
  TCCR1B = 0; // timer stop
  TCNT1 = 0x0BDC; // setarea valorii initiale
  TCCR1B = (1 << CS12); // timer start
  rf12_initialize(myNodeID,freq,network);
  rf12_recvDone();
  Wire.begin();
  Serial.begin(9600);
  if (!bmp.begin()) 
      {  
      Serial.println("Nu am gasit un senzor BMP280 valid, verificati firele!");
      while (1);
      }
}

void loop()
{
  double T,P,p0,a;
  digitalWrite( 9, LOW );
  int chk = DHT.read11(DHT11_PIN);
  Serial.print("Temperatura = ");
  Serial.println(DHT.temperature);
  Serial.print("Umiditate = ");
  Serial.println(DHT.humidity);
  humidity = DHT.humidity;
  temperatura = DHT.temperature;
  Serial.println(bmp.readPressure()* 0.000295333727);
  roua = dewPoint(temperatura, humidity);
  if ((val<10)&(sumvin==0)) {
      Serial.println("Conectati bateria...");
      }
  else {
     i = (vin * 1000) / (R1+R2);
     nivel_baterie = vin/initialvin*100;
     cap = (sumvin * 10.0) / (R1+R2);
     timp = float(sec) / 60.0;
      }
  Serial.print("Nivel baterie: ");
  Serial.println(nivel_baterie);
  if(nivel_baterie<=20)
     digitalWrite( 10, HIGH );
  if(nivel_baterie>20)
     digitalWrite( 10,LOW );
  Serial.println();
  delay(1000);
  CerereBaza( );
}

double dewPoint(double temperatura, double humidity)
{
  double RATIO = 373.15 / (273.15 + temperatura);
  double RHS = -7.90298 * (RATIO - 1);
  RHS += 5.02808 * log10(RATIO);
  RHS += -1.3816e-7 * (pow(10, (11.344 * (1 - 1/RATIO ))) - 1) ;
  RHS += 8.1328e-3 * (pow(10, (-3.49149 * (RATIO - 1))) - 1) ;
  RHS += log10(1013.246);
  double VP = pow(10, RHS - 3) * humidity;
  double T = log(VP/0.61078);
  return (241.88 * T) / (17.558 - T);
}

void CerereBaza()
{
    Baza.temp = temperatura;
    Baza.umid = humidity;    
    Baza.pres = bmp.readPressure()* 0.00750061683;
    Baza.punctRoua = roua;
    Baza.nivelBaterie = nivel_baterie;
    int i = 0; while (!rf12_canSend()) {rf12_recvDone(); i++;}
    rf12_sendStart(0, &Baza, sizeof Baza);
    Serial.println();
    Serial.print("[Exterior] Temperatura transmisa:  ");
    Serial.print(Baza.temp);
    Serial.println();
    Serial.print("[Exterior] Umiditate transmisa:  ");
    Serial.print(Baza.umid);
    Serial.println();
    Serial.print("[Exterior] Presiune transmisa:  ");
    Serial.print(Baza.pres);
    Serial.println();
    Serial.print("[Exterior] Punct de roua transmisa:  ");
    Serial.print(Baza.punctRoua);
    Serial.println();
    Serial.print("[Exterior] Nivelul bateriei transmis:  ");
    Serial.print(Baza.nivelBaterie);
    Serial.println();
    Serial.println();
    digitalWrite( 9, HIGH );
    delay( 1000 );
    digitalWrite( 9, LOW );
}

ISR(TIMER1_OVF_vect) 
{
  TCNT1=0x0BDC;
  sec++;
  val = analogRead(voltmetrupin);
  vout = (val * 5) / 1024.0;
  vin = vout / (R2/(R1+R2));
  if (!(sec%10)) 
      sumvin+=vin; 
}


