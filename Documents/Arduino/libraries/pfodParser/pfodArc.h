#ifndef pfodArc_h
#define pfodArc_h
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

class pfodArc : public pfodDwgsBase {
  public:
    pfodArc();
    pfodArc &radius(float _radius); // default 1
    pfodArc &start(float _start);
    pfodArc &angle(float _angle);
    pfodArc &color(int _color); // default BLACK_WHITE
    pfodArc &idx(uint16_t _idx); // default 0 i.e. not set
    pfodArc &filled(); // default not filled
    pfodArc &offset(float _colOffset, float _rowOffset); // default 0,0
    void init(Print *out, struct VALUES* _values);
    void send(char _startChar = '|');
};
#endif // pfodArc_h
