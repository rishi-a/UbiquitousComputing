/*
 * (c)2014-2017 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provide this copyright is maintained.
 */

#include "VSlider.h"

VSlider::VSlider(char _cmd, pfodDwgs *_dwgsPtr) : pfodControl(_dwgsPtr) {
  sliderCmd = _cmd;
  z_idx = pfodDwgs::reserveIdx(3); // z_idx for button, z_idx+1 OFF label, z_idx+2 ON label
}

char VSlider::getCmd() {
  return sliderCmd;
}

void VSlider::draw() {
  dwgsPtr->pushZero(0, 0, 0.125);  // scale down 255 to 32 cols
  dwgsPtr->rectangle().filled().color(dwgsPtr->GREY).size(1, -255).offset( -0.5, 0).send();

  dwgsPtr->label().fontSize(18).text(F("0%")).color(dwgsPtr->RED).offset(30, 0).send();
  dwgsPtr->line().color(dwgsPtr->RED).offset( -7.5, 0).size( 15, 0).send();
  dwgsPtr->label().fontSize(18).text(F("100%")).color(dwgsPtr->RED).offset( 30, -255).send();
  dwgsPtr->line().color(dwgsPtr->RED).offset( -7.5, -255).size( 15, 0).send();

  // =============== start active area
  // define button active region and buttonCmd
  dwgsPtr->touchZone().cmd(sliderCmd).size(1, -255).offset(-0.5, 0).filter(dwgsPtr->DOWN_UP).send(); //
  dwgsPtr->touchAction() .cmd(sliderCmd).action( //
    dwgsPtr->label().decimals(1).intValue(dwgsPtr->TOUCHED_ROW).units(F("%")).maxValue(-255)
    .displayMax(100).fontSize(18).color(dwgsPtr->BLACK).offset(-65, dwgsPtr->TOUCHED_ROW ) //
  ).send();
  dwgsPtr->touchAction().cmd(sliderCmd).action( //
    dwgsPtr->line().color(dwgsPtr->BLACK).offset(-40, dwgsPtr->TOUCHED_ROW ).size(35, 0)//
  ).send();
  // ========== end active area

  update(); // update with current state
  dwgsPtr->popZero();
}

void VSlider::update() {
  dwgsPtr->circle().filled().radius(8).idx(z_idx).color(dwgsPtr->BLACK).offset( 0, -value).send();
  dwgsPtr->rectangle().filled().idx(z_idx + 1).color(dwgsPtr->BLACK).size(5, -value).offset(-2.5, 0).send();
  dwgsPtr->label().text(label).decimals(1).idx(z_idx + 2).intValue(value).units(F("%")).maxValue(255).displayMax(100)
  .fontSize(28).color(dwgsPtr->RED).offset(0, 45).send();
}


