#ifndef Gauge_h
#define Gauge_h
/*
 * (c)2014-2017 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provide this copyright is maintained.
 */

#include "pfodDwgControls.h"

class Gauge : public pfodControl {
  public:
    Gauge( pfodDwgs *_dwgsPtr);
    void draw();
    void update();

  private:
    int fullScaleAngle;
    int z_idx;
};
#endif // Gauge_h
