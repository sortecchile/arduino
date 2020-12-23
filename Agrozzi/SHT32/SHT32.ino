#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_SHT31.h"
#include "OneWire.h"
#include "DallasTemperature.h"


//OneWire oneWire(22);
//DallasTemperature tempSensor(&oneWire);

 
Adafruit_SHT31 sht31 = Adafruit_SHT31(); //Amarillo=21=SDA, blanco = 22=SCL


 
void setup()
{
  //tempSensor.begin();


 // bool status;

  // default settings
  // (you can also pass in a Wire library object like &Wire2)
  //status = sht31.begin(0x44, &SHT31); 
  
Serial.begin(115200);
if (! sht31.begin(0x44))
{
Serial.println("Couldn't find SHT31");
while (1) delay(1);
}

}
 
void loop()
{
  
float t = sht31.readTemperature();
float h = sht31.readHumidity();
 
if (! isnan(t))
{
Serial.print("Temp *C = "); Serial.println(t);
}
else
{
Serial.println("Failed to read temperature");
}
 
if (! isnan(h))
{
Serial.print("Hum. % = "); Serial.println(h);
}
else
{
Serial.println("Failed to read humidity");
}
Serial.println();
  /*
  tempSensor.requestTemperaturesByIndex(0);
  Serial.print("Temperature: ");
  Serial.print(tempSensor.getTempCByIndex(0));
  Serial.println(" C");
  */
delay(1000);
}
