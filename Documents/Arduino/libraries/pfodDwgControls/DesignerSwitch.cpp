/*
 * (c)2014-2017 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provide this copyright is maintained.
 */

#include "DesignerSwitch.h"

DesignerSwitch::DesignerSwitch(char _cmd, pfodDwgs *_dwgsPtr) : pfodControl(_dwgsPtr) {
  buttonCmd = _cmd;
  z_idx = pfodDwgs::reserveIdx(3); // reserve indices for this control
}

char DesignerSwitch::getCmd() {
  return buttonCmd;
}

void DesignerSwitch::draw() {
  dwgsPtr->label().fontSize(2).bold().text(label).color(pfodDwgs::RED).offset(0, 4).send();
  dwgsPtr->rectangle().color(pfodDwgs::GREY).rounded().filled().size(12, 3).offset(-6, -1.5).send();
  // 6x3 + 6x3
  // add on/off labels with higher z-order then the button rectangle so they are always drawn on top of it
  dwgsPtr->label().fontSize(-1).text(F("OFF")).idx(z_idx + 1).color(pfodDwgs::WHITE).offset(-3, 0).send();
  dwgsPtr->label().fontSize(-1).text(F("ON")).idx(z_idx + 2).color(pfodDwgs::WHITE).offset(3, 0).send();
  update(); // update with current state
}

void DesignerSwitch::update() {
  // place the large rectangle on the correct side
  dwgsPtr->rectangle().filled().rounded().idx(z_idx).size(6, 4).offset(((value != 0) ? 0 : -6), -2)
  .color((value != 0) ? pfodDwgs::GREEN : pfodDwgs::BLACK).send();
  // update the touchZone filters to ignore the currently set side
  dwgsPtr->touchZone().cmd(buttonCmd).size(6, 4).offset((value != 0) ? -6 : 0, -2).filter(pfodDwgs::TOUCH).send();
  // set the action for the other size
  // if On set action for off, and set colour for off BLACK, set offset for off
  // else Off set action for on, and set colour for on GREEN, set offset for on
  dwgsPtr->touchAction().cmd(buttonCmd).action(     // create the action
    dwgsPtr->rectangle().filled().rounded().idx(z_idx).size(6, 3).offset(((value != 0) ? -6 : 0), -1.5)
    .color((value != 0) ? pfodDwgs::BLACK : pfodDwgs::GREEN)
  ).send(); // send the touchAction
}


