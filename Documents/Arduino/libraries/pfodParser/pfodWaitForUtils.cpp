/*
 * (c)2014-2017 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provide this copyright is maintained.
 */

#include "pfodWaitForUtils.h"
#include "pfodParserUtils.h"

unsigned long pfodWaitForUtils::timeout = _pfodWaitForUtils_waitForTimeout;

void pfodWaitForUtils::setWaitForTimeout(unsigned long _timeout) {
  timeout = _timeout;
}

/** 
 * capture input and save in buffer upto maxLen-1 bytes, '\0' always put at maxLen dump rest and wait for timeout with no input received.
 * returns number of bytes saved.
 */
size_t pfodWaitForUtils::captureReply(char* buffer, size_t maxLen, Stream* input, Stream* output, unsigned long timeout) {
  size_t bufferIdx = 0;
  if (maxLen < 1) {
  	  return bufferIdx;
  }
  unsigned long startTime = millis();
  while (millis() - startTime < timeout) {
    if (input->available()) {
      char c = input->read();
      if (bufferIdx < (maxLen-1)) {
      	  buffer[bufferIdx++] = c;
      } // else just dump
      if (output) {
        output->print(c); // echo it
      }
      startTime = millis(); // reset start time
    }
  }
  if (bufferIdx < maxLen) {
  	  buffer[bufferIdx] = '\0'; // terminate result
  } 
  return bufferIdx;
}

boolean pfodWaitForUtils::waitFor(const __FlashStringHelper *ifsh, Stream* input, Stream* output ) {
  const int MAX_LEN = 16;
  char str[MAX_LEN]; // allow for null
  getProgStr(ifsh, str, MAX_LEN);
  return waitFor((char*)str, input, output);
}

boolean pfodWaitForUtils::waitFor(const __FlashStringHelper *ifsh, unsigned long timeout, Stream* input, Stream* output ) {
  const int MAX_LEN = 16;
  char str[MAX_LEN]; // allow for null
  getProgStr(ifsh, str, MAX_LEN);
  return waitFor((char*)str, timeout, input, output);
}

boolean pfodWaitForUtils::waitForOK(Stream* input, Stream* output ) {
  return waitFor(F("OK"), input, output);
}

boolean pfodWaitForUtils::waitForOK(unsigned long timeout, Stream* input, Stream* output ) {
  return waitFor(F("OK"), timeout, input, output);
}

boolean pfodWaitForUtils::waitFor(const char* str, Stream* input, Stream* output ) {
  return waitFor(str, timeout, input, output);
}

boolean pfodWaitForUtils::waitFor(const char* str, unsigned long timeout, Stream* input, Stream* output ) {
  size_t targetLen = 0;
  const char* target = str;
  size_t index = 0;
  targetLen = strlen(str);
  long startTime = millis();
  if (targetLen == 0) {
    return true; // always matches
  }
  while (millis() - startTime < timeout) {
    // else
    if (input->available()) {
      char c = input->read();
      if (output) {
        output->print(c);
      }
      if (c != target[index]) {
        index = 0; // reset index if any char does not match
      } // no else here
      if (c == target[index]) {
        //Serial.print("found "); Serial.write(c); Serial.print("index now"); Serial.println(index+1);
        if (++index >= targetLen) { // return true if all chars in the target match
          return true;
        }
      }
    }
  }
  return false; // timed out
}

/**
  returns -1 if timeout
  else returns 0 if str1 matched or 1 if str2 matched
*/
int pfodWaitForUtils::waitFor(const char* str1, const char* str2,  Stream* input, Stream* output, unsigned long _timeout ) {
  size_t targetLen1 = 0;
  size_t targetLen2 = 0;
  const char* target1 = str1;
  const char* target2 = str2;
  size_t index1 = 0;
  size_t index2 = 0;
  targetLen1 = strlen(str1);
  targetLen2 = strlen(str2);
  long startTime = millis();
  if ((targetLen1 == 0) && (targetLen2 == 0)) {
    return 0; // always matches str1
  }
  unsigned long time_out = _timeout;
  if (time_out <= 0) {
    time_out = timeout; // use global setting
  }

  while (millis() - startTime < time_out) {
    // else
    if (input->available()) {
      char c = input->read();
      if (output) {
        output->print(c);
      }
      // check target 1
      if (c != target1[index1]) {
        index1 = 0; // reset index if any char does not match
      } // no else here
      if (c == target1[index1]) {
        //Serial.print("found "); Serial.write(c); Serial.print("index now"); Serial.println(index+1);
        if (++index1 >= targetLen1) { // return true if all chars in the target match
          return 0;
        }
      }
      // check target 2
      if (c != target2[index2]) {
        index2 = 0; // reset index if any char does not match
      } // no else here
      if (c == target2[index2]) {
        //Serial.print("found "); Serial.write(c); Serial.print("index now"); Serial.println(index+1);
        if (++index2 >= targetLen2) { // return true if all chars in the target match
          return 1;
        }
      }

    }
  }
  return -1; // timed out
}

/**
  returns -1 if timeout
  else returns 0 if str1 matched or 1 if str2 matched or 2 if str3 matched
*/
int pfodWaitForUtils::waitFor(const char* str1, const char* str2,  const char* str3, Stream* input, Stream* output, unsigned long _timeout ) {
  size_t targetLen1 = 0;
  size_t targetLen2 = 0;
  size_t targetLen3 = 0;
  const char* target1 = str1;
  const char* target2 = str2;
  const char* target3 = str3;
  size_t index1 = 0;
  size_t index2 = 0;
  size_t index3 = 0;
  targetLen1 = strlen(str1);
  targetLen2 = strlen(str2);
  targetLen3 = strlen(str2);
  long startTime = millis();
  if ((targetLen1 == 0) && (targetLen2 == 0) && (targetLen3 == 0)) {
    return 0; // always matches str1
  }
  unsigned long time_out = _timeout;
  if (time_out <= 0) {
    time_out = timeout; // use global setting
  }

  while (millis() - startTime < time_out) {
    // else
    if (input->available()) {
      char c = input->read();
      if (output) {
        output->print(c);
      }
      // check target 1
      if (c != target1[index1]) {
        index1 = 0; // reset index if any char does not match
      } // no else here
      if (c == target1[index1]) {
        //Serial.print("found "); Serial.write(c); Serial.print("index now"); Serial.println(index+1);
        if (++index1 >= targetLen1) { // return true if all chars in the target match
          return 0;
        }
      }
      // check target 2
      if (c != target2[index2]) {
        index2 = 0; // reset index if any char does not match
      } // no else here
      if (c == target2[index2]) {
        //Serial.print("found "); Serial.write(c); Serial.print("index now"); Serial.println(index+1);
        if (++index2 >= targetLen2) { // return true if all chars in the target match
          return 1;
        }
      }
      // check target 3
      if (c != target3[index3]) {
        index3 = 0; // reset index if any char does not match
      } // no else here
      if (c == target3[index3]) {
        //Serial.print("found "); Serial.write(c); Serial.print("index now"); Serial.println(index+1);
        if (++index3 >= targetLen3) { // return true if all chars in the target match
          return 2;
        }
      }

    }
  }
  return -1; // timed out
}

// just dump chars from gprs to console until none found for 100mS
void pfodWaitForUtils::dumpReply(Stream* input, Stream* output, unsigned long timeout) {
  unsigned long startTime = millis();
  //const unsigned long timeout = 100;
  while (millis() - startTime < timeout) {
    if (input->available()) {
      char c = input->read();
      if (output) {
        output->print(c); // echo it
      }
      startTime = millis(); // reset start time
    }
  }
}

