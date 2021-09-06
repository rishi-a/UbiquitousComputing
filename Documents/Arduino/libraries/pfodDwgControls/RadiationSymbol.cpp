/*
 * (c)2014-2017 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provide this copyright is maintained.
 */

#include "RadiationSymbol.h"


RadiationSymbol::RadiationSymbol(pfodDwgs *_dwgsPtr) : pfodControl(_dwgsPtr) {
  z_idx = dwgsPtr->reserveIdx(1); //  one idx for label update
}


void RadiationSymbol::draw() {
  dwgsPtr->circle().color(dwgsPtr->YELLOW).filled().radius(9).send();
  dwgsPtr->circle().color(dwgsPtr->BLACK).radius(9).send();
  dwgsPtr->arc().color(dwgsPtr->MAGENTA).filled().radius(8).start(0).angle(60).send();
  dwgsPtr->arc().color(dwgsPtr->MAGENTA).filled().radius(8).start(120).angle(60).send();
  dwgsPtr->arc().color(dwgsPtr->MAGENTA).filled().radius(8).start(-60).angle(-60).send();
  dwgsPtr->circle().filled().color(dwgsPtr->YELLOW).radius(2).send();
  dwgsPtr->circle().filled().color(dwgsPtr->MAGENTA).radius(1).send();
  update(); // send label
}


void RadiationSymbol::update() {
  // update the reading
  dwgsPtr->label().bold().floatReading(reading).decimals(2).units(F("<-2>\nsv/hr")).idx(z_idx).color(dwgsPtr->RED).offset(0, 12).send();
}

float RadiationSymbol::getReading() {
  return reading;
}
void RadiationSymbol::setReading(float newReading) {
  reading = newReading;
}

