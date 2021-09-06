#ifndef pfodControl_h
#define pfodControl_h
/*
 * (c)2014-2017 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provide this copyright is maintained.
 */

#include "pfodDwgs.h"

class pfodControl {

  public:
    pfodControl(pfodDwgs *_dwgsPtr);

    // every subclass must define these two methods
    virtual void draw() = 0;
    virtual void update() = 0;
    //========================================

    void setValue(int newValue);
    int getValue();

    void setLabel(const __FlashStringHelper *_label);

  protected:
    pfodDwgs *dwgsPtr;
    const __FlashStringHelper *label;
    int value;
};
#endif //pfodControl_h 
