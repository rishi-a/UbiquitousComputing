	/**
  pfodMAC for Arduino
  Updates power cycles, builds the Challenge, keeps track in message counts and
  hashes and check message hashes
  It uses upto 19 bytes of EEPROM starting from the address passed to init()
*/
/*
   (c)2014-2018 Forward Computing and Control Pty. Ltd.
   NSW Australia, www.forward.com.au
   This code is not warranted to be fit for any purpose. You may only use it at your own risk.
   This code may be freely used for both private and commercial use
   Provide this copyright is maintained.
*/

#include "pfodMAC.h"
#include "pfodRandom.h"
#include "HexConversionUtils.h"
#include "SipHash_2_4.h"
#include "pfodParserUtils.h"

//#define DEBUG_BUILD_CHALLENGE
//#define DEBUG_CHAP_EEPROM
//#define DEBUG_HASH
// this next define will keep the challange the same for ALL connections.
// MUST only be used for testing, make use this line is commented out for REAL use
//#define DEBUG_DISABLE_POWER_AND_CONNECTION_COUNTERS

// the current address in the EEPROM (i.e. which byte
// we're going to write to next)
const  uint8_t pfodMAC::powerCycleSize = 2; // uint16_t
const  uint8_t pfodMAC::counterSize = 2; // uint16_t
const  uint8_t pfodMAC::timerSize = 4; // uint32_t

const  uint8_t pfodMAC::PowerCyclesOffset = 0;
const  uint8_t pfodMAC::KeyOffset = pfodMAC::PowerCyclesOffset + pfodMAC::powerCycleSize; // length comes first then key

const uint8_t pfodMAC::maxKeySize;// = 16;


uint8_t pfodMAC::eeprom_read(int address) {
#ifdef __no_pfodEEPROM__
	(void)(address);
  return 0;
#else
  return EEPROM.read(address);
#endif
}

void pfodMAC::eeprom_write(int address, uint8_t value) {
#ifdef __no_pfodEEPROM__
	(void)(address);
	(void)(value);
#else
  EEPROM.write(address, value);
#endif
}

union _uintBytes {
  uint16_t i;
  byte b[sizeof(uint16_t)]; // 2 bytes
} ;
union _ulongBytes {
  uint32_t l;
  byte b[sizeof(uint32_t)]; // 4 bytes
} ;


pfodMAC::pfodMAC() {
  debugOut = NULL;
  keyLen = 0;
  bigEndian = isBigEndian();
  connectionCounter = 0; //0 increment each connection
  powerCycles = 0;
  initCalled = false;
  CHAP_EEPROM_Address = 0;
}

/**
  Override debugOut stream.
  This can be called before or after calling pfodMAC::init()

  debugOut is not written to unless you uncomment one of the #define DEBUG_ settings above
*/
void pfodMAC::setDebugStream(Print* out) {
  debugOut = out;
}

/**
   Use this call to reset the power cycles to 0xffff
   Then remove the call from the code for normal operation

   @param eepromAddress the base address on the eepromStorage, defaults to 0
*/
void pfodMAC::resetPowerCycles(int eepromAddress) {
#ifdef __no_pfodEEPROM__
	(void)(eepromAddress);
  // do nothing
#else

  if (eepromAddress >= 0) {
#if defined( ARDUINO_ARCH_AVR ) || defined( ARDUINO_ARCH_ARC32 )  || defined( TEENSYDUINO )
    EEPROM.begin(); // does nothing in Teensy
#else
    EEPROM.begin(512);  // only use 20bytes for pfodSecurity
#endif
    int add = eepromAddress + PowerCyclesOffset;
    _uintBytes intBytes;
    intBytes.i = 0xffff; // set powercycles to max

    for (size_t i = 0; i < sizeof(uint16_t); i++) {
      // save back in the same byte order as read
      // no need to worry about endian ness
      eeprom_write(add++, intBytes.b[i]);
    }
#if defined( ARDUINO_ARCH_ESP8266 ) || defined( ARDUINO_ARCH_ESP32 ) 
// not all eeprom implementations have commit()
    EEPROM.commit();
#endif
    EEPROM.end();  // AVR and ARC32 have end()
    // so does Teensy but it does nothing
  }
#endif //__no_pfodEEPROM__
  while (1) {
    delay(10); // loop forever here
  }
}

/**
   Returns true if uC stores numbers in BigEndian format,
   else returns false if stored in LittleEndian format
*/
boolean pfodMAC::isBigEndian() {
  boolean bigEndian = true;
  _ulongBytes longBytes;
  longBytes.l = 1;
  if (longBytes.b[0] == 1) {
    bigEndian = false;
  } else {
    bigEndian = true;
  }
  return bigEndian;
}

void pfodMAC::noEEPROM() {
  disableEEPROM = true;
}

/**
   Initialize the class

   @param debug_out -- the stream to used for debug output
   @param eepromAddress -- the starting address in the EEPROM where the powercycles, keyLen and key are stored i.e. upto 19 bytes in all
          if eepromAddress < 0 disable EEPROM
   @return true if powercycles still > 0
*/
boolean pfodMAC::init(int eepromAddress) {
  if (initCalled) {
    return true;  // only do this init once!! per power cycle
  }
  initCalled = true;
  disableEEPROM = false;
#ifdef DEBUG_HASH
  if (debugOut != NULL) {
    debugOut->print(F("isBigEndian = "));
    if (bigEndian) {
      debugOut->println(F("true"));
    } else {
      debugOut->println(F("false"));
    }
  }
#endif

  if (eepromAddress < 0) {
    noEEPROM();
#ifdef DEBUG_CHAP_EEPROM
    if (debugOut != NULL) {
      debugOut->println(F(" eepromAddress < 0 EEPROM use disabled "));
    }
#endif
  }
#ifdef __no_pfodEEPROM__
  eepromAddress = -1;
  noEEPROM();
#ifdef DEBUG_CHAP_EEPROM
  if (debugOut != NULL) {
    debugOut->println(F(" NO EEPROM available, EEPROM use disabled "));
  }
#endif
#endif

  CHAP_EEPROM_Address = eepromAddress;
  powerCycles = readPowerCycles(CHAP_EEPROM_Address + PowerCyclesOffset); // return (rand()&0xFFFF) if EEPROM disabled
#ifdef DEBUG_CHAP_EEPROM
  if (debugOut != NULL) {
    debugOut->print(F("powerCycles:"));
    debugOut->println(powerCycles);
    debugOut->println();
  }
#endif

  return isValid(); // tests powerCycles > 0 if using EEPROM
}

/**
   Set globals key and keyLen
*/
void pfodMAC::setSecretKey(byte * _key, int len) {
  keyLen = len;
  if (keyLen > pfodMAC::maxKeySize) {
    keyLen = pfodMAC::maxKeySize;
  }
  for (int i = 0; i < keyLen; i++) {
    key[i] = _key[i];
  }

  // key must be 16 bytes, if keyLen >0 and < 16 padd with zeros
  if ((keyLen > 0) && (keyLen < pfodMAC::maxKeySize)) {
    for (uint8_t i = keyLen; i < pfodMAC::maxKeySize; i++) {
      key[i] = 0;
    }
    keyLen = pfodMAC::maxKeySize;
  }
#ifdef DEBUG_CHAP_EEPROM
  if (debugOut != NULL) {
    debugOut->print(F(" Secret Key length:"));
    debugOut->print(keyLen);
    debugOut->print(F(" 0x"));
    printBytesInHex(debugOut, key, keyLen);
    debugOut->println();
  }
#endif
}

/**
  Is this MAC still valid
  currently only checks that the the powerCycles still > 0
*/
boolean pfodMAC::isValid() {
  if ((disableEEPROM) || (keyLen == 0)) {
    return true; // just loop power cycles if not EEPROM or no key
  } // else
  return (powerCycles > 0);
}

/**
  Hashes challenge of size challengeSize

  returns a pointer to an 8 byte array, the hash
*/
byte *pfodMAC::hashChallenge(byte *challenge) {
  sipHash.initFromRAM(key);
  for (int i = 0; i < challengeByteSize; i++) {
    sipHash.updateHash(challenge[i]);
  }
  sipHash.finish();
  return sipHash.result;
}

/**
  Init the underlying hash with the key that has been loaded into RAM
*/
void pfodMAC::initHash() {
#ifdef DEBUG_HASH
  if (debugOut != NULL) {
    debugOut->println(F(" initHash"));
  }
#endif
  sipHash.initFromRAM(key);
}

/**
  hash another byte
*/
void pfodMAC::putByteToHash(byte b) {
#ifdef DEBUG_HASH
  if (debugOut != NULL) {
    byte bs[1];
    bs[0] = b;
    debugOut->println();
    debugOut->print(F("hash "));
    if (b < 32 || b >= 127) {
      printBytesInHex(debugOut, bs, 1);
    } else {
      debugOut->print((char)b);
    }
    debugOut->println();
  }
#endif
  sipHash.updateHash(b);
}

/**
  hash byte[]
*/
void pfodMAC::putBytesToHash(byte* b, int len) {
  for (int i = 0; i < len; i++) {
#ifdef DEBUG_HASH
    if (debugOut != NULL) {
      byte bs[1];
      bs[0] = b[i];
      debugOut->println();
      debugOut->print(F("hash "));
      if (b[i] < 32 || b[i] >= 127) {
        printBytesInHex(debugOut, bs, 1);
      } else {
        debugOut->print((char)b[i]);
      }
      debugOut->println();
    }
#endif
    sipHash.updateHash(b[i]);
  }
}

/**
  hash a long (convert to bigEndian first)
*/
void pfodMAC::putLongToHash(uint32_t l) {
  union _ulongBytes longBytes;
  longBytes.l = l;
  if (!bigEndian) {
    reverseBytes(longBytes.b, sizeof(uint32_t));
  }
#ifdef DEBUG_HASH
  if (debugOut != NULL) {
    debugOut->print(F("Hash long"));
    printBytesInHex(debugOut, longBytes.b, sizeof(uint32_t));
    debugOut->println();
  }
#endif
  for (uint8_t i = 0; i < sizeof(uint32_t); i++) {
    sipHash.updateHash(longBytes.b[i]);
  }
}


/**
  finish the hash to get result
  The result is available from getHashedResult()
*/
void pfodMAC::finishHash() {
#ifdef DEBUG_HASH
  if (debugOut != NULL) {
    debugOut->println(F(" finishHash"));
  }
#endif
  sipHash.finish();
#ifdef DEBUG_HASH
  if (debugOut != NULL) {
    printBytesInHex(debugOut, sipHash.result, 8);
    debugOut->println();
  }
#endif
}

/**
  Get the hash result
  Returns a byte* to the 8 byte array containing the hash result
*/
byte* pfodMAC::getHashedResult() {
  return sipHash.result;
}


/**
  Converts msgHash from hex Digits to bytes
  and then compares the resulting byte[] with the current hash result
  @param msgHash null terminated ascii string of hex chars (0..9A..F) or (0..9a..f)
       this input is overwritten with the converted bytes
  @param hashSize the number of bytes expected in the hash must be < length of msgHash since the converted bytes are store in that array.
*/
boolean pfodMAC::checkMsgHash(const byte * msgHash, uint16_t hashSize) {
  uint16_t len = asciiToHex((const char*)msgHash, (uint8_t*)msgHash, hashSize);
  if (len != hashSize) {
    return false;
  }
  return cmpToResult(msgHash, hashSize);
}


/**
  returns true if response bytes match upto length len
  @param response byte* the response to check
  @param len int the number of bytes to check. Must be <= 8
*/
boolean pfodMAC::cmpToResult(const byte * response, uint16_t len) {
  byte *result = getHashedResult();
#ifdef DEBUG_HASH
  if (debugOut != NULL) {
    debugOut->print(F("cmpToResult sipHash.result:"));
    printBytesInHex(debugOut, result, len);
    debugOut->print(F(" response:"));
    printBytesInHex(debugOut, response, len);
    debugOut->println();
  }
#endif

  for (uint16_t i = 0; i < len; i++) {
    if (*result++ != *response++) {
      return false;
    }
  }
  return true;
}

/**
  For each byte in the array upto length len
  convert to a two digit HEX and print to Stream
  @param out the Stream* to print to
  @param b the byte* to the bytes to be converted and printed
  @param len the number of bytes to print
*/
void pfodMAC::printBytesInHex(Print * out, const byte * b, size_t len) {
  char hex[3]; // two hex bytes and null
  for (size_t i = 0; i < len; i++) {
    hexToAscii(b, 1, hex, 3);
    b++;
    out->print(hex);
  }
}

/**
   read secret key from EEPROM and return len
   only reads upto maxKeySize bytes regardless
   returns 0 if eeprom disabled

  @param bRtn -- pointer to array long enough to hold the key read from EEPROM
  @param keyAdd -- EEPROM address where the key lengh (byte) followed by the key is stored
*/
unsigned  int pfodMAC::readSecretKey(byte * bRtn, int keyAdd) {
  if (disableEEPROM) {
#ifdef DEBUG_CHAP_EEPROM
    if (debugOut != NULL) {
      debugOut->println(F(" EEPROM disabled readSecretKey returning 0 length"));
    }
#endif
    return 0;
  }
  byte keyLen;
  keyLen = eeprom_read(keyAdd);
  byte *keyPtr = bRtn;

  int add = keyAdd + 1;
  for (uint16_t i = 0; (i < keyLen) && (i < maxKeySize); i++) {
    *keyPtr++ = eeprom_read(add++);
  }

#ifdef DEBUG_CHAP_EEPROM
  if (debugOut != NULL) {
    debugOut->print(F(" Read Key from EEPROM length:"));
    debugOut->print(keyLen);
    debugOut->print(F(" 0x"));
    printBytesInHex(debugOut, bRtn, keyLen);
    debugOut->println();
  }
#endif
  return keyLen;
}


/**
  read power cycles if >0 decrement by 1 and save again
  else if 0 leave as is

  @param powerCycleAdd -- the EEPROM address where the 2byte power cycle uint16_t is stored

  returns the updated power cycle count as an uint16_t
*/
uint16_t pfodMAC::readPowerCycles(int powerCycleAdd) {
  if (disableEEPROM) {
    uint16_t r = pfodRandom16();
    uint16_t mask = 0xffff;

#ifdef DEBUG_CHAP_EEPROM
    if (debugOut != NULL) {
      debugOut->println(F(" EEPROM disabled readPowerCycles returning random result 0x"));
      debugOut->println((uint16_t)(mask & r), HEX);
      //      debugOut->println(r,HEX);
      //      debugOut->println(mask,HEX);
    }
#endif
    uint16_t rtn = (mask & r);
    if (rtn == 0) { // never return 0 for powerCycles
      rtn = 0xffff;
    }
    return rtn;
  }

  uint16_t rtn = 0;
  union _uintBytes intBytes;

  // read it back
  intBytes.i = 0;
  int add = powerCycleAdd;
  for (size_t i = 0; i < sizeof(uint16_t); i++) {
    // note since the power cycle count is save the same way
    // no need to worry about endian ness
    intBytes.b[i] = eeprom_read(add++);
  }
#ifdef DEBUG_CHAP_EEPROM
  if (debugOut != NULL) {
    debugOut->print(F(" PowerCycles read from EEPROM 0x"));
    printBytesInHex(debugOut, intBytes.b, sizeof(uint16_t));
    debugOut->println();
  }
#endif
  rtn = intBytes.i;
  if (intBytes.i != 0) {
    intBytes.i--;
#ifdef DEBUG_CHAP_EEPROM
    if (debugOut != NULL) {
      debugOut->print(F(" powerCycles is not zero Subtract 1 bytes before write 0x"));
      printBytesInHex(debugOut, intBytes.b, sizeof(uint16_t));
      debugOut->println();
    }
#endif

    add = powerCycleAdd;
    for (size_t i = 0; i < sizeof(uint16_t); i++) {
      // save back in the same byte order as read
      // no need to worry about endian ness
      eeprom_write(add++, intBytes.b[i]);
    }
#ifdef DEBUG_CHAP_EEPROM
    if (debugOut != NULL) {
      debugOut->println(F(" EEPROM written"));
    }
#endif
  }

  return rtn;
}

/**
  buildChallenge  -- NOTE: each call to this method increments the connection counter!!!
  Challenge built from
  powerCycles + connectionCounter + mS_sinceLastConnection (limited to 31 bits) 32th bit set if last connection failed
  finally hashes the challange bytes (result stored in hash fn)
  use getHashedResult() to retrieve pointer to result when needed for checking return value

  All counters stored in BigEndian format into the challenge byte array
  powerCycles read on start up

  @param challengeBytes byte* to array to store challenge
  @param mS_sinceLastConnection  mS since last connection (max value about 25days), -ve if last connection failed

  returns true if isValid() is true;
*/
boolean pfodMAC::buildChallenge(byte * challengeBytes, uint32_t mS_sinceLastConnection) {
  connectionCounter++;
  // when using eeprom stop when powercycles == 0
  if (connectionCounter == 0) { // wrapped around uint16_t
    if (disableEEPROM) {
      // just dec power cycle
      powerCycles = powerCycles-1;
      if (powerCycles == 0) {
        powerCycles = 0xffff;
      }
    } else { // use EEPROM
      // read next power cycle
      powerCycles = readPowerCycles(CHAP_EEPROM_Address);
#ifdef DEBUG_BUILD_CHALLENGE
      if (debugOut != NULL) {
        debugOut->println(F(" decremented Power cycles "));
        debugOut->println(F("----------------------------"));
      }
#endif
    }
    if (!isValid()) {
      return false;
    }
    connectionCounter = 1;
  }

  uint8_t lastConnectionRandom = pfodRandom8(); // only randomize the lowest 8bits of the last connection time

#ifdef DEBUG_DISABLE_POWER_AND_CONNECTION_COUNTERS
  if (debugOut != NULL) {
    powerCycles = 0xffff;
    connectionCounter = 0;
    mS_sinceLastConnection = 0;
    lastConnectionRandom = 0;
  }
#endif
#ifdef DEBUG_BUILD_CHALLENGE
  if (debugOut != NULL) {
    debugOut->print(F("powerCycles:"));
    debugOut->println(powerCycles);
    debugOut->print(F(" 0x"));
    {
      union _uintBytes intBytes;
      intBytes.i = powerCycles;
      if (!bigEndian) {
        reverseBytes(intBytes.b, powerCycleSize);
      }
      printBytesInHex(debugOut, intBytes.b, powerCycleSize);
      debugOut->println();
    }
    debugOut->print(F("connectionCounter:"));
    debugOut->println(connectionCounter);
    debugOut->print(F(" 0x"));
    {
      union _uintBytes intBytes;
      intBytes.i = connectionCounter;
      if (!bigEndian) {
        reverseBytes(intBytes.b, counterSize);
      }
      printBytesInHex(debugOut, intBytes.b, counterSize);
      debugOut->println();
    }
    debugOut->print(F("mS_sinceLastConnection is :"));
    debugOut->println(mS_sinceLastConnection);
    debugOut->print(F(" 0x"));
    {
      union _ulongBytes longBytes;
      longBytes.l = mS_sinceLastConnection;
      if (!bigEndian) {
        reverseBytes(longBytes.b, timerSize);
      }
      printBytesInHex(debugOut, longBytes.b, timerSize);
      debugOut->println();
    }
    debugOut->print(F("lastConnectionRandom is :"));
    debugOut->println(lastConnectionRandom);
    debugOut->print(F(" 0x"));
    debugOut->println(lastConnectionRandom, HEX);

    uint32_t randTime = mS_sinceLastConnection ^ ((uint32_t)lastConnectionRandom);
    debugOut->print(F("randomized mS_sinceLastConnection is :"));
    debugOut->println(randTime);
    debugOut->print(F(" 0x"));
    {
      union _ulongBytes longBytes;
      longBytes.l = randTime;
      if (!bigEndian) {
        reverseBytes(longBytes.b, timerSize);
      }
      printBytesInHex(debugOut, longBytes.b, timerSize);
      debugOut->println();
    }

  }
#endif
  // set challenge bytes
  // randomize the last byte of the time since last connection
  mS_sinceLastConnection = mS_sinceLastConnection ^ ((uint32_t)lastConnectionRandom);

  int challengeIdx;
  challengeIdx = 0;
  union _uintBytes intBytes;
  intBytes.i = powerCycles;
  if (!bigEndian) {
    reverseBytes(intBytes.b, powerCycleSize);
  }
  for (int i = 0; i < powerCycleSize; i++) {
    challengeBytes[challengeIdx++] = intBytes.b[i];
  }
  intBytes.i = connectionCounter;
  if (!bigEndian) {
    reverseBytes(intBytes.b, counterSize);
  }
  for (int i = 0; i < counterSize; i++) {
    challengeBytes[challengeIdx++] = intBytes.b[i];
  }
  union _ulongBytes longBytes;
  longBytes.l = mS_sinceLastConnection;
  if (!bigEndian) {
    reverseBytes(longBytes.b, timerSize);
  }
  for (int i = 0; i < timerSize; i++) {
    challengeBytes[challengeIdx++] = longBytes.b[i];
  }

  hashChallenge(challengeBytes); // set result

#ifdef DEBUG_BUILD_CHALLENGE
  if (debugOut != NULL) {
    debugOut->println(F("Challenge "));
    printBytesInHex(debugOut, challengeBytes, challengeByteSize);
    debugOut->println();
    debugOut->println(F("Hash of Challenge "));
    printBytesInHex(debugOut, getHashedResult(), challengeByteSize);
    debugOut->println();
  }
#endif

  return true; // valid challenge
}

/**
  reverse the bytes in the byte array of length len
  @param b byte* pointer to start position on byte array
  @param len length of bytes to swap

*/
void pfodMAC::reverseBytes(byte * b, int len) {
  byte *b_r = b + len - 1;
  for (int i = 0; i < (len >> 1); i++) {
    byte b1 = *b;
    byte b2 = *b_r;
    *b++ = b2;
    *b_r-- = b1;
  }
}

