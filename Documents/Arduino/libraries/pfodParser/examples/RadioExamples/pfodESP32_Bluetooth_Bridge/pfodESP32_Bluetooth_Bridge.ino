// pfodESP32 Classic Bluetooth Bridge
// NOTE: The Sparkfun's ESP32-Thing sometimes (always?) gets caught in boot mode when powered up from a USB power supply.
// i.e not powered/programmed from the computer
// To avoid this, power the board via the 3V3 pin from other board
// Connect the Tx (ESP32) -> Rx (other) and Rx (ESP32) -> Tx (other) with 120R to 330R resistors to protect against mis-wiring

// Using ESP32 based board programmed via Arduino IDE
// follow the steps given on http://www.forward.com.au/pfod/ESP32/index.html to install ESP32 support for Arduino IDE
/*
   (c)2014-2018 Forward Computing and Control Pty. Ltd.
   NSW Australia, www.forward.com.au
   This code is not warranted to be fit for any purpose. You may only use it at your own risk.
   This generated code may be freely used for both private and commercial use
   provide this copyright is maintained.
*/

#include <BluetoothSerial.h>
// connection between Classic Bluetooth and Serial2
// uncomment the next line to echo the bridge msgs to the USB Serial
//#define DEBUG

// Download pfodESP32BufferedClient library from http://www.forward.com.au/pfod/pfodParserLibraries/index.html
// pfodESP32BufferedClient.zip contains pfodESP32BufferedClient and pfodESP32Utils
#include <pfodESP32Utils.h>
// download the libraries from http://www.forward.com.au/pfod/pfodParserLibraries/index.html
// pfodParser.zip V3.26+ contains pfodParser, pfodSecurity, pfodBLEBufferedSerial, pfodSMS and pfodRadio
// Download pfodParser library from http://www.forward.com.au/pfod/pfodParserLibraries/index.html
#include <pfodBufferedStream.h>
#include <pfodRawCmdParser.h>
#include <pfodBLEBufferedSerial.h>

pfodRawCmdParser parser;  // return cmd char plus full cmd { .. } does not parse into cmd + args, use parser.getRawCmd() to get full cmd {..}
// create a parser to handle the pfod messages

pfodBLEBufferedSerial bleBufferedSerial; // create a buffer for the BLE serial connection
// used to prevent flooding bluetooth sends

const size_t serialBufferSize = 256;
uint8_t serialBuffer[serialBufferSize];
pfodBufferedStream bufferedStream(115200, serialBuffer, serialBufferSize, false); // buffering for sending commands to Uart connection, max cmd is 265 so buffer this much only
// set blocking to false so that if the connected board locks up this board keeps running.

const char DEVICE_NAME[]  = "ESP32_BT"; // <<<<< set your device name here
BluetoothSerial SerialBT;

// serial 1 => rx pin 9, tx pin 10, not available on sparkfun ESP32-Thing
HardwareSerial Serial2(2); // serial 2 => rx pin 16, tx pin 17

// the setup routine runs once on reset:
void setup() {

#ifdef DEBUG
  Serial.begin(115200);
  Serial.println();
#endif

  SerialBT.begin(DEVICE_NAME); //Bluetooth device name
#ifdef DEBUG
  Serial.println("Bluetooth started, now you can pair it (no pin needed)");
#endif

#ifdef DEBUG
  Serial.println("BLE Server and Advertising started");
#endif

  Serial2.begin(115200);
  bufferedStream.connect(&Serial2); // buffer output going to Serial2

  // connect parser
  parser.connect(bleBufferedSerial.connect(&SerialBT)); // connect the parser to the i/o stream via buffer
}

// the loop routine runs over and over again forever:
void loop() {
  uint8_t cmd = parser.parse(); // parse incoming data from connection
  // parser returns non-zero when a pfod command is fully parsed
  if (cmd != 0) { // have parsed a complete msg { to }
#ifdef DEBUG
    Serial.println(); Serial.print(F("Incoming cmd:"));
    Serial.println((char*)parser.getRawCmd()); // echo cmds to USB Serial
#endif
    // getRawCmd() give access to the full command i.e {.} etc to send on via the bridge
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

  bufferedStream.available(); // this call will release next char to Serial2 at 115200 baud if needed
}

void closeConnection(Stream *io) {
  // nothing special here
}

