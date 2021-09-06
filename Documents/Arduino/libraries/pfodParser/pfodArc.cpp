/*
 * (c)2014-2017 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provide this copyright is maintained.
 */
#include "pfodArc.h"

pfodArc::pfodArc()  {
}

void pfodArc::init(Print *_out, struct VALUES* _values) {
  initValues(_values);
  out = _out;
  //valuesPtr->lastDwg = this;
}

pfodArc &pfodArc::offset(float _colOffset, float _rowOffset) {
  valuesPtr->colOffset = _colOffset;
  valuesPtr->rowOffset = _rowOffset;
  return *this;
}

pfodArc &pfodArc::idx(uint16_t _idx) {
  valuesPtr->idx = _idx;
  return *this;
}
pfodArc &pfodArc::filled() {
  valuesPtr->filled = 1;
  return *this;
}

pfodArc &pfodArc::radius(float _radius) {
  valuesPtr->radius = _radius;
  return *this;
}

pfodArc &pfodArc::start(float _start) {
  valuesPtr->startAngle = _start;
  return *this;
}

pfodArc &pfodArc::angle(float _angle) {
  valuesPtr->arcAngle = _angle;
  return *this;
}

pfodArc &pfodArc::color(int _color) {
  valuesPtr->color = _color;
  return *this;
}


void pfodArc::send(char _startChar) {
  out->print(_startChar);
  if (valuesPtr->filled != 0) {
    out->print('A');
  } else {
    out->print('a');
  }
  printIdx();
  printColor();
  printFloat(valuesPtr->arcAngle);
  printFloat(valuesPtr->startAngle);
  printFloat(valuesPtr->radius);
  colRowOffset();
 // valuesPtr->lastDwg = NULL; // sending now
}


