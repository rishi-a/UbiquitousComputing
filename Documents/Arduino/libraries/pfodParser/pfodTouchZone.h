#ifndef pfodTouchZone_h
#define pfodTouchZone_h
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

class pfodTouchZone : public pfodDwgsBase {
  public:
    pfodTouchZone();
    pfodTouchZone &size(float width, float height); // default 1x1
    pfodTouchZone &cmd(const char _cmd); // default ' ' not set
    pfodTouchZone &cmd(const char* _cmdStr); 
    pfodTouchZone &idx(uint16_t _idx); // default 0 i.e. not set
    pfodTouchZone &offset(float _colOffset, float _rowOffset); // default 0,0
    pfodTouchZone &centered(); // default not centered
    pfodTouchZone &filter(uint16_t _filter); // default 0 TOUCH
    void init(Print *out, struct VALUES* _values);
    void send(char _startChar = '|');
};
#endif // pfodTouchZone_h
