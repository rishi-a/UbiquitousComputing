
#include "OnOffSlider.h"
/*
 * (c)2014-2017 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provide this copyright is maintained.
 */

OnOffSlider::OnOffSlider(char _cmd, pfodDwgs *_dwgsPtr) : pfodControl(_dwgsPtr) {
  buttonCmd = _cmd;
  z_idx = pfodDwgs::reserveIdx(1); // z_idx for dot
}

char OnOffSlider::getCmd() {
  return buttonCmd;
}

void OnOffSlider::draw() {
  dwgsPtr->pushZero(0, 0, 1.5); // scale up by 1.5
  dwgsPtr->label().text(label).color(dwgsPtr->RED).offset(0, -3).send();
  // add on/off labels with higher z-order then the button rectangle so they are always drawn on top of it
  dwgsPtr->label().bold().fontSize(-6).text(("Off")).offset(-2, 2.3).send();
  dwgsPtr->label().bold().fontSize(-6).text(("On")).offset(2, 2.3).send();

  dwgsPtr->rectangle().rounded().filled().size(4.5, 1).offset(-2.25, -0.5).send();  // 4.5x1 + 4.5x1
  update(); // update with current state
  dwgsPtr->popZero();
}

void OnOffSlider::update() {
  String rtn = "";
  // place the button on the correct side
  dwgsPtr->circle().filled().idx(z_idx).radius(1).offset((value != 0 ? 1.5 : -1.5), 0).color(value != 0 ? dwgsPtr->GREEN : dwgsPtr->BLACK).send();
  dwgsPtr->touchZone().cmd(buttonCmd).size(3, 3).offset((value != 0)  ? -3 : 0, -1.5).send(); // respond to TOUCH
  dwgsPtr->touchAction().cmd(buttonCmd).action(	  // create the action
    dwgsPtr->circle().filled().idx(z_idx).radius(1.3).offset( (value != 0) ? -1.5 : 1.5, 0)
    .color((value != 0) ? dwgsPtr->BLACK : dwgsPtr->GREEN)
  ).send(); // send the touchAction
}

