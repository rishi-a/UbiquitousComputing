/*
 * (c)2014-2017 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provide this copyright is maintained.
 */
#include "pfodDwgs.h"

pfodCircle& pfodDwgs::circle() {
  c.init(out, &values); // set output
  return c;
}

pfodLabel& pfodDwgs::label() {
  t.init(out, &values); // set output
  return t;
}

pfodRectangle& pfodDwgs::rectangle() {
  r.init(out, &values); // set output
  return r;
}

pfodTouchZone& pfodDwgs::touchZone() {
  x.init(out, &values); // set output
  return x;
}

pfodTouchAction& pfodDwgs::touchAction() {
  X.init(out, &values); // set output
  return X;
}

pfodLine& pfodDwgs::line() {
  l.init(out, &values); // set output
  return l;
}

pfodArc& pfodDwgs::arc() {
  a.init(out, &values); // set output
  return a;
}

pfodErase& pfodDwgs::erase() {
  e.init(out, &values); // set output
  return e;
}

pfodHide& pfodDwgs::hide() {
  h.init(out, &values); // set output
  return h;
}
pfodUnhide& pfodDwgs::unhide() {
  u.init(out, &values); // set output
  return u;
}
pfodIndex& pfodDwgs::index() {
  i.init(out, &values); // set output
  return i;
}

pfodInsertDwg& pfodDwgs::insertDwg() {
  d.init(out, &values); // set output
  return d;
}

pfodDwgs::pfodDwgs(Print *_out) {
  out = _out;
}

void pfodDwgs::start(int cols, int rows, int backgroundColor,  uint8_t moreData) {
  out->print('{'); out->print('+'); out->print(backgroundColor); out->print('`');  out->print(cols); out->print('`'); out->print(rows);
  out->print('~');
  if (moreData != 0) {
    out->print('m');
  }
}

void pfodDwgs::startUpdate(uint8_t moreData) {
  out->print('{'); out->print('+');
  if (moreData != 0) {
    out->print('~');
    out->print('m');
  }
}

void pfodDwgs::end() {
  out->print('}');
}

//void pfodDwgs::pushZero(int16_t col, int16_t row, float scale) {
//  out->print('|'); out->print('z');
//  out->print('`');  out->print(col); out->print('`'); out->print(row);
//  if (scale != 1) {
//    printFloat(scale);
//  }
//}

//void pfodDwgs::pushZero(int col, int row = 0, float scale = 1.0f){
//	pushZero((int16_t)col,(int16_t)row,(float)scale);
//}

void pfodDwgs::pushZero(double col, double row, double scale) {
  out->print('|'); out->print('z');
  printFloat((float)col); printFloat((float)row);
  if (scale != 1) {
    printFloat((float)scale);
  }
}

void pfodDwgs::popZero() {
  out->print('|'); out->print('z');
}

int pfodDwgs::reserveIdx(int numToReserve) {
  int rtn = _idx;
  _idx = _idx + numToReserve;
  return rtn;
}

int pfodDwgs::_idx = 1;



