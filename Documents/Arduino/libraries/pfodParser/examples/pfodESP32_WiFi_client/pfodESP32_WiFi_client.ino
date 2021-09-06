
// Using ESP32 based board programmed via Arduino IDE
// follow the steps given on http://www.forward.com.au/pfod/ESP32/index.html to install ESP32 support for Arduino IDE

// You need to modify the WLAN_SSID, WLAN_PASS settings below
// to match your network settings

/*
   (c)2014-2018 Forward Computing and Control Pty. Ltd.
   NSW Australia, www.forward.com.au
   This code is not warranted to be fit for any purpose. You may only use it at your own risk.
   This generated code may be freely used for both private and commercial use
   provide this copyright is maintained.
*/

#include <pfodEEPROM.h>
#include <WiFi.h>
#define DEBUG

// Download pfodESP32BufferedClient library from http://www.forward.com.au/pfod/pfodParserLibraries/index.html
#include "pfodESP32Utils.h"
#include "pfodESP32BufferedClient.h"

// Download pfodParser library from http://www.forward.com.au/pfod/pfodParserLibraries/index.html
#include "pfodSecurityClient.h"

pfodESP32BufferedClient bufferedClient;

#define WLAN_SSID       "ssid"        // cannot be longer than 32 characters!
#define WLAN_PASS       "password"

const int portNo = 4989; // What TCP port to listen on for connections.
const char staticIP[] = "10.1.1.222";  // set this the static IP you want to connect to, e.g. "10.1.1.200"

// add your pfod Password here for 128bit security
// eg "b0Ux9akSiwKkwCtcnjTnpWp" but generate your own key, "" means no pfod password
#define pfodSecurityCode ""
// see http://www.forward.com.au/pfod/ArduinoWiFi_simple_pfodDevice/index.html for more information and an example
// and QR image key generator.

WiFiClient client;
boolean alreadyConnected = false; // whether or not the client was connected previously
pfodRingBuffer inputBuffer;
const size_t inputBufferSize = 255 + 2; // max cmds 255;
byte inBuf[inputBufferSize];

pfodSecurityClient pfodClient; // create a parser with menu version string to handle the pfod messages
Stream* rawDataStream = pfodClient.getRawDataStream();

// the setup routine runs once on reset:
void setup() {
  // Explicitly set the ESP32 to be a WiFi-client, otherwise, it by default,
  // would try to act as both a client and an access-point and could cause
  // network-issues with your other WiFi-devices on your WiFi-network.
  WiFi.mode(WIFI_STA);

  inputBuffer.init(inBuf, inputBufferSize);

#ifdef DEBUG
  Serial.begin(115200);
  Serial.println();
#endif
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
#ifdef DEBUG
    Serial.print(".");
#endif
  }
#ifdef DEBUG
  Serial.println();
  Serial.println("WiFi connected");
#endif
  IPAddress local = WiFi.localIP();
#ifdef DEBUG
  Serial.println();
  Serial.print("Local Address:"); Serial.println(local.toString());
#endif
  //pfodClient.setDebugStream(&Serial);  for debuging

  pfodClient.setResponseTimeout(5000);  // in mS
  openConnection();
}

unsigned long connectionRetryTimer;
const unsigned long connectionRetryInterval = 5000; // ms
unsigned long connectionRetry = 0; // ms

void openConnection() {
  // see if we are still pausing after last attempt
  if ((connectionRetry != 0) && ((millis() - connectionRetryTimer) > connectionRetry)) {
    connectionRetry = 0; // timed out
  }
  if (connectionRetry != 0) {
    return;
  }
  IPAddress ip(pfodESP32Utils::ipStrToNum(staticIP));
#ifdef DEBUG
  Serial.println("\nStarting connection to server...");
  Serial.println(ip);
#endif
  if (!client.connect(ip, portNo)) {
#ifdef DEBUG
    Serial.println("Connection failed! reset retry interval");
#endif
    connectionRetry = connectionRetryInterval;
    connectionRetryTimer = millis(); // reset timer and value
    return;
  } // else
#ifdef DEBUG
  Serial.println("Connected to server!");
#endif
  pfodClient.connect(bufferedClient.connect(&client),  F(pfodSecurityCode)); //bufferedClient
}

// the loop routine runs over and over again forever:
void loop() {
  if (rawDataStream->available()) {
    Serial.print("---RawData--");
    while (rawDataStream->available()) {
      Serial.print((char)rawDataStream->read());
    }
    Serial.println("--");
  }
  if (pfodClient.available() != 0) {
    Serial.print("---");
    while (pfodClient.available()) {
      uint8_t c = pfodClient.read();
      if (c == 0xff) {
#ifdef DEBUG
        Serial.println("Link closed (failed)");
#endif
        closeConnection(pfodClient.getLinkStream());
      } else {
        Serial.print((char)c);
      }
    }
    Serial.println("---");
  }

  while (Serial.available()) {
    uint8_t c = Serial.read();
    Serial.print((char)c);
    inputBuffer.write(c);
  }
  if (inputBuffer.available()) {
    if (!client.connected()) {
      openConnection();
      // try again in a few sec if fails
    }
    if (client.connected()) {
      while (inputBuffer.available()) {
        pfodClient.write(inputBuffer.read());
      }
    } // else try again next loop
  }
}


void closeConnection(Stream *io) {
#ifdef DEBUG
  Serial.println(F("closeConnection"));
#endif
  bufferedClient.stop(); // clears client reference
  client.stop(); // cleans up memory
}



