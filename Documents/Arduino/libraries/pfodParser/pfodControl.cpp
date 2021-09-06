#include "pfodControl.h"
/*
 * (c)2014-2017 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provide this copyright is maintained.
 */

pfodControl::pfodControl(pfodDwgs *_dwgsPtr) {
  dwgsPtr = _dwgsPtr;
  label = NULL;
  value = 0;
}

// every subclass must define these two methods
//void draw()
//void update()
//========================================

// override this to limit value to sensible values
void pfodControl::setValue(int newValue) {
  value = newValue;
}

int pfodControl::getValue() {
  return value;
}

void pfodControl::setLabel(const __FlashStringHelper *_label) {
  label = _label;
}

