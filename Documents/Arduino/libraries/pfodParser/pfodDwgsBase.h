#ifndef pfodDwgsBase_h
#define pfodDwgsBase_h
/*
 * (c)2014-2017 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provide this copyright is maintained.
 */
#include <stdint.h>
#include <Arduino.h>


struct VALUES {
  float startAngle;
  float arcAngle;
  const char *text;
  const __FlashStringHelper *textF;
  const char *units;
  const __FlashStringHelper *unitsF;
  float reading;
  uint8_t haveReading;
  int32_t value;
  uint8_t haveValue;
  float displayMax;
  float displayMin;
  int32_t max;
  int32_t min;
  int decPlaces;
  int fontSize;
  uint8_t bold;
  uint8_t italic;
  uint8_t underline;
  int color; // BLACK_WHITE is -1 for contrast black or white
  float width;
  float height;
  float colOffset;
  float rowOffset;
  uint16_t idx;
  float radius;
  char cmd;
  const char* cmdStr; // for loading inserted dwg
  char loadCmd; // for erase inserted dwg
  const char* loadCmdStr; // for erase inserted dwg
  uint16_t filter;
  uint8_t filled;
  uint8_t rounded;
  uint8_t centered;
  char align;
};

class pfodDwgsBase {
  public:
    virtual void send(char _startChar = '|');
    Print *out;
    static const int BLACK_WHITE = -1;
    static const int BLACK = 0;
    static const int MAROON = 1;
    static const int GREEN = 2;
    static const int OLIVE = 3;
    static const int NAVY = 4;
    static const int PURPLE = 5;
    static const int TEAL = 6;
    static const int SILVER = 7;
    static const int GREY = 8;
    static const int RED = 9;
    static const int LIME = 10;
    static const int YELLOW = 11;
    static const int BLUE = 12;
    static const int FUCHSIA = 13;
    static const int MAGENTA = FUCHSIA;
    static const int AQUA = 14;
    static const int WHITE = 15;
    // touchZone filters
    const static int TOUCH = 0; // touches blocked if pfodApp busy waiting for response
    const static int DOWN = 1;  // queued if pfodApp busy waiting for response
    const static int DRAG = 2; // queued if pfodApp busy waiting for response
    const static int UP = 4; // queued if pfodApp busy waiting for response
    const static int CLICK = 8; // queued if pfodApp busy waiting for response
    const static int PRESS = 16; // queued if pfodApp busy waiting for response
    const static int ENTRY = 32; // queued if pfodApp busy waiting for response
    const static int EXIT = 64; // queued if pfodApp busy waiting for response
    // these flags are NEVER sent to pfodApp
    const static int DOWN_UP = 256; // msg not sent until finger removed (UP) but updates touchAction
    const static int TOUCH_DISABLED = 512; // capture touch to prevent scroll but do not send msg

    // These are only valid for size and offset, and label values
    const static uint16_t TOUCHED_COL = 65534; // only used in touchZone actions to specify touched col value
    const static uint16_t TOUCHED_ROW = 65532; // only used in touchZone actions to specify touched row value
    
    void printFloatNumber(float f); // prints just number, no ~
  protected:
    void startText();
    void printFloat(float f); // prints ~ + number
    void printFloatDecimals(float f,int decPlaces); // prints float with +/- decimals, no ~
    void sendColRowVars(float val); // replace special valuse with c r rc or rr
    void printColor();
    void printIdx();
    void colRowOffset();
    void colWidthHeight();
    void initValues(struct VALUES* _valuesPtr);
    struct VALUES* valuesPtr;
    pfodDwgsBase();
  private:
};

#endif // pfodDwgsBase_h
