#ifndef DesignerSwitch_h
#define DesignerSwitch_h
/*
 * (c)2014-2017 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provide this copyright is maintained.
 */

#include "pfodControl.h"

class DesignerSwitch : public pfodControl {
  public:
    DesignerSwitch(char _cmd, pfodDwgs *_dwgsPtr);

    void draw();
    void update();
    char getCmd();

  private:
    char buttonCmd;
    int z_idx;
};
#endif // DesignerSwitch_h
