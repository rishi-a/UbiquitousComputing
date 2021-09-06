// pfodESP32 WiFi Bridge
// NOTE: The Sparkfun's ESP32-Thing sometimes (always?) gets caught in boot mode when powered up from a USB power supply.
// i.e not powered/programmed from the computer
// To avoid this, power the board via the 3V3 pin from other board
// Connect the Tx (ESP32) -> Rx (other) and Rx (ESP32) -> Tx (other) with 120R to 330R resistors to protect against mis-wiring

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
// connection between WiFi and Serial2
// uncomment the next line to echo the bridge msgs to the USB Serial
//#define DEBUG

#include <pfodEEPROM.h>
#include <WiFi.h>

// Download pfodESP32BufferedClient library from http://www.forward.com.au/pfod/pfodParserLibraries/index.html
// pfodESP32BufferedClient.zip contains pfodESP32BufferedClient and pfodESP32Utils
#include <pfodESP32Utils.h>
#include <pfodESP32BufferedClient.h>

// download the libraries from http://www.forward.com.au/pfod/pfodParserLibraries/index.html
// pfodParser.zip V3.29+ contains pfodParser, pfodSecurity, pfodBLEBufferedSerial, pfodSMS and pfodRadio
// Download pfodParser library from http://www.forward.com.au/pfod/pfodParserLibraries/index.html
#include <pfodBufferedStream.h>
#include <pfodSecurity.h> 

pfodSecurity parser(""); // create a parser with menu version string to handle the pfod messages
pfodESP32BufferedClient bufferedClient;

const size_t serialBufferSize = 256;
uint8_t serialBuffer[serialBufferSize];
pfodBufferedStream bufferedStream(115200, serialBuffer, serialBufferSize, false); // buffering for sending commands to Uart connection, max cmd is 265 so buffer this much only
// set blocking to false so that if the connected board locks up this board keeps running.

#define WLAN_SSID       "myNetwork"        // cannot be longer than 32 characters!
#define WLAN_PASS       "myWiFiPassword"

const int portNo = 4989; // What TCP port to listen on for connections.
const char staticIP[] = "";  // set this the static IP you want, e.g. "10.1.1.200" or leave it as "" for DHCP. DHCP is not recommended.

// add your pfod Password here for 128bit security
// eg "b0Ux9akSiwKkwCtcnjTnpWp" but generate your own key, "" means no pfod password
#define pfodSecurityCode ""
// see http://www.forward.com.au/pfod/ArduinoWiFi_simple_pfodDevice/index.html for more information and an example
// and QR image key generator.

WiFiServer server(portNo);
WiFiClient client;
boolean alreadyConnected = false; // whether or not the client was connected previously

// led pin on the Sparkfun ESP32 Thing
// the Blue led will come on when there is a WiFi command from pfodApp
const int ledPin = 5;
const unsigned long LED_FLASH_INTERVAL = 500; // 1/2 sec flash each time cmd received from pfodApp
unsigned long ledFlashInterval = 0; // not running
unsigned long ledFlashTimerStart = 0;
// the RED led comes on when there is power 

// serial 1 => rx pin 9, tx pin 10, not available on sparkfun ESP32-Thing
HardwareSerial Serial2(2); // serial 2 => rx pin 16, tx pin 17

// the setup routine runs once on reset:
void setup() {
  // Explicitly set the ESP32 to be a WiFi-client, otherwise, it by default,
  // would try to act as both a client and an access-point and could cause
  // network-issues with your other WiFi-devices on your WiFi-network.
  WiFi.mode(WIFI_STA);
  EEPROM.begin(512);  // only use 20bytes for pfodSecurity but reserve 512

#ifdef DEBUG
  Serial.begin(115200);
  Serial.println();
  //parser.setDebugStream(&Serial); // to debug pfodSecurity. Need to un-comment #define DEBUG.. in pfodSecurity.cpp also
#endif
  Serial2.begin(115200);
  bufferedStream.connect(&Serial2); // buffer output going to Serial2
  parser.setIdleTimeout(6000); // 6sec idle timeout, pfodApp has default keep alive set of 5sec
  
  /* Initialise wifi module */
  if (*staticIP != '\0') {
    IPAddress ip(pfodESP32Utils::ipStrToNum(staticIP));
    IPAddress gateway(ip[0], ip[1], ip[2], 1); // set gatway to ... 1
#ifdef DEBUG
    Serial.print(F("Setting gateway to: "));
    Serial.println(gateway);
#endif
    IPAddress subnet(255, 255, 255, 0);
    WiFi.config(ip, gateway, subnet);
  }
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

  // Start the server
  server.begin();
#ifdef DEBUG
  Serial.println("Server started");
#endif

  // Print the IP address
#ifdef DEBUG
  Serial.println(WiFi.localIP());
#endif

  // initialize client
  client = server.available(); // evaluates to false if no connection
  // <<<<<<<<< Your extra setup code goes here
  // set up connected led
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

}

// the loop routine runs over and over again forever:
void loop() {
  if (!client) { // see if a client is available
    client = server.available(); // evaluates to false if no connection
  } else {
    // have client
    if (!client.connected()) {
      if (alreadyConnected) {
        // client closed so clean up
        closeConnection(parser.getPfodAppStream());
      }
    } else {
      // have connected client
      if (!alreadyConnected) {
        parser.connect(bufferedClient.connect(&client), F(pfodSecurityCode)); // sets new io stream to read from and write to
        EEPROM.commit(); // does nothing if nothing to do
        alreadyConnected = true;
      }

      uint8_t cmd = parser.parse(); // parse incoming data from connection
      // parser returns non-zero when a pfod command is fully parsed
      if (cmd != 0) { // have parsed a complete msg { to }
#ifdef DEBUG
        Serial.println(); Serial.print(F("Incoming cmd:"));
        Serial.println((char*)parser.getRawCmd()); // echo cmds to USB Serial
#endif
        ledFlashInterval = LED_FLASH_INTERVAL;
        ledFlashTimerStart = millis();
        digitalWrite(ledPin, HIGH); // turn led on when we get a connection from pfodApp

        // getRawCmd() gives access to the full command i.e {.} etc to send on via the bridge
        // any secuity hash is removed first
        bufferedStream.print((char*)parser.getRawCmd()); // buffer the cmd to send via Serial at 11500
        // without buffering a 255 byte cmd would block here for about 22mS

        if (cmd == '!') { // if the last cmd was a close connection then close this link
          closeConnection(parser.getPfodAppStream());
        }
      }

      while (bufferedStream.available()) { // check input from LoRa link, via Serial2
        char c = bufferedStream.read();
#ifdef DEBUG
        Serial.write(c); // echo responses to USB Serial
#endif
        parser.write(c); // send response,  this write is buffered in parser, via bufferedClient
      }
    }
  }
  if (!alreadyConnected) {
    // clear any pending input from Serial2 until there is a connection to send it to
    while (bufferedStream.available()) {
      char c = bufferedStream.read();
    }
  }
  
  bufferedStream.available(); // this call will release next char to Serial2 at 115200 baud if needed
    // check if flash has timed out.
  if ((ledFlashInterval > 0) && ((millis() - ledFlashTimerStart) > ledFlashInterval)) {
    ledFlashInterval = 0; // STOP
    digitalWrite(ledPin, LOW);
  }

}

void closeConnection(Stream *io) {
#ifdef DEBUG
  Serial.println(F("closeConnection called"));
#endif
  // add any special code here to force connection to be dropped
  parser.closeConnection(); // nulls io stream
  alreadyConnected = false;
  digitalWrite(ledPin, LOW); // turn off connected LED
  bufferedClient.stop(); // clears client reference
  client.stop(); // cleans up memory
  client = server.available(); // evaluates to false if no connection
}
