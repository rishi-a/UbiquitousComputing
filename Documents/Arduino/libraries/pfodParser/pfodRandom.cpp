#include "pfodRandom.h"
#include <Arduino.h>
//unsigned long micros(void); // from arduino for initialization
#include <stdlib.h>

static bool _pfodRandomInitialized = false; // global

// return random as a long
long getRandom() {
  if (!_pfodRandomInitialized) {
    _pfodRandomInitialized = true;
    unsigned long uS = micros();
    srand(uS);
  }
  return rand(); // return rand result
}

// return lower 8 bits of random
uint8_t pfodRandom8() {
  return (uint8_t)getRandom(); // 0 to 255
}

// return lower 16 bits of random
uint16_t pfodRandom16() {
  return (uint16_t)getRandom(); 
}
