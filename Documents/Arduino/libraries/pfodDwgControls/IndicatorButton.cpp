/*
 * (c)2014-2017 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provide this copyright is maintained.
 */

#include "IndicatorButton.h"

IndicatorButton::IndicatorButton(char _cmd, pfodDwgs *_dwgsPtr) : pfodControl(_dwgsPtr) {
  buttonCmd = _cmd;
  z_idx = pfodDwgs::reserveIdx(1); // z_idx for circle
}

char IndicatorButton::getCmd() {
  return buttonCmd;
}

void IndicatorButton::draw() {
  // circle in a rectangle with a label below
  if (label != NULL) { // only send label if one set
    dwgsPtr->label().text(label).color(dwgsPtr->RED).offset(0, 4.3).send();
  }
  dwgsPtr->rectangle().filled().color(dwgsPtr->GREY).rounded().size(5.5, 5.5).offset(-2.75, -2.75).send();
  update(); // update with current state
}

void IndicatorButton::update() {
  dwgsPtr->circle().filled().idx(z_idx).color((value != 0) ? dwgsPtr->YELLOW : dwgsPtr->BLACK).radius(1.5).send();
  dwgsPtr->touchZone().cmd(buttonCmd).size(4, 4).offset(-2, -2).send();
  dwgsPtr->touchAction().cmd(buttonCmd).action( //
    dwgsPtr->circle().filled().idx(z_idx).color((value != 0) ? dwgsPtr->GREY : dwgsPtr->YELLOW).radius(2.3)
  ).send();
}


