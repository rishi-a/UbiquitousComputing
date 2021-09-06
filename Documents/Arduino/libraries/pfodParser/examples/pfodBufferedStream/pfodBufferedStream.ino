#include "pfodBufferedStream.h"

/*
 * (c)2018 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This generated code may be freely used for both private and commercial use
 * provided this copyright is maintained.
 */

//Example of using bufferedStream to release bytes at 9600 baud, buffer size 10
const size_t bufSize = 10;
uint8_t buf[bufSize];
pfodBufferedStream bufferedStream(9600,buf,bufSize);  

void setup() {
  Serial.begin(9600);
  for (int i = 10; i > 0; i--) {
    Serial.print(i); Serial.print(' ');
  }
  Serial.println();
  //bufferedStream.setDebugStream(&Serial); // need to uncomment #define DEBUG in pfodBufferedStream as well.
  bufferedStream.connect(&Serial);
}

void loop() {
  // calling any bufferedStream's Stream methods, read(), write(), peek(), available(), print...  
  // will output next byte at the baud rate specified
  while (bufferedStream.available()) {  
    bufferedStream.write(bufferedStream.read()); // just echo back out
  }
}
