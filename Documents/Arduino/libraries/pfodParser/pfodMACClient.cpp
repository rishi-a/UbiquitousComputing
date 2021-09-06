/**
  pfodMACClient for Arduino
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

#include "pfodMACClient.h"
#include "HexConversionUtils.h"
#include "SipHash_2_4.h"
#include "pfodParserUtils.h"

//#define DEBUG_BUILD_CHALLENGE
//#define DEBUG_CHAP_EEPROM
//#define DEBUG_HASH
// this next define will keep the challange the same for ALL connections.
// MUST only be used for testing, make use this line is commented out for REAL use
//#define DEBUG_DISABLE_POWER_AND_CONNECTION_COUNTERS


//const unsigned char pfodMACClient::maxKeySize;// = 16;


union _uintBytes {
  uint16_t i;
  byte b[sizeof(uint16_t)]; // 2 bytes
} ;
union _ulongBytes {
  uint32_t l;
  byte b[sizeof(uint32_t)]; // 4 bytes
} ;


pfodMACClient::pfodMACClient() {
  debugOut = NULL;
  keyLen = 0;
  bigEndian = isBigEndian();
  initCalled = false;
}

/**
  Override debugOut stream.
  This can be called before or after calling pfodMACClient::init()

  debugOut is not written to unless you uncomment one of the #define DEBUG_ settings above
*/
void pfodMACClient::setDebugStream(Print* out) {
  debugOut = out;
}

/**
   Returns true if uC stores numbers in BigEndian format,
   else returns false if stored in LittleEndian format
*/
boolean pfodMACClient::isBigEndian() {
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


/**
   Initialize the class

   @param debug_out -- the stream to used for debug output
   @param eepromAddress -- the starting address in the EEPROM where the powercycles, keyLen and key are stored i.e. upto 19 bytes in all
   @return true if powercycles still > 0
*/
boolean pfodMACClient::init() {
  if (initCalled) {
#ifdef DEBUG_CHAP_EEPROM
    if (debugOut != NULL) {
      debugOut->print(F("pfodMACClient.init() already called just return"));
      debugOut->println();
    }
#endif
    return true;  // only do this init once!! per power cycle
  }

  initCalled = true;

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



  return true;
}

/**
   Set globals key and keyLen
*/
void pfodMACClient::setSecretKey(byte * _key, int len) {
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
  Hashes challenge of size challengeSize

  returns a pointer to an 8 byte array the hash
*/
byte *pfodMACClient::hashChallenge(byte *challenge) {
  sipHash.initFromRAM(key);
  for (int i = 0; i < pfodMAC::challengeByteSize; i++) {
    sipHash.updateHash(challenge[i]);
  }
  sipHash.finish();
  return sipHash.result;
}

/**
  Init the underlying hash with the key that has been loaded into RAM
*/
void pfodMACClient::initHash() {
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
void pfodMACClient::putByteToHash(byte b) {
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
void pfodMACClient::putBytesToHash(byte* b, int len) {
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
void pfodMACClient::putLongToHash(uint32_t l) {
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
void pfodMACClient::finishHash() {
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
byte* pfodMACClient::getHashedResult() {
  return sipHash.result;
}


/**
  Converts msgHash from hex Digits to bytes
  and then compares the resulting byte[] with the current hash result
  @param msgHash null terminated ascii string of hex chars (0..9A..F) or (0..9a..f)
       this input is overwritten with the converted bytes
  @param hashSize the number of bytes expected in the hash must be < length of msgHash since the converted bytes are store in that array.
*/
boolean pfodMACClient::checkMsgHash(const byte * msgHash, uint16_t hashSize) {
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
boolean pfodMACClient::cmpToResult(const byte * response, uint16_t len) {
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
void pfodMACClient::printBytesInHex(Print * out, const byte * b, size_t len) {
  char hex[3]; // two hex bytes and null
  for (size_t i = 0; i < len; i++) {
    hexToAscii(b, 1, hex, 3);
    b++;
    out->print(hex);
  }
}



/**
  reverse the bytes in the byte array of length len
  @param b byte* pointer to start position on byte array
  @param len length of bytes to swap

*/
void pfodMACClient::reverseBytes(byte * b, int len) {
  byte *b_r = b + len - 1;
  for (int i = 0; i < (len >> 1); i++) {
    byte b1 = *b;
    byte b2 = *b_r;
    *b++ = b2;
    *b_r-- = b1;
  }
}

