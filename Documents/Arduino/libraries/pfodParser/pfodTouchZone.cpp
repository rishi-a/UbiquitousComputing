/*
 * (c)2014-2017 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provide this copyright is maintained.
 */
#include "pfodTouchZone.h"

pfodTouchZone::pfodTouchZone()  {
}

void pfodTouchZone::init(Print *_out, struct VALUES* _values) {
  initValues(_values);
  valuesPtr->width = 0;
  valuesPtr->height = 0; // default 0x0 for touchzones
  out = _out;
  //valuesPtr->lastDwg = this;
}

pfodTouchZone &pfodTouchZone::offset(float _colOffset, float _rowOffset) {
  valuesPtr->colOffset = _colOffset;
  valuesPtr->rowOffset = _rowOffset;
  return *this;

}

pfodTouchZone &pfodTouchZone::idx(uint16_t _idx) {
  valuesPtr->idx = _idx;
  return *this;
}

pfodTouchZone &pfodTouchZone::size(float _width, float _height) {
  valuesPtr->width = _width;
  valuesPtr->height = _height;
  return *this;
}

pfodTouchZone &pfodTouchZone::cmd(const char _cmd) {
  valuesPtr->cmd = _cmd;
  valuesPtr->cmdStr = NULL;
  return *this;
}

pfodTouchZone &pfodTouchZone::cmd(const char* _cmdStr) {
  valuesPtr->cmd = ' ';
  valuesPtr->cmdStr = _cmdStr;
  return *this;
}


pfodTouchZone &pfodTouchZone::filter(uint16_t _filter) {
  valuesPtr->filter = _filter;
  return *this;
}

pfodTouchZone &pfodTouchZone::centered() {
  valuesPtr->centered = 1;
  return *this;
}

void pfodTouchZone::send(char _startChar) {
  out->print(_startChar);
  out->print('x');
  if (valuesPtr->centered != 0) {
    out->print('c');
  }
  printIdx();
  out->print('~');
  if (valuesPtr->cmdStr) {
    out->print(valuesPtr->cmdStr);
  } else {
    out->print(valuesPtr->cmd);
  }
  colWidthHeight();
  colRowOffset();
  if (valuesPtr->filter != 0) {
    out->print('`');
    out->print(valuesPtr->filter);
  } 
  //valuesPtr->lastDwg = NULL; // sent now
}


