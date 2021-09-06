#include <ArduinoBLE.h>
BLEService sensorService("1101");
BLEUnsignedCharCharacteristic pulseLevelChar("2101", BLERead | BLENotify);




void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  while (!Serial);
  
  pinMode(LED_BUILTIN, OUTPUT);
  if (!BLE.begin()) {
  Serial.println("starting BLE failed!");
  while (1);
  }
  
  BLE.setLocalName("RishiSensorRead");
  BLE.setAdvertisedService(sensorService);
  sensorService.addCharacteristic(pulseLevelChar);
  BLE.addService(sensorService);
  
  BLE.advertise();
  Serial.println("Bluetooth device active, waiting for connections...");
}

void loop() {
  // put your main code here, to run repeatedly:
  BLEDevice central = BLE.central();

  if (central) {
  Serial.print("Connected to central: ");
  Serial.println(central.address());
  digitalWrite(LED_BUILTIN, HIGH);

  while (central.connected()) {
      int pulseLevel = analogRead(A0);
      Serial.print("Pulse Level is now: ");
      Serial.println(pulseLevel);
      pulseLevelChar.writeValue(pulseLevel);
      delay(200);
   }
    digitalWrite(LED_BUILTIN, LOW);
    Serial.print("Disconnected from central: ");
  }


}
