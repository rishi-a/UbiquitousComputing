/*
 SipHash_2_4.cpp
 SipHash for 8bit Atmel processors

 Note: one instance sipHash is already constructed in .cpp file

Usage:
     uint8_t key[] PROGMEM = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                               0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
     sipHash.initFromPROGMEM(key); // initialize with key NOTE: key is stored in flash (PROGMEM)
     // use sipHash.initFromRAM(key); if key in RAM
     for (int i=0; i<msgLen;i++) {
      sipHash.updateHash((byte)c); // update hash with each byte of msg
    }
    sipHash.finish(); // finish
    // SipHash.result then contains the 8bytes of the hash in BigEndian format


 see https://131002.net/siphash/ for details of algorithm
*/
/*
 * (c)2014-2017 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provide this copyright is maintained.
 */

#include <string.h>
#include "SipHash_2_4.h"

#define XOR64(v,v1) \
  {\
    for (int i=0; i<8; i++) {\
      v[i] ^= v1[i];\
    }\
  }

void rotl64_16(uint8_t *v) {
  uint8_t v0 = v[0];
  uint8_t v1 = v[1];
  for (int i = 0; i < 6; i++) {
    v[i] = v[i + 2];
  }
  v[6] = v0;
  v[7] = v1;
}

#define ROTL64_16(v) \
  {\
    uint8_t v0 = v[0];\
    uint8_t v1 = v[1];\
    for (int i=0; i<6; i++) {\
      v[i] = v[i+2];\
    }\
    v[6] = v0;\
    v[7] = v1;\
  }

#define ROTL64_32(v) \
  {\
    uint8_t vTemp;\
    for (int i=0; i<4; i++) {\
      vTemp = v[i];\
      v[i] = v[i+4];\
      v[i+4]=vTemp;\
    }\
  }

void reverse64(uint8_t *x) {
  uint8_t xTemp;
  for (int i = 0; i < 4; i++) {
    xTemp = x[i];
    x[i] = x[7 - i];
    x[7 - i] = xTemp;
  }
}

#define ADD64(v, s) \
  { \
    uint16_t carry = 0;\
    for (int i=7; i>=0; i--) { \
      carry += v[i];\
      carry += s[i];\
      v[i] = carry; \
      carry = carry>>8; \
    }\
  }

#define ROTL64_xBITS(v,x) \
  {\
    uint8_t v0 = (v)[0];\
    for (int i=0; i<7; i++) {\
      (v)[i] = ((v)[i]<<(x)) | ((v)[i+1]>>(8-(x)));\
    }\
    (v)[7] =  ((v)[7]<<(x)) | (v0>>(8-(x)));\
  }

#define ROTR64_xBITS(v,x) \
  {\
    uint8_t v7 = (v)[7];\
    for (int i=7; i>0; i--) {\
      (v)[i] = ((v)[i]>>(x)) | ((v)[i-1]<<(8-(x)));\
    }\
    (v)[0] =  ((v)[0]>>(x)) | (v7<<(8-(x)));\
  }


#define ROL_17BITS(v) rotl64_16(v);\
  ROTL64_xBITS(v,1);

#define ROL_21BITS(v) rotl64_16(v);\
  ROTL64_xBITS(v,5);

#define ROL_13BITS(v) rotl64_16(v);\
  ROTR64_xBITS(v,3);

// init values in FLASH
const uint8_t v0_init[] PROGMEM = {0x73, 0x6f, 0x6d, 0x65, 0x70, 0x73, 0x65, 0x75};
const uint8_t v1_init[] PROGMEM = {0x64, 0x6f, 0x72, 0x61, 0x6e, 0x64, 0x6f, 0x6d};
const uint8_t v2_init[] PROGMEM = {0x6c, 0x79, 0x67, 0x65, 0x6e, 0x65, 0x72, 0x61};
const uint8_t v3_init[] PROGMEM = {0x74, 0x65, 0x64, 0x62, 0x79, 0x74, 0x65, 0x73};

// total ram 42 bytes + stack usage for calls


// This class already defines an instance sipHash, see the SipHashTest.ino
SipHash_2_4::SipHash_2_4(void) {
  result = v0;
}

/*
**  use this init if the key is in an flash (program) memory
// Define your 'secret' 16 byte key in program memory (flash memory)
 uint8_t key[] PROGMEM = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                               0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
 // to start hashing initialize with your key
 sipHash.initFromPROGMEM(key);
 */
void SipHash_2_4::initFromPROGMEM(const uint8_t *keyPrgPtrIn) {
  uint8_t key[16];
  memcpy_P(key, keyPrgPtrIn, 16);
  initFromRAM(key);
}

/*
**  use this init if the key is in an RAM memory
// Define your 'secret' 16 byte key array
 uint8_t key[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
 // to start hashing initialize with your key
 sipHash.initFromRAM(key);
 */
void SipHash_2_4::initFromRAM(const uint8_t *key) {
  // init load initial state
  // and xors with key
  memcpy_P(v0, v0_init, 8);
  memcpy_P(v1, v1_init, 8);
  memcpy_P(v2, v2_init, 8);
  memcpy_P(v3, v3_init, 8);
  // now load first 8 bytes of the key reverse it and XOR with v0,v2
  memcpy(m, key, 8);
  reverse64(m);
  XOR64(v0, m);
  XOR64(v2, m);
  memcpy(m, key + 8, 8);
  reverse64(m);
  XOR64(v1, m);
  XOR64(v3, m);
  m_idx = 7;
  msg_byte_counter = 0;
}

void SipHash_2_4::updateHash(uint8_t c) {
  msg_byte_counter++; // count this one % 256
  m[m_idx--] = c;
  if (m_idx < 0) {
    m_idx = 7; // reset index
    // hash this 64 bits
    hash(m);
  }
}

/************************************************************************
** m is the 8 bytes to hash, last set of 8 have length + padding added
** m is in LittleEndian storage, i.e.
** reference msg of 0x00,0x01,0x02,..,0x0b,0x0c,0x0d,0x0e  + length is stored as
** uint8_t m0[8] = {0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00};
** uint8_t m1[8] = {0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08};
**
** result (after finalize()) is in BigEndian storage format
**
************************************************************************/
void SipHash_2_4::hash(uint8_t* m) {
  XOR64(v3, m);
  siphash_round();
  siphash_round();
  XOR64(v0, m);
}

void SipHash_2_4::siphash_round() {
  ADD64(v0, v1);
  ADD64(v2, v3);
  ROL_13BITS(v1);
  ROTL64_16(v3);

  XOR64(v1, v0);
  XOR64(v3, v2);
  ROTL64_32(v0);

  ADD64(v2, v1);
  ADD64(v0, v3);
  ROL_17BITS(v1);
  ROL_21BITS(v3);

  XOR64(v1, v2);
  XOR64(v3, v0);
  ROTL64_32(v2);
}

/***********************************************************************
** result in v0
**
************************************************************************/
void SipHash_2_4::finish() {
  // add msg length hash padd hash it and then do finish rounds
  // m index is in range 7 to 0
  // zero out to index 0 and then put msg_byte_counter there

  // save msgLen before padding
  uint8_t msgLen = msg_byte_counter; // count this one % 256

  while (m_idx > 0) {
    updateHash(0);
  }
  updateHash(msgLen);

  v2[7] ^= 0xff; //	xor_ff(v2);
  siphash_round();
  siphash_round();
  siphash_round();
  siphash_round();

  XOR64(v0, v1);
  XOR64(v0, v2);
  XOR64(v0, v3);
  // answer in result in BigEndian format
  // reverse64(v0); // uncomment this line to get LittleEndian to match SipHash_2_4 reference C result array.
  // NOTE: the sample test hash in the pdf is shown in BigEndian order to the and matches the result returned here.
}


SipHash_2_4 sipHash;
