/*
 * (c)2014-2017 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provide this copyright is maintained.
 */

#include "Slider.h"

Slider::Slider(char _cmd, pfodDwgs *_dwgsPtr) : pfodControl(_dwgsPtr) {
  baseCol = 0; baseRow = 0;
  sliderCmd = _cmd;
  z_idx = pfodDwgs::reserveIdx(3); // z_idx for button, z_idx+1 OFF label, z_idx+2 ON label
}

char Slider::getCmd() {
  return sliderCmd;
}

void Slider::setBase(int c, int r) {
	baseCol = c;
	baseRow = r;
}

void Slider::draw() {
  dwgsPtr->pushZero(baseCol, baseRow, 0.125);  // scale down 255 to 32 cols
  dwgsPtr->rectangle().filled().color(dwgsPtr->GREY).size(255, 1).offset(0, -0.5).send();

  dwgsPtr->label().fontSize(18).text("0%").color(dwgsPtr->RED).offset(0, 18).send();
  dwgsPtr->line().color(dwgsPtr->RED).offset(0, -7.5).size(0, 15).send();
  dwgsPtr->label().fontSize(18).text("100%").color(dwgsPtr->RED).offset(255, 18).send();
  dwgsPtr->line().color(dwgsPtr->RED).offset(255, -7.5).size(0, 15).send();

  // =============== start active area
  // define button active region and buttonCmd
  dwgsPtr->touchZone().cmd(sliderCmd).size(255, 1).offset(0, -0.5).filter(dwgsPtr->DOWN_UP).send(); //
  dwgsPtr->touchAction()
  .cmd(sliderCmd)
  .action( //
    dwgsPtr->label().idx(z_idx + 2).decimals(1).intValue(dwgsPtr->TOUCHED_COL).units("%").maxValue(255)
    .displayMax(100).fontSize(18).color(dwgsPtr->BLACK).offset(dwgsPtr->TOUCHED_COL, -50)//
  ).send();
  dwgsPtr->touchAction().cmd(sliderCmd).action( //
    dwgsPtr->line().color(dwgsPtr->BLACK).offset(dwgsPtr->TOUCHED_COL, -40).size(0, 40)//
  ).send();
  // ========== end active area

  update(); // update with current state
  dwgsPtr->popZero();
}

void Slider::update() {
  dwgsPtr->circle().filled().radius(8).idx(z_idx).color(dwgsPtr->BLACK).offset(value, 0).send();
  dwgsPtr->rectangle().filled().idx(z_idx + 1).color(dwgsPtr->BLACK).size(value, 5).offset(0, -2.5).send();
  dwgsPtr->label().text(label).decimals(1).idx(z_idx + 2).intValue(value).units("%").maxValue(255).displayMax(100)
  .fontSize(24).color(dwgsPtr->RED).offset(128, -30).send();
}


