#include "pfod_Base.h"
/**
pfod_Base for Arduino
Base class for all pfod_Base_xxxx classes
The subclasses pfod_Base_xxx must override all the methods below
*/
/*
 * (c)2014-2017 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provide this copyright is maintained.
 */

void pfod_Base::_setLinkTimeout(unsigned long _linkTimeout_mS) {
	(void)(_linkTimeout_mS);
}

/* 
 * returns true if connected else false, may block for a little while
 */
bool pfod_Base::connect() {
	return true; // default does nothing
}

size_t pfod_Base::write(uint8_t c) {
  (void)(c);
  return 0;
}

int pfod_Base::available() {
  return 0;
}

int pfod_Base::peek() {
  return -1;
}

void pfod_Base::flush()	{}

void pfod_Base::setDebugStream(Print* out) {
  (void)(out);
}

