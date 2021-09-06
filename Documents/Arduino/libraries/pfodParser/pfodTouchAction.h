#ifndef pfodTouchAction_h
#define pfodTouchAction_h
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

class pfodTouchAction : public pfodDwgsBase {
  public:
    pfodTouchAction();
    pfodTouchAction &action(pfodDwgsBase &_action); 
    pfodTouchAction &cmd(const char _cmd); // default ' ' not set
    pfodTouchAction &cmd(const char* _cmdStr);
    void init(Print *out, struct VALUES* _values);
    void send(char _startChar = '|');
  private:
  	pfodDwgsBase *actionPtr;
	char actionCmd;
	const char* actionCmdStr;
};
#endif // pfodTouchAction_h
