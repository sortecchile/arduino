#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h> 
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
 
/* Set these to your desired credentials. */
const char *ssid = "iPhone de Nicolás";  //ENTER YOUR WIFI SETTINGS
const char *password = "Poillpoll123159";
const int pinDatosDQ = 0; //D3 de wemos
 
//Link to read data from https://jsonplaceholder.typicode.com/comments?postId=7
//Web/Server address to read/write from 
const char *host = "api.citylink.cl";
const int httpsPort = 443;  //HTTPS= 443 and HTTP = 80
 
//SHA1 finger print of certificate use web browser to view and copy
const char fingerprint[] PROGMEM = "F5 07 8D C4 76 6F 6E D5 CE 77 59 EE 93 7A E0 DF 1C 2D 8E EC";
//=======================================================================
//                    Power on setup
//=======================================================================

OneWire oneWireObjeto(pinDatosDQ);
DallasTemperature sensorDS18B20(&oneWireObjeto);

void setup() {
  delay(1000);
  Serial.begin(115200);
  sensorDS18B20.begin(); 
  WiFi.mode(WIFI_OFF);        //Prevents reconnection issue (taking too long to connect)
  delay(1000);
  WiFi.mode(WIFI_STA);        //Only Station No AP, This line hides the viewing of ESP as wifi hotspot
  
  WiFi.begin(ssid, password);     //Connect to your WiFi router
  Serial.println("");
 
  Serial.print("Connecting");
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
 
  //If connection successful show IP address in serial monitor
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());  //IP address assigned to your ESP
}
 
//=======================================================================
//                    Main Program Loop
//=======================================================================
void loop() {
  WiFiClientSecure httpsClient;    //Declare object of class WiFiClient
 
  Serial.println(host);
 
  Serial.printf("Using fingerprint '%s'\n", fingerprint);
  httpsClient.setFingerprint(fingerprint);
  httpsClient.setTimeout(15000); // 15 Seconds
  delay(1000);
  
  Serial.print("HTTPS Connecting");
  int r=0; //retry counter
  while((!httpsClient.connect(host, httpsPort)) && (r < 30)){
      delay(100);
      Serial.print(".");
      r++;
  }
  if(r==30) {
    Serial.println("Connection failed");
  }
  else {
    Serial.println("Connected to web");
  }
    // Mandamos comandos para toma de temperatura a los sensores
    Serial.println("Mandando comandos a los sensores");
    sensorDS18B20.requestTemperatures();
 
    // Leemos y mostramos los datos de los sensores DS18B20
    Serial.print("Temperatura: ");
    Serial.print(sensorDS18B20.getTempCByIndex(0));
    Serial.println(" C");
    int ADCData = int(sensorDS18B20.getTempCByIndex(0));
  
  //String ADCData;
  String Link;
  //int adcvalue= 150;  //Read Analog value of LDR
  //ADCData = String(adcvalue);   //String to interger conversion
 
  //GET Data
 Link = "/metrics/7d384f6a?value=" + ADCData;
 
  Serial.print("requesting URL: ");
  Serial.println(host+Link);
 
  httpsClient.print(String("POST ") + Link + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +               
               "Connection: close\r\n\r\n");
 
  Serial.println("request sent");
                  
  while (httpsClient.connected()) {
    String line = httpsClient.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("headers received");
      break;
    }
  }
 
  Serial.println("reply was:");
  Serial.println("==========");
  String line;
  while(httpsClient.available()){        
    line = httpsClient.readStringUntil('\n');  //Read Line by Line
    Serial.println(line); //Print response
  }
  Serial.println("==========");
  Serial.println("closing connection");
    
  delay(2000);  //GET Data at every 2 seconds
}