#include <ArduinoWebsockets.h>


// Initialize variables
const char *websockets_server = "ws://34.70.117.79:8765/uri";
bool is_connected = false;

// Declare a new WebSocketClient instance
WebsocketsClient client;

/**
 * listen all events from websocket
 * @param event WebSocketEvent type of event triggered
 * @param data  String data received during event execution
 */
void onEventsCallback(WebsocketsEvent event, String data)
{
  if (event == WebsocketsEvent::ConnectionClosed)
  {
    // closed connection cannot be reused
    Serial.println("Connnection Closed");
    is_connected = false;
  }
}


/**
 * Init Websocket client connection
 * @return true if connection is established
 */
bool ConnectWebSocket(WebsocketsClient client, char *websockets_server)
{
  // Setup Callbacks
  client.onEvent(onEventsCallback);

  // Connect to server
  return client.connect(websockets_server);
}

/**
 * Send data to websocket server
 * @param client WebsocketsClient websocket client instance
 * @param String id unique identifier of the chart to update
 * @param float data data to be sent to the chart
 */
void SendData(WebsocketsClient client, String id, float data)
{
  client.send(String("add:") + id + data);
}


void loop()
{
  if (is_connected)
  {
    // send data to id ad804cb8, add : at the end
    SendData(client, String("ad804cb8:"), 10.0);

    // send data to id kdjdsa12, add : at the end
    SendData(client, String("kdjdsa12:"), 11.0);
  }
  else
  {
    // connect websocket
    is_connected = ConnectWebSocket(client, websockets_server);
    delay(3000);  // wait some time to retry connection
  }
}
