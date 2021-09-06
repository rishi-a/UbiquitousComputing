#ifndef pfodDwgs_h
#define pfodDwgs_h
/*
 * (c)2014-2017 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provide this copyright is maintained.
 */
#include <stdint.h>
#include <Arduino.h>
#include "pfodDwgsBase.h"
#include "pfodCircle.h"
#include "pfodRectangle.h"
#include "pfodLabel.h"
#include "pfodTouchZone.h"
#include "pfodTouchAction.h"
#include "pfodLine.h"
#include "pfodArc.h"
#include "pfodErase.h"
#include "pfodHide.h"
#include "pfodUnhide.h"
#include "pfodIndex.h"
#include "pfodInsertDwg.h"


class pfodDwgs : public pfodDwgsBase {
  public:

    // returns the first index of the reserved ones
    static int reserveIdx(int numToReserve);

    pfodDwgs(Print *out);
    void start(int cols, int rows, int backgroundColor = WHITE, uint8_t moreData = 0);
    void startUpdate(uint8_t moreData = 0);
    void end();
    void pushZero(double col, double row = 0.0, double scale = 1.0);
    void popZero(); // restore previous zero
    pfodRectangle& rectangle();
    pfodCircle& circle();
    pfodLabel& label();
    pfodTouchZone& touchZone();
    pfodTouchAction& touchAction();
    pfodInsertDwg& insertDwg();
    pfodLine& line();
    pfodArc& arc();
    pfodErase& erase();
    pfodHide& hide();
    pfodUnhide& unhide();
    pfodIndex& index();

  protected:
    struct VALUES values;
	
  private:
    pfodCircle c;
    pfodRectangle r;
    pfodLabel t;
    pfodTouchZone x;
    pfodTouchAction X;
    pfodLine l;
    pfodArc a;
    pfodErase e;
    pfodHide h;
    pfodUnhide u;
    pfodIndex i;
    pfodInsertDwg d;
    static int _idx;

};
#endif // pfodDwgs_h
