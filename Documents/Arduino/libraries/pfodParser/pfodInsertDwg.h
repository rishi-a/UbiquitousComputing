#ifndef pfodInsertDwg_h
#define pfodInsertDwg_h
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

class pfodInsertDwg : public pfodDwgsBase {
  public:
    pfodInsertDwg();
    pfodInsertDwg &loadCmd(char _loadCmd); 
    pfodInsertDwg &loadCmd(const char* _loadCmdStr); 
    pfodInsertDwg &offset(float _colOffset, float _rowOffset); // default 0,0
//    pfodInsertDwg &size(float _width, float _height); // default 
    void init(Print *out, struct VALUES* _values);
    void send(char _startChar = '|');
};
#endif // pfodInsertDwg_h
