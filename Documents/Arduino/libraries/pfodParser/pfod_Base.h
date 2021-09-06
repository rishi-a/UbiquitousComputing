#ifndef pfod_Base_h
#define pfod_Base_h
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

#include "pfodStream.h"

class pfod_Base : public Stream {
  public:
    // The subclasses, pfod_Base_xxx, must override all the methods below
    virtual bool connect(); // returns true if connected else false, may block for a little while
    virtual void setDebugStream(Print* out); // for pfod_Base_xxx debugging
    virtual Print* getRawDataOutput() = 0; // for pfod_Base_xxx to supply object to pfodSecurity
    // (actually a pfod_Base_rawOutput object is returned)
    virtual void _setLinkTimeout(unsigned long _linkTimeout_mS); // set by pfodSecurity from idleTimeout  not set for client
    virtual size_t writeRawData(uint8_t c) = 0; // for pfod_Base_xxx, used by pfod_Base_rawOutput object
    virtual size_t write(uint8_t); // for Print
    virtual int available(); // for Stream
    virtual int read() = 0; // for Stream
    virtual int peek(); // for Stream
    virtual void flush(); // for Stream
    virtual void _closeCurrentConnection()=0;
    virtual unsigned long getDefaultTimeOut() = 0; // 600sec for SMS
  protected:
};
#endif //pfod_Base_h
