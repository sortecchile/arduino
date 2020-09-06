/*
░█████╗░██╗████████╗██╗░░░██╗██╗░░░░░██╗███╗░░██╗██╗░░██╗
██╔══██╗██║╚══██╔══╝╚██╗░██╔╝██║░░░░░██║████╗░██║██║░██╔╝
██║░░╚═╝██║░░░██║░░░░╚████╔╝░██║░░░░░██║██╔██╗██║█████═╝░
██║░░██╗██║░░░██║░░░░░╚██╔╝░░██║░░░░░██║██║╚████║██╔═██╗░
╚█████╔╝██║░░░██║░░░░░░██║░░░███████╗██║██║░╚███║██║░╚██╗
░╚════╝░╚═╝░░░╚═╝░░░░░░╚═╝░░░╚══════╝╚═╝╚═╝░░╚══╝╚═╝░░╚═╝

codigo experimental #1 para control automatico y medicion de flujo a trabes de socket.

*/

//---------[LIBRERIAS]---------
#include <Wire.h>
#include <ArduinoWebsockets.h>
#include "WiFi.h"
#include "Adafruit_MCP23017.h"
#include "Adafruit_SHT31.h"

//---------[ID-RELAYS]---------

String Relay_1 = "ad804cb8";
String Relay_2 = "b0a5f4e2";
String Relay_3 = "b5747c6e";
String Relay_4 = "b31bb41e";
String Relay_5 = "ad804cb2";
String Relay_6 = "b0a5f4e3";
String Relay_7 = "b5747c64";
String Relay_8 = "b31bb415";
String Relay_9 = "ad804cb6";
String Relay_10 = "b0a5f4e7";
String Relay_11 = "b5747c68";
String Relay_12 = "b31bb419";

//---------[PINES]---------
//definir pines de conexion (reles) del integrado MCP23017
int RELAY_1 = 0;
int RELAY_2 = 1;
int RELAY_3 = 2;
int RELAY_4 = 3;   
int RELAY_5 = 4;
int RELAY_6 = 5;
int RELAY_7 = 7; 
int RELAY_8 = 6; 
int RELAY_9 = 8; 
int RELAY_10 = 9;
int RELAY_11 = 11;
int RELAY_12 = 10; 

//---------[CONSTANTES]---------
//Constantes wifi
const char* ssid = "Melocoton";
const char* password = "riesco193";
#define wifi_timeout 2000 


//Constantes flujometro
const int sensorPin = 2;
const int measureInterval = 2500;
volatile int pulseConter;
const float factorK = 3.5;
float volume = 0;
long t0 = 0;

//Constantes socket
const char *websockets_server = "ws://34.70.117.79:8765/uri"; //server adress and port
bool is_connected = false; 
const int sensor_data_delta = 10;  // in seconds


//Constantes del sensor de humedad y temperatura
Adafruit_SHT31 sht31 = Adafruit_SHT31();

//Connstantes del integrado;
Adafruit_MCP23017 mcp;

//---------[FUNCIONES]---------

using namespace websockets; //websocket

//configuracion de la conexion wifi

void ConnectToWifi()  
{
   Serial.println("Conectando al wifi");
   WiFi.mode(WIFI_STA);
   if ( !WiFi.isConnected() ) 
     WiFi.begin(ssid, password);

   unsigned long startAttemptTime = millis(); 

   while(WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < wifi_timeout) 
   {
     Serial.print("...");
     delay(100);
   }

   if(WiFi.status() != WL_CONNECTED) 
   {
     Serial.println ("Fallido!!!"); 
     ConnectToWifi ();
   }
   else
   {
     Serial.print ("conectado!!!");
     Serial.println (WiFi.localIP());
   }
}


//Configuracion del sensor de flujo 
void ISRCountPulse()
{
   pulseConter++;
}
 
float GetFrequency()
{
   pulseConter = 0;
   interrupts();
   delay(measureInterval);
   noInterrupts();
   return (float)pulseConter * 1000 / measureInterval;
}
 
void SumVolume(float dV)
{
   volume += dV / 60 * (millis() - t0) / 1000.0;
   t0 = millis();
}

//Configuracion del WEBSOCKET

WebsocketsClient client;
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
  if (event == WebsocketsEvent::ConnectionOpened)
  {
    Serial.println("Connnection Opened");
  }
  else if (event == WebsocketsEvent::ConnectionClosed)
  {
    Serial.println("Connnection Closed");
    is_connected = false;
  }
  else if (event == WebsocketsEvent::GotPing)
  {
    Serial.println("Got a Ping!");
  }
  else if (event == WebsocketsEvent::GotPong)
  {
    Serial.println("Got a Pong!");
  }
}

void connect()
{
  // Setup Callbacks
  client.onMessage(onMessageCallback);
  client.onEvent(onEventsCallback);

  // Connect to server
  if (!client.connect(websockets_server))
  {
    Serial.println("no connected");
    delay(3000);
    return;
  }

  is_connected = true;
  client.send("connect:" + Relay_1);
  client.send("connect:" + Relay_2);
  client.send("connect:" + Relay_3);
  client.send("connect:" + Relay_4);
  client.send("connect:" + Relay_5);
  client.send("connect:" + Relay_6);
  client.send("connect:" + Relay_7);
  client.send("connect:" + Relay_8);
  client.send("connect:" + Relay_9);
  client.send("connect:" + Relay_10);
  client.send("connect:" + Relay_11);
  client.send("connect:" + Relay_12);
 
  Serial.println("connected");
}

void ConnectSensors()
{
  // Setup Callbacks
  client.onMessage(onMessageCallback);
  client.onEvent(onEventsCallback);

  // Connect to server
  if (!client.connect(websockets_server))
  {
    Serial.println("no connected");
    delay(3000);
    return;
  }

  is_connected = true;
  client.send("connect:" + Relay_1);
  client.send("connect:" + Relay_2);
  client.send("connect:" + Relay_3);
  client.send("connect:" + Relay_4);
  client.send("connect:" + Relay_5);
  client.send("connect:" + Relay_6);
  client.send("connect:" + Relay_7);
  client.send("connect:" + Relay_8);
  client.send("connect:" + Relay_9);
  client.send("connect:" + Relay_10);
  client.send("connect:" + Relay_11);
  client.send("connect:" + Relay_12);
 
  Serial.println("connected");
}

 

//configuracion de las valvulas de riego  
void onPinToggle(String id, String _status)
{
  
  Serial.print("id : ");
  Serial.println(id);
  Serial.print("status : ");
  Serial.println(_status);

  if ((id == Relay_1) and (_status == "on")){
    mcp.digitalWrite (RELAY_1, HIGH);}
  if ((id == Relay_1) and (_status == "off")){
    mcp.digitalWrite (RELAY_1, LOW);}

  if ((id == Relay_2) and (_status == "on")){
    mcp.digitalWrite (RELAY_2, HIGH);}
  if ((id == Relay_2) and (_status == "off")){
    mcp.digitalWrite (RELAY_2, LOW);}
    
  if ((id == Relay_3) and (_status == "on")){
    mcp.digitalWrite (RELAY_3, HIGH);}
  if ((id == Relay_3) and (_status == "off")){
    mcp.digitalWrite (RELAY_3, LOW);}

  if ((id == Relay_4) and (_status == "on")){
    mcp.digitalWrite (RELAY_4, HIGH);}
  if ((id == Relay_4) and (_status == "off")){
    mcp.digitalWrite (RELAY_4, LOW);}

  if ((id == Relay_5) and (_status == "on")){
    mcp.digitalWrite (RELAY_5, HIGH);}
  if ((id == Relay_5) and (_status == "off")){
    mcp.digitalWrite (RELAY_5, LOW);}
    
  if ((id == Relay_6) and (_status == "on")){
    mcp.digitalWrite (RELAY_6, HIGH);}
  if ((id == Relay_6) and (_status == "off")){
    mcp.digitalWrite (RELAY_6, LOW);}

  if ((id == Relay_7) and (_status == "on")){
    mcp.digitalWrite (RELAY_7, HIGH);}
  if ((id == Relay_7) and (_status == "off")){
    mcp.digitalWrite (RELAY_7, LOW);}

  if ((id == Relay_8) and (_status == "on")){
    mcp.digitalWrite (RELAY_8, HIGH);}
  if ((id == Relay_8) and (_status == "off")){
    mcp.digitalWrite (RELAY_8, LOW);}
    
  if ((id == Relay_9) and (_status == "on")){
    mcp.digitalWrite (RELAY_9, HIGH);}
  if ((id == Relay_9) and (_status == "off")){
    mcp.digitalWrite (RELAY_9, LOW);}

  if ((id == Relay_10) and (_status == "on")){
    mcp.digitalWrite (RELAY_10, HIGH);}
  if ((id == Relay_10) and (_status == "off")){
    mcp.digitalWrite (RELAY_10, LOW);}

  if ((id == Relay_11) and (_status == "on")){
    mcp.digitalWrite (RELAY_11, HIGH);}
  if ((id == Relay_11) and (_status == "off")){
    mcp.digitalWrite (RELAY_11, LOW);}
    
  if ((id == Relay_12) and (_status == "on")){
    mcp.digitalWrite (RELAY_12, HIGH);}
  if ((id == Relay_12) and (_status == "off")){
    mcp.digitalWrite (RELAY_12, LOW);}
}

void setup()
{
  Serial.begin (115200); 

  //Definir salidas (relays) del integrado MCP23017
  mcp.begin(); 
  mcp.pinMode(RELAY_1, OUTPUT);
  mcp.pinMode(RELAY_2, OUTPUT);
  mcp.pinMode(RELAY_3, OUTPUT);
  mcp.pinMode(RELAY_4, OUTPUT);
  mcp.pinMode(RELAY_5, OUTPUT);
  mcp.pinMode(RELAY_6, OUTPUT);
  mcp.pinMode(RELAY_7, OUTPUT);
  mcp.pinMode(RELAY_8, OUTPUT);
  mcp.pinMode(RELAY_9, OUTPUT);
  mcp.pinMode(RELAY_10, OUTPUT);
  mcp.pinMode(RELAY_11, OUTPUT);
  mcp.pinMode(RELAY_12, OUTPUT);
  
  //iniciar conexion de wifi
  ConnectToWifi (); 

  
  //Inicia sensor de humedad y temperatura
  if (! sht31.begin(0x44)) 
  {
    Serial.println("Couldn't find SHT31");
  }
  
  attachInterrupt(digitalPinToInterrupt(sensorPin), ISRCountPulse, RISING); //conexion con el sensor y la funcion de interrupcion
  t0 = millis();
}


// Configuración del tempoizador 
float oldTime = 0;
float delta_accumulator = 0;
float CalculateDeltaTime(){
  float currentTime = millis();
  float deltaTime = currentTime - oldTime;
  oldTime = currentTime;
  return deltaTime;
}
 
void loop()
{

  
   //iniciar sensor de flujo
   if (Serial.available()) {
    if(Serial.read()=='r')volume=0;//restablecemos el volumen si recibimos 'r'
   }
   // obtener frecuencia en Hz
   float frequency = GetFrequency();
 
   // calcular caudal L/min
   float flow_Lmin = frequency / factorK;
   SumVolume(flow_Lmin);
 
   Serial.print(" Caudal: ");
   Serial.print(flow_Lmin, 3);
   Serial.print(" (L/min)\tConsumo:");
   Serial.print(volume, 1);
   Serial.println(" (L)");
   
//---------[SOCKET]---------

  if (is_connected)
  {
    delta_accumulator += CalculateDeltaTime();
    // 1 seconds = 1000 millis
    if (delta_accumulator > sensor_data_delta * 1000)
    {
      delta_accumulator = 0; // reset accumulator
      // humedad
      //client.send(String("add:bf9c3de0:") + sht31.readHumidity());

      // temperatura
     // client.send(String("add:7d384f6a:") + sht31.readTemperature());

      // flujometro
     client.send(String("add:6cedc79c:") + flow_Lmin);
    }
  }

  if (!is_connected)
  {
    connect();
  }
  else
  {
    client.poll();
  }



}
