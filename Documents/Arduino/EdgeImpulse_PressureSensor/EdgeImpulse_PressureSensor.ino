#include "Arduino.h"
#include "Nano33BLEPressure.h"

#define FREQUENCY_HZ        100
#define INTERVAL_MS         (1000 / (FREQUENCY_HZ + 1))

Nano33BLEPressureData pressureData;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("Started");
  Pressure.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
  static unsigned long last_interval_ms = 0;
  if (millis() > last_interval_ms + INTERVAL_MS) {
    last_interval_ms = millis();
    if(Pressure.pop(pressureData)){
      Serial.printf("%f\r\n",pressureData.barometricPressure);
    }
  }

}
