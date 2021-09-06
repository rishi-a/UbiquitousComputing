/**
pfodUtils for Arduino
*/
/*
 * (c)2014-2017 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provide this copyright is maintained.
 */
#include "Arduino.h"
#include "pfodParserUtils.h"

// will always put '\0\ at dest[maxLen]
// return the number of char copied excluding the terminating null
size_t strncpy_safe(char* dest, const char* src, size_t maxLen) {
  size_t rtn = 0;
  if (src == NULL) {
    dest[0] = '\0';
  } else {
    strncpy(dest, src, maxLen);
    rtn = strlen(src);
    if ( rtn > maxLen) {
      rtn = maxLen;
    }
  }
  dest[maxLen] = '\0';
  return rtn;
}

/**
 * ipStrToNum
 * works for IPV4 only
 * parses  "10.1.1.200" and "10,1,1,200" strings 
 * extra spaces ignored  eg "10, 1, 1, 200" is OK also 
 * return uint32_t for passing to ipAddress( );
 * NOTE: for WICED uint32_t stored in reverse order to Arduino Uno
 */
uint32_t ipStrToNum(const char* ipStr) {
  const int SIZE_OF_NUMS = 4;
  union {
  uint8_t bytes[SIZE_OF_NUMS];  // IPv4 address
  uint32_t dword;
  } _address;
  _address.dword = 0; // clear return

  int i = 0;
  uint8_t num = 0; // start with 0
  while ((*ipStr) != '\0') {
    // while not end of string
    if ((*ipStr == '.') || (*ipStr == ',')) {
      // store num and move on to next position
      _address.bytes[i++] = num;
      num = 0;
      if (i>=SIZE_OF_NUMS) {
        break; // filled array
      }
    } else {  
      if (*ipStr != ' ') {      // skip blanks
        num = (num << 3) + (num << 1); // *10 = *8+*2
        num = num +  (*ipStr - '0');
      }  
    }  
    ipStr++;
  }  
  if (i<SIZE_OF_NUMS) {
    // store last num
    _address.bytes[i++] = num;
  }
  return _address.dword; 
}  


// maxLen includes null at end
void getProgStr(const __FlashStringHelper *ifsh, char*str,
                int maxLen) {
  const char *p = (const char *)ifsh;
  int i = 0;
  while (i < maxLen - 1) {
    unsigned char c = pgm_read_byte(p++);
    str[i++] = c;
    if (c == 0) {
      break;
    }
  }
  str[i] = '\0'; // terminate
}


