#include <Arduino.h>
#include "pfodBLEBufferedSerial.h"
/**
  (c)2015 Forward Computing and Control Pty. Ltd.
  This code may be freely used for both private and commerical use.
  Provide this copyright is maintained.
*/

// uncomment this next line and call setDebugStream(&Serial); to enable debug out
//#define DEBUG

void pfodBLEBufferedSerial::setDebugStream(Print* out) {
  debugOut = out;
}

pfodBLEBufferedSerial::pfodBLEBufferedSerial() {
  stream = NULL;
  debugOut = NULL;
  size_t _bufferSize = PFOD_DEFAULT_SEND_BUFFER_SIZE;
  setBuffer(_bufferSize);
  sendDelay_uS = DEFAULT_BLE_SEND_DELAY_TIME*1000;
  connectCalled = false;
}

pfodBLEBufferedSerial::pfodBLEBufferedSerial(size_t _bufferSize) {
  stream = NULL;
  debugOut = NULL;
  setBuffer(_bufferSize);
}

void pfodBLEBufferedSerial::setBuffer(size_t _bufferSize) {
  sendBufferIdxHead = 0;
  sendBufferIdxTail = 0;
  sendTimerStart = 0;

  if ((sendBuffer == NULL) && (_bufferSize > defaultBufferSize)) {
    bufferSize = _bufferSize;
    sendBuffer = (uint8_t*)malloc(bufferSize);
  }
  if (sendBuffer == NULL) {
    bufferSize = defaultBufferSize;
    sendBuffer = defaultBuffer;
  }
}

pfodBLEBufferedSerial* pfodBLEBufferedSerial::connect(Stream* _stream) {
#ifdef DEBUG
  if (debugOut) {
    debugOut->println("pfodBLEBufferedSerial connected.");
    debugOut->print("buffersize:");    debugOut->println(bufferSize);
  }
#endif // DEBUG		
  stream = _stream;
  clearTxBuffer();
  connectCalled = true;
  return this;
}

void pfodBLEBufferedSerial::clearTxBuffer() {
#ifdef DEBUG
  if (debugOut) {
    debugOut->println("pfodBLEBufferedSerial clearTxBuffer.");
  }
#endif // DEBUG	
  sendBufferIdxHead = 0;
  sendBufferIdxTail = 0;
  timerRunning = false;
}	

size_t pfodBLEBufferedSerial::write(const uint8_t *buf, size_t size) {
  if (!stream) {
    return 0;
  }
  for (size_t i = 0; i < size; i++) {
    _write(buf[i]); // may block if buffer is full
  }
  return size;
}

size_t pfodBLEBufferedSerial::write(uint8_t c) {
  if (!stream) {
    return 0;
  }
  return _write(c);
}

size_t pfodBLEBufferedSerial::_write(uint8_t c) {
  if (!stream) {
    return 0;
  }

  sendAfterDelay(); // try sending first to free some buffer space

  size_t rtn = 1;
  if (!timerRunning) {
    timerRunning = true;
    sendTimerStart = micros();
  }

  size_t i = (sendBufferIdxHead + 1) % bufferSize;
  if (i == sendBufferIdxTail) {
    // If the output buffer is full, just drop the char
#ifdef DEBUG
    if (debugOut != NULL) {
      debugOut->print("buffer full wait.. "); debugOut->print((char)c);
    }
#endif // DEBUG    
  }
  
  while (i == sendBufferIdxTail) {
  	  sendAfterDelay(); // block here until we have space
  }
  sendBuffer[sendBufferIdxHead] = c;
  sendBufferIdxHead = i;
#ifdef DEBUG
  if (debugOut != NULL) {
    debugOut->print((char)c);
  }
#endif // DEBUG    
  sendAfterDelay(); // try sending last if this was first write

  return rtn;
}

size_t pfodBLEBufferedSerial::bytesToBeSent() {
  if (sendBufferIdxTail <= sendBufferIdxHead) {
    return (sendBufferIdxHead - sendBufferIdxTail);
  } // else
  return (sendBufferIdxHead + bufferSize - sendBufferIdxTail);
}

/** NOTE: count is number of 1.25mS to delay between send blocks of BLE data
   this number should be > maxConnection interval number so previous block is picked up prior to sending next one.
   must be call BEFORE connect() is called, otherwise ignored
*/   
void pfodBLEBufferedSerial::setBLEBlockSendDelay(uint16_t count) {
	if (connectCalled) {
#ifdef DEBUG
      if (debugOut != NULL) {
        debugOut->print("setBLEBlockSendDelay call ignored, since connect has been called.");
      }
#endif // DEBUG    
		return; // ignore this
	}
	// else
	sendDelay_uS = count + (count>>2); // * 1.25mS
	if (count & 0x03) {
		// have remainder add one
		sendDelay_uS += 1;
	}
     sendDelay_uS = sendDelay_uS*1000;
     if (sendDelay_uS == 0) {
     	 sendDelay_uS = 500; // 0.5mS
     }
#ifdef DEBUG
      if (debugOut != NULL) {
        debugOut->print("setBLEBlockSendDelay set sendDelay to ");
        debugOut->print(sendDelay_uS);
        debugOut->println(" uS");
      }
#endif // DEBUG  
}	

void pfodBLEBufferedSerial::sendAfterDelay() {
  if ((!stream) || (sendBufferIdxHead == sendBufferIdxTail)) {  // common cases
    return; // nothing to do
  }
  if (timerRunning && ((micros() - sendTimerStart) > sendDelay_uS) ) {
    sendTimerStart = micros(); // update for next send
#ifdef DEBUG
    if (debugOut != NULL) {
      debugOut->println();
      debugOut->print("sendAfterDelay() "); debugOut->print(bytesToBeSent()); debugOut->println(" bytes waiting to be sent");
      debugOut->println(millis());
    }
#endif // DEBUG    
    // send next 20 bytes
    size_t i = 0; // max to send is
    while ((i < BLE_SEND_BLOCK_SIZE) && (sendBufferIdxHead != sendBufferIdxTail)) {
      sendBlock[i] = (const uint8_t)(sendBuffer[sendBufferIdxTail]);
#ifdef DEBUG
      if (debugOut != NULL) {
        debugOut->print((char)sendBuffer[sendBufferIdxTail]);
      }
#endif // DEBUG    
      i++;
      sendBufferIdxTail = (sendBufferIdxTail + 1) % bufferSize;
    }
    stream->write((const uint8_t *)sendBlock,i); // write as block ESP32 does not like writing one byte at a time to Bluetooth Serial
    if (sendBufferIdxHead == sendBufferIdxTail) {
      // empty
#ifdef DEBUG
    if (debugOut != NULL) {
      debugOut->println("empty");
    }
#endif // DEBUG    
      timerRunning = false;
    }
  }
}


// expect available to ALWAYS called before read() so update timer here
int pfodBLEBufferedSerial::available() {
  sendAfterDelay();
  if (!stream) {
    return 0;
  }
  return stream->available();
}

int pfodBLEBufferedSerial::read() {
  sendAfterDelay();
  if (!stream) {
    return -1;
  }
  int c = stream->read();
  return c;
}

int pfodBLEBufferedSerial::peek() {
  sendAfterDelay();
  if (!stream) {
    return -1;
  }
  return stream->peek();
}

void pfodBLEBufferedSerial::flush() {
  sendAfterDelay();
}
