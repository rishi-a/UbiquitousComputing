// NonBlockingInput.cpp
/*
   (c)2019 Forward Computing and Control Pty. Ltd.
   NSW Australia, www.forward.com.au
   This code is not warranted to be fit for any purpose. You may only use it at your own risk.
   This generated code may be freely used for both private and commercial use
   provided this copyright is maintained.
*/

#include "pfodNonBlockingInput.h"

pfodNonBlockingInput::pfodNonBlockingInput() {
  nonBlockingInput_clearFlag = false;
}

/**
   Set the input to read from non-blocking
*/
pfodNonBlockingInput* pfodNonBlockingInput::connect(Stream* _io) {
  input = _io;
  return this;
}

/**
   Will clear any more input until nothing received for
   NON_BLOCKING_INPUT_CLEAR_TIMEOUT_MS, i.e. 10mS
*/
void pfodNonBlockingInput::clearInput() {
  if (input == NULL) {
    return;
  }
  nonBlockingInput_clearFlag = true;
  while (input->available()) {
    input->read(); // discard
  }
  nonBlockingInputClearTimeOut.start(NON_BLOCKING_INPUT_CLEAR_TIMEOUT_MS);
}

size_t pfodNonBlockingInput::write(const uint8_t *buf, size_t size) {
  if (!input) {
    return 0;
  }
  checkClearInput();
  return input->write(buf, size);
}

size_t pfodNonBlockingInput::write(uint8_t c) {
  if (!input) {
    return 0;
  }
  checkClearInput();
  return input->write(c);
}

int pfodNonBlockingInput::available() {
  if (!input) {
    return 0;
  }
  checkClearInput();
  return input->available();
}

//  availableForWrite() not available in ESP32 Stream (and others?);
/**
int pfodNonBlockingInput::availableForWrite() {
  if (!input) {
    return 0;
  }
  checkClearInput();
  return input->availableForWrite();
}
**/

int pfodNonBlockingInput::read() {
  if (!input) {
    return -1;
  }
  checkClearInput();
  return input->read();
}

int pfodNonBlockingInput::peek() {
  if (!input) {
    return -1;
  }
  checkClearInput();
  return input->peek();
}

void pfodNonBlockingInput::flush() {
  if (!input) {
    return;
  }
  checkClearInput();
  input->flush();
}

void pfodNonBlockingInput::checkClearInput() {
  if (nonBlockingInput_clearFlag) {
    bool foundInput = false;
    while (input->available()) {
      input->read(); // discard
      foundInput = true;
    }
    if (foundInput) {
      nonBlockingInputClearTimeOut.start(NON_BLOCKING_INPUT_CLEAR_TIMEOUT_MS);
    }
  }

  if (nonBlockingInputClearTimeOut.justFinished()) {
    // first call after timeout no chars received in the last 10mS
    nonBlockingInput_clearFlag = false;     // input is clear
    // next char will be returned
  }
}

// readFirstLine does not timeout or block
// returns -1 until the null terminated buffer contains length-1 chars or until read \r or \n. The \r or \n are not added to the buffer
// then returns number of chars read.
// an empty input line i.e. just \r or \n returns 0.
// if echo true then each char added to the buffer is echoed back to this stream
// buffer is null terminated after each call so you can check progress and apply an external timeout.
// clearInput is called when a non-zero is returned to clears any following input, e.g. \n of \r\n pair or any chars after buffer filled
int pfodNonBlockingInput::readInputLine( char *buffer, size_t length, bool echo) {
  checkClearInput();
  if (!available()) {
    return -1;
  }
  size_t len = strlen(buffer);
  if (len >= (length - 1)) { // already full
    clearInput();
    return len;
  }
  while (available()) {
    char c = read();
    if ((c == '\n') || (c == '\r') ) {
      if (echo) {
        println(); // terminate output line using our idea of end of line
      }
      clearInput();
      return len;
    } else {
      // have some input
      if (echo) { //echo it
        write(c);
      }
      buffer[len++] = c;
      buffer[len] = '\0'; // keep buffer terminated
      if (len >= (length - 1)) {
        clearInput();
        return len;
      }
    }
  }
  return -1;
}

