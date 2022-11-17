/*
  Project     : NPK sensor node reader  by IoWLavs
  Description : This is a IoT sensor for measure soil quality
  This device consist of an ESP32 module, an NPK sensor and a LoRa module for
  communicactition. The ESP32 read de data from the sensor, use a ringbuffer
  to filter the data, publish the data through LoRa rf protocol.
  the data is sended in a JSON structure.
  The device use a Industrial Sensor board developed by IoWLabs

  Authors:
    WAC@IOWLAB

  Connections:


    MAX485 CH1  - ESP32    - SENSOR
    ------------------


    MAX485 CH2(NPK)
    ------------------
    A  (Amarillo)
    B  (Azul)
    V+ (Café )
    V- (Negro)

    RS_DI_CH2 - TX2(17)
    RS_RO_CH2 - RX2(16)


    LoRa module
    -----------------
    Lora Rx
    Lora Tx


  CoComunicationmunication:
    - Json structure:
      RCVED MSG:
      {"id":"NODE_ID" "cmd":"CMD_RX","arg":1}
      SENDED MSG:
      {"id":"ain_01", "time" : 456739, "N": 11.23 ,"s2":224.5, "P":230.34,"K":123.123,"resp":"ok"}
    - List of cmds
      - RESET: reset the micro
      - led:  turn of the on bord LED. used for debugging.
          arg = 0  LED off
          arg = 1  LED on
      - pub: publish the state of the sensors

  LIBS:
  the code need the following libraries (all of them availables in the official
  arduino repo):
  - ArduinoJson     bblanchon/ArduinoJson @ ^6.18.0
  - Ring buffer     robtillaart/RunningAverage @ ^0.4.2
  - TimerOne
*/

//-------------
//  LIBS
//-------------
#include <ArduinoWebsockets.h>
#include "WiFi.h"
#include <Arduino.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include <RunningAverage.h>

//-------------
//  DEFINES
//-------------
// PINS

#define RS485_CH1_EN  2
#define RS485_CH2_EN  15
#define I2C_SCL  22
#define I2C_SDA  21

#define SERIAL_RS485_CH1_RX 33
#define SERIAL_RS485_CH1_TX 32
#define SERIAL_RS485_CH2_RX 16
#define SERIAL_RS485_CH2_TX 17

// Version (mayor, minor, patch, build)
#define VERSION    "CitiLinkNPK-v.0.1.0.b.1" //Atlas sensor board
#define ID         "cl-01"

// SAMPLING
#define TIMER_F_SCLK        10000   // TIMER SOURCE CLOCK. IN kHz
#define TIMER_DEFAULT_TS    30      // TIMER DEFAULT PERIOD. IN SECONDS
#define N_SAMPLES           1      // NUMBER OF SAMPLES TO CONSIDERAR IN THE AVERAGE OF SAMPLES

//#define SerialLoRa    Serial1
#define SerialRS485_1 Serial1
#define SerialRS485_2 Serial2
#define BR_LORA     9600
#define BR_DEBUG    9600
#define BR_RS485_1  4800 //needed to npk sensor
#define BR_RS485_2  4800

#define IDLE      0
#define RUNNING   1
#define SLAVE     2
#define TEST      3

#define DEBUG     1
#if DEBUG
#define printdb(s) {Serial.println((s));}
#else
#define printdb(s)
#endif

#define RESPONSE_OK     "OK"
#define RESPONSE_ERROR  "ERROR"

//-------------
// INSTANCES
//-------------
//JSON to SEND
char json_tx[1024];

RunningAverage N_buff(N_SAMPLES);
RunningAverage P_buff(N_SAMPLES);
RunningAverage K_buff(N_SAMPLES);

//Coneccion wifi
bool is_connected     = false;
//const char *ssid      = "ZTE-C16367";
//const char *password  = "53927124";
const char* ssid      = "Red_Taller_ing";//CLaves del taller de ing
const char* password  = "aiC#aim3Eas7gae";//taller de ing
const int sensor_data_delta  = 10;  // in seconds
#define wifi_timeout 2000

const char *websockets_server = "ws://34.70.117.79:8765/uri"; //server adress and port
using namespace websockets;
WebsocketsClient client;


//-------------
// VARIABLES
//-------------

// CMD recived
const char* rcvd_id;
const char* cmd;
int arg = 0;

int mode = RUNNING;
unsigned long sample_time = TIMER_DEFAULT_TS * 1000000;
bool  timer_flag    = false;
bool  publish_flag  = false;
int   count         = 0;

//monitoreables variables
int p  = 0;
int k  = 0;
int n  = 0;

//averaged variables
float p_avg  = 0.0;
float n_avg  = 0.0;
float k_avg  = 0.0;

 const char* response = RESPONSE_OK;

//NTU/SST
byte n_trama[8] = { 0x01,0x03,0x00,0x1E,0x00,0x01,0xE4,0x0C};//read data
byte p_trama[8] = { 0x01,0x03,0x00,0x1F,0x00,0x01,0xB5,0xCC};//read data
byte k_trama[8] = { 0x01,0x03,0x00,0x20,0x00,0x01,0x85,0xC0};//read data
byte rcv_buff [7];

byte temp_buff[2];

// delta
float oldTime = 0;
float delta_accumulator = 0;

//-------------
// FUNCTIONS
//-------------
void clearBuff();
void connectToWifi();
void connect();
void onPinToggle(String id, String _status);
void onMessageCallback(WebsocketsMessage message);
void onEventsCallback(WebsocketsEvent event, String data);
float CalculateDeltaTime();
void publishData();
void publishResponse();
int  readSensor485(uint8_t option);
void readSensor();

void setup()
{
  pinMode(RS485_CH2_EN,OUTPUT);
  pinMode(RS485_CH1_EN,OUTPUT);

  Wire.begin();
  Serial.begin(BR_DEBUG);
  delay(500);

  //SerialRS485_2.begin(BR_RS485_2);
  SerialRS485_2.begin(BR_RS485_2, SERIAL_8N1, SERIAL_RS485_CH2_RX, SERIAL_RS485_CH2_TX);
  delay(100);

  clearBuff();
}

void loop()
{
  if(timer_flag)
  {
    timer_flag = 0;
    readSensor();
    count +=  1;
    if(count >= N_SAMPLES)
    {
      publish_flag = 1;
    }
  }
  if(publish_flag && mode == RUNNING)
  {
    publishData();
    count = 0;
    publish_flag = 0;
  }
  delay(1000);
  timer_flag = true;

}

/*
 clear all the buffs to start
*/
void clearBuff()
{
  N_buff.clear();
  P_buff.clear();
  K_buff.clear();
}

void connectToWifi()
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

void onPinToggle(String id, String _status)
{
  Serial.print("id : ");
  Serial.println(id);
  Serial.print("status : ");
  Serial.println(_status);
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

/*
   Read the sensor data. with option you can chosee the variable to read:
    1 - n
    2 - p
    3 - k
    The result is a int with the data.
    This function specificly use the RS485 CH2
*/
int readSensor485(uint8_t option)
{
  digitalWrite(RS485_CH2_EN,HIGH);
  delay(2000);

  switch (option) {
    case 1:
      SerialRS485_2.write(n_trama,8);
      break;
    case 2:
      SerialRS485_2.write(p_trama,8);
      break;
    case 3:
      SerialRS485_2.write(k_trama,8);
      break;
  }

  SerialRS485_2.flush();
  digitalWrite(RS485_CH2_EN,LOW);
  delay(500);
  while( (SerialRS485_2.available()) == 0){delay(10);}
  for(byte i = 0; i<7 ; i++)
  {
    rcv_buff[i] = SerialRS485_2.read();
    Serial.print(rcv_buff[i],HEX);
  }
  Serial.println(" ");
  return int(rcv_buff[4]);
}

void readSensor()
{
   //readSensor485N();
  n =  readSensor485(1);
  N_buff.addValue(n);
  p =  readSensor485(2);
  P_buff.addValue(p);
  k =  readSensor485(3);
  K_buff.addValue(k);

  if(DEBUG)
  {
    Serial.print("n: ");
    Serial.print(n);
    Serial.print(", ");
    Serial.print("p: ");
    Serial.print(p);
    Serial.print(", ");
    Serial.print("k: ");
    Serial.println(k);
  }
}

float CalculateDeltaTime()
{
  float currentTime = millis();
  float deltaTime = currentTime - oldTime;
  oldTime = currentTime;
  return deltaTime;
}

/*
  get the avg of each buff and then Publish the json data structure by Lora.
  (For debug, also send the json msg on usb serial)
*/
void publishData()
{
  StaticJsonDocument<256> doc_tx;

  p_avg  = P_buff.getAverage();
  n_avg  = N_buff.getAverage();
  k_avg  = K_buff.getAverage();

  doc_tx["id"]    = ID;
  doc_tx["time"]  = millis();
  JsonObject d  = doc_tx.createNestedObject("d");
  d["p"]   = p_avg;
  d["k"]  = k_avg;
  d["n"]    = n_avg;

  String json;
  serializeJson(doc_tx, json);


  if (!is_connected)
  {
    connectToWifi();
    Serial.println("Wifi o socket desconectado");
    connect();
  }
  else{    client.poll();  }

  if (is_connected)
  {
    delta_accumulator += CalculateDeltaTime();
    //1,800,000 = 30 minutos
    //600000 = 10 minutos
    if (delta_accumulator >= 1800000)
    {
      printdb(json);
      // humedad
      client.send(String("add:ad804cb8:") + n);
      // temperatura
      client.send(String("add:b0a5f4e2:") + p);

      // velocidad del viento
      client.send(String("add:b5747c6e:") + k);

      delta_accumulator = 0; // reset accumulator

    }
  }
}
