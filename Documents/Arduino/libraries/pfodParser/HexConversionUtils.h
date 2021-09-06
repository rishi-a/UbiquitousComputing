#ifndef hexConversionUtils_h
#define hexConversionUtils_h
/**
hexConversionUtils for Arduino
*/
/*
 * (c)2014-2017 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provide this copyright is maintained.
 */

extern "C" {
	
  /**
  * isAllHex
  * str is null terminated chars (bytes)
  * result 1 if all chars are hex, else 0
  */
  int isAllHex(const char *str);

  /**
  * Converts pairs of hex digits (upper or lower case) to bytes
  * i.e. "0f"  -> (byte)15, as does "0F"
  *      "ff"  -> (byte)255, as does "FF"
  *
  * str is null terminated hex chars (converted as pairs)
  * result is where the converted bytes are stored
  * maxHexLen is max number of bytes that can be stored
  *
  * returns number of bytes actually stored
  */
  unsigned int asciiToHex(const char *str, unsigned char *result, unsigned int maxHexLen);

  /**
  * Converts bytes to hex digits and adds terminating null
  *  i.e. (byte)15 -> '0' 'F' '\0'
  *
  * hex pointer to bytes to be converted to hex digits
  * hexLen is the number of bytes to convert to hex,
  * str pointer to where the result is to be stored
  * maxStrLen is space available in string (including terminating null)
  */
  int hexToAscii(const unsigned char *hex, unsigned int hexLen, char *str, unsigned int maxStrLen);
}


#endif // hexConversionUtilsr_h

