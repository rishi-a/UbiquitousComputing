#ifndef pfodRingBuffer_h
#define pfodRingBuffer_h
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
/*
   (c)2014-2017 Forward Computing and Control Pty. Ltd.
   NSW Australia, www.forward.com.au
   This code is not warranted to be fit for any purpose. You may only use it at your own risk.
   This code may be freely used for both private and commercial use
   Provide this copyright is maintained.
*/

#include <Arduino.h>
#include "pfodStream.h"

class pfodRingBuffer: public Stream {
  public:
    pfodRingBuffer(); // if using this constructor need to call init(..) also
    /**
       _buf must be at least _size in length
       _size is limited to 32766
    */
    void init(uint8_t* _buf, size_t _size);
    pfodRingBuffer(uint8_t* _buf, size_t _size);
    
    void clear();
    void markWrite();
    void resetWrite();
    /**
     copyTo()
     copies contents to the print stream 
     unlike read() copyTo does not remove any data from the buffer or move the current read position
    */
    void copyTo(Print *outputPtr);
    
    // from Stream
    int available();
    int peek();
    int read();
    void flush();
    size_t write(uint8_t b); // does not block, drops bytes if buffer full
    size_t write(const uint8_t *buffer, size_t size); // does not block, drops bytes if buffer full
    int availableForWrite();
    size_t getSize(); // size of ring buffer

  private:
    uint8_t* buf;
    uint16_t bufSize;
    uint16_t buffer_head;
    uint16_t buffer_tail;
    uint16_t buffer_count;
    uint16_t wrapBufferIdx(uint16_t idx);
    uint16_t writeMark;
    uint16_t writeMarkCount;
    bool isWriteMarked;

};
#endif
