#ifndef pfodWaitForUtils_h
#define pfodWaitForUtils_h
/*
 * (c)2014-2017 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provide this copyright is maintained.
 */

#include "Arduino.h"

#include "pfodStream.h"

#define _pfodWaitForUtils_waitForTimeout 3000
class pfodWaitForUtils {
  public:
    static boolean waitFor(const __FlashStringHelper *ifsh, Stream* input, Stream* output = NULL);
    static boolean waitFor(const __FlashStringHelper *ifsh, unsigned long timeout, Stream* input, Stream* output = NULL);
    static boolean waitForOK(Stream* input, Stream* output = NULL);
    static boolean waitForOK(unsigned long timeout, Stream* input, Stream* output = NULL);
    static boolean waitFor(const char* str, Stream* input, Stream* output = NULL);
    static boolean waitFor(const char* str, unsigned long timeout, Stream* input, Stream* output = NULL);
    //static boolean waitFor2(const char* str1, const char* str2, unsigned long timeout, Stream* input, Stream* output = NULL );
    static int waitFor(const char* str1, const char* str2,  Stream* input, Stream* output = NULL, unsigned long timeout = 0);
    static int waitFor(const char* str1, const char* str2,  const char* str3, Stream* input, Stream* output, unsigned long _timeout );
    static size_t captureReply(char* buffer, size_t maxLen, Stream* input, Stream* output = NULL, unsigned long timeout = 100);
    static void dumpReply(Stream* input, Stream* output = NULL, unsigned long timeout = 100);
    static void setWaitForTimeout(unsigned long _timeout);
  private:
    static unsigned long timeout;// = 3000; _pfodWaitForUtils_waitForTimeout
};
#endif // pfodWaitForUtils_h

