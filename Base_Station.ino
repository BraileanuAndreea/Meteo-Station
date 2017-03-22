#include <dht.h>
#include <Wire.h>
#include <SPI.h>
#include <JeeLib.h>
#include <Adafruit_GFX.h>
#include <TFT_ILI9163C.h>
#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <string.h>
#include "utility/debug.h"
#include <RTClib.h>
 
 
//Colors
#define BLACK   0x0000
#define YELLOW  0xffe0
#define RED     0xf00
#define REDREAL 0xf800
#define WHITE   0xFFFF
#define ST7735_YELLOW 0xffe0
#define ST7735_WHITE 0xffff
#define ST7735_BLUE 0x00ff
 
//Radio module
#define myNodeID 10 // 30 pentru RX
#define network 255
#define freq RF12_433MHZ
 
//Humidity sensor
#define DHT11_PIN 6
 
 //Display
#define CS 10
#define DC 9
 
//Wifi module
// These are the interrupt and control pins
#define ADAFRUIT_CC3000_IRQ   3  // MUST be an interrupt pin!
// These can be any two pins
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    4
// Use hardware SPI for the remaining pins
// On an UNO, SCK = 13, MISO = 12, and MOSI = 11
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT,
                                         SPI_CLOCK_DIVIDER); // you can change this clock speed but DI
 
#define WLAN_SSID       "Vlad"        // cannot be longer than 32 characters!
#define WLAN_PASS       ""
// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_WPA2
 
//---------------Wunderground Info--------------------------
#define WEBPAGE "GET /weatherstation/updateweatherstation.php?"
#define ID "IBUCHARE75"
#define PASSWORD "s3thmg2s"
//---------------Realtime update server--------------------------
#define SERVER "rtupdate.wunderground.com"
#define UPDATE "&action=updateraw&realtime=1&rtfreq=2.5"
//-----------------standard server-------------------------------
 
//#define SERVER "weatherstation.wunderground.com"
//#define UPDATE "action=updateraw"
 
#define IDLE_TIMEOUT_MS 20000 // Amount of time to wait (in milliseconds) with no data
 
Adafruit_CC3000_Client client;
uint32_t ip;
RTC_Millis rtc;
 
const int M0_NodeID=20;
 
const int tmp102Address = 0x48;
float temperatura;
float umiditate;
float presiune;
double punct_roua;
double pct_roua_ext;
float temp_ext;
float umid_ext;
float presiune_ext;
double nivel_bat_ext;
 
TFT_ILI9163C tft = TFT_ILI9163C(CS, DC);
typedef struct { float temp; float umid; float pres; double punct_roua; double nivel_bat;} Cerere;
Cerere Exterior;
dht DHT;
 
void setup() {
 Serial.begin(9600);
 
 rf12_initialize(myNodeID,freq,network);
 rf12_recvDone();
 
 tft.begin();
 tft.fillScreen();
 
 Wire.begin();
 
  /* Initialise the module */
 // Serial.println(F("\nInitialising the CC3000 ..."));
  if (!cc3000.begin())
  {
   // Serial.println(F("Unable to initialise the CC3000! Check your wiring?"));
    while(1);
  }
  char *ssid = WLAN_SSID;             /* Max 32 chars */
 // Serial.print(F("\nAttempting to connect to ")); Serial.println(ssid);
  if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
  //  Serial.println(F("Failed!"));
    while(1);
  }  
 // Serial.println(F("Connected!"));
 
  //rtc
  while (!Serial);
  //rtc.begin();
  //if (! rtc.isrunning()) {
  //Serial.println("RTC is NOT running!");
  // following line sets the RTC to the date & time this sketch was compiled
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  // This line sets the RTC with an explicit date & time, for example to set
  // January 21, 2014 at 3am you would call:
  // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  //}
  //rtc.begin(DateTime(F(__DATE__), F(__TIME__)));
 
  /* Wait for DHCP to complete */
 // Serial.println(F("Request DHCP"));
  while (!cc3000.checkDHCP())
  {
  delay(100); // ToDo: Insert a DHCP timeout!
  }
 
  ip = 0;
  // Try looking up the website's IP address
  // Serial.print(SERVER); Serial.print(F(" -> "));
  while (ip == 0) {
  if (! cc3000.getHostByName(SERVER, &ip)) {
 // Serial.println(F("Couldn't resolve!"));
  }
  delay(500);
  }
 
  //cc3000.printIPdotsRev(ip);
 // Serial.println("\nEnd of Setup");
 
}
 
void loop(){
 int chk = DHT.read11(DHT11_PIN);
 
 temperatura = DHT.temperature;
 umiditate = DHT.humidity;
 
 tft.setTextColor(RED);
 //Serial.print("Interior: ");
 tft.setTextColor(WHITE);
 
 //Serial.print("Temperatura: ");
 //Serial.println(temperatura);
 
 //Serial.print("Umiditate: ");
 //Serial.println(umiditate);
 
 punct_roua = dewPoint(temperatura, umiditate);
 CerereExterior( );
 
  //Wheather Underground
  char sendbuffer[80];
  char numberbuffer[20];
  DateTime now = rtc.now();
  client = cc3000.connectTCP(ip, 80);
  //Serial.println();
  //Serial.println("Connecting");
  if (!client.connected())
  client.connect(ip, 80);
 
  else if (client.connected()) {
 
    DateTime now = rtc.now();
  /*  Serial.println(F("Connected! *"));
    Serial.println(F("Sending*"));*/
    client.print(F(WEBPAGE));
 
    client.print(F("ID="));
 
    client.print(F(ID));
 
    client.print(F("&PASSWORD="));
 
    client.print(F(PASSWORD));
 
    client.print("&dateutc=");
 
    client.print(now.year());
 
    client.print("-");
 
    client.print(now.month());
 
    client.print("-");
 
    client.print(now.day());
 
    client.print("+");
 
    client.print(now.hour());
 
    client.print("%3A");
 
    client.print(now.minute());
 
    client.print("%3A");
    //Serial.print(F("*"));
    client.print(now.second());
   // Serial.print(F("*"));
   
    client.print(F("&tempf="));
   // Serial.print(F("*"));
    client.print(temp_ext*1.8+32);
 
   
    client.print(F("&humidity="));
   // Serial.print(F("*"));
    client.print(umid_ext);
   
    strcpy(sendbuffer, "&tempf=,");
   
    // add temp value
    dtostrf(temp_ext, 2, 1, numberbuffer);
    strcat(sendbuffer, numberbuffer);
   
    // add new line and humidity feed name
    strcat(sendbuffer, "\n&humidity=,");
   
    // add humidity value
    dtostrf(umid_ext, 2, 1, numberbuffer);
    strcat(sendbuffer, numberbuffer);
 
    client.print(F(UPDATE));//Rapid Fire &realtime=1&rtfreq=2.5
    //client.print(UPDATE);
    Serial.println(F("* DONE"));
    client.println();
   
    //time to wait if no data received
    unsigned long lastRead = millis();
    // waiting for A responce from the server
    while (client.connected() && (millis() - lastRead < IDLE_TIMEOUT_MS)) {
      while (client.available()) {
        char c = client.read();
        //print the responce from the server
      //  Serial.print(c);
        lastRead = millis();
      }
    }
  }
  else {
 //   Serial.println("Connection failed");
    //client.close();
  }
 
//End wheather underground
 
 myParams(temperatura, umiditate, punct_roua);
 delay(3000);
}
 
 
void sun( int x, int y, int raza )
{
  tft.drawCircle(x,y,raza,ST7735_YELLOW);  
  tft.fillCircle(x,y,raza,ST7735_YELLOW);
  tft.drawFastVLine(x,y+raza,raza,ST7735_YELLOW);
  tft.drawFastVLine(x,y-raza,raza,ST7735_YELLOW);
  tft.drawFastVLine(x+raza,y,raza,ST7735_YELLOW);
  tft.drawFastVLine(x-raza,y,raza,ST7735_YELLOW);
  tft.drawLine(x,y,x+1.7*raza,y+1.7*raza,ST7735_YELLOW);
  tft.drawLine(x,y,x-1.7*raza,y-1.7*raza,ST7735_YELLOW);
  tft.drawLine(x,y,x-1.7*raza,y+1.7*raza,ST7735_YELLOW);
  tft.drawLine(x,y,x+1.7*raza,y-1.7*raza,ST7735_YELLOW);
  tft.drawLine(x,y,x+2*raza,y,ST7735_YELLOW);
  tft.drawLine(x,y,x-1.7*raza,y,ST7735_YELLOW);
  tft.drawLine(x,y,x,y+1.7*raza,ST7735_YELLOW);
  tft.drawLine(x,y,x,y-1.7*raza,ST7735_YELLOW);
}
 
void clouds( int x, int y, int raza )
{
  tft.drawRect( x, y,3*raza,raza,ST7735_WHITE);
  tft.fillRect(x,y,3*raza,raza,ST7735_WHITE);
  tft.drawCircle(x,y+raza/2.2,raza/2,ST7735_WHITE);
  tft.fillCircle(x,y+raza/2.2,raza/2,ST7735_WHITE);
 
  tft.drawCircle(x+raza,y,raza/1.2,ST7735_WHITE);
  tft.fillCircle(x+raza,y,raza/1.2,ST7735_WHITE);
 
  tft.drawCircle(x+2.2*raza,y+raza/8,raza/1.5,ST7735_WHITE);
  tft.fillCircle(x+2.2*raza,y+raza/8,raza/1.5,ST7735_WHITE);
 
  tft.drawCircle(x+3*raza,y+raza/3,raza/2,ST7735_WHITE);
  tft.fillCircle(x+3*raza,y+raza/3,raza/2,ST7735_WHITE);
 
}
 
void rain( int x, int y, int raza )
{
  tft.drawRect( x, y,3*raza,raza,ST7735_WHITE);
  tft.fillRect(x,y,3*raza,raza,ST7735_WHITE);
  tft.drawCircle(x,y+raza/2.2,raza/2,ST7735_WHITE);
  tft.fillCircle(x,y+raza/2.2,raza/2,ST7735_WHITE);
 
  tft.drawCircle(x+raza,y,raza/1.2,ST7735_WHITE);
  tft.fillCircle(x+raza,y,raza/1.2,ST7735_WHITE);
 
  tft.drawCircle(x+2.2*raza,y+raza/8,raza/1.5,ST7735_WHITE);
  tft.fillCircle(x+2.2*raza,y+raza/8,raza/1.5,ST7735_WHITE);
 
  tft.drawCircle(x+3*raza,y+raza/3,raza/2,ST7735_WHITE);
  tft.fillCircle(x+3*raza,y+raza/3,raza/2,ST7735_WHITE);
 
  tft.drawLine(x+raza/4,y+1.5*raza,x+raza/4,y+1.8*raza,ST7735_WHITE);
  tft.drawLine(x+raza/4,y+1.5*raza,x+raza/4,y+1.8*raza,ST7735_WHITE);
 
  tft.drawLine(x+1.5*raza,y+1.5*raza,x+1.5*raza,y+1.8*raza,ST7735_WHITE);
  tft.drawLine(x+1.5*raza,y+1.5*raza,x+1.5*raza,y+1.8*raza,ST7735_WHITE);
 
  tft.drawLine(x+3*raza,y+1.5*raza,x+3*raza,y+1.8*raza,ST7735_WHITE);
  tft.drawLine(x+3*raza,y+1.5*raza,x+3*raza,y+1.8*raza,ST7735_WHITE);
 
 
}
 
/*void snow( int x, int y, int raza )
{
  tft.drawLine(x,y,x,y+raza,ST7735_BLUE);
  tft.drawLine(x,y,x,y-raza,ST7735_BLUE);
  tft.drawLine(x,y,x+raza,y,ST7735_BLUE);
  tft.drawLine(x,y,x-raza,y,ST7735_BLUE);
  tft.drawLine(x,y,x+raza,y+raza,ST7735_BLUE);
  tft.drawLine(x,y,x-raza,y-raza,ST7735_BLUE);
  tft.drawLine(x,y,x-raza,y+raza,ST7735_BLUE);
  tft.drawLine(x,y,x+raza,y-raza,ST7735_BLUE);
}
*/
double dewPoint(double celsius, double humidity)
{
  // (1) Saturation Vapor Pressure = ESGG(T)
  double RATIO = 373.15 / (273.15 + celsius);
  double RHS = -7.90298 * (RATIO - 1);
  RHS += 5.02808 * log10(RATIO);
  RHS += -1.3816e-7 * (pow(10, (11.344 * (1 - 1/RATIO ))) - 1) ;
  RHS += 8.1328e-3 * (pow(10, (-3.49149 * (RATIO - 1))) - 1) ;
  RHS += log10(1013.246);
 
        // factor -3 is to adjust units - Vapor Pressure SVP * humidity
  double VP = pow(10, RHS - 3) * humidity;
 
        // (2) DEWPOINT = F(Vapor Pressure)
  double T = log(VP/0.61078);   // temp var
  return (241.88 * T) / (17.558 - T);
}
 
 
 
unsigned long myParams(float temperatura, float umiditate, double punct_roua){
 
 
  tft.setCursor(20,70);
  tft.setTextColor(WHITE);  
  tft.setTextSize(1);
  tft.setTextColor(RED);
  tft.setTextSize(2);
  tft.println("Interior");
  tft.setTextColor(WHITE);
 
  tft.setTextSize(1);
  tft.setCursor(0,88);
  tft.setTextColor(YELLOW);
  tft.print("T:" );
  tft.print(temperatura);
  tft.print("'C");
 
  tft.setCursor(0,96);
  tft.print("U:" );
  tft.print(umiditate);
  tft.print("%");
 
  tft.setCursor(0, 104);
  tft.print("R:" );
  tft.print(punct_roua);
 
 
}
unsigned long testText( float temp, float umid, float pres, double punct_roua, double nivel_bat) {
  tft.clearScreen();
  tft.setCursor(20,0);
  tft.setTextColor(WHITE);  
  tft.setTextSize(1);
  tft.setTextColor(RED);
  tft.setTextSize(2);
  tft.println("Exterior");
  tft.setTextSize(1);
  tft.setTextColor(WHITE);
 
  if ( pres > 767.0 )
        {
          sun(85, 35, 6);
         /* tft.setCursor(29,11);
          tft.print("Senin");*/
        }
        else if ( pres <= 767 && pres >= 757.0 )
        {
           clouds(85, 35, 6);
           //tft.setCursor(34,9);
           //tft.print("Innorat");
        }
        else if ( pres < 757.0 && temp >= 0.0 )
        {
          rain(85, 35, 6);
          //tft.setCursor(29,11);
          //tft.print("Ploaie");
        }
        /*else if ( pres < 757.0 && temp < 0.0 )
        {
          snow(8, 13, 3.9);
          tft.setCursor(29,11);
          tft.print("Ninsoare");
        }*/
 
  tft.setTextColor(YELLOW);
  tft.setCursor(0,21);
  tft.print("T:" );
  tft.print(temp);
  tft.print("'C");
 
  tft.setCursor(0,30);
  tft.print("U:" );
  tft.print(umid);
  tft.print("%");
 
 
  tft.setCursor(0,39);
  tft.print("P:" );
  tft.print(pres);
  tft.print("mmHg");
 
 
  tft.setCursor(0,48);
  tft.print("R:" );
  tft.print(punct_roua);
 
  tft.setTextColor(WHITE);
  tft.setCursor(0, 57);
  tft.print("Baterie:" );
  if(nivel_bat < 20.00){
    tft.setTextColor(REDREAL);
    }
    else tft.setTextColor(RED);
  tft.print(nivel_bat);
  tft.print("%");
 
}
 
void CerereExterior( )
{
  float i=-1;
  while (i==-1) {
    if (rf12_recvDone()){    
    if (rf12_crc == 0 && (rf12_hdr & RF12_HDR_CTL) == 0) {
      int node_id = (rf12_hdr & 0x1F);
      if (node_id == M0_NodeID)  {
        Exterior=*(Cerere*) rf12_data;
        i = 1;
        temp_ext = Exterior.temp;
        umid_ext = Exterior.umid;
        presiune_ext = Exterior.pres;
        pct_roua_ext = Exterior.punct_roua;
        nivel_bat_ext = Exterior.nivel_bat;
       
       // Serial.println();
       // Serial.print("Exterior: Temperatura ");
       // Serial.print(temp_ext);
       
       // Serial.println();
       // Serial.print("Umiditate ");
       // Serial.print(umid_ext);
 
      //   Serial.println();
      //  Serial.print("Presiune ");
      //  Serial.print(presiune_ext);
 
      //   Serial.println();
      //  Serial.print("Punct roua ");
      //  Serial.print(pct_roua_ext);
 
      //   Serial.println();
      //  Serial.print("Nivel baterie ");
      //  Serial.print(nivel_bat_ext);
       
        testText(temp_ext, umid_ext, presiune_ext, pct_roua_ext, nivel_bat_ext);
     //   Serial.println();
        //delay(1000);
      }
    }
    }
  }
}
