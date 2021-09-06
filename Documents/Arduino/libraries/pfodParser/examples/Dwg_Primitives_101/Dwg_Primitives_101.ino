
/* ===== pfod Command for Menu_1 ====
  pfodApp msg {.} --> {,~`0~V4|+A~z}
*/
// Using Arduino/Genunio 101 BLE Board (Intel Curie Boards V1.0.7)
// Use Arduino V1.6.8 IDE
/*
 * (c)2014-2017 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provide this copyright is maintained.
 */

#include <CurieBLE.h>
// download the libraries from http://www.forward.com.au/pfod/pfodParserLibraries/index.html
// pfodParser V3.1+, pfodDwgControls V1.0+
#include <pfodParser.h>
#include <pfodDwgControls.h>

int swap01(int); // method prototype for slider end swaps

// =========== pfodBLESerial definitions
const char* localName = "101 BLE";  // <<<<<<  change this string to customize the adverised name of your board (max 8 chars)
class pfodBLESerial : public BLEPeripheral, public Stream {
  public:
    pfodBLESerial(); void begin(); void poll(); size_t write(uint8_t); size_t write(const uint8_t*, size_t); int read();
    int available(); void flush(); int peek(); void close(); bool isConnected();
  private:
    const static uint8_t pfodEOF[1]; const static char* pfodCloseConnection;  static const int BLE_MAX_LENGTH = 20;
    static const int BLE_RX_MAX_LENGTH = 256; static volatile size_t rxHead; static volatile size_t rxTail;
    volatile static uint8_t rxBuffer[BLE_RX_MAX_LENGTH];  volatile static bool connected;
    size_t txIdx;  uint8_t txBuffer[BLE_MAX_LENGTH]; static void connectHandler(BLECentral& central);
    static void disconnectHandler(BLECentral& central); static void receiveHandler(BLECentral& central, BLECharacteristic& rxCharacteristic);
    static void addReceiveBytes(const uint8_t* bytes, size_t len); BLEService uartService = BLEService("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
    BLEDescriptor uartNameDescriptor = BLEDescriptor("2901", localName);
    BLECharacteristic rxCharacteristic = BLECharacteristic("6E400002-B5A3-F393-E0A9-E50E24DCCA9E", BLEWrite, BLE_MAX_LENGTH);
    BLEDescriptor rxNameDescriptor = BLEDescriptor("2901", "RX - (Write)");
    BLECharacteristic txCharacteristic = BLECharacteristic("6E400003-B5A3-F393-E0A9-E50E24DCCA9E", BLEIndicate, BLE_MAX_LENGTH);
    BLEDescriptor txNameDescriptor = BLEDescriptor("2901", "TX - (Indicate)");
};
volatile size_t pfodBLESerial::rxHead = 0; volatile size_t pfodBLESerial::rxTail = 0;
volatile uint8_t pfodBLESerial::rxBuffer[BLE_RX_MAX_LENGTH]; const uint8_t pfodBLESerial::pfodEOF[1] = {(uint8_t) - 1};
const char* pfodBLESerial::pfodCloseConnection = "{!}"; volatile bool pfodBLESerial::connected = false;
// =========== end pfodBLESerial definitions

pfodParser parser(""); // create a parser to handle the pfod messages
pfodBLESerial bleSerial; // create a BLE serial connection

pfodDwgs dwgs(&parser);  // drawing support
// Commands for drawing controls only need to unique per drawing, you can re-use cmds 'a' in another drawing.



// the setup routine runs once on reset:
void setup() {

  // set advertised local name and service UUID
  // begin initialization
  bleSerial.begin();
  parser.connect(&bleSerial);

  // <<<<<<<<< Your extra setup code goes here
}


void loop() {
  uint8_t cmd = parser.parse(); // parse incoming data from connection
  // parser returns non-zero when a pfod command is fully parsed
  if (cmd != 0) { // have parsed a complete msg { to }
    uint8_t* pfodFirstArg = parser.getFirstArg(); // may point to \0 if no arguments in this msg.
    pfod_MAYBE_UNUSED(pfodFirstArg); // may not be used, just suppress warning
    long pfodLongRtn; // used for parsing long return arguments, if any
    pfod_MAYBE_UNUSED(pfodLongRtn); // may not be used, just suppress warning
    if ('.' == cmd) {
      // pfodApp has connected and sent {.} , it is asking for the main menu
      if (!parser.isRefresh()) {
        sendMainMenu(); // send back the menu designed
      } else {
        sendMainMenuUpdate(); // menu is cached just send update
      }

      // now handle commands returned from button/sliders
    } else if ('A' == cmd) { // user pressed drawing loaded with load cmd 'z'
      // in the main Menu of Menu_1
      byte dwgCmd = parser.parseDwgCmd();  // parse rest of dwgCmd, return first char of active cmd
      pfod_MAYBE_UNUSED(dwgCmd); // may not be used, just suppress warning
      sendDrawingUpdates_z(); // update the drawing
      // always send back a response or pfodApp will timeout

    } else if ('z' == cmd) { // pfodApp is asking to load dwg 'z'
      if (!parser.isRefresh()) { // not refresh send whole dwg
        sendDrawing_z();
      } else { // refresh just update drawing state
        sendDrawingUpdates_z();
      }

    } else if ('!' == cmd) {
      // CloseConnection command
      closeConnection(parser.getPfodAppStream());
    } else {
      // unknown command
      parser.print(F("{}")); // always send back a pfod msg otherwise pfodApp will disconnect.
    }
  }
  //  <<<<<<<<<<<  Your other loop() code goes here

}

void closeConnection(Stream *io) {
  // add any special code here to force connection to be dropped
  ((pfodBLESerial*)io)->close();
}

void sendMainMenu() {
  // !! Remember to change the parser version string
  //    every time you edit this method
  parser.print(F("{,"));  // start a Menu screen pfod message
  // send menu background, format, prompt, refresh and version
  parser.print(F("~`0"));
  parser.sendVersion(); // send the menu version
  // send menu items
  parser.print(F("|+A~z"));
  parser.print(F("}"));  // close pfod message
}

void sendMainMenuUpdate() {
  parser.print(F("{;"));  // start an Update Menu pfod message
  // send menu items
  parser.print(F("|A"));
  parser.print(F("}"));  // close pfod message
  // ============ end of menu ===========
}


void sendDrawing_z() {
  dwgs.start(50, 40, dwgs.WHITE); // background defaults to WHITE if omitted i.e. dwgs.start(50,30);
  parser.sendVersion(); // send the parser version to cache this image
  dwgs.pushZero(25, 3);
  dwgs.label().text("Drawing Primitives").fontSize(+3).bold().color(dwgs.NAVY).send();
  dwgs.popZero();
  dwgs.pushZero(0, 6, 1.5);
  dwgs.line().color(dwgs.NAVY).size(9.1, 1.8).offset(3.3, 1.05).send();
  dwgs.rectangle().color(dwgs.BLUE).size(4.2, 2.9).offset(2.02, 4.1).send();
  dwgs.rectangle().color(dwgs.BLUE).filled().size(4.2, 2.9).offset(9.03, 4.1).send();
  dwgs.rectangle().color(dwgs.BLACK).rounded().size(4.2, 2.9).offset(2.03, 8.1).send();
  dwgs.rectangle().color(dwgs.BLACK).filled().rounded().size(4.2, 2.9).offset(9.04, 8.1).send();
  dwgs.arc().color(dwgs.GREEN).start(200).angle(220).radius(2.1).offset(4.2, 14.12).send();
  dwgs.arc().color(dwgs.GREEN).filled().start(-220).angle(-220).radius(2.1).offset(11.2, 14.12).send();
  dwgs.circle().color(dwgs.RED).radius(2).offset(17.5, 5).send();
  dwgs.circle().filled().color(dwgs.RED).filled().radius(2).offset(17.5, 10).send();
  dwgs.popZero();
  dwgs.pushZero(40, 15);
  dwgs.touchZone().cmd('a').size(1, 1).send();
  dwgs.label().fontSize(-1).bold().text(F("touchZone\nSends cmd\nwhen touched.")).offset(0, 10).send();
  dwgs.label().fontSize(-1).italic().text(F("Normally invisible except\nwhen in Debug mode.")).offset(-5, 17).send();
  dwgs.popZero();
  dwgs.end();
}

void sendDrawingUpdates_z() {
  dwgs.startUpdate();
  // nothing to update here
  dwgs.end();
}

// ========== pfodBLESerial methods
pfodBLESerial::pfodBLESerial() : BLEPeripheral() {
  setLocalName(localName);  addAttribute(uartService);  addAttribute(uartNameDescriptor);  setAdvertisedServiceUuid(uartService.uuid());
  addAttribute(rxCharacteristic);  addAttribute(rxNameDescriptor);  rxCharacteristic.setEventHandler(BLEWritten, pfodBLESerial::receiveHandler);
  setEventHandler(BLEConnected, pfodBLESerial::connectHandler);  setEventHandler(BLEDisconnected, pfodBLESerial::disconnectHandler);
  addAttribute(txCharacteristic);  addAttribute(txNameDescriptor);
};

bool pfodBLESerial::isConnected() {
  return (connected && txCharacteristic.subscribed());
}
void pfodBLESerial::begin() {
  BLEPeripheral::begin();
}

void pfodBLESerial::close() {
  BLEPeripheral::disconnect();
}

void pfodBLESerial::poll() {
  BLEPeripheral::poll();
}

size_t pfodBLESerial::write(const uint8_t* bytes, size_t len) {
  for (size_t i = 0; i < len; i++) {
    write(bytes[i]);
  }
  return len; // just assume it is all written
}

size_t pfodBLESerial::write(uint8_t b) {
  BLEPeripheral::poll();
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
  BLEPeripheral::poll();
  flush(); // send any pending data now. This happens at the top of each loop()
  int rtn = ((rxHead + sizeof(rxBuffer)) - rxTail ) % sizeof(rxBuffer);
  return rtn;
}

void pfodBLESerial::flush() {
  if (txIdx == 0) {
    return;
  }
  txCharacteristic.setValue(txBuffer, txIdx);
  txIdx = 0;
  BLEPeripheral::poll();
}

int pfodBLESerial::peek() {
  BLEPeripheral::poll();
  if (rxTail == rxHead) {
    return -1;
  }
  size_t nextIdx = (rxTail + 1) % sizeof(rxBuffer);
  uint8_t byte = rxBuffer[nextIdx];
  return byte;
}

void pfodBLESerial::connectHandler(BLECentral& central) {
  // clear parser with -1 incase partial message left
  // should not be one
  addReceiveBytes(pfodEOF, sizeof(pfodEOF));
  connected = true;
}

void pfodBLESerial::disconnectHandler(BLECentral& central) {
  // parser.closeConnection();
  // clear parser with -1 and insert {!} incase connection just lost
  addReceiveBytes(pfodEOF, sizeof(pfodEOF));
  addReceiveBytes((const uint8_t*)pfodCloseConnection, sizeof(pfodCloseConnection));
  connected = false;
}

void pfodBLESerial::addReceiveBytes(const uint8_t* bytes, size_t len) {
  // note increment rxHead befor writing
  // so need to increment rxTail befor reading
  for (size_t i = 0; i < len; i++) {
    rxHead = (rxHead + 1) % sizeof(rxBuffer);
    rxBuffer[rxHead] = bytes[i];
  }
}

void pfodBLESerial::receiveHandler(BLECentral& central, BLECharacteristic& rxCharacteristic) {
  size_t len = rxCharacteristic.valueLength();
  const unsigned char *data = rxCharacteristic.value();
  addReceiveBytes((const uint8_t*)data, len);
}
//======================= end pfodBLESerial methods


int swap01(int in) {
  return (in == 0) ? 1 : 0;
}
// ============= end generated code =========

