/*
 * (c)2014-2017 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provide this copyright is maintained.
 */
#include "pfodCircle.h"

pfodCircle::pfodCircle()  {
}

void pfodCircle::init(Print *_out, struct VALUES* _values) {
  initValues(_values);
  out = _out;
  //valuesPtr->lastDwg = this;
}

pfodCircle &pfodCircle::offset(float _colOffset, float _rowOffset) {
  valuesPtr->colOffset = _colOffset;
  valuesPtr->rowOffset = _rowOffset;
  return *this;
}

pfodCircle &pfodCircle::idx(uint16_t _idx) {
  valuesPtr->idx = _idx;
  return *this;
}
pfodCircle &pfodCircle::filled() {
  valuesPtr->filled = 1;
  return *this;
}

pfodCircle &pfodCircle::radius(float _radius) {
  valuesPtr->radius = _radius;
  return *this;
}

pfodCircle &pfodCircle::color(int _color) {
  valuesPtr->color = _color;
  return *this;
}


void pfodCircle::send(char _startChar) {
  out->print(_startChar);
  if (valuesPtr->filled != 0) {
    out->print('C');
  } else {
    out->print('c');
  }
  printIdx();
  printColor();
  printFloat(valuesPtr->radius);
  colRowOffset();
 // valuesPtr->lastDwg = NULL; // sending now
}


