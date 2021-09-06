// Feather9x_TX  node Id 0x38 to connect to server 0x68
/**
   Example Feather MO LoRa radio pfodClient using RadioHead library for low level support
   Select Feather MO as the board to compile this sketch
*/
/*
   (c)2014-2018 Forward Computing and Control Pty. Ltd.
   NSW Australia, www.forward.com.au
   This code is not warranted to be fit for any purpose. You may only use it at your own risk.
   This code may be freely used for both private and commercial use
   Provide this copyright is maintained.
*/

#include <SPI.h>
#include <RH_RF95.h>
// download the libraries from http://www.forward.com.au/pfod/pfodParserLibraries/index.html
// pfodParser.zip V3.26+ contains pfodParser, pfodSecurity, pfodBLEBufferedSerial, pfodSMS and pfodRadio

#include <pfodRadio.h>
#include <pfodSecurityClient.h>
#include <pfodBufferedStream.h>

// uncomment the follow line to see the cmds/responses flowing through this bridge
//#define DEBUG

// uncomment the following line to send cmds / receive responses via USB Serial
//#define DEBUG_SEND_VIA_USB

// use last byte of LoRa address as nodeID  (need to check it is unique here)
const uint8_t NODE_ID = 0x38;  // address of this client
const uint8_t SERVER_NODE_ID = 0x68; // address of pfodServer to connect to

// Change to 434.0 or other frequency, must match the RX's, pfodServer's, freq!  AND must match the range allowed in your country.
#define RF95_FREQ 915.0

// add your pfod Password here for 128bit security
// eg "b0Ux9akSiwKkwCtcnjTnpWp" but generate your own key, "" means no pfod password
#define pfodSecurityCode ""
// see http://www.forward.com.au/pfod/ArduinoWiFi_simple_pfodDevice/index.html for more information and an example
// and QR image key generator.

// Radio sends are broken up into blocks of <255 bytes, with measured tx time of 460mS per 255 byte block
// so sends from Server (max 1023 bytes) can take upto 2secs to be received at the client
// Each radio msg is acked. The default ack timeout is 500mS, i.e. random timeout in the range 500mS to 1000mS  (use setAckTimeout(mS) to change this)
// This time allows for another, interfering, radio to finish its attempted transmission
// After 5 retries the link is marked as failed. (use setNoOfRetries() to change this number)
// Most pfod commands are very short, <20 bytes (max 254 bytes), but responses from the pfodServer (pfodDevice) can be upto 1023 bytes.

//==================================== pfodRHDriver ================
// The class to interface with the low lever radio driver.  Must extend from pfodRadioDrive interface
// class implementation is at the bottom of this file.
class pfodRHDriver : public pfodRadioDriver {
  public:
    pfodRHDriver(RHGenericDriver* _driver);  // RHGenericDriver is the low lever RadioHead class used to talk to the radio module
    bool init();  int16_t lastRssi();   int getMode();   uint8_t getMaxMessageLength();   void setThisAddress(uint8_t addr);
    bool receive(uint8_t* buf, uint8_t* len);   bool send(const uint8_t* data, uint8_t len);   uint8_t headerTo();
    uint8_t headerFrom();   uint8_t headerId();   uint8_t headerFlags(); void setHeaderTo(uint8_t to);
    void setHeaderFrom(uint8_t from);   void setHeaderId(uint8_t id);  void setHeaderFlags(uint8_t flags);
  private:
    RHGenericDriver* driver;
};
//============= end of pfodRHDriver header

// defines for RadioHead driver for Feather M0
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3

// Singleton instance of the low level radio driver
RH_RF95 driver(RFM95_CS, RFM95_INT);

pfodRHDriver pfodDriver(&driver); // create the pfodRadioDriver class
pfodRadio radio(&pfodDriver, NODE_ID); // the high level pfod radio driver class
pfodSecurityClient pfodClient;  // the pfodClient with optional 128bit security

Stream* rawDataStream = pfodClient.getRawDataStream();  // the stream where any RawData received is delivered

const size_t serialBufferSize = 1024 + 1024; // +1024 bytes for rawData
uint8_t serialBuffer[serialBufferSize];
pfodBufferedStream bufferedStream(115200, serialBuffer, serialBufferSize); // buffering for sending responses to UART connection, max response is 1024 so buffer at least this much + raw data size
// pfodBufferedStream limit release of data to match baud rate specified to avoid blocking program waiting for UART to send data
// at 11500 it takes about 88mS to send 1024 bytes. Without pfodBufferedStream this would block the program for almost 1/10sec

const int ledPin = 13;
const unsigned long LED_FLASH_INTERVAL = 500; // 1/2 sec flash each time response received server via radio
unsigned long ledFlashInterval = 0; // not running
unsigned long ledFlashTimerStart = 0;

void manualResetRadio() {
  // manual radio reset
  digitalWrite(RFM95_RST, LOW);
  delay(20);
  digitalWrite(RFM95_RST, HIGH);
  delay(100); // wait 0.1 sec
}

void setup() {


#if defined (DEBUG) || defined (DEBUG_SEND_VIA_USB)
  Serial.begin(115200);
#endif

#ifdef DEBUG_SEND_VIA_USB
  bufferedStream.connect(&Serial); // buffer I/O to Serial for send receive via USB
#else  // normal bridge
  Serial1.begin(115200); // Serial I/O for commands / responses.  Bridge to other network
  delay(100);
  bufferedStream.connect(&Serial1); // buffer I/O output to Serial1
#endif

#if defined (DEBUG) || defined (DEBUG_SEND_VIA_USB)
  for (int i = 5; i > 0; i--) {
    Serial.print(i); Serial.print(' ');
    delay(500);
  }
  Serial.println("Feather M0 LoRa pfodClient");
#endif

#ifdef DEBUG
  //radio.setDebugStream(&Serial);  // uncomment this to get debug statements from pfodRadio.  The defines in pfodRadio.cpp determine what is output.
  //pfodClient.setDebugStream(&Serial); // uncomment this to get debug statements from pfodSecurityClient. The defines in pfodSecurityClient.cpp determine what is output.
#endif

  driver.setPromiscuous(false); // only accept our address from low level driver
  // low level drive reset pin setup
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);
  bool radioInitialized = false;
  bool freqChanged = false;
  bool configSet = false;
  while ((!radioInitialized) || (!freqChanged) || (!configSet)) {  // loop here until radio inits
    manualResetRadio(); // delays 0.1sec after
    radioInitialized = radio.init();
    if (!radioInitialized) {
#if defined (DEBUG) || defined (DEBUG_SEND_VIA_USB)
      Serial.println("LoRa radio init failed");
#endif
      delay(1000); // pause and then try again
      continue;
    }
    // setup LoRa radio
    // Defaults after init are 434.0MHz, modulation Bw125Cr45Sf128, +13dbM
    freqChanged = driver.setFrequency(RF95_FREQ);
    if (!freqChanged) {
#if defined (DEBUG) || defined (DEBUG_SEND_VIA_USB)
      Serial.println("LoRa radio freq set failed");
#endif
      delay(1000); // pause and then try again
      continue;
    }
    configSet = driver.setModemConfig(RH_RF95::Bw125Cr45Sf128); // default
    if (!configSet) {
#if defined (DEBUG) || defined (DEBUG_SEND_VIA_USB)
      Serial.println("LoRa radio modem config set failed");
#endif
      delay(1000); // pause and then try again
      continue;
    }
  }
#if defined (DEBUG) || defined (DEBUG_SEND_VIA_USB)
  Serial.println("LoRa radio init OK!");
#endif
#if defined (DEBUG_SEND_VIA_USB)
  Serial.println("Send cmds from Serial Monitor. e.g. {.} gets main menu");
#endif


  driver.setTxPower(23, false);  // set max Tx power
  radio.connectTo(SERVER_NODE_ID); // set the server address to connect to

  pfodClient.setResponseTimeout(5000);  // in mS.  Expect { ... } response from pfodServer within 5secs, else connection lost

#ifdef DEBUG_SEND_VIA_USB
  pfodClient.setKeepAliveInterval(5000); // enable keep alive msg every 5 sec to keep connection open,
  // default is no keepalives from here, the bridge network must supply them
  // send cmd {!} to close the connection
  // connect will be automatically established again on next command sent
#endif

  pfodClient.connect(&radio, F(pfodSecurityCode)); // setup connection. Nothing sent until there is some input

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
}

void loop() {
  bufferedStream.available(); // check each loop for release of more data to uart at 115200

  while (rawDataStream->available()) {
    uint8_t b = rawDataStream->read();
    bufferedStream.write((uint8_t)b);  // comment this line out if you want to ignore any RawData sent by pfodServer
#ifdef DEBUG
#ifndef DEBUG_SEND_VIA_USB
    Serial.write((char)b);
#endif
#endif
  }

  if (pfodClient.available()) {
    ledFlashInterval = LED_FLASH_INTERVAL;
    ledFlashTimerStart = millis();
    digitalWrite(ledPin, HIGH);  // start flash on new response from server

    while (pfodClient.available()) {
      uint8_t c = pfodClient.read();
      bufferedStream.write(c);
#ifdef DEBUG
#ifndef DEBUG_SEND_VIA_USB
     Serial.write((char)c);
#endif
#endif
    }
  }


  while (bufferedStream.available()) {
    uint8_t c = bufferedStream.read();
    pfodClient.write(c); // this write is fully buffered in pfodClient
#if defined (DEBUG) || defined (DEBUG_SEND_VIA_USB)
    Serial.write((char)c);
#endif
  }

  // check if flash has timed out.
  if ((ledFlashInterval > 0) && ((millis() - ledFlashTimerStart) > ledFlashInterval)) {
    ledFlashInterval = 0; // STOP
    digitalWrite(ledPin, LOW);
  }

}

// implementation of pfodRHDriver
pfodRHDriver::pfodRHDriver(RHGenericDriver* _driver) {
  driver = _driver;
}
bool pfodRHDriver::init() {
  return driver->init();
}
int16_t pfodRHDriver::lastRssi() {
  return driver->lastRssi();
}
int pfodRHDriver::getMode() {
  return driver->mode();
}
uint8_t pfodRHDriver::getMaxMessageLength() {
  return driver->maxMessageLength();
}
void pfodRHDriver::setThisAddress(uint8_t addr) {
  driver->setThisAddress(addr);
}
bool pfodRHDriver::receive(uint8_t* buf, uint8_t* len) {
  return driver->recv(buf, len);
}
bool pfodRHDriver::send(const uint8_t* data, uint8_t len) {
  return driver->send(data, len);
}
uint8_t pfodRHDriver::headerTo() {
  return driver->headerTo();
}
uint8_t pfodRHDriver::headerFrom() {
  return driver->headerFrom();
}
uint8_t pfodRHDriver::headerId() {
  return driver->headerId();
}
uint8_t pfodRHDriver::headerFlags() {
  return driver->headerFlags();
}
void pfodRHDriver::setHeaderTo(uint8_t to) {
  driver->setHeaderTo(to);
}
void pfodRHDriver::setHeaderFrom(uint8_t from) {
  driver->setHeaderFrom(from);
}
void pfodRHDriver::setHeaderId(uint8_t id) {
  driver->setHeaderId(id);
}
void pfodRHDriver::setHeaderFlags(uint8_t flags) {
  driver->setHeaderFlags(flags, 0xff);
}
// end of pfodRHDriver implementation

