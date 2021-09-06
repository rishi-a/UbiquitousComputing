/*
 * (c)2014-2017 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provide this copyright is maintained.
 */
#include "pfodUnhide.h"

pfodUnhide::pfodUnhide()  {
}

void pfodUnhide::init(Print *_out, struct VALUES* _values) {
  initValues(_values);
  out = _out;
  //valuesPtr->lastDwg = this;
}

pfodUnhide &pfodUnhide::idx(uint16_t _idx) {
  valuesPtr->idx = _idx;
  return *this;
}


pfodUnhide &pfodUnhide::cmd(const char _cmd) {
  valuesPtr->cmd = _cmd;
  valuesPtr->cmdStr = NULL;
  valuesPtr->loadCmd = ' ';
  valuesPtr->loadCmdStr = NULL;
  return *this;
}
pfodUnhide &pfodUnhide::cmd(const char* _cmdStr) {
  valuesPtr->cmdStr = _cmdStr;
  valuesPtr->cmd = ' ';
  valuesPtr->loadCmd = ' ';
  valuesPtr->loadCmdStr = NULL;
  return *this;
}

pfodUnhide &pfodUnhide::loadCmd(const char _loadCmd) {
  valuesPtr->loadCmd = _loadCmd;
  valuesPtr->loadCmdStr = NULL;
  valuesPtr->cmd = ' ';
  valuesPtr->cmdStr = NULL;
  return *this;
}

pfodUnhide &pfodUnhide::loadCmd(const char* _loadCmdStr) {
  valuesPtr->loadCmd = ' ';
  valuesPtr->loadCmdStr = _loadCmdStr;
  valuesPtr->cmd = ' ';
  valuesPtr->cmdStr = NULL;
  return *this;
}

// if loadCmd( ) then send 'uhd' cmd else send 'uh'
void pfodUnhide::send(char _startChar) {
  out->print(_startChar);
  if ((valuesPtr->loadCmd != ' ') || (valuesPtr->loadCmdStr)) {  	  
  	out->print("uhd");
    out->print('~');
    if (valuesPtr->loadCmdStr) {
      out->print(valuesPtr->loadCmdStr);
    } else {
      out->print(valuesPtr->loadCmd);
    }
  } else { 	  
    out->print("uh");
    if (valuesPtr->idx > 0) {
      printIdx();
    } else {
      out->print('~');
      if (valuesPtr->cmdStr) {
        out->print(valuesPtr->cmdStr);
      } else {
        out->print(valuesPtr->cmd);
      }
    }
  }  
 // valuesPtr->lastDwg = NULL; // sent now
}

