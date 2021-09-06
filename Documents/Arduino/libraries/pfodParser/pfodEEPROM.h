/*
 * (c)2014-2017 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provide this copyright is maintained.
 */
#ifndef pfodEEPROM_h
#define pfodEEPROM_h

#if defined( ARDUINO_ARCH_AVR ) || defined( ARDUINO_ARCH_ESP8266 ) || defined( ARDUINO_ARCH_ESP32 ) || defined( ARDUINO_ARCH_ARC32 ) || defined( TEENSYDUINO )
// NOTE: Teensy does not need to use begin() or end() but no harm in calling them 
#include <EEPROM.h>
#else
//#ifdef __RFduino__
//#define __no_pfodEEPROM__
// all other arch not explicitly mentioned do no support EEPROM.H
// they may support FLASH EEPROM simulation but that is not included here
// if __no_pfodEEPROM__ is defined then use randu instead of EEPROM for power cycle count
// initialize randu from analogRead(A0) and micros() to complete setup
#define __no_pfodEEPROM__
#endif


#endif //pfodEEPROM_h

