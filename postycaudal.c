#include <WiFi.h>
#include <HTTPClient.h>

 
const char* ssid = "iPhone de Nicolás";
const char* password = "Poillpoll123159";
const int sensorPin = 2;
const int measureInterval = 2500;
volatile int pulseConter;

const float factorK = 3.5;
float volume = 0;
long t0 = 0;
 
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
 

void setup() {
 
  Serial.begin(115200);
  attachInterrupt(digitalPinToInterrupt(sensorPin), ISRCountPulse, RISING);
  t0 = millis();
  delay(1000);
 
  WiFi.begin(ssid, password); 
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
 
  Serial.println("Connected to the WiFi network");
}
 
const char* root_ca= \
"-----BEGIN PUBLIC KEY-----\n"\
"MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAsj496jJ99veEXO7WdxGQ\n" \
"Z7idtCnDcjZqQeDiy6057SwXj9yDUVnqhwo/yII8+y6Jpk3g75LpPpYNjiOwYp/J\n" \
"kpWbpBAd1FWlvXJo/eZS+TwuIYb7JSc2H3NDDKt2VV5SSKQdXOkDNqq7BisOFp2/\n" \
"TYwCMZboLufwRR5fKxL0nTKIOCwpnH8k//UdWpvTgIixDGLYQCwHt0fYEo49jFeD\n" \
"aKD4WMBPq6Tx1iKWBhw3HVc/OyvI3yjRAx4Anf/DCSt9YTW6f/ND4O/fOowcfW5T\n" \
"7zii1Kw0yw+ulBrE/xe6taVhL+QR0MXNkQV2iHNN85swidwMtcdGI8g3fYL48bSR\n" \
"ywIDAQAB\n" \
"-----END CERTIFICATE-----\n";

String HTTPGet(String url) {
  if ((WiFi.status() == WL_CONNECTED)) { //Check the current connection status
 
    HTTPClient http;
 
    http.begin("https://api.citylink.cl/" + url, root_ca); //Specify the URL and certificate
    int httpCode = http.GET();                                                  //Make the request
 
    if (httpCode > 0) { //Check for the returning code
 
        String payload = http.getString();
        Serial.println(httpCode);
        return payload;
      }
 
    else {
      Serial.println("Error on HTTP request");
    }
 
    http.end(); //Free the resources
  }

  return "";
}

String HTTPPost(String url, String _data) {
  if ((WiFi.status() == WL_CONNECTED)) { //Check the current connection status
 
    HTTPClient http;
 
    http.begin("https://api.citylink.cl/" + url, root_ca); //Specify the URL and certificate
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    int httpCode = http.POST(_data);                                                  //Make the request
 
    if (httpCode > 0) { //Check for the returning code
 
        String payload = http.getString();
        Serial.println(httpCode);
        return payload;
      }
 
    else {
      Serial.println("Error on HTTP request");
    }
 
    http.end(); //Free the resources
  }

  return "";
}

void loop() {

  //int sensor = digitalRead(1);
  
  // obtener frecuencia en Hz
  float frequency = GetFrequency();

  // calcular caudal L/min
  float flow_Lmin = frequency / factorK;
  SumVolume(flow_Lmin);

  String json = HTTPPost("/metrics/7d384f6a", "value=" + String(flow_Lmin));
  Serial.println(json);

  int val = digitalRead(1);
  Serial.println(val);
 
  delay(10000);
}