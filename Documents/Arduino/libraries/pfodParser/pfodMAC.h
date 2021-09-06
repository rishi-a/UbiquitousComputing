#ifndef pfodMAC_h
#define pfodMAC_h
/**
  pfodMAC for Arduino
  Updates power cycles, builds the Challenge, keeps track in message counts and
  hashes and check message hashes
  It uses upto 19 bytes of EEPROM starting from the address passed to init()
*/
/*
   (c)2014-2017 Forward Computing and Control Pty. Ltd.
   NSW Australia, www.forward.com.au
   This code is not warranted to be fit for any purpose. You may only use it at your own risk.
   This code may be freely used for both private and commercial use
   Provide this copyright is maintained.
*/

#include <Arduino.h>
#include "pfodStream.h"
#include "pfodEEPROM.h"

#define MAX_ELAPSED_TIME 0x7fffffff

class pfodMAC {
  public:

    pfodMAC();
    boolean init(int eepromAddress);  // if eepromAddress < 0 disable EEPROM use

    /**
       Use this call to reset the power cycles to 0xffff
       Then remove the call from the code for normal operation

       @param eepromAddress the base address on the eepromStorage, defaults to 0
    */
    static void resetPowerCycles(int eepromAddress = 0);

    void setSecretKey(byte *key, int len);

    void setDebugStream(Print* out);
    boolean isValid();  // powercycles > 0
    boolean isBigEndian(); // is 1 stored as 0x00000001  false for Atmel 8 bit micros
    void noEEPROM(); // disable eeprom

    unsigned int readSecretKey(byte* bRtn, int keyAdd);  // returns 0 length if EEPROM disabled

    // utility methods
    uint8_t eeprom_read(int address); // always returns 0 if EEPROM disabled
    static void eeprom_write(int address, uint8_t value); // does nothing if EEPROM disabled

    // useful for debugging
    void printBytesInHex(Print *out, const byte* b, size_t len);

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
    boolean buildChallenge(byte* challengeBytes, uint32_t mS_sinceLastConnection);

    void initHash(); // start a new hash by initializing with the secret key
    void putByteToHash(byte b); // add a byte to the hash
    void putBytesToHash(byte* b, int len); // add a byte array to the hash
    void putLongToHash(uint32_t l); // add a long to the hash (in BigEndian format)
    void finishHash(); // finish the hash and calculate the result
    byte* getHashedResult(); // get pointer to the 8 byte result


    /**
      Converts msgHash from hex Digits to bytes
      and then compares the resulting byte[] with the current hash result
      @param msgHash null terminated ascii string of hex chars (0..9A..F) or (0..9a..f)
           this input is overwritten with the converted bytes
      @param hashSize the number of bytes expected in the hash must be < length of msgHash since the converted bytes are store in that array.
    */
    boolean checkMsgHash(const byte* msgHash, uint16_t len);

    void reverseBytes(byte *b, int len); // swap from order of bytes upto len, converts between Endians, Big to Little, Little to Big

    const static uint8_t msgHashByteSize = 4; // number of byte in per msg hash
    const static uint8_t challengeHashByteSize = 8; // number of bytes in challenge hash
    const static uint8_t challengeByteSize = 8; // number of bytes in challenge
    const static uint8_t PowerCyclesOffset; // == 0 offset to power cycles from eepromAddress
    const static uint8_t KeyOffset; // offset to keyLen from eepromAddress, length (1 byte) comes first then key
    const static uint8_t maxKeySize = 16; // max size of key 128 bits

  private:
    boolean initCalled; // set true after first init() call. Stops multiple calls to get readPowerCycles
    boolean cmpToResult(const byte* response, uint16_t len);
    int CHAP_EEPROM_Address; // set to -1 if eeprom disabled
    Print *debugOut;
    boolean disableEEPROM; // true if NO EEPROM or eepromAddress arg is < 0
    const static uint8_t powerCycleSize; // sizeof(uint16_t)
    const static uint8_t timerSize; // sizeof(uint32_t)
    const static uint8_t counterSize; // int
    uint16_t connectionCounter; // increment each connection
    uint16_t powerCycles;
    byte key[maxKeySize]; // the secret key after reading from EEPROM and expanding to 16 bytes
    uint8_t keyLen; // 0 if no key, else 16 (after padding key with 0 if shorter)
    boolean bigEndian;  // true if bigEndian uC
    uint8_t *hashChallenge(byte *challenge); // hashes challenge returns pointer to 8 byte result

    /**
      read power cycles if >0 decrement by 1 and save again
      else if 0 leave as is

      @param powerCycleAdd -- the EEPROM address where the 2byte power cycle unsigned int is stored

      @return the updated power cycle count as an unsigned int
    */
    uint16_t readPowerCycles(int powerCycleAdd);

};


#endif // pfodMAC_h

