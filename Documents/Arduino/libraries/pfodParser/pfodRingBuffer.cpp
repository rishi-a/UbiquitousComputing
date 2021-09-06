/*
   (c)2014-2018 Forward Computing and Control Pty. Ltd.
   NSW Australia, www.forward.com.au
   This code is not warranted to be fit for any purpose. You may only use it at your own risk.
   This code may be freely used for both private and commercial use
   Provide this copyright is maintained.
*/

/**
  pfodRingBuffer for Arduino
  Implements a ring buffer implementation of an Arduino Stream
  upto 32K buffers
  NOTE: this implementation DOES NOT BLOCK
  if the buffer is full extra bytes written are quietly dropped.
  markWrite() marks current write position
  resetWrite() resets buffer to previous markWrite effectively deleting any bytes written between markWrite() and resetWrite()
  any call to read() or flush() or clear() or resetWrite() clears the markWrite
  multiple calls to markWrite() only saves the last one.
*/

#include <limits.h>
#include "pfodRingBuffer.h"


pfodRingBuffer::pfodRingBuffer() {
  buf = NULL;
  bufSize = 0;
  clear();
}

pfodRingBuffer::pfodRingBuffer(uint8_t* _buf, size_t _size) {
  init(_buf,_size);
}

/**
   _buf must be at least _size in length
   _size is limited to 32766
   assumes size_t is atleast 16bits as specified by C spec
*/
void pfodRingBuffer::init(uint8_t* _buf, size_t _size) {
  clear();
  if ((_buf == NULL) || (_size == 0)) {
    buf = _buf;
    bufSize = 0;
  } else {
    // available etc returns int, check that _size fits in int
    // limit _size to max int16_t - 1
    if (_size >= 32766) {
      _size = 32766; // (2^16/2)-1 minus 1 since uint16_t vars used
    }
    buf = _buf;
    bufSize = _size; // buffer_count use to detect buffer full
  }
}

void pfodRingBuffer::clear() {
  buffer_head = 0;
  buffer_tail = buffer_head;
  buffer_count = 0;
  isWriteMarked = false;
}

void pfodRingBuffer::markWrite() {
  writeMark = buffer_head;
  writeMarkCount = buffer_count;
  isWriteMarked = true;
}

void pfodRingBuffer::resetWrite() {
  if (isWriteMarked) {
    buffer_head = writeMark;
    buffer_count = writeMarkCount;
    isWriteMarked = false;
  } // else do nothing
}

/*
   This should return size_t,
   but someone stuffed it up in the Arduino libraries
*/
int pfodRingBuffer::available() {
  return buffer_count;
}

size_t pfodRingBuffer::getSize() {
  return bufSize;
}

/*
   This should return size_t,
   but someone stuffed it up in the Arduino HardwareSerial.h
*/
int pfodRingBuffer::availableForWrite() {
  if (bufSize == 0) {
    return 0;
  }
  return  (bufSize - buffer_count);
}


int pfodRingBuffer::peek() {
  if (buffer_count == 0) {
    return -1;
  } else {
    return buf[buffer_tail];
  }
}

int pfodRingBuffer::read() {
  isWriteMarked = false; // clear mark on any call to read() or flush()
  if (buffer_count == 0) {
    return -1;
  } else {
    unsigned char c = buf[buffer_tail];
    buffer_tail = wrapBufferIdx(buffer_tail);
    // if (buffer_count > 0) { checked above
    buffer_count--;
    return c;
  }
}

/**
  copy()
  copies contents to the print stream 
  unlike read() copy does not remove any data or move the current read position
*/
void pfodRingBuffer::copyTo(Print *outputPtr) {
  uint16_t byteCount = buffer_count;	
  if (buffer_count == 0) {
    return; // empty
  }
  if (outputPtr == NULL) {
  	  return;
  }
  // else {
  uint16_t tail = buffer_tail;
  for (byteCount = buffer_count; byteCount > 0; byteCount--) {
    outputPtr->write(buf[tail]);	
    tail = wrapBufferIdx(tail);
  }
} 

/**
   clears mark but does nothing else
*/
void pfodRingBuffer::flush() {
  isWriteMarked = false; // clear mark on any call to read() or flush()
  // nothing here
}

size_t pfodRingBuffer::write(const uint8_t *_buffer, size_t _size) {
  if (_size > ((size_t)availableForWrite())) {
    _size = availableForWrite();
  }
  size_t written = 0;
  for (size_t i = 0; i < _size; i++) {
    written += write(_buffer[i]);
  }
  return written;
}

size_t pfodRingBuffer::write(uint8_t b) {
  if (bufSize == 0) {
    return 0;
  } // else
  // check for buffer full
  if (buffer_count >= bufSize) {
    buffer_count = bufSize; // clean it up
    return 0;
  }
  // else
  buf[buffer_head] = b;
  buffer_head = wrapBufferIdx(buffer_head);
  // if (buffer_count < bufSize) { check above
  buffer_count++;
  return 1;
}

uint16_t pfodRingBuffer::wrapBufferIdx(uint16_t idx) {
  if (idx >= (bufSize - 1)) {
    // wrap around
    idx = 0;
  } else {
    idx++;
  }
  return idx;
}


