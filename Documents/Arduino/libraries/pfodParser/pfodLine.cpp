/*
 * (c)2014-2017 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provide this copyright is maintained.
 */
#include "pfodLine.h"

pfodLine::pfodLine()  {
}

void pfodLine::init(Print *_out, struct VALUES* _values) {
  initValues(_values);
  out = _out;
  //valuesPtr->lastDwg = this;
}

pfodLine &pfodLine::offset(float _colOffset, float _rowOffset) {
  valuesPtr->colOffset = _colOffset;
  valuesPtr->rowOffset = _rowOffset;
  return *this;
}

pfodLine &pfodLine::idx(uint16_t _idx) {
  valuesPtr->idx = _idx;
  return *this;

}

pfodLine &pfodLine::size(float _width, float _height) {
  valuesPtr->width = _width;
  valuesPtr->height = _height;
  return *this;
}

pfodLine &pfodLine::color(int _color) {
  valuesPtr->color = _color;
  return *this;
}


void pfodLine::send(char _startChar) {
  out->print(_startChar);
  out->print('l');
  printIdx();
  printColor();
  colWidthHeight();
  colRowOffset();
  //valuesPtr->lastDwg = NULL; // sent now
}


