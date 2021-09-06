#ifndef Help_h
#define Help_h
/*
 * (c)2014-2017 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provide this copyright is maintained.
 */

#include "pfodDwgControls.h"

class Help : public pfodControl {
  public:
    Help(char _cmd, pfodDwgs *_dwgsPtr);

    void draw();
    void update();
    char getCmd();
    void setBase(double c, double r);
    void setBaseScaling(double scaling);
    
    /**
     * sets the width and height of text background box
     * start with 
     * width == 2x number of chars wide
     * height = 3 x lines heigh and round up to next even int
     * then adjust if needed
     */
    void setWidthHeight(int w, int h);
    
    /**
     * offset to popup text from ?
     */
    void setPopUpOffset(int c,int r);

  private:
    int popUpOffsetCol;// = -3; // offset to popup text from ?
    int popUpOffsetRow;// = -3;
    char buttonCmd;
    int	width;
    int height;
    float baseCol;
    float baseRow;
    float baseScaling;
    int z_idx;
};
#endif // Help_h
