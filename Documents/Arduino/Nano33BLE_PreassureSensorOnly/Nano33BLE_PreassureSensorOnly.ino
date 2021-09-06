#include "Arduino.h"
#include "Nano33BLEPressure.h"
#include "Nano33BLETemperature.h"

Nano33BLEPressureData pressureData;
Nano33BLETemperatureData temperatureData;

void setup(){
  Pressure.begin();
  //Temperature.begin();
  //Serial.printf("Preassure, Temperature, Humidity");
}

void loop(){
  /*
  if(Pressure.pop(pressureData) && Temperature.pop(temperatureData)){
      Serial.printf("%f,%f,%f\r\n", pressureData.barometricPressure, temperatureData.temperatureCelsius, temperatureData.humidity);
  }
  */
  if(Pressure.pop(pressureData)){
      Serial.printf("%f\r\n",pressureData.barometricPressure);
  }
  delay(50);
}
