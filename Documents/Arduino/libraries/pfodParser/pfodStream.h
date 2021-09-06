#ifndef pfodStream_h
#define pfodStream_h

#include <Arduino.h>
// This include handles the rename of Stream for MBED compiles
// it is included for pfodParser, pfodBLEBufferedStream, pfod_Base, pfodSMS_SIM900 and pfodWaitForUtils 
#if defined(ARDUINO_ARDUINO_NANO33BLE) || defined(ARDUINO_ARCH_SAMD) || defined(ARDUINO_ARCH_MBED_RP2040)
#include <Stream.h>
#elif defined( __MBED__ ) || defined( MBED_H )
#include <WStream.h>
#define Stream WStream
#else
#include <Stream.h>
#endif

#endif //pfodStream_h
