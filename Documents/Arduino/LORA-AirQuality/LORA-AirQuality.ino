#include "Adafruit_CCS811.h"
#include <SPI.h>
#include <RH_RF95.h>

Adafruit_CCS811 ccs;

/* for feather32u4 */ 
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 7


// Change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ 915.0

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

/*Config for dust sensor*/
int pin = 5;
unsigned long duration;
unsigned long starttime;
unsigned long sampletime_ms = 30000;//sampe 30s ;
unsigned long lowpulseoccupancy = 0;
float ratio = 0;
float concentration = 0;
/*ENDS*/

void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);

  /*Config for dust sensor*/
  pinMode(pin,INPUT);
  starttime = millis();//get the current time;
  /*ENDS*/

  Serial.println("CCS811 test");

  if(!ccs.begin()){
    Serial.println("Failed to start sensor! Please check your wiring.");
    while(1);
  }

  // Wait for the sensor to be ready
  while(!ccs.available());
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);
  delay(100);
  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    while (1);
  }
  Serial.println("LoRa radio init OK!");

  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  }
  Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);
  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then 
  // you can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(2, false);
  //rf95.setSpreadingFactor(12);  
}

int16_t packetnum = 0;  // packet counter, we increment per xmission

void loop(){
  
  if(ccs.available()){
    if(!ccs.readData()){
     char co2_tvoc[8];
     itoa(ccs.geteCO2(), co2_tvoc, 10);
     itoa(ccs.getTVOC(), co2_tvoc+4, 10);
     rf95.send((uint8_t *)co2_tvoc, 8);
     Serial.println("Waiting for packet to complete...");
     rf95.sleep();
     delay(10000); //wait for 40 secods after sending the first packet 
    }
    else{
      Serial.println("ERROR!");
      while(1);
    }
  }
  else{
      digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
      delay(5000);                       // wait for a second
      digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
      delay(5000);                       // wait for a second
    }
  delay(10000);
  /*DUST SENSOR IN ACTION*/
  char dustValue[8];
  duration = pulseIn(pin, LOW);
  lowpulseoccupancy = lowpulseoccupancy+duration;
  if ((millis()-starttime) > sampletime_ms)//if the sampel time == 30s{
      ratio = lowpulseoccupancy/(sampletime_ms*10.0);  // Integer percentage 0=>100
      concentration = 1.1*pow(ratio,3)-3.8*pow(ratio,2)+520*ratio+0.62; // using spec sheet curve
      Serial.print(lowpulseoccupancy);
      Serial.print(",");
      Serial.print(ratio);
      Serial.print(",");
      Serial.println(concentration);
      
      lowpulseoccupancy = 0;
      starttime = millis();
  } 
  
}
