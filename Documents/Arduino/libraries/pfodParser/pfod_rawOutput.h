#ifndef pfod_rawOutput_h
#define pfod_rawOutput_h

#include <Arduino.h>
#include "pfod_Base.h"

/*
 * (c)2014-2017 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provide this copyright is maintained.
 */

class pfod_rawOutput : public Print {
  public:
    size_t write(uint8_t c);
    size_t write(const uint8_t *buffer, size_t size);
    void set_pfod_Base(pfod_Base* arg);
  private:
    pfod_Base* _pfod_Base;

};
#endif // pfod_rawOutput_h
