#ifndef pfodMACClient_h
#define pfodMACClient_h
/**
pfodMACClient for Arduino
 Updates power cycles, builds the Challenge, keeps track in message counts and
 hashes and check message hashes
 It uses upto 19 bytes of EEPROM starting from the address passed to init()
*/
/*
 * (c)2014-2017 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provide this copyright is maintained.
 */

#include <Arduino.h>
#include "pfodStream.h"
#include "pfodMAC.h"

class pfodMACClient {
  public:

    pfodMACClient();
    boolean init();  


    void setSecretKey(byte *key, int len);

    void setDebugStream(Print* out);
    boolean isValid();  // powercycles > 0
    boolean isBigEndian(); // is 1 stored as 0x00000001  false for Atmel 8 bit micros



    // useful for debugging
    void printBytesInHex(Print *out, const byte* b, size_t len);


    void initHash(); // start a new hash by initializing with the secret key
    void putByteToHash(byte b); // add a byte to the hash
    void putBytesToHash(byte* b, int len); // add a byte array to the hash
    void putLongToHash(uint32_t l); // add a long to the hash (in BigEndian format)
    void finishHash(); // finish the hash and calculate the result
    byte* getHashedResult(); // get pointer to the 8 byte result


    /**
    * Converts msgHash from hex Digits to bytes
    * and then compares the resulting byte[] with the current hash result
    * @param msgHash null terminated ascii string of hex chars (0..9A..F) or (0..9a..f)
    *      this input is overwritten with the converted bytes
    * @param hashSize the number of bytes expected in the hash must be < length of msgHash since the converted bytes are store in that array.
    */
    boolean checkMsgHash(const byte* msgHash, uint16_t len);

    void reverseBytes(byte *b, int len); // swap from order of bytes upto len, converts between Endians, Big to Little, Little to Big

//    const static uint8_t msgHashByteSize = 4; // number of byte in per msg hash
//    const static uint8_t challengeHashByteSize = 8; // number of bytes in challenge hash
//    const static uint8_t challengeByteSize = 8; // number of bytes in challenge
//    const static uint8_t maxKeySize = 16; // max size of key 128 bits

    unsigned char *hashChallenge(byte *challenge); // hashes challenge returns pointer to 8 byte result

  private:
    boolean initCalled; // set true after first init() call. Stops multiple calls to get readPowerCycles
    boolean cmpToResult(const byte* response, uint16_t len);
    Print *debugOut;
    byte key[pfodMAC::maxKeySize]; // the secret key after reading from EEPROM and expanding to 16 bytes
    uint8_t keyLen; // 0 if no key, else 16 (after padding key with 0 if shorter)
    boolean bigEndian;  // true if bigEndian uC


};


#endif // pfodMACClient_h

