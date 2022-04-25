#include <ArduinoWebsockets.h>
#include "WiFi.h"
#include "RTClib.h"
#include "Adafruit_SHT31.h"
#include <Wire.h>
#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

//Constantes Anemometro y pluviometro:

OneWire ourWire(4);

DallasTemperature sensors(&ourWire);

float PIN_ANEMOMETER = 2;     // Digital 2 (Rojo: 2 , verde: GND)
float PIN_RAINGAUGE = 5;

//Amarillo 21

// How often we want to calculate wind speed or direction
#define MSECS_CALC_WIND_SPEED 5000
#define MSECS_CALC_RAIN_FALL  5000

volatile int numRevsAnemometer = 0;
volatile int numDropsRainGauge = 0;

int nextCalcSpeed;
int nextCalcRain;

time_t now;

//Coneccion wifi
bool is_connected = false;
const char *ssid = "ZTE-C16367";
const char *password = "53927124";
const int sensor_data_delta  = 10;  // in seconds
#define wifi_timeout 2000

const char *websockets_server = "ws://34.70.117.79:8765/uri"; //server adress and port

using namespace websockets;

// Constantes Humedad y temperatura:

Adafruit_SHT31 sht31 = Adafruit_SHT31();




//Funciones Anemometro y pluviometro:

void countRainGauge() {
   numDropsRainGauge++;

}

void countAnemometer() {
   numRevsAnemometer++;

}

void setup() {
   Serial.begin(115200);
   sensors.begin();
   pinMode(PIN_RAINGAUGE, INPUT_PULLUP);
   pinMode(PIN_ANEMOMETER, INPUT_PULLUP);
   digitalWrite(PIN_RAINGAUGE, HIGH);
   digitalWrite(PIN_ANEMOMETER, HIGH);
   attachInterrupt(5, countRainGauge, FALLING);
   attachInterrupt(2, countAnemometer, FALLING);
   nextCalcRain = millis() +  MSECS_CALC_RAIN_FALL;
   nextCalcSpeed = millis() + MSECS_CALC_WIND_SPEED;

      if (! sht31.begin(0x44))
  {
  Serial.println("Couldn't find SHT31");
  while (1) delay(1);
}

}

//Funciones conexión wifi:

void ConnectToWifi()
{
  Serial.println("Conectando al wifi");
  WiFi.mode(WIFI_STA);
  if (!WiFi.isConnected())
    WiFi.begin(ssid, password);

  unsigned long startAttemptTime = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < wifi_timeout)
  {
    Serial.print("...");
    delay(100);
  }

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Fallido!!!");
    //ConnectToWifi();
  }
  else
  {
    Serial.print("conectado!!!");
    Serial.println(WiFi.localIP());
  }
}

WebsocketsClient client;


//Websockets:

void onPinToggle(String id, String _status)
{
  Serial.print("id : ");
  Serial.println(id);
  Serial.print("status : ");
  Serial.println(_status);

  /*if ((id == Relay_1) and (_status == "on")){
    digitalWrite (RELAY_1, HIGH);}
  if ((id == Relay_1) and (_status == "off")){
    digitalWrite (RELAY_1, LOW);}
    */

}

void onMessageCallback(WebsocketsMessage message)
{
  Serial.print("Got Message: ");
  Serial.println(message.data());

  String themessage = message.data();

  int index = themessage.indexOf(":");
  String id = themessage.substring(0, index);
  String _status = themessage.substring(index+1, themessage.length());

  onPinToggle(id, _status);
}

void onEventsCallback(WebsocketsEvent event, String data)
{
  if (event == WebsocketsEvent::ConnectionClosed)
  {
    Serial.println("Connnection Closed");
    is_connected = false;
  }
}

void connect()
{
  // Setup Callbacks
  client.onMessage(onMessageCallback);
  client.onEvent(onEventsCallback);

  // Connect to server
  if (!client.connect("ws://34.70.117.79:8765/uri"))
  {
    Serial.println("no connected");
    delay(3000);
    return;
  }

  is_connected = true;
  client.send("connect:ad804cb8");
  client.send("connect:b0a5f4e2");

  Serial.println("connected");
}

// delta
float oldTime = 0;
float delta_accumulator = 0;
float CalculateDeltaTime(){
  float currentTime = millis();
  float deltaTime = currentTime - oldTime;
  oldTime = currentTime;
  return deltaTime;
}


//=======================================================
// Calculate the wind speed, and display it (or log it, whatever).
// 1 rev/sec = 1.492 mph = 2.40114125 kph
//=======================================================
float calcWindSpeed(int y1, int y2) {
   int x1, x2, iSpeed;
   // This will produce kph * 10
   // (didn't calc right when done as one statement)
   long speed = 24011;
   speed *= y1; //  numRevsAnemometer = y1
   speed /= y2; // MSECS_CALC_WIND_SPEED = y2
   iSpeed = speed;         // Need this for formatting below

   //Serial.print("Wind speed: ");
   //Serial.println(speed);
   //Serial.println(MSECS_CALC_WIND_SPEED);
   x1 = iSpeed / 10;
   //Serial.print("Primer termino:");
   //Serial.print(x1);
   //Serial.print('.');
   //Serial.print("Segundo termino:");
   x2 = iSpeed % 10;
   float x3 = x2;
   //Serial.print(x2);
   //Serial.println();
   numRevsAnemometer = 0;        // Reset counter

   return float (x1 + x3/10);



}


//=======================================================
// Calculate the rain , and display it (or log it, whatever).
// 1 bucket = 0.2794 mm
//=======================================================
float calcRainFall(int y1, int y2) {
   int x1, x2, iVol;
   // This will produce mm * 10000
   // (didn't calc right when done as one statement)
   long vol = 2794; // 0.2794 mm
   vol *= y1;
   vol /= y2;
   iVol = vol;         // Need this for formatting below

   //Serial.print("Rain fall: ");
   x1 = iVol / 10000;
   //Serial.print(x1);
   //Serial.print('.');
   x2 = iVol % 10000;
   //Serial.print(x);
   float x3 = x2;
   //Serial.println();

   //numDropsRainGauge = 0;        // Reset counter
   return float (x1+x3/10000);
}



void loop() {
  now = millis();
  sensors.requestTemperatures();
  float temp= sensors.getTempCByIndex(0);
  float t = sht31.readTemperature();
  float h = sht31.readHumidity();

   //Serial.println(calcWindSpeed(numRevsAnemometer,MSECS_CALC_WIND_SPEED));
   //Serial.println(calcRainFall(numDropsRainGauge, PIN_RAINGAUGE));
   delay(1000);


  if (!is_connected)
  {

    ConnectToWifi();
    Serial.println("Wifi o socket desconectado");
    connect();

  }
    else
  {
    client.poll();
  }

  if (is_connected)
  {

      delta_accumulator += CalculateDeltaTime();

      //1,800,000 = 30 minutos
      //600000 = 10 minutos

      if (delta_accumulator >= 1800000){



      Serial.println("Velocidad del viento; ");
      Serial.println(calcWindSpeed(numRevsAnemometer,MSECS_CALC_WIND_SPEED));
      Serial.println("lluvia:");
      Serial.println(calcRainFall(numDropsRainGauge,PIN_RAINGAUGE));
      Serial.println("Humedad: ");
      Serial.println(h);
      Serial.println("Temperatura: ");
      Serial.println(t);

      Serial.print("Temperatura_tierra= ");
      Serial.print(temp);
      Serial.println(" C");

      // humedad
      client.send(String("add:ad804cb8:") + h);

      // temperatura
      client.send(String("add:b0a5f4e2:") + t);

      // velocidad del viento
      client.send(String("add:b5747c6e:") + int(calcWindSpeed(numRevsAnemometer,MSECS_CALC_WIND_SPEED)*1000));

      // Pluviometro
      client.send(String("add:b31bb41e:") + calcRainFall(numDropsRainGauge,PIN_RAINGAUGE)*1000);

      //
      client.send(String("add:ad804cb2:") + temp);

      delta_accumulator = 0; // reset accumulator

    }
  }
}
