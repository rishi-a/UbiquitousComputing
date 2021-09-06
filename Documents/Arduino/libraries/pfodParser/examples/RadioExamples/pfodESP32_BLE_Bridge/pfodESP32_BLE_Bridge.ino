// pfodESP32 BLE Bridge
// NOTE: The Sparkfun's ESP32-Thing sometimes (always?) gets caught in boot mode when powered up from a USB power supply.
// i.e not powered/programmed from the computer
// To avoid this, power the board via the 3V3 pin from other board
// Connect the Tx (ESP32) -> Rx (other) and Rx (ESP32) -> Tx (other) with 120R to 330R resistors to protect against mis-wiring

// Using ESP32 based board programmed via Arduino IDE
// follow the steps given on http://www.forward.com.au/pfod/ESP32/index.html to install ESP32 support for Arduino IDE
//Based on Neil Kolban example https://github.com/nkolban/ESP32_BLE_Arduino
/*
   (c)2014-2018 Forward Computing and Control Pty. Ltd.
   NSW Australia, www.forward.com.au
   This code is not warranted to be fit for any purpose. You may only use it at your own risk.
   This generated code may be freely used for both private and commercial use
   provide this copyright is maintained.
*/

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

// connection between BLE and Serial2
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
#include <pfodBLEBufferedSerial.h> // used to prevent flooding bluetooth sends

// =========== pfodBLESerial definitions
const char* localName = "ESP32 BLE";  // <<<<<<  change this string to customize the adverised name of your board
class pfodBLESerial : public Stream, public BLEServerCallbacks, public BLECharacteristicCallbacks {
  public:
    pfodBLESerial(); void begin(); void poll(); size_t write(uint8_t); size_t write(const uint8_t*, size_t); int read();
    int available(); void flush(); int peek(); void close(); bool isConnected();
    static void addReceiveBytes(const uint8_t* bytes, size_t len);
    const static uint8_t pfodEOF[1]; const static char* pfodCloseConnection;
    volatile static bool connected;
    void onConnect(BLEServer* serverPtr);
    void onDisconnect(BLEServer* serverPtr);
    void onWrite(BLECharacteristic *pCharacteristic);

  private:
    static const int BLE_MAX_LENGTH = 20;
    static const int BLE_RX_MAX_LENGTH = 256; static volatile size_t rxHead; static volatile size_t rxTail;
    volatile static uint8_t rxBuffer[BLE_RX_MAX_LENGTH];
    size_t txIdx;  uint8_t txBuffer[BLE_MAX_LENGTH];
};
volatile size_t pfodBLESerial::rxHead = 0; volatile size_t pfodBLESerial::rxTail = 0;
volatile uint8_t pfodBLESerial::rxBuffer[BLE_RX_MAX_LENGTH]; const uint8_t pfodBLESerial::pfodEOF[1] = {(uint8_t) - 1};
const char* pfodBLESerial::pfodCloseConnection = "{!}"; volatile bool pfodBLESerial::connected = false;

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"
BLEServer *serverPtr = NULL;
BLECharacteristic * characteristicTXPtr;
// =========== end pfodBLESerial definitions

pfodRawCmdParser parser;  // return cmd char plus full cmd { .. } does not parse into cmd + args, use parser.getRawCmd() to get full cmd {..}
// create a parser to handle the pfod messages
pfodBLESerial bleSerial; // create a BLE serial connection
pfodBLEBufferedSerial bleBufferedSerial; // create a buffer for the BLE serial connection

const size_t serialBufferSize = 256;
uint8_t serialBuffer[serialBufferSize];
pfodBufferedStream bufferedStream(115200, serialBuffer, serialBufferSize, false); // buffering for sending commands to Uart connection, max cmd is 265 so buffer this much only
// set blocking to false so that if the connected board locks up this board keeps running.

// serial 1 => rx pin 9, tx pin 10, not available on sparkfun ESP32-Thing
HardwareSerial Serial2(2); // serial 2 => rx pin 16, tx pin 17

// the setup routine runs once on reset:
void setup() {

#ifdef DEBUG
  Serial.begin(115200);
  Serial.println();
#endif

  Serial2.begin(115200);
  bufferedStream.connect(&Serial2); // buffer output going to Serial2

  // Create the BLE Device
  BLEDevice::init(localName);
  // Create the BLE Server
  serverPtr = BLEDevice::createServer();
  serverPtr->setCallbacks(&bleSerial);
  // Create the BLE Service
  BLEService *servicePtr = serverPtr->createService(SERVICE_UUID);
  // Create a BLE Characteristic
  characteristicTXPtr = servicePtr->createCharacteristic(CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_NOTIFY);
  characteristicTXPtr->addDescriptor(new BLE2902());
  BLECharacteristic * characteristicRXPtr = servicePtr->createCharacteristic(CHARACTERISTIC_UUID_RX, BLECharacteristic::PROPERTY_WRITE);
  characteristicRXPtr->setCallbacks(&bleSerial);

  // Start the service
  servicePtr->start();
  // Start advertising
  serverPtr->getAdvertising()->start();
#ifdef DEBUG
  Serial.println("BLE Server and Advertising started");
#endif

  bleSerial.begin();
  // connect parser
  parser.connect(bleBufferedSerial.connect(&bleSerial)); // connect the parser to the i/o stream via buffer
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

// ========== pfodBLESerial methods
pfodBLESerial::pfodBLESerial() {}

bool pfodBLESerial::isConnected() {
  return (connected);
}
void pfodBLESerial::begin() {}

void pfodBLESerial::close() {}

void pfodBLESerial::poll() {}

size_t pfodBLESerial::write(const uint8_t* bytes, size_t len) {
  for (size_t i = 0; i < len; i++) {
    write(bytes[i]);
  }
  return len; // just assume it is all written
}

size_t pfodBLESerial::write(uint8_t b) {
  if (!isConnected()) {
    return 1;
  }
  txBuffer[txIdx++] = b;
  if ((txIdx == sizeof(txBuffer)) || (b == ((uint8_t)'\n')) || (b == ((uint8_t)'}')) ) {
    flush(); // send this buffer if full or end of msg or rawdata newline
  }
  return 1;
}

int pfodBLESerial::read() {
  if (rxTail == rxHead) {
    return -1;
  }
  // note increment rxHead befor writing
  // so need to increment rxTail befor reading
  rxTail = (rxTail + 1) % sizeof(rxBuffer);
  uint8_t b = rxBuffer[rxTail];
  return b;
}

// called as part of parser.parse() so will poll() each loop()
int pfodBLESerial::available() {
  flush(); // send any pending data now. This happens at the top of each loop()
  int rtn = ((rxHead + sizeof(rxBuffer)) - rxTail ) % sizeof(rxBuffer);
  return rtn;
}

void pfodBLESerial::flush() {
  if (txIdx == 0) {
    return;
  }
  characteristicTXPtr->setValue((uint8_t*)txBuffer, txIdx);
  txIdx = 0;
  characteristicTXPtr->notify();
}

int pfodBLESerial::peek() {
  if (rxTail == rxHead) {
    return -1;
  }
  size_t nextIdx = (rxTail + 1) % sizeof(rxBuffer);
  uint8_t byte = rxBuffer[nextIdx];
  return byte;
}

void pfodBLESerial::addReceiveBytes(const uint8_t* bytes, size_t len) {
  // note increment rxHead befor writing
  // so need to increment rxTail befor reading
  for (size_t i = 0; i < len; i++) {
    rxHead = (rxHead + 1) % sizeof(rxBuffer);
    rxBuffer[rxHead] = bytes[i];
  }
}

//=========== ESP32 BLE callback methods
void pfodBLESerial:: onConnect(BLEServer* serverPtr) {
  // clear parser with -1 in case partial message left, should not be one
  addReceiveBytes(bleSerial.pfodEOF, sizeof(pfodEOF));
  connected = true;
}

void pfodBLESerial::onDisconnect(BLEServer* serverPtr) {
  // clear parser with -1 and insert {!} incase connection just lost
  addReceiveBytes(bleSerial.pfodEOF, sizeof(pfodEOF));
  addReceiveBytes((const uint8_t*)pfodCloseConnection, sizeof(pfodCloseConnection));
  connected = false;
}

void pfodBLESerial::onWrite(BLECharacteristic *pCharacteristic) {
  std::string rxValue = pCharacteristic->getValue();
  uint8_t *data = (uint8_t*)rxValue.data();
  size_t len = rxValue.length();
  addReceiveBytes((const uint8_t*)data, len);
}
//======================= end pfodBLESerial methods

