#ifndef pfodRandom_h
#define pfodRandom_h
#include <stdint.h>

// return random as a long
long pfodRandom();

// return lower 8 bits of random
uint8_t pfodRandom8();

// return lower 16 bits of random
uint16_t pfodRandom16();

#endif // pfodRandom_h