/*
 * (c)2014-2017 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provide this copyright is maintained.
 */
#include "pfodRectangle.h"

pfodRectangle::pfodRectangle()  {
}

void pfodRectangle::init(Print *_out, struct VALUES* _values) {
  initValues(_values);
  out = _out;
  //valuesPtr->lastDwg = this;
}

pfodRectangle &pfodRectangle::offset(float _colOffset, float _rowOffset) {
  valuesPtr->colOffset = _colOffset;
  valuesPtr->rowOffset = _rowOffset;
  return *this;
}

pfodRectangle &pfodRectangle::idx(uint16_t _idx) {
  valuesPtr->idx = _idx;
  return *this;

}
pfodRectangle &pfodRectangle::filled() {
  valuesPtr->filled = 1;
  return *this;
}

pfodRectangle &pfodRectangle::rounded() {
  valuesPtr->rounded = 1;
  return *this;
}

pfodRectangle &pfodRectangle::centered() {
  valuesPtr->centered = 1;
  return *this;
}

pfodRectangle &pfodRectangle::size(float _width, float _height) {
  valuesPtr->width = _width;
  valuesPtr->height = _height;
  return *this;
}

pfodRectangle &pfodRectangle::color(int _color) {
  valuesPtr->color = _color;
  return *this;
}


void pfodRectangle::send(char _startChar) {
  out->print(_startChar);
  if (valuesPtr->rounded != 0) { // r->rr R->RR
    if (valuesPtr->filled != 0) {
      out->print('R');
    } else {
      out->print('r');
    }
  }
  if (valuesPtr->filled != 0) {
    out->print('R');
  } else {
    out->print('r');
  }
  if (valuesPtr->centered != 0) {
    out->print('c');
  }
  printIdx();
  printColor();
  colWidthHeight();
  colRowOffset();
  //valuesPtr->lastDwg = NULL; // sent now
}


