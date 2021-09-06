/**
  pfodSecurity for Arduino
  Parses commands of the form { cmd | arg1 ` arg2 ... } hashCode
  Arguments are separated by `
  The | and the args are optional
  This is a complete paser for ALL commands a pfodApp will send to a pfodDevice

  pfodSecurity is also used to send pfod messages to the pfodApp and add a SipHash code to the end
  of each messge so that the authenticity and correctness of the message can be verified.

  see www.pfod.com.au  for more details and example applications
  see http://www.forward.com.au/pfod/secureChallengeResponse/index.html for the details on the
  128bit security and how to generate a secure secret password

  pfodSecurity adds about 2100 bytes to the program and uses about 400 bytes RAM
  and upto 19 bytes of EEPROM starting from the address passed to init()


  The pfodSecurity parses messages of the form
  { cmd | arg1 ` arg2 ` ... } hashCode
  The hashCode is checked against the SipHash_2_4 using the 128bit secret key.
  The hash input is the message count, the challenge for this connection and the message
  If the hash fails the cmd is set to DisconnectNow ('!')
  It is then the responsability of the calling program to close the connection so another user can connect.

  If the hash passes the message is parsed into the args array
  by replacing '|', '`' and '}' with '/0' (null)

  When the the end of message } is seen
  parse() returns the first byte of the cmd
  getCmd() returns a pointer to the null terminated cmd
  skipCmd() returns a pointer to the first arg (null terminated)
  or a pointer to null if there are no args
  getArgsCount() returns the number of args found.
  These calls are valid until the start of the next msg { is parsed.
  At which time they are reset to empty command and no args.
*/
/*
   (c)2014-2018 Forward Computing and Control Pty. Ltd.
   NSW Australia, www.forward.com.au
   This code is not warranted to be fit for any purpose. You may only use it at your own risk.
   This code may be freely used for both private and commercial use
   Provide this copyright is maintained.
*/

#include "pfodSecurity.h"
#include "SipHash_2_4.h"
#include "pfodParserUtils.h"
#include "HexConversionUtils.h"
#include <string.h>

#define CR 13
#define NL 10

//#define DEBUG_RAWIO
//#define DEBUG_AUTHORIZATION
//#define DEBUG_EEPROM_KEY_UPDATE
#define DEBUG_TIMER
//#define DEBUG_PARSER
//#define DEBUG_ECHO_INPUT
// WARNING do not uncomment this define when connect to the network
// as it sends the expected response as well as the challenge
// ONLY use this via the serial monitor for debugging
//#define DEBUG_DISPLAY_RESPONSE

// program defines DO NOT CHANGE
#define NOT_AUTHORIZING 0
// set in disconnnected
#define AUTHORIZING_START (NOT_AUTHORIZING+1)
// set in connect or from loop if NOT_AUTHORIZING to go from disconnect to AUTHORIZING_START if not call to connect()
#define AUTHORIZING_SENT_CHALLENGE (AUTHORIZING_START+1)
#define AUTHORIZING_SUCCESS (AUTHORIZING_SENT_CHALLENGE+1)
#define AUTHORIZING_CMD '_'

pfodSecurity::pfodSecurity() {
  pfodSecurity("");
}

pfodSecurity::pfodSecurity(const char *_version) : parser(_version) {
  debugOut = NULL;
  pfod_Base_set = NULL;
  doFlush = false; // set to true for SMS only, otherwise false
  initialization = true;
  idleTimeout = 10 * 1000; // timeout use setIdleTimeOut(seconds); to change
  setIdleTimeoutCalled = false;
  closeConnection();
  timeSinceLastConnection = 0;
  parser.ignoreSeqNum();
  timerDebug_ms = millis();
}


/**
   Set the idle Timeout in seconds  i.e. 60 ==> 60sec ==> 1min
   Default is 10sec
   When set to >0 then if no incoming msg received for this time (in sec), then the
   parser will return DisconnectNow to disconnect the link

   Setting this to non-zero value protects against a hacker taking over your connection and just hanging on to it
   not sending any msgs but not releasing it, so preventing you from re-connecting.
   when setting to non-zero pfodApp keepAlive msg for bluetooth and WiFi every 5sec prevent the connection timing out
   while the pfodApp is running. See the pfod Specification for details.

   Set to >0 to enable the timeout (recommended)

   Note: this method does NOT change the timeout for ESP8266_AT, use ESP8266_AT.setIdleTimeout( ) to change this from
   the default 40sec.  A non-zero value is used in the ESP8266_AT because the ESP8266-01 has a link timeout setting.
*/
void pfodSecurity::setIdleTimeout(unsigned long timeout_in_seconds) {
  setIdleTimeoutCalled = true;
  idleTimeout = timeout_in_seconds * 1000;
  if (pfod_Base_set != NULL) {
    // note this only called on the first connect as setIdleTimeout sets setIdleTimeoutCalled == true
    pfod_Base_set->_setLinkTimeout(idleTimeout); //
  }

#ifdef DEBUG_TIMER
  if (debugOut != NULL) {
    debugOut->print(F("setIdleTimeout() set idleTimeout to "));
    debugOut->println(idleTimeout);
  }
#endif
}

// true if no new cmd within idle timeout
bool pfodSecurity::isIdleTimeout() {
  if (connectionTimer == 0) {
  	  return false;
  }

#ifdef DEBUG_TIMER
  if (debugOut != NULL) {
    if (((millis() - timerDebug_ms) > 1000) && (connectionTimer != 0)) {
      timerDebug_ms = millis();
      debugOut->print(F("Connection Timer:"));
      debugOut->println(timerDebug_ms-connectionTimerStart);
    }
  }
#endif

  if ((connectionTimer != 0) && ((millis() - connectionTimerStart) > connectionTimer)) {
    connectionTimer = 0; // clear it now
#ifdef DEBUG_TIMER
    if (debugOut != NULL) {
      debugOut->println(F(" timer timed out, isIdleTimeout returns true."));
    }
#endif
    return true;
  } // else {
  return false;
} 

int pfodSecurity::read() {
#ifdef PFOD_RAW_CMD_PARSER
  return rawCmdParser.read();
#else
  return 0;
#endif
}

int pfodSecurity::available() {
#ifdef PFOD_RAW_CMD_PARSER
  return rawCmdParser.available();
#else
  return 0;
#endif
}

int pfodSecurity::peek() {
#ifdef PFOD_RAW_CMD_PARSER
  return rawCmdParser.peek();
#else
  return 0;
#endif
}

#ifdef PFOD_RAW_CMD_PARSER
byte* pfodSecurity::getRawCmd() {
  return rawCmdParser.getRawCmd();
}
#endif

/**
   Call this before calling connect() if you want the debug output
   to go to some other Stream, other then the communication link

   debugOut is not written to unless you uncomment one of the #define DEBUG_ settings above
*/
void pfodSecurity::setDebugStream(Print* out) {
  debugOut = out;
  mac.setDebugStream(debugOut);
}

/**
   connect(...

   initialize the Security parser
   args
   io_arg the Stream pointer to read and write to for pfod messages

   eepromAddress the starting address in eeprom to save the key and power cycles
      amount of eeprom used is (2 bytes power cycles + 1 byte key length + key bytes) ==> 3 + (input hexKey length)/2,
      use -1 to disable use of EEPROM, pfod_EEPROM.h will also disable EEPROM use for those boards that do not have EEPROm

   hexKey  pointer to program memory F("hexString") holding the key
    if this key is different from the current one in eeprom the eeprom is updated and the
    power cycles are reset to 0xffff
    if changing the key suggest you add 2 to your eepromAddress to move on from the previous
    one.  The power cycle EEPROM addresses are are written to on each power up
    if hexKey is omitted or empty or blank then there is no sercurity used and EEPROM is not used.

*/
void pfodSecurity::connect(pfod_Base* base_arg) {
  connect(base_arg, F(""), 0);
}

void pfodSecurity::connect(pfod_Base* base_arg,  const __FlashStringHelper *hexKeyPgr, int eepromAddress) {
  pfod_Base_set = base_arg;
  doFlush = true; // this is SMS or ESP-AT
  // once pfod_Base_set not null these calls to setIdleTimeout zero the pfodSecurity timeout and leave
  // it to the pfod_Base to handle time out
  if (!setIdleTimeoutCalled) { // user has not called setIdleTimeout yet, use base default setting, user can override this later with a call to setIdleTimeout
    setIdleTimeoutCalled = true;
    if (pfod_Base_set != NULL) {
      // note this only called on the first connect as setIdleTimeout sets setIdleTimeoutCalled == true
      setIdleTimeout(pfod_Base_set->getDefaultTimeOut()); // once pfod_Base_set is not
    } // else no link class
  } // else leave as default 10sec
  if (pfod_Base_set != NULL) {
    pfod_Base_set->_setLinkTimeout(idleTimeout); // in mS  now a null OP,  SMS timesout on idleTimeout timer, Radio timesout on ackTimeout * 1.5 * retries
  }
  connect(base_arg, base_arg->getRawDataOutput() , hexKeyPgr, eepromAddress);
}

void pfodSecurity::connect(Stream* io_arg) {
  connect(io_arg, io_arg);
}

void pfodSecurity::connect(Stream* io_arg, Print* raw_io_arg) {
  connect(io_arg, raw_io_arg, F(""), 0);
}

void pfodSecurity::connect(Stream* io_arg, const __FlashStringHelper *hexKeyPgr_arg, int eepromAddress_arg) {
  connect(io_arg, io_arg, hexKeyPgr_arg, eepromAddress_arg);
}

/**
   connect(...

   initialize the Security parser
   args
   io_arg the Stream pointer to read and write to for pfod messages

   eepromAddress the starting address in eeprom to save the key and power cycles
      amount of eeprom used is (2 bytes power cycles + 1 byte key length + key bytes) ==> 3 + (input hexKey length)/2,
      use -1 to disable use of EEPROM, pfod_EEPROM.h will also disable EEPROM use for those boards that do not have EEPROm

   hexKey  pointer to program memory F("hexString") holding the key
    if this key is different from the current one in eeprom the eeprom is updated and the
    power cycles are reset to 0xffff
    if changing the key suggest you add 2 to your eepromAddress to move on from the previous
    one.  The power cycle EEPROM addresses are are written to on each power up
    if hexKey is omitted or empty or blank then there is no sercurity used and EEPROM is not used.

*/
void pfodSecurity::connect(Stream * io_arg, Print * raw_io_arg, const __FlashStringHelper * hexKeyPgr_arg, int eepromAddress_arg) {
  // max key size for this code which uses SipHash is 128 bits (16 bytes) (32 hex digits)
  // if eepromAddress is <0 or hexKeyPtr NULL then no security and EEPROM not used

  noPassword = false;
  //timeSinceLastConnection = 0;  // don't do this here do in create
  failedHash = false;
  io = io_arg;
  raw_io_connect_arg = raw_io_arg; // save for enabling raw data on connect
  hexKeyPgr = hexKeyPgr_arg;
  eepromAddress = eepromAddress_arg;  // if <0 the eeprom disabled
#ifdef __no_pfodEEPROM__
  eepromAddress = -1; // disable EEPROM usage
#endif

  if (!lastConnectionClosed) { // if close_pfodSecurityConnection called this is skipped
#ifdef DEBUG_AUTHORIZATION
  if (debugOut != NULL) {
    debugOut->println(F(" !lastConnectionClosed call closeConnection() "));
  }
#endif
    closeConnection(); // start with connection closed also calls init()
  }

  init();
  //raw_io = raw_io_arg; // set raw_io AFTER init() set after hand shake on first cmd
  lastConnectionClosed = false;
  
  setAuthorizeState(AUTHORIZING_START);
  parser.connect(io); // clear disconnect not cmd
#ifdef DEBUG_PARSER
  if (debugOut != NULL) {
    debugOut->println(F(" pfodSecurity::connect() parsing true"));
  }
#endif // DEBUG_PARSER
  parsing = true;
  connectionTimer = idleTimeout; // overridded below if no password to idleTimeout
  connectionTimerStart = millis();
#ifdef DEBUG_TIMER
  if (debugOut != NULL) {
    debugOut->print(F(" initialize authorization timer to "));
    debugOut->println(connectionTimer);
  }
#endif

  // if (debugOut == NULL) {
  //   debugOut = io; // debugOut is not written to unless you uncomment one of the #define DEBUG_ settings above
  // }

  // else create mac if null
  mac.setDebugStream(debugOut);
  mac.init(eepromAddress); // disables EEPROM access if eepromAddress < 0

  // now clean up the key and check if it is empty then save in mac
  char hexKey[mac.maxKeySize * 2 + 2]; // max  32 hexDigits for hex key
  const char *p = (const char *)hexKeyPgr;
  size_t n = 0;
  // start by trimming leading and trailing spaces
  int foundNonSpace = 0; // skip leading space
  // may need to rewrite pfodApp to check for utf8 'spaces' currently using java trim() fn.

  while (1) {
    unsigned char c = pgm_read_byte(p++);
    if (c == 0) {
      hexKey[n] = c;
      break; // store terminator
    };

    if ((c <= ' ')  && (!foundNonSpace)) {
      // skip
      continue;
    } else {
      foundNonSpace = 1; // found one
    }
    hexKey[n++] = c;
    if (n >= mac.maxKeySize * 2) {
      hexKey[n] = 0; // limit to mac.maxKeySize*2 hexDigits
      break;
    }
  }

  // now start at back and trim trailing spaces
  for (int i = n - 1; i >= 0; i--) {
    if (hexKey[i] <= ' ') {
      hexKey[i] = '\0';
      n--; // one less
    } else {
      break; // found non-space
    }
  }

#ifdef DEBUG_EEPROM_KEY_UPDATE
  if (debugOut != NULL) {
    debugOut->print(F(" Input Key of length "));
    debugOut->print(n);
    debugOut->println();
  }
#endif

  byte keyBytes[mac.maxKeySize];  // for input key
  int keyLen = 0;

  // ok get max 32 chars (bytes)
  // first check if all hex if so just pad with 0 and convert
  // this is for backward compatibility
  if (isAllHex(hexKey)) {
    keyLen = asciiToHex(hexKey, keyBytes, mac.maxKeySize); // note this handles odd number bytes implies trailing 0
#ifdef DEBUG_EEPROM_KEY_UPDATE
    if (debugOut != NULL) {
      debugOut->print(F(" New AllHex Key of length "));
      debugOut->print(keyLen);
      debugOut->print(F(" 0x"));
      mac.printBytesInHex(debugOut, keyBytes, keyLen);
      debugOut->println();
    }
#endif
  } else {
    // convert as base64 encoded bytes with encoding
    // a..zA..Z0..9() all other chars converted as (54 + (c - '0'))
    keyLen = getBytesFromPassword(hexKey, n, keyBytes, mac.maxKeySize);
#ifdef DEBUG_EEPROM_KEY_UPDATE
    if (debugOut != NULL) {
      debugOut->print(F(" New ascii Key of length "));
      debugOut->print(keyLen);
      debugOut->print(F(" 0x"));
      mac.printBytesInHex(debugOut, keyBytes, keyLen);
      debugOut->println();
    }
#endif
  }

  mac.setSecretKey(keyBytes, keyLen);

#ifdef DEBUG_PARSER
  if (debugOut != NULL) {
    debugOut->println(F(" pfodSecurity::connect() start with connection closed"));
  }
#endif // DEBUG_PARSER
  if (keyLen == 0) {
    noPassword = true;
    startIdleTimeoutTimer();
    setAuthorizeState(AUTHORIZING_SUCCESS);
#ifdef DEBUG_EEPROM_KEY_UPDATE
    if (debugOut != NULL) {
      debugOut->println(F(" No Password"));
      debugOut->print(F(" idle connectionTimer = "));
      debugOut->println(connectionTimer);
    }
#endif
    // no key passed in so do not use security and do not use EEPROM
    mac.noEEPROM(); // do not use EEPROM
#if defined( DEBUG_AUTHORIZATION ) || defined( DEBUG_EEPROM_KEY_UPDATE )
    if (debugOut != NULL) {
      debugOut->println(F(" Zero length key. Security and EEPROM use disabled"));
    }
#endif
  }

  lastMillis = millis(); // initialize for connection timeout
}

/**
   Called on each call to connect(..)
*/
void pfodSecurity::init() {
  setAuthorizeState(NOT_AUTHORIZING);
  raw_io = NULL;
  parser.init();
#ifdef PFOD_RAW_CMD_PARSER  
  rawCmdParser.init();
#endif  
#ifdef DEBUG_PARSER
  if (debugOut != NULL) {
    debugOut->println(F(" pfodSecurity::init() parsing true"));
  }
#endif // DEBUG_PARSER
  parsing = true;
  connectionTimer = 0; // will start on call to connect()
  connectionTimerStart = millis();
#ifdef DEBUG_TIMER
  if (debugOut != NULL) {
    debugOut->print(F("init() clear connectionTimer to "));
    debugOut->println(connectionTimer);
  }
#endif

  inMsgCount = 0;
  outMsgCount = 0;
  msgHashCount = -1; // stop collecting hash
  outputParserState = pfodParser::pfodWaitingForStart;
}

/**
   Call when the parser returns !
   any bytes in the input buffer after ! is returned are dropped
   any bytes recieved while not parsing are dropped
*/
void pfodSecurity::closeConnection() {
#ifdef DEBUG_AUTHORIZATION
  if (debugOut != NULL) {
    debugOut->println(F(" pfodSecurity::closeConnection() called"));
  }
#endif // DEBUG_AUTHORIZATION
  init(); // clears connection timer and sets parsing
  lastConnectionClosed = true;
  if (pfod_Base_set) {
    pfod_Base_set->_closeCurrentConnection(); // just times it out
  }
  parser.closeConnection(); // clears io but that is not a problem
  // also clears out cmd so need to add back ! if needed
#ifdef PFOD_RAW_CMD_PARSER  
  rawCmdParser.closeConnection();
#endif  
//  io = NULL;
}

/**
   Call when get {_} while previous connection still active
*/
void pfodSecurity::close_pfodSecurityConnection() {
#ifdef DEBUG_AUTHORIZATION
  if (debugOut != NULL) {
    debugOut->println(F(" pfodSecurity::close_pfodSecurityConnection() called"));
  }
#endif // DEBUG_AUTHORIZATION
  // init(); // clears connection timer
  lastConnectionClosed = true;
  // skip this for this call
  //  if (pfod_Base_set) {
  //    pfod_Base_set->_closeCurrentConnection(); // just times it out
  //  }
  parser.closeConnection(); // clears io but that is not a problem
  // also clears out cmd so need to add back ! if needed
#ifdef PFOD_RAW_CMD_PARSER  
  rawCmdParser.closeConnection();
#endif  
//  io = NULL;
}

void pfodSecurity::startIdleTimeoutTimer() {
  connectionTimer = idleTimeout;
  connectionTimerStart = millis();
}

/**
   if msg starts with {: the : is dropped from the cmd
   else if msg starts with {vers:  vers is check against current parser version
*/
bool pfodSecurity::isRefresh() {
  return parser.isRefresh();
}

const char* pfodSecurity::getVersionRequested() {
  return parser.getVersionRequested();
}

const char* pfodSecurity::getVersion() {
  return parser.getVersion();
}

void pfodSecurity::sendVersion() {
  print('~');
  print(getVersion());
}

// send `refresh_mS ~ version to parser.print
void pfodSecurity::sendRefreshAndVersion(unsigned long refresh_mS) {
  print('`');
  print(refresh_mS);
  sendVersion();
}

// static
int pfodSecurity::getBytesFromPassword(char *hexKey, int hexKeyLen, byte *keyBytes, int keyMaxLen) {
  int j = 0; // index into keyBytes;
  int k = 0; // index into hexKey
  while (j < keyMaxLen) { // loop until key is full
    // do 4 at a time
    uint32_t p = decodePasswordBytes((byte*)hexKey, k, hexKeyLen); // this handles k>= hexKeyLen padds with zeros
    // store p as 3 bytes in keyBytes
    if (j < keyMaxLen) {
      keyBytes[j++] = (byte)((0x000000ff) & (p >> 16));
    }
    if (j < keyMaxLen) {
      keyBytes[j++] = (byte)((0x000000ff) & (p >> 8));
    }
    if (j < keyMaxLen) {
      keyBytes[j++] = (byte)((0x000000ff) & (p));
    }
    k += 4;
  }
  return j; // should be == keyMaxLen here
}

/**
   uint32_t is 4 bytes so will take 4 6bit bytes
   static
*/
uint32_t pfodSecurity::decodePasswordBytes(byte* bytes, int idx, int bytesLen) {
  uint32_t rtn = 0;
  for (int i = 0; i < 4; i++) {
    // do three
    uint8_t bInt = 0;
    if (idx < bytesLen) {
      bInt = byte64ToByte(bytes[idx]);
      idx++;
    } // else just padd with 0
    rtn = rtn << 6; // shift result and add next 6 bits
    rtn += ((0x0000003f) & ((uint32_t)bInt)); // only add lower 6 bits
  }
  return rtn;
}

/**
   decodes encoded char to 6 bit byte
   Returns 0xff if encode char is invalid
   else returns (0x3f & decoded char)
   static
*/
uint8_t pfodSecurity::byte64ToByte(uint8_t b) {
  uint8_t rtn = 0;
  if ((b >= (uint8_t) 'a') && (b <= (uint8_t) 'z')) {
    rtn = (b - (uint8_t) 'a');
  } else if ((b >= (uint8_t) 'A') && (b <= (uint8_t) 'Z')) {
    rtn = 26 + (b - (uint8_t) 'A');
  } else if (b == (uint8_t) '(') {
    rtn = 62;
  } else if (b == (uint8_t) ')') {
    rtn = 63;
  } else {
    // assume 0 to 9 else if ((b >= (byte) '0') && (b <= (byte) '9')) {
    rtn = 52 + (b - (uint8_t) '0');
  }
  return (0x3F & rtn); // clear upper bits
}

/**
   Return the stream pfodSecurity is writing its output to
   Don't write directly to this stream
   unless forceing a disconnect
*/
Stream* pfodSecurity::getPfodAppStream() {
  return io;
}

/**
   Implement the methods need for print
*/
size_t pfodSecurity::write(uint8_t b) {
  return writeToPfodApp((byte) b);
}

size_t pfodSecurity::write(const uint8_t *buffer, size_t size) {
  size_t rtn = 0;
  for (size_t i = 0; i < size; i++) {
    rtn += writeToPfodApp(buffer[i]);
  }
  return rtn;
}


/**
   Return pointer to start of args[]
*/
byte* pfodSecurity::getCmd() {
  return parser.getCmd();
}

/**
   Return pointer to first arg (or pointer to null if no args)

   Start at args[0] and scan for first null
   if argsCount > 0 increment to point to  start of first arg
   else if argsCount == 0 leave pointing to null
*/
byte* pfodSecurity::getFirstArg() {
  return parser.getFirstArg();
}

/**
   Return pointer to next arg or pointer to null if end of args
   Need to call getFirstArg() first
   Start at current pointer and scan for first null
   if scanned over a non empty arg then skip terminating null and return
   pointer to next arg, else return start if start points to null already
*/
byte* pfodSecurity::getNextArg(byte * start) {
  return parser.getNextArg(start);
}

/**
   Return number of args in last parsed msg
*/
byte pfodSecurity::getArgsCount() {
  return parser.getArgsCount();
}

/**
   parse
   Inputs:
   byte in -- byte read from Serial port
   Return:
   return 0 if complete message not found yet
   else return first char of cmd when see closing }
   or ignore msg if pfodMaxCmdLen bytes after {
   On non-zero return args[] contains
   the cmd null terminated followed by the args null terminated
   argsCount is the number of args

   parses
   { cmd | arg1 ` arg2 ... }
   save the cmd in args[] replace | with null (\0)
   then save arg1,arg2 etc in args[] replaceing ` with null
   count of args saved in argCount
   on seeing } return first char of cmd
   if no } seen for pfodMaxCmdLen bytes after starting { then
   ignore msg and start looking for { again

   States:
   before {   parserState == pfodWaitingForStart
   when   { seen parserState == pfodInMsg
*/

void pfodSecurity::setAuthorizeState(int authState) {
  if (noPassword) {
    authorizing = AUTHORIZING_SUCCESS;
  } else {
    authorizing = authState;
  }
}

/**
   Write bytes to serial out until reach null '/0'
   Inputs:
    idxPtr - byte* pointer to start of bytes to write
   return
*/
size_t pfodSecurity::writeToPfodApp(uint8_t* idxPtr) {
  size_t rtn = 0;
  while (*idxPtr != 0) {
    byte b = *idxPtr;
    rtn += writeToPfodApp(b);
    ++idxPtr;
  }
  return rtn;
}

void pfodSecurity::flush() {
  if (io != NULL) {
    io->flush();
  }
}


// for SMS need to split between rawData and command responses
// command responses take precedence
/**
    write a byte to the connect stream
    If the byte is part of a { } msg then add it to the hash
    and write the hash after the final }
    Raw data, i.e. data outside { } is just passed through
*/
size_t pfodSecurity::writeToPfodApp(uint8_t b) {
  //size_t rtn = 0;
  if (authorizing != AUTHORIZING_SUCCESS) {
    // have not completed authorization yet so just consume all output
    // and tell the program that the byte has been handled
    return 1;
  }
  if (!io) {
    return 1;
  }
  // possible outputParserState values are:-
  // pfodWaitingForStart = 0xff;
  // pfodMsgStarted = '{';
  // pfodInMsg = 0;
  if (outputParserState == pfodParser::pfodWaitingForStart) {
    if (b == pfodParser::pfodMsgStarted) {
      // found { while waiting for start
      outputParserState = pfodParser::pfodMsgStarted;
      return 1; // nothing else to do
    } else {
      // NOT in msg started
      // is outside { }
    }
  } else if (outputParserState == pfodParser::pfodMsgStarted) {
    // {} Empty  {@ Language {. Menu {: Update menu/navigation items {^ Navigation input {= Streaming raw data
    // {' String input {# Numeric input {? Single Selection input {* Multiple  {! close connection
    if ((b == '}') || (b == '@') || (b == '.') || (b == ':') || (b == '^') || (b == '=') || (b == '\'') // '
        || (b == '#') || (b == '?') || (b == '*') || (b == '!') //
        || (b == '+') // dwg
        || (b == ',') || (b == ';')) { // new menus
      // this is real command
      // initial hash
      if (!noPassword) {
#ifdef DEBUG_DISPLAY_RESPONSE
        if (debugOut != NULL) {
          debugOut->println(F(" init output hash"));
        }
#endif
        mac.initHash();
        mac.putByteToHash('{');
      } else {
        // no password skip hash
      }
      // write the { that started this response
      //int rtn_tmp = 0;
      //while (!rtn_tmp) {
        // loop here until write succeeds
        //rtn_tmp = io->write(pfodParser::pfodMsgStarted); // response
        io->write(pfodParser::pfodMsgStarted); // response
      //}
      outputParserState = pfodParser::pfodInMsg; // in msg
    } else {
      // not a real command response so all raw data
      // write out the held back {
      if (raw_io) {
        //int rtn_tmp = 0;
#ifdef DEBUG_RAWIO
        if (debugOut != NULL) {
          debugOut->print(F(" raw_io:"));
          debugOut->print((char)pfodParser::pfodMsgStarted);
        }
#endif
        // don't reset connection timer for raw data rely in pfodApp keepAlive msgs
        //        if (authorizing == AUTHORIZING_SUCCESS) {
        //          connectionTimer = idleTimeout; // clear timer and set idle timeout when sending raw data
        //            connectionTimerStart = millis();
        //       }
        //while (!rtn_tmp) {
          // loop here until write succeeds
          //rtn_tmp = raw_io->write(pfodParser::pfodMsgStarted); // raw data
          raw_io->write(pfodParser::pfodMsgStarted); // raw data
        //}
      } else {
#ifdef DEBUG_RAWIO
        if (debugOut != NULL) {
          //  debugOut->print(F("raw_io null "));
        }
#endif
      }

      if (b == pfodParser::pfodMsgStarted) {
        // {{ go back to pfodMsgStarted
        // hold back second { until next char read
        outputParserState = pfodParser::pfodMsgStarted; // finished
        return 1;
      } else {
        // not a real command just raw data
        outputParserState = pfodParser::pfodWaitingForStart; // finished
        // write this byte below
      }
    }
  } else if (outputParserState == pfodParser::pfodInMsg) {
    // check for } below
  } // else pfodMsgEnd but that should not happen for output

#ifdef DEBUG_DISPLAY_RESPONSE
  if (debugOut != NULL) {
    debugOut->write(b);
  }
#endif

  if (outputParserState == pfodParser::pfodInMsg) {
    //while (!rtn) {
      // loop here until write succeeds
      //rtn = io->write(b); // response
      io->write(b); // response
    //}
    if (!noPassword) {
      // add it to the hash
      mac.putByteToHash(b);
    }
    if (b == pfodParser::pfodMsgEnd) {
      // found closing }
      outputParserState = pfodParser::pfodWaitingForStart; // finished
      if (noPassword) {
        if (doFlush) {
          io->flush();
        }
      } else { // have password
        // create hash and send
#ifdef DEBUG_DISPLAY_RESPONSE
        if (debugOut != NULL) {
          debugOut->println(F(" send hash"));
          debugOut->print(F(" out msg Count "));
          debugOut->println(outMsgCount);
        }
#endif
        mac.putLongToHash(outMsgCount);
        outMsgCount++; // increment for next msg out
        for (int i = 0; i < pfodMAC::challengeByteSize; i++) {
          mac.putByteToHash(challenge[i]);
        }
        mac.finishHash();
#ifdef DEBUG_DISPLAY_RESPONSE
        if (debugOut != NULL) {
          debugOut->println(F(" add hashResult"));
        }
#endif
        // convert to hex
        hexToAscii((const unsigned char *) mac.getHashedResult(), mac.msgHashByteSize, (char*) msgHashBytes,
                   Msg_Hash_Size + 1);
        for (int i = 0; i < Msg_Hash_Size; i++) {
          //i += io->write(msgHashBytes[i]);
          io->write(msgHashBytes[i]);
          // note if output is full may lose byte here and hash at the other end will fail
          // so increment i from return from write so will loop here until write of all bytes is successful
        }
        if (doFlush) {
          io->flush();
        }
      }
    }
  } else { // not pfodMsgStarted or pfodInMsg so must be waiting for start
    // NOTE if was pfodMsgStarted then returned above
    // raw data
    if (raw_io) {
#ifdef DEBUG_RAWIO
      if (debugOut != NULL) {
        //debugOut->print(F(" raw_io:"));
        debugOut->print((char)b);
      }
#endif
      // do not reset connection if sending raw data rely on keepAlive msgs
      //      if (authorizing == AUTHORIZING_SUCCESS) {
      //          connectionTimer = idleTimeout; // clear timer and set idle timeout when sending raw data
      //            connectionTimerStart = millis();
      //      }
      //while (!rtn) {
        // loop here until write succeeds
        //rtn = raw_io->write(b); // response
        raw_io->write(b); // response
      //}
    } else {
#ifdef DEBUG_RAWIO
      if (debugOut != NULL) {
        //   debugOut->print(F("raw_io null "));
      }
#endif
      //rtn = 1;
    }
  }
  return 1; //rtn;
}

/**
   parseLong
   will parse between  -2,147,483,648 to 2,147,483,647
   No error checking done.
   will return 0 for empty string, i.e. first byte == null

   Inputs:
    idxPtr - byte* pointer to start of bytes to parse
    result - long* pointer to where to store result
   return
     byte* updated pointer to bytes after skipping terminator

*/
byte* pfodSecurity::parseLong(byte * idxPtr, long * result) {
  return parser.parseLong(idxPtr, result);
}
byte pfodSecurity::parseDwgCmd() {
  return parser.parseDwgCmd();
}
const byte* pfodSecurity::getDwgCmd() {
  return parser.getDwgCmd();
}

uint8_t pfodSecurity::getTouchType() {
  return parser.getTouchType();
}

bool pfodSecurity::isTouch() {
  return parser.isTouch();
}
bool pfodSecurity::isPress() {
  return parser.isPress();
}
bool pfodSecurity::isClick() {
  return parser.isClick();
}
bool pfodSecurity::isDown() {
  return parser.isDown();
}
bool pfodSecurity::isDrag() {
  return parser.isDrag();
}
bool pfodSecurity::isUp() {
  return parser.isUp();
}
//bool pfodSecurity::isEntry() {
//	return parser.isEntry();
//}
//bool pfodSecurity::isExit() {
//	return parser.isExit();
//}

int pfodSecurity::getTouchedCol() {
  return parser.getTouchedCol();
}

int pfodSecurity::getTouchedRow() {
  return parser.getTouchedRow();
}
//uint8_t pfodSecurity::getRawTouchCol() {
//	return parser.getRawTouchCol();
//}
//uint8_t pfodSecurity::getRawTouchRow() {
//	return parser.getRawTouchRow();
//}

byte pfodSecurity::parse() {
  const byte DisconnectNow = '!';
  unsigned long deltaMillis = 0; // clear last result
  unsigned long thisMillis = millis(); // do this just once to prevent getting different answers from multiple calls to millis()

  if (thisMillis != lastMillis) {
    // we have ticked over
    // calculate how many millis have gone past
    deltaMillis = thisMillis - lastMillis; // note this works even if millis() has rolled over back to 0
    lastMillis = thisMillis;
  }

  timeSinceLastConnection += deltaMillis;
  if (timeSinceLastConnection > MAX_ELAPSED_TIME) {
    // limit it
    timeSinceLastConnection = MAX_ELAPSED_TIME;
  }

  byte cmd = 0;
  byte in = 0;

  if (!io) {
#ifdef DEBUG_ECHO_INPUT
      if (debugOut != NULL) {
        debugOut->println(F("io null return."));
      }
#endif
    return 0; // no stream to read
  }
  // else not in getPasswordMode
  if ((!parsing) || (!mac.isValid())) {  // parsing and (powerCycles > 0 OR no password)
    while (io->available()) {
      in = io->read();
#ifdef DEBUG_ECHO_INPUT
      if (debugOut != NULL) {
        if ((in == NL) || (in == CR)) {
          debugOut->println();
        } else {
          debugOut->print((char)in);
        }
      }
#endif
    }
#ifdef DEBUG_PARSER
    if (debugOut != NULL) {
      debugOut->print(F(" !parsing || !mac.isValid  cmd=DisconnectNow"));
    }
#endif
    cmd = DisconnectNow;
  } else {  // parsing && mac.isValid()
  	if (isIdleTimeout()) {
#ifdef DEBUG_TIMER
        if (debugOut != NULL) {
          debugOut->println(F(" timer timed out, return DisconnectNow, ! "));
        }
#endif
        cmd = DisconnectNow;
    }  		
    // parsing
    if (cmd != DisconnectNow) {
#ifdef DEBUG_ECHO_INPUT
      if (debugOut != NULL) {
        if (io->available()) {
          debugOut->println(F("start io->available() "));
        }
      }
#endif
      //if (io->available()) {
      while ((io->available()) && (!cmd)) {
        in = io->read();
#ifdef DEBUG_ECHO_INPUT
        if (debugOut != NULL) {
          debugOut->print(F(" read "));
          if (in == 0xff) {
            debugOut->print(F("EOF (0xff)"));
          } else {
            debugOut->println((char)in);
          }
        }
#endif
        if (in == 0xff) {
          cmd = DisconnectNow;
          continue; // will break
        } // else
        if ((!noPassword) && (msgHashCount >= 0)) {  // collecting hash
          msgHashBytes[msgHashCount++] = in;
#ifdef DEBUG_DISPLAY_RESPONSE
          if (debugOut != NULL) {
            debugOut->print(F(" hashHex:"));
            debugOut->println((char)in);
          }
#endif
          if (msgHashCount == Msg_Hash_Size) {
            msgHashBytes[Msg_Hash_Size] = 0; //terminate
            // check hash
            msgHashCount = -1;
#ifdef DEBUG_DISPLAY_RESPONSE
            if (debugOut != NULL) {
              debugOut->print(F(" checkMsgHash: inMsgCount:"));
              debugOut->print(inMsgCount);
              debugOut->print(" ");
              mac.printBytesInHex(debugOut, mac.getHashedResult(), Msg_Hash_Size_Bytes);
              debugOut->println();
              debugOut->println((char*)msgHashBytes);
            }
#endif
            if (mac.checkMsgHash(msgHashBytes, Msg_Hash_Size_Bytes)) {
#ifdef DEBUG_ECHO_INPUT
              if (debugOut != NULL) {
                debugOut->println((char)in);
              }
#endif
              cmd = *parser.getCmd(); // reset cmd from parser
              inMsgCount++;
            } else {
#if defined( DEBUG_ECHO_INPUT ) || defined( DEBUG_DISPLAY_RESPONSE )
              if (debugOut != NULL) {
                debugOut->println(F("Hash failed "));
              }
#endif
              failedHash = true;
              cmd = DisconnectNow;
            }
          }
        } else { // not collecting hash
          // if not completed authorization and  authorization timer zero start it now
          if ((authorizing != AUTHORIZING_SUCCESS) && (connectionTimer == 0)) {
            connectionTimer = idleTimeout;
            connectionTimerStart = millis();
#ifdef DEBUG_TIMER
            if (debugOut != NULL) {
              debugOut->print(F(" reset connection timer to "));
              debugOut->print(connectionTimer);
              debugOut->println(F(" because == 0 and not success"));
            }
#endif
          }
          if (authorizing == NOT_AUTHORIZING) {
            setAuthorizeState(AUTHORIZING_START);
          }
#ifdef DEBUG_ECHO_INPUT
          if (debugOut != NULL) {
            if ((in == NL) || (in == CR)) {
              debugOut->println();
            } else {
              debugOut->print((char)in);
            }
          }
#endif
          cmd = parser.parse(in);
#ifdef PFOD_RAW_CMD_PARSER          
          rawCmdParser.parse(in);
#endif          
#ifdef DEBUG_AUTHORIZATION
          if (debugOut != NULL) {
            if (cmd != 0) {
              debugOut->print(F(" parser.parse(in) returned "));
              debugOut->println((char)cmd);
            }
          }
#endif
          // this section for testing reconnection only
          //#ifdef DEBUG_AUTHORIZATION
          //          if (debugOut != NULL) {
          //            if (cmd  == DisconnectNow) {
          //              debugOut->println(F(" ignore ! for testing "));
          //            }
          //          }
          //#endif
          //          if (cmd == DisconnectNow) {
          //            cmd = 0;
          //            parser.setCmd(0);
          //          }

          if (cmd == DisconnectNow) {
            // act on it even without hash
          } else if (authorizing == AUTHORIZING_SUCCESS) {
#ifdef DEBUG_AUTHORIZATION
            if (debugOut != NULL) {
              debugOut->println(F(" AUTHORIZING_SUCCESS"));
            }
#endif
            if (noPassword) {
              // skip this
            } else {
#ifdef DEBUG_AUTHORIZATION
              if (debugOut != NULL) {
                debugOut->println(F(" with password"));
              }
#endif
              if (parser.getParserState() == parser.pfodMsgStarted) {
                // initial hash
#ifdef DEBUG_AUTHORIZATION
                if (debugOut != NULL) {
                  debugOut->println(F(" init hash"));
                }
#endif
                mac.initHash();
                mac.putByteToHash(parser.pfodMsgStarted);
              } else if (parser.getParserState() == parser.pfodInMsg) {
                mac.putByteToHash(in);
              } else if (parser.getParserState() == parser.pfodMsgEnd) {
                if ((cmd == AUTHORIZING_CMD) && (strlen((const char*) parser.getCmd()) == 1) && (parser.getArgsCount() == 0)) {
                  if (authorizing == AUTHORIZING_SENT_CHALLENGE) {
                    // pfodApp trying to start a new connection but we think we are have already started one
                    // i.e. got {_} and send challenge and are waiting for response
                    // note real response will NOT have zero args
                    // so other side just sending {_} followed by {_}
                    // just disconnnect now to resync
#ifdef DEBUG_AUTHORIZATION
                    if (debugOut != NULL) {
                      debugOut->println(F(" got {_} response to AUTHORIZING_SENT_CHALLENGE -- disconnect."));
                    }
#endif
                    cmd = DisconnectNow;
                  } else {
                    // got through challenge and response other side just disconnect and reconnected
                    // else call disconnect /connect
                    close_pfodSecurityConnection();
                    connect(io, raw_io_connect_arg, hexKeyPgr, eepromAddress); // set authorizing to AUTHORIZING_START
                    parser.setCmd(AUTHORIZING_START); // connect initializes parser
                    // Note NO hash for {_} msg
                  }
                } else {
#ifdef DEBUG_DISPLAY_RESPONSE
                  if (debugOut != NULL) {
                    debugOut->println(F(" clear cmd"));
                  }
#endif
                  mac.putByteToHash(in);
                  cmd = 0; // clear cmd
                  msgHashCount = 0;
#ifdef DEBUG_DISPLAY_RESPONSE
                  if (debugOut != NULL) {
                    debugOut->print(F(" msg Count "));
                    debugOut->println(inMsgCount);
                  }
#endif
                  mac.putLongToHash(inMsgCount);
                  for (int i = 0; i < pfodMAC::challengeByteSize; i++) {
                    mac.putByteToHash(challenge[i]);
                  }
                  mac.finishHash();
                }
              } else {
                // ignore
              }

            }
          }
        }
      } //  while ((io->available()) && (!cmd)) {
    } //    if (cmd != DisconnectNow) { from connectionTimer timeout
  } // end  if (!parsing)

  if (cmd != 0) {
    if (cmd == DisconnectNow) {
#ifdef DEBUG_PARSER
      if (debugOut != NULL) {
        debugOut->println(F(" cmd == DisconnectNow "));
      }
#endif
    } else {
#ifdef DEBUG_PARSER
      if (debugOut != NULL) {
        debugOut->print(F(" found cmd "));
        debugOut->println((char)cmd);
      }
#endif
    }
  }

  if ((cmd != DisconnectNow) && (cmd != 0)) {
    if (authorizing == AUTHORIZING_START) { // will not get this state if noPassword see setAuthorizeState()
      if ((cmd == AUTHORIZING_CMD) && (strlen((const char*) parser.getCmd()) == 1) && (parser.getArgsCount() == 0)) {
        // send challenge
        setAuthorizeState(AUTHORIZING_SENT_CHALLENGE);
        connectionTimer = idleTimeout; // reset timer
        connectionTimerStart = millis();
#ifdef DEBUG_TIMER
        if (debugOut != NULL) {
          debugOut->print(F("AUTHORIZING_SENT_CHALLENGE reset connection timer to "));
          debugOut->println(connectionTimer);
        }
#endif
        // get challenge
        long mS = (long) timeSinceLastConnection;
        if (failedHash) {
          mS = -mS;
        }
        mac.buildChallenge(challenge, mS);
        // start again
        timeSinceLastConnection = 0;
        failedHash = false;
        // add the hash type
        challenge[pfodMAC::challengeByteSize] = 2;
        // send challenge
        io->print(F("{"));
        io->print(AUTHORIZING_CMD);
        mac.printBytesInHex(io, challenge, pfodMAC::challengeByteSize + 1);
        io->print(F("}"));
        if (doFlush) {
          io->flush();
        }
#ifdef DEBUG_DISPLAY_RESPONSE
        if (debugOut != NULL) {
          debugOut->println(F("hashed result"));
          mac.printBytesInHex(debugOut, mac.getHashedResult(), pfodMAC::challengeByteSize);
          debugOut->println();
        }
#endif
        cmd = 0; // finished with this command
      } else {
        // command not correct drop connection
#ifdef DEBUG_PARSER
        if (debugOut != NULL) {
          debugOut->println(F(" challenge request missing"));
        }
#endif
        failedHash = true;
        cmd = DisconnectNow;
      }
    } else if (authorizing == AUTHORIZING_SENT_CHALLENGE) {
      // looking for = cmd
      if ((cmd == AUTHORIZING_CMD) && (strlen((const char*) parser.getCmd()) == 1) && (parser.getArgsCount() == 1)) {
        // check response
        if (mac.checkMsgHash((const byte*) parser.getFirstArg(), pfodMAC::challengeHashByteSize)) {
          cmd = 0; //
          setAuthorizeState(AUTHORIZING_SUCCESS);
          startIdleTimeoutTimer();
#ifdef DEBUG_TIMER
          if (debugOut != NULL) {
            debugOut->print(F("AUTHORIZING_SUCCESS reset connection timer to "));
            debugOut->println(connectionTimer);
          }
#endif
#ifdef DEBUG_PARSER
          if (debugOut != NULL) {
            debugOut->println(F(" response matched, initialized idle timout"));
          }
#endif
          io->print(F("{"));
          io->print(AUTHORIZING_CMD);
          io->print(F("}"));
          if (doFlush) {
            io->flush();
          }
          cmd = 0; // finished with this command
        } else {
#ifdef DEBUG_PARSER
          if (debugOut != NULL) {
            debugOut->println(F(" response did not match"));
          }
#endif
          failedHash = true;
          cmd = DisconnectNow;
        }
      } else { // not = or not one arg
        cmd = DisconnectNow;
      }
    } else if (authorizing == AUTHORIZING_SUCCESS) {
      // either no password OR finished auth
      if ((cmd == AUTHORIZING_CMD) && (strlen((const char*) parser.getCmd()) == 1) && (parser.getArgsCount() == 0)) {
        // got {_}
        if (noPassword) {
#ifdef DEBUG_AUTHORIZATION
          if (debugOut != NULL) {
            debugOut->println(F(" no password just send back {}"));
          }
#endif
          //no password just send back {}
          io->print(F("{}"));
          if (doFlush) {
            io->flush();
          }
          cmd = 0;
          parser.init(); //setCmd(0);
#ifdef PFOD_RAW_CMD_PARSER          
          rawCmdParser.init();
#endif          
        } else {
          // have password should not get this
#ifdef DEBUG_AUTHORIZATION
          if (debugOut != NULL) {
            debugOut->println(F(" Authorized with password but got {_}"));
          }
#endif
          failedHash = true;
          cmd = DisconnectNow;
        }
      } // else not {_}
    }
  } //  if ((cmd != DisconnectNow) && (cmd != 0)) {

  if ((cmd == DisconnectNow) || (cmd == '!') || (cmd == AUTHORIZING_CMD)) {
    if (cmd == AUTHORIZING_CMD) {
#ifdef DEBUG_AUTHORIZATION
      if (debugOut != NULL) {
        debugOut->println();
        debugOut->println(F(" found cmd {_  disconnect since invalid"));
      }
#endif
    } else if (cmd == '!') {
#ifdef DEBUG_PARSER
      if (debugOut != NULL) {
        debugOut->println();
        debugOut->println(F(" parsing true and found cmd {!} so return DisconnectNow and set parsing false was true"));
      }
#endif
    }
    cmd = DisconnectNow;
#ifdef DEBUG_PARSER
    if (debugOut != NULL) {
      debugOut->println(F(" set parsing false"));
    }
#endif
    parsing = false;
    msgHashCount = -1;
    setAuthorizeState(NOT_AUTHORIZING);
    connectionTimer = 0; //stop timer
    connectionTimerStart = millis();
#ifdef DEBUG_TIMER
    if (debugOut != NULL) {
      debugOut->print(F(" clear connection timer to "));
      debugOut->print(connectionTimer);
      debugOut->println(F(" because cmd set to DisconnectNow"));
    }
#endif
    parser.setCmd(DisconnectNow);
#ifdef PFOD_RAW_CMD_PARSER    
    rawCmdParser.setDisconnect();
#endif    
  } else {
    if (cmd != 0) {
      if ((authorizing == AUTHORIZING_SUCCESS) && (cmd != 0)) {
        // got another command reset idle timeout
        startIdleTimeoutTimer();
#ifdef DEBUG_TIMER
        if (debugOut != NULL) {
          debugOut->print(F("Got another cmd reset connection timer to "));
          debugOut->println(connectionTimer);
        }
#endif
        // and set raw_io
        raw_io = raw_io_connect_arg; // enable raw data now
        // read the next 8 bytes for hash
#ifdef DEBUG_PARSER
        if (debugOut != NULL) {
          debugOut->println();
          debugOut->print(F(" found cmd "));
          debugOut->println((char)cmd);
        }
#endif
      }
    }
  }
#ifdef DEBUG_PARSER
  if (debugOut != NULL) {
    if (cmd != 0) {
      if (cmd != DisconnectNow) {
        debugOut->print(F(" found cmd "));
        debugOut->println((char)cmd);
      } else {
        debugOut->print(F(" found cmd "));
        debugOut->println(F(" !"));
      }
    }
  }
#endif
  if (cmd == DisconnectNow) { // == '!'
#ifdef DEBUG_AUTHORIZATION
    if (debugOut != NULL) {
      debugOut->println(F("DisconnectNow"));
    }
#endif    
    closeConnection(); // close connection now
    parser.setCmd(DisconnectNow);
#ifdef PFOD_RAW_CMD_PARSER    
    rawCmdParser.setDisconnect();
#endif    
  }
  return cmd;
}

