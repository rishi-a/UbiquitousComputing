#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include "Adafruit_CCS811.h"
#include "Adafruit_seesaw.h"
 
/* Set these to your desired credentials. */
const char *ssid = "IITGN";  //ENTER YOUR WIFI SETTINGS
const char *password = "12345678";
//int prevVal = LOW;
long ppm = 0.0;
String incoming,val;
float temp;
uint16_t capread;

Adafruit_CCS811 ccs;
Adafruit_seesaw ss;
 
//=======================================================================
//                    Power on setup
//=======================================================================
 
void setup() {
  Serial.begin(9600);
   pinMode(12, OUTPUT);
   pinMode(15, OUTPUT);
   pinMode(13, OUTPUT);
WiFi.mode(WIFI_OFF);//Prevents reconnection issue (taking too long to connect)
  WiFi.mode(WIFI_STA);//This line hides the viewing of ESP as wifi hotspot
  WiFi.begin(ssid, password);     //Connect to your WiFi router
  Serial.println("");
  Serial.print("Connecting");
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");

  if (!ss.begin(0x36)) {
    Serial.println("ERROR! seesaw not found");
  } else {
    Serial.print("seesaw started! version: ");
    Serial.println(ss.getVersion(), HEX);
  }
   
  }
 
  //If connection successful show IP address in serial monitor
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());  //IP address assigned to your ESP
    Serial.println("CCS811 test");
  
  if(!ccs.begin()){
    Serial.println("Failed to start sensor! Please check your wiring.");
    while(1);
  }
  while(!ccs.available());
  float temp = ccs.calculateTemperature();
  ccs.setTempOffset(temp - 25.0);
  
}
 String postData,ccs811;
//=======================================================================
//                    Main Program Loop
//=======================================================================
void loop() {
  HTTPClient http;  ;//Declare object of class HTTPClient
 if(ccs.available()){
  digitalWrite(12,HIGH);
    temp = ccs.calculateTemperature();
    ccs811=temp;
    if(!ccs.readData()){
      ppm=ccs.geteCO2();
      ccs811=ccs811+","+ppm + ","+String(ccs.getTVOC());
     
    }
    else{
      Serial.println("ERROR!");
      while(1);
    }
  }
    delay(100);
    digitalWrite(12,LOW);
    digitalWrite(15,HIGH);

    capread = ss.touchRead(0);
    capread=map(capread, 200, 1200, 0, 100);
    delay(100);
    digitalWrite(15,LOW); 

  digitalWrite(13,HIGH);

  postData = ccs811+","+capread; //+","+ppm2 +"ppm"+","+incoming;
   Serial.println(postData);
  
  http.begin("http://10.0.62.222:5005/"); //10.0.137.171//10.1.139.137
 
  int httpCode = http.POST(postData);   //Send the request
  String payload = http.getString();    //Get the response payload
 
  http.end();  //Close connection
  delay(100);
  digitalWrite(13,LOW);
  delay(5000);  //Post Data at every 5 seconds
}
