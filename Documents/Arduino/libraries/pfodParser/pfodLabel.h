#ifndef pfodLabel_h
#define pfodLabel_h
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

class pfodLabel : public pfodDwgsBase {
  public:
    pfodLabel();
    pfodLabel &text(const char *txt); // default ""
    pfodLabel &text(const __FlashStringHelper *txtF);
    pfodLabel &units(const char *txt); // default ""
    pfodLabel &units(const __FlashStringHelper *txtF);
    pfodLabel &floatReading(float value);
    pfodLabel &intValue(int32_t _value); 
    pfodLabel &displayMax(float _displayMax);  // default 1  
    pfodLabel &displayMin(float _displayMin);  // default 0
    pfodLabel &maxValue(int32_t _max); // default 1
    pfodLabel &minValue(int32_t _min); // default 0
    pfodLabel &decimals(int _decPlaces); // default 2  limits to -6 to +6
    pfodLabel &color(int _color); // default BLACK_WHITE
    pfodLabel &fontSize(int _font); // default 0 = <+0>
    pfodLabel &bold(); 
    pfodLabel &center(); 
    pfodLabel &left(); 
    pfodLabel &right(); 
    pfodLabel &italic(); 
    pfodLabel &underline();
    pfodLabel &idx(uint16_t _idx); // default 0 i.e. not set
    pfodLabel &offset(float _colOffset, float _rowOffset); // default 0,0
    void init(Print *out, struct VALUES* _values);
    void send(char _startChar = '|');
  private:
    void sendValue(int32_t val);
};
#endif // pfodLabel_h
