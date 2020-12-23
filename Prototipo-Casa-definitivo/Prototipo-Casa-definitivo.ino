#include <ArduinoWebsockets.h>
#include "WiFi.h"
#include "RTClib.h"

String Relay_1 = "ad804cb8"; //Id de relay a encender


int RELAY_1 = 4; //Pin de la placa conectada al relay
const int sensorPin = 2;  //pin de la placa para el flujometro

//Configuraciones flujometro
const int measureInterval = 2500;
volatile int pulseConter;
 
const float factorK = 8.5;

float volume = 0;
long t0 = 0;


bool state = false; //Variable modulo horario
RTC_DS3231 rtc; //Modulo horario

//Coneccio wifi 
bool is_connected = false;
const char *ssid = "ZTE-C16367";
const char *password = "53927124";
const int sensor_data_delta = 10;  // in seconds
#define wifi_timeout 2000

const char *websockets_server = "ws://34.70.117.79:8765/uri"; //server adress and port

String daysOfTheWeek[7] = { "Domingo", "Lunes", "Martes", "Miercoles", "Jueves", "Viernes", "Sabado" };
String monthsNames[12] = { "Enero", "Febrero", "Marzo", "Abril", "Mayo",  "Junio", "Julio","Agosto","Septiembre","Octubre","Noviembre","Diciembre" };
 

using namespace websockets;


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
void setup()
{
  pinMode(RELAY_1, OUTPUT);
  Serial.begin(115200);
  //ConnectToWifi(); Lo saco para que se conecte en el loop

    //Setup del flujometro
   attachInterrupt(digitalPinToInterrupt(sensorPin), ISRCountPulse, RISING);
   t0 = millis();
  
  if (!rtc.begin()) {
      Serial.println(F("Couldn't find RTC"));
      while (1);
   }
 
   // Si se ha perdido la corriente, fijar fecha y hora
   if (rtc.lostPower()) {
      // Fijar a fecha y hora de compilacion
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
      
      // Fijar a fecha y hora específica. En el ejemplo, 21 de Enero de 2016 a las 03:00:00
      // rtc.adjust(DateTime(2016, 1, 21, 3, 0, 0));
   }
   rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}

/**
 * ids
 * ad804cb8
  b0a5f4e2
  b5747c6e
  b31bb41e
  ad804cb2
  b0a5f4e3
  b5747c64
  b31bb415
  ad804cb6
  b0a5f4e7
  b5747c68
  b31bb419
 */
void onPinToggle(String id, String _status)
{
  Serial.print("id : ");
  Serial.println(id);
  Serial.print("status : ");
  Serial.println(_status);

  if ((id == Relay_1) and (_status == "on")){
    digitalWrite (RELAY_1, HIGH);}
  if ((id == Relay_1) and (_status == "off")){
    digitalWrite (RELAY_1, LOW);}
  
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

//Función para imprimir la fecha y hora:
void printDate(DateTime date)
{
   Serial.print(date.year(), DEC);
   Serial.print('/');
   Serial.print(date.month(), DEC);
   Serial.print('/');
   Serial.print(date.day(), DEC);
   Serial.print(" (");
   Serial.print(daysOfTheWeek[date.dayOfTheWeek()]);
   Serial.print(") ");
   Serial.print(date.hour(), DEC);
   Serial.print(':');
   Serial.print(date.minute(), DEC);
   Serial.print(':');
   Serial.print(date.second(), DEC);
   Serial.println();
}

//Funciones para medicion de flujo
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


//Definimos la hora de riego:

bool RelayOnOff(DateTime date)
{
  float hours = date.hour() + date.minute() / 60.0;

  // Riega De 09:30 a 11:30 y de 21:00 a 23:00
  bool hourCondition = (hours > 13.50 && hours < 14.00) || (hours > 20.50 && hours < 21.00); //Los numeros que corresponden a las horas, podrían ser variables que se cambien en la nube
  if (hourCondition)
  {
    return true;
    }
    return false;
  
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



void programa_riego()
{
   DateTime now = rtc.now();
   printDate(now);

   if (state == false && RelayOnOff(now))  // Apagado y debería estar encendido
   {
      digitalWrite (RELAY_1, HIGH);
      //Esto intenté, mejorar
      state = true;
      Serial.println("Riego Activado");
   }
   else if (state == true && !RelayOnOff(now)) // Encendido y debería estar apagado
   {
      digitalWrite (RELAY_1, LOW);
      state = false;
      Serial.println("Riego Desactivado");
   }
}

void loop() 
{
  Serial.println(__DATE__);
  Serial.println(__TIME__);
  
  //Primera prueba: Se conecta al wifi. Lo apago, se intenta reconectar, pero no funciona el programa por horario
  //Segunda prueba: Se conecta al wifi. Lo apago, se enciende el relay (Ya que estamos en el horario) Funciona. (Saqué el connectwifi() del void setup()
  //Tercera prueba. apago el relay por socket, ahora si desconecto el wifi, el relay no se enciende (Apesar de estar en el horario de regado)
  //yo apago el riego, apesar de desconectarse del wifi, sigue estando en estado off (incluso en el horario de riego por modulo)
  //Cuarta prueba: Tendré el wifi conectado y el socket en off. Luego, apagaré el wifi, pero haré que justo en 10 min mas tarde, llegue la hòra de regado.
  //No se enciendó el relay. Eso significa (A priori) que si el estado es off, a pesar de desconectarse, el codigo para encender el relay no funciona
  //Lo dejé correr un rato y algo pasó (Foto que le mande al ricki) es como si la placa se hubiera reseteado y el estado off se hubiera borrado, ya que ahí si se encendió el relay (Incluso 2 min tarde de lo que debería haberse encendido)
  //Quimta prueba: Haré el ejercicio contrario, lo encendí por socket, apagué el wifi y esperaré a la hora en la que se tiene que apagar para ver si se apaga. 
  //Igual que no prueba anterior, esta vez no se apagó el relay.
  //Ahora, si yo reinicio la placa manualmente, este si se apaga.
  //Una posible solución podría ser que cuando no haya wifi y se esté dentro del riego, el valor interno del socket cambie. 
   float frequency = GetFrequency();
 
   // calcular caudal L/min
   float flow_Lmin = frequency / factorK;
   SumVolume(flow_Lmin);

   Serial.print(" Caudal: ");
   Serial.print(flow_Lmin, 3);
   Serial.print(" (L/min)\tConsumo:");
   Serial.print(volume, 1);
   Serial.println(" (L)");
  
  if (!is_connected) 
  {
    programa_riego(); //El error aqui, es que antes se conecta al wifi, pero no al socket, entonces igual ejecuta el código de riego por horario
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
    // 1 seconds = 1000 millis
    if (delta_accumulator > sensor_data_delta * 1000)
    {
       DateTime date = rtc.now();
      delta_accumulator = 0; // reset accumulator
      // humedad
      client.send(String("add:bf9c3de0:") + (date.hour() + date.minute() / 60.0));

      // temperatura
      //client.send(String("add:7d384f6a:") + __DATE__);

      // flujometro
      client.send(String("add:6cedc79c:") + volume);

      
    }
  }

}
