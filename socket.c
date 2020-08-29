#include <ArduinoWebsockets.h>
#include "WiFi.h"

bool is_connected = false;
const char *ssid = "lp";
const char *password = "theewoklinelp";
#define wifi_timeout 2000

const char *websockets_server = "ws://34.70.117.79:8765/uri"; //server adress and port

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
    ConnectToWifi();
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
  Serial.begin(115200);
  ConnectToWifi();
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

void loop()
{
  if (!is_connected)
  {
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
    if (delta_accumulator > 30000)
    {
      delta_accumulator = 0; // reset accumulator
      // humedad
      client.send(String("add:bf9c3de0:") + analogRead(33));

      // temperatura
      client.send(String("add:7d384f6a:") + analogRead(34));

      // flujometro
      client.send(String("add:6cedc79c:") + analogRead(35));
    }
  }

}
