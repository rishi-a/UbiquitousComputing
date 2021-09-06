#include <ArduinoBLE.h>
#include <Arduino_HTS221.h>

float initTemp=1.0;
float initHumid=1.0;


BLEService customService("1101");
BLEUnsignedIntCharacteristic customTemp("2101", BLERead | BLENotify);
BLEUnsignedIntCharacteristic customHumid("2102", BLERead | BLENotify);

void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  
  while (!Serial);
  // put your setup code here, to run once:
  if (!HTS.begin()) {
    Serial.println("Failed to initialize humidity temperature sensor!");
    while (1);
  }

  if (!BLE.begin()) {
  Serial.println("BLE failed to Initiate");
  delay(500);
  while (1);
  }

  BLE.setLocalName("Temperature-Humidity-Room-307");
  BLE.setAdvertisedService(customService);
  customService.addCharacteristic(customTemp);
  customService.addCharacteristic(customHumid);
  BLE.addService(customService);
  customTemp.writeValue(initTemp);
  customHumid.writeValue(initHumid);
  
  BLE.advertise();
  
  Serial.println("Bluetooth device is now active, waiting for connections...");

  }

void loop() {
  // put your main code here, to run repeatedly:
  BLEDevice central = BLE.central();
  if (central) {
    Serial.print("Connected to central: ");
    Serial.println(central.address());
    digitalWrite(LED_BUILTIN, HIGH);
    while (central.connected()) {
    delay(200);
  
    
    float temperature = HTS.readTemperature(CELSIUS);
    float humidity    = HTS.readHumidity();
  
    customTemp.writeValue(temperature);
    customHumid.writeValue(humidity);
  
    Serial.print("At Main Function");
    Serial.print("Humidity    = ");
    Serial.print(humidity);
    Serial.println(" %");
    Serial.print("Temperature = ");
    Serial.print(temperature);
    Serial.println(" °C");
    Serial.println();
    delay(1000);
    }
  }
  digitalWrite(LED_BUILTIN, LOW);
  Serial.print("Disconnected from central: ");
  Serial.println(central.address());
}
