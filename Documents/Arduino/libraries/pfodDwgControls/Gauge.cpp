/*
 * (c)2014-2017 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provide this copyright is maintained.
 */

#include "Gauge.h"


Gauge::Gauge(pfodDwgs *_dwgsPtr) : pfodControl(_dwgsPtr) {
  z_idx = pfodDwgs::reserveIdx(3);
}

void Gauge::draw() {
  dwgsPtr->arc().filled().color(dwgsPtr->GREY).radius(5).start(217.5).angle(-255).send();
  dwgsPtr->arc().filled().color(dwgsPtr->WHITE).radius(4).idx(z_idx+1).start(217.5).angle(-255).send();

  dwgsPtr->arc().filled().color(pfodDwgs::BLACK).radius(5.5).start(217.5).angle(0).send();
  dwgsPtr->arc().filled().color(pfodDwgs::BLACK).radius(5.5).start(90).angle(0).send();
  dwgsPtr->arc().filled().color(pfodDwgs::BLACK).radius(5.5).start(-37.5).angle(0).send();

  dwgsPtr->label().offset(-5.5,3).fontSize(-6).text(F("0%")).send();
  dwgsPtr->label().offset(0,-6.5).fontSize(-6).text(F("50%")).send();
  dwgsPtr->label().offset(6.5,3).fontSize(-6).text(F("100%")).send();
  update(); // update with current state
}

void Gauge::update() {
  dwgsPtr->arc().filled().color(dwgsPtr->RED).radius(5).idx(z_idx).start(217.5).angle(-value).send();
  dwgsPtr->label().color(dwgsPtr->RED).idx(z_idx+2).fontSize(-3).bold().text(label).intValue(value).maxValue(255).displayMax(100).decimals(1).units(F("%")).send();
}

