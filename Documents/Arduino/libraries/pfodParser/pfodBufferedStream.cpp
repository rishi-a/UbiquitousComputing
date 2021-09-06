#include <Arduino.h>
#include "pfodBufferedStream.h"

/**
  (c)2018 Forward Computing and Control Pty. Ltd.
  This code may be freely used for both private and commerical use.
  Provide this copyright is maintained.
*/

// uncomment this next line and call setDebugStream(&Serial); to enable debug out
//#define DEBUG

void pfodBufferedStream::setDebugStream(Print* out) {
  debugOut = out;
}

/**
   baudRate -- the maximum rate at which the bytes are to be released.  Bytes will be relased slower depending on how long your loop() method takes to execute
     You need to call one of available(), write(), read(), peek() or flush() each loop to actually send the data.
     Usually just add a call to available() at the top of your loop() method.
   bufferSize -- number of bytes to buffer,max bufferSize is limited to 32766
   buf -- the user allocated buffer to store the bytes, must be at least bufferSize long.
   blocking -- default true,  excess bytes block loop() until there is free space in the buffer.  false drops excess bytes
*/
pfodBufferedStream::pfodBufferedStream(const uint32_t _baudRate, uint8_t *_buf, size_t _bufferSize, bool _blocking) {
  stream = NULL;
  debugOut = NULL;
  blocking = _blocking; // default true is not passed in call.
  baudRate = _baudRate;
  uS_perByte = ((unsigned long)(13000000.0 / (float)baudRate)) + 1; // 1sec / (baud/13) in uS  baud is in bits
  // => ~13bits/byte, i.e. start+8+parity+2stop+1  may be less if no parity and only 1 stop bit
  ringBuffer.init(_buf, _bufferSize);
}

uint32_t pfodBufferedStream::getBaudRate() {
	return baudRate;
}

size_t pfodBufferedStream::getSize() {
  return ringBuffer.getSize();
}


void pfodBufferedStream::clearTxBuffer() {
  ringBuffer.clear();
}

int pfodBufferedStream::availableForWrite() {
  return ringBuffer.availableForWrite();
}

pfodBufferedStream* pfodBufferedStream::connect(Stream* _stream) {
#ifdef DEBUG
  if (debugOut) {
    debugOut->print("pfodBufferedStream connected with "); debugOut->print(ringBuffer.getSize()); debugOut->println(" byte buffer.");
    debugOut->print(" BaudRate:");  debugOut->print(getBaudRate()); 
    debugOut->print(" Send interval "); debugOut->print(uS_perByte); debugOut->print("uS");
    debugOut->println();
  }
#endif // DEBUG		
  stream = _stream;
  clearTxBuffer();
  sendTimerStart = micros();
  return this;
}

size_t pfodBufferedStream::write(const uint8_t *buffer, size_t size) {
  if (!stream) {
    return 0;
  }
  for (size_t i = 0; i < size; i++) {
    write(buffer[i]);
  }
  return size;
}

size_t pfodBufferedStream::write(uint8_t c) {
  if (!stream) {
    return 0;
  }
#ifdef DEBUG
  bool showDelay = true;
#endif // DEBUG    

  sendNextByte(); // try sending first to free some buffer space
  if (ringBuffer.availableForWrite() != 0){ 
    return ringBuffer.write(c);
  } else {
    if (blocking) {
      while (ringBuffer.availableForWrite() == 0) {
        // spin
#ifdef DEBUG
        if (showDelay) {
          showDelay = false; // only show this once per write
          if (debugOut != NULL) {
            debugOut->print("-1-"); // indicate delay
          }
        }
#endif // DEBUG    
        delay(1); // wait 1mS, should include yield() for those boards that need it e.g. ESP8266 and ESP32
        sendNextByte(); // try sending first to free some buffer space
      }
      return ringBuffer.write(c);
    } else { // else not blocking just drop this excess byte
      return 0; // byte was dropped
    }
  }
}


size_t pfodBufferedStream::bytesToBeSent() {
  return ringBuffer.available();
}

void pfodBufferedStream::sendNextByte() {
  if ((!stream) || (ringBuffer.available() == 0)) {
    return; // nothing connected or nothing to do
  }

  // micros() has 8uS resolution on 8Mhz systems, 4uS on 16Mhz system
  unsigned long uS = micros();
  if ((uS - sendTimerStart) > uS_perByte) {
#ifdef DEBUG
    if (debugOut != NULL) {
      debugOut->print("  !!At "); debugOut->print((uS - sendTimerStart)); debugOut->print("uS bytesToBeSent():"); debugOut->println(bytesToBeSent());
    }
#endif // DEBUG    
    sendTimerStart = uS;
  } else {
    return; // nothing to do
  }

#ifdef DEBUG
  //  if (debugOut != NULL) {
  //    debugOut->println();
  //    debugOut->print("At "); debugOut->print(uS); debugOut->print("uS ");
  //    debugOut->print(bytesToBeSent()); debugOut->println(" bytes waiting.");
  //  }
#endif // DEBUG    
  // send next byte
  stream->write((uint8_t)ringBuffer.read());
#ifdef DEBUG
  if (debugOut != NULL) {
    debugOut->println();
  }
#endif // DEBUG    

}


// expect available to ALWAYS called before read() so update timer here
int pfodBufferedStream::available() {
  sendNextByte();
  if (!stream) {
    return 0;
  }
  return stream->available();
}

int pfodBufferedStream::read() {
  sendNextByte();
  if (!stream) {
    return -1;
  }
  int c = stream->read();
  return c;
}

int pfodBufferedStream::peek() {
  sendNextByte();
  if (!stream) {
    return -1;
  }
  return stream->peek();
}

void pfodBufferedStream::flush() {
  while (ringBuffer.available() != 0) {
    sendNextByte();
  }
}
