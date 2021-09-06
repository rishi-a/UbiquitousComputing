#ifndef pfodLine_h
#define pfodLine_h
/*
 * (c)2014-2017 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provide this copyright is maintained.
 */
#include <stdint.h>
#include <Arduino.h>
#include "pfodDwgsBase.h"

class pfodLine : public pfodDwgsBase {
  public:
    pfodLine();
    pfodLine &size(float width, float height); // default 1x1
    pfodLine &color(int _color); // default BLACK_WHITE
    pfodLine &idx(uint16_t _idx); // default 0 i.e. not set
    pfodLine &offset(float _colOffset, float _rowOffset); // default 0,0
    void init(Print *out, struct VALUES* _values);
    void send(char _startChar = '|');
};
#endif // pfodLine_h
