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
}

void onMessageCallback(WebsocketsMessage message)
{
  Serial.print("Got Message: ");
  Serial.println(message.data());
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
}
