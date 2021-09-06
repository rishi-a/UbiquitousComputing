
#include "OnOffSwitch.h"
/*
 * (c)2014-2017 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provide this copyright is maintained.
 */

OnOffSwitch::OnOffSwitch(char _cmd, pfodDwgs *_dwgsPtr) : pfodControl(_dwgsPtr) {
  buttonCmd = _cmd;
  z_idx = pfodDwgs::reserveIdx(3); // z_idx for button, z_idx+1 OFF label, z_idx+2 ON label
}

char OnOffSwitch::getCmd() {
  return buttonCmd;
}
void OnOffSwitch::draw() {
  if (label != NULL) { // only send label if one set
    dwgsPtr->label().bold().fontSize(2).text(label).color(dwgsPtr->RED).offset(0, 4).send();
  }
  dwgsPtr->rectangle().color(dwgsPtr->GREY).rounded().filled().size(12, 3).offset(-6, -1.5).send();
  // add on/off labels with higher z-order then the button rectangle so they are always drawn on top of it
  dwgsPtr->label().fontSize(-1).text(F("OFF")).idx(z_idx + 1).color(dwgsPtr->WHITE).offset(-3, 0).send();
  dwgsPtr->label().fontSize(-1).text(F("ON")).idx(z_idx + 2).color(dwgsPtr->WHITE).offset(3, 0).send();
  update(); // update with current state
}

void OnOffSwitch::update() {
  dwgsPtr->rectangle().filled().rounded().idx(z_idx).size(6, 4).offset(((value != 0) ? 0 : -6), -2)
  .color((value != 0) ? dwgsPtr->GREEN : dwgsPtr->BLACK).send();
  // update the touchZone and action to the opposite side then is currently set
  dwgsPtr->touchZone().cmd(buttonCmd).size(6, 4).offset(((value != 0) ? -6 : 0), -2).filter(dwgsPtr->TOUCH).send();
  dwgsPtr->touchAction().cmd(buttonCmd).action( // create the action
    dwgsPtr->rectangle().filled().rounded().idx(z_idx).size(6, 3).offset(((value != 0) ? -6 : 0), -1.5)
    .color((value != 0) ? dwgsPtr->BLACK : dwgsPtr->GREEN) //
  ).send(); // send the touchAction
}


