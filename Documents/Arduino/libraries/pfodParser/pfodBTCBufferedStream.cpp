#include "pfodBTCBufferedStream.h"
/**
  (c)2015 Forward Computing and Control Pty. Ltd.
  This code may be freely used for both private and commerical use.
  Provide this copyright is maintained.
*/

// uncomment this next line and call setDebugStream(&Serial); to enable debug out
#define DEBUG

void pfodBTCBufferedStream::setDebugStream(Print* out) {
  debugOut = out;
}

pfodBTCBufferedStream::pfodBTCBufferedStream() {
  stream = NULL;
  debugOut = NULL;
  //  size_t _bufferSize = PFOD_DEFAULT_BTC_SEND_BUFFER_SIZE;
  // setBuffer(_bufferSize);
  bufferSize = PFOD_DEFAULT_BTC_SEND_BUFFER_SIZE;
  sendDelayTime = DEFAULT_SEND_DELAY_TIME;
  connectCalled = false;
}

/**
  pfodBTCBufferedStream::pfodBTCBufferedStream(size_t _bufferSize) {
  stream = NULL;
  debugOut = NULL;
  // setBuffer(_bufferSize);
  sendDelayTime = DEFAULT_SEND_DELAY_TIME;
  connectCalled = false;
  }
**/

/***
  void pfodBTCBufferedStream::setBuffer(size_t _bufferSize) {
  sendBufferIdx = 0;
      timerRunning = false;
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
***/

pfodBTCBufferedStream* pfodBTCBufferedStream::connect(Stream* _stream) {
#ifdef DEBUG
  if (debugOut) {
    debugOut->println("pfodBTCBufferedStream connected.");
    debugOut->print("buffersize:");    debugOut->println(bufferSize);
  }
#endif // DEBUG    
  stream = _stream;
  clearTxBuffer();
  connectCalled = true;
  return this;
}

void pfodBTCBufferedStream::clearTxBuffer() {
#ifdef DEBUG
  if (debugOut) {
    debugOut->println("pfodBTCBufferedStream clearTxBuffer.");
  }
#endif // DEBUG 
  sendBufferIdx = 0;
  timerRunning = false;
}


size_t pfodBTCBufferedStream::write(const uint8_t *buf, size_t size) {
  if (!stream) {
    return 0;
  }
  for (size_t i = 0; i < size; i++) {
    _write(buf[i]); // may block if buffer fills and client starts sending packet
  }
  return size;
}

size_t pfodBTCBufferedStream::write(uint8_t c) {
  if (!stream) {
    return 0;
  }
  return _write(c);
}

size_t pfodBTCBufferedStream::_write(uint8_t c) {
  if (!stream) {
    return 0;
  }

  sendAfterDelay(); // try sending first to free some buffer space

  size_t rtn = 1;
  if (!timerRunning) {
    timerRunning = true;
    sendTimerStart = micros();
  }

  sendBuffer[sendBufferIdx++] = c;
  if (sendBufferIdx == bufferSize) {
#ifdef DEBUG
    if (debugOut != NULL) {
      debugOut->print("buffer full write "); debugOut->print(sendBufferIdx); debugOut->println(" bytes to client");
      debugOut->print("mS:"); debugOut->println(millis());
    }
#endif // DEBUG    
    // buffer full
    // returns ((size_t)-1) if cannot write in 5 sec
    stream->write((const uint8_t *)sendBuffer, sendBufferIdx);
#ifdef DEBUG
    if (debugOut != NULL) {
      debugOut->println(); debugOut->write((const uint8_t *)sendBuffer, sendBufferIdx); debugOut->print(" mS:"); debugOut->print(millis()); debugOut->println(" after write");
    }
#endif // DEBUG    
    sendBufferIdx = 0;
    timerRunning = false;
  }
  delay(0); // yield
  return rtn;
}

void pfodBTCBufferedStream::sendAfterDelay() {
  if ((!stream) || (!sendBufferIdx)) {  // common cases
    return;
  }
  if (((micros() - sendTimerStart) > sendDelayTime)) {
#ifdef DEBUG
    if (debugOut != NULL) {
      debugOut->print("sendAfterDelay() "); debugOut->print(sendBufferIdx); debugOut->println(" bytes to client");
      debugOut->print("mS:"); debugOut->println(millis());
    }
#endif // DEBUG    
    stream->write((const uint8_t *)sendBuffer, sendBufferIdx);
#ifdef DEBUG
    if (debugOut != NULL) {
      debugOut->println(); debugOut->write((const uint8_t *)sendBuffer, sendBufferIdx); debugOut->print(" mS:"); debugOut->print(millis()); debugOut->println(" after sendAfterDelay() write");
    }
#endif // DEBUG    
    sendBufferIdx = 0;
    timerRunning = false;
  }
  delay(0); // yield
}

// force send the buffer block if necessary
void pfodBTCBufferedStream::forceSend() {
  if ((!stream) || (!sendBufferIdx)) {  // common cases
    return;
  }
  stream->write((const uint8_t *)sendBuffer, sendBufferIdx);
  sendBufferIdx = 0;
  timerRunning = false;
  delay(0);
}

size_t pfodBTCBufferedStream::bytesToBeSent() {
  // bytes in buffer to be sent
  return sendBufferIdx;
}


// expect available to ALWAYS called before read() so update timer here
int pfodBTCBufferedStream::available() {
  sendAfterDelay();
  if (!stream) {
    return 0;
  }
  return stream->available();
}

int pfodBTCBufferedStream::read() {
  sendAfterDelay();
  if (!stream) {
    return -1;
  }
  int c = stream->read();
  delay(0);
  return c;
}

int pfodBTCBufferedStream::peek() {
  sendAfterDelay();
  if (!stream) {
    return -1;
  }
  return stream->peek();
}

/**
  Forces send of any buffered data now. May block if last packet no ACKed yet
*/
void pfodBTCBufferedStream::flush() {
  forceSend();
}
