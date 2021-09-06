/*
 * (c)2014-2017 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provide this copyright is maintained.
 */

#include "Help.h"

/**
 * Move to bottom right corner of dwg and scale if necessary
 * e.g. for a 70x60 dwg  squares run from (0,0) to (69,59) 
 *  so  setBase(69,59);   
 *      setBaseScaling(1.2);   
 * The help.draw() code then moves back enough to show the ?
 */ 
Help::Help(char _cmd, pfodDwgs *_dwgsPtr) : pfodControl(_dwgsPtr) {
  buttonCmd = _cmd;
  width = 10;
  height = 10; // start with something incase setWidthHeight not called
  popUpOffsetCol = -3; // offset to popup text from ?
  popUpOffsetRow = -3;
  baseCol = 0;
  baseRow = 0;
  baseScaling = 1;
  z_idx = 500+pfodDwgs::reserveIdx(5); // reserve 5 unique indices 
  // and then add 500 to put these z_idx on top of everything else in the drawing
}

char Help::getCmd() {
  return buttonCmd;
}

void Help::setBase(double c, double r) {
  baseCol = (float)c;
  baseRow = (float)r;
}

void Help::setBaseScaling(double scaling) {
  baseScaling = (float)scaling;
}

/**
 * sets the width and height of text background box
 * start with 
 * width == 2x number of chars wide
 * height = 3 x lines heigh and round up to next even int
 * then adjust if needed
 */
void Help::setWidthHeight(int w, int h) {
  width = w;
  height = h;
}

void Help::setPopUpOffset(int c,int r) {
  // default for bottom right
  popUpOffsetCol = c;
  popUpOffsetRow = r;
}

void Help::draw() {
  dwgsPtr->pushZero(baseCol, baseRow,baseScaling);
  dwgsPtr->circle().idx(z_idx+1).filled().color(dwgsPtr->NAVY).radius(1.8).send();
  dwgsPtr->label().idx(z_idx+2).bold().text("?").color(dwgsPtr->WHITE).send();
  dwgsPtr->touchZone().idx(z_idx).cmd(buttonCmd).centered().filter(dwgsPtr->DOWN_UP).size(3.6,3.6).send(); // cover circle
  dwgsPtr->touchAction().cmd(buttonCmd).action( //
    dwgsPtr->rectangle().idx(z_idx+3).filled().rounded().color(dwgsPtr->BLUE).size(width,height).offset(popUpOffsetCol-width,popUpOffsetRow-height)
  ).send();
  dwgsPtr->touchAction().cmd(buttonCmd).action( //
    dwgsPtr->label().idx(z_idx+4).text(label).color(dwgsPtr->WHITE).offset(popUpOffsetCol+(-width)/2,popUpOffsetRow+(-height)/2)
  ).send();
  update(); // update with current state
  dwgsPtr->popZero(); // undo initial move
}

void Help::update() {
  // nothing here
}
