#include "pfod_rawOutput.h"
/*
 * (c)2014-2017 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provide this copyright is maintained.
 */

void pfod_rawOutput::set_pfod_Base( pfod_Base* arg) {
  _pfod_Base = arg;
}

size_t pfod_rawOutput::write(uint8_t c)  {
  return _pfod_Base->writeRawData(c);
}

/* default implementation: may be overridden */
size_t pfod_rawOutput::write(const uint8_t *buffer, size_t size) {
  size_t n = 0;
  while (size--) {
    n += write(*buffer++);
  }
  return n;
}

