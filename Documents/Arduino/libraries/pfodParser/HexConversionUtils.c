/**
HexConversionUtils for Arduino
*/
/*
 * (c)2014-2017 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provide this copyright is maintained.
 */

/**
* isAllHex
* str is null terminated chars (bytes)
* result 1 if all chars are hex, else 0
*/
int isAllHex(const char *str) {
	while ((*str != 0)) {
    char c = *str++;
    if ((c >= 'a') && (c <= 'f')) {
    	continue;
    }
    if ((c >= 'A') && (c <= 'Z')) {
    	continue;
    }
    if ((c >= '0') && (c <= '9')) {
    	continue;
    }
    // else
    return 0; // found non-hex char
  }  
  return 1; // true all hex
}	

/**
* str is null terminated hex chars (converted as pairs)
* result is where the converted bytes are stored
* maxHexLen is max number of bytes that can be stored
* Note: if odd number of chars supplied extra '0' assumed in result
*
* returns number of bytes actually stored
*/
unsigned int asciiToHex(const char *str, unsigned char *result, unsigned int maxHexLen) {
  unsigned int i = 0;
  unsigned char  firstHexDigit = ~(0); // true Oxff
  while ((*str != 0)) {
    unsigned char c = *str++;
    if (firstHexDigit) {
      *result = 0;  // clear it first
    }
    if (c >= 'a') {
      *result += c - 'a' + 10;
    } else if (c >= 'A') {
      *result += c - 'A' + 10;
    } else {
      *result += c - '0';
    }
    if (firstHexDigit) {
      *result = *result << 4;
      i++; // stored something here
    } else {
      result++;
      if (i >= maxHexLen) {
        break;
      }
    }
    firstHexDigit = ~firstHexDigit;
  }
  return i;
}

/**
* hexLen is the number of bytes to convert to hex,
* maxStrLen is space available in string (including terminating null)
*
* returns number of char saved (not counting the terminating null)
*/
unsigned int hexToAscii(const unsigned char *hex, unsigned int hexLen, char *str, unsigned int maxStrLen) {
  unsigned int j = 0; // count string len
  char *strPtr = str;
  unsigned int i = 0;
  unsigned int maxLen = maxStrLen - 1; // allow for terminating null
  for (i = 0; i < hexLen; i++) {
    if (j >= maxLen) {
      break;
    }
    unsigned char b = (unsigned char)hex[i] >> 4;
    *strPtr++ = (char)((b >= 10) ? (b - 10 + 'A') : b + '0');
    j++;
    if (j >= maxLen) {
      break;
    }
    b = (unsigned char)hex[i] & 15;
    *strPtr++ = (char)((b >= 10) ? (b - 10 + 'A') : b + '0');
    j++;
  }
  *strPtr = 0; // store terminating null
  return j; // return number of char saved not counting terminating null
}



