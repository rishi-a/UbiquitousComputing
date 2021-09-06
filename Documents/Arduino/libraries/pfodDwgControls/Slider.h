#ifndef Slider_h
#define Slider_h
/*
 * (c)2014-2017 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provide this copyright is maintained.
 */

#include <pfodControl.h>

class Slider : public pfodControl {
  public:
    Slider(char _cmd, pfodDwgs *_dwgsPtr);
    void draw();
    void update();
    char getCmd();
    void setBase(int c, int r);

  private:
    char sliderCmd;
    int baseCol;
    int baseRow;
    int z_idx; // z-order start index
};
#endif // Slider_h
