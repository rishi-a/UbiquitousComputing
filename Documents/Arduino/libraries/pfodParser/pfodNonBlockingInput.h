// NonBlockingInput.h
/*
   (c)2019 Forward Computing and Control Pty. Ltd.
   NSW Australia, www.forward.com.au
   This code is not warranted to be fit for any purpose. You may only use it at your own risk.
   This generated code may be freely used for both private and commercial use
   provided this copyright is maintained.
*/
#ifndef pfodNonBlockingInput_h
#define pfodNonBlockingInput_h

#include <pfodDelay.h>
#include <pfodStream.h>

class pfodNonBlockingInput : public Stream {
  public:
    /**
       constuctor
    */
    pfodNonBlockingInput();

    /**
       connect to Stream* to read from
    */
    pfodNonBlockingInput* connect(Stream* _io);

    /**
       Will clear any more input until nothing received for
       NON_BLOCKING_INPUT_CLEAR_TIMEOUT_MS, i.e. 10mS
    */
    void clearInput();

    // readFirstLine does not timeout or block
    // returns -1 until the null terminated buffer contains length-1 chars or until read \r or \n. The \r or \n are not added to the buffer
    // then returns number of chars read.
    // an empty input line i.e. just \r or \n returns 0.
    // if echo true then each char added to the buffer is echoed back to this stream
    // buffer is null terminated after each call so you can check progress and apply an external timeout.
    // clearInput is called when a non-zero is returned to clears any following input, e.g. \n of \r\n pair or any chars after buffer filled
    int readInputLine( char *buffer, size_t length, bool echo = false); // read until full or get \r or \n
    int readInputLine( uint8_t *buffer, size_t length, bool echo = false) {
      return readInputLine((char *)buffer, length, echo);  // read until full or get \r or \n
    }

   // --------------------------------------------------------------
    virtual size_t write(uint8_t);
    virtual size_t write(const uint8_t *buf, size_t size);
    virtual int available();
    virtual int read();
    virtual int peek();
    virtual void flush(); // this blocks until write buffer empty
    // availableForWrite() not available in ESP32 Stream (and others?);

  private:
    void checkClearInput();
    pfodDelay nonBlockingInputClearTimeOut;
    const uint32_t NON_BLOCKING_INPUT_CLEAR_TIMEOUT_MS = 200; // 0.2sec
    bool nonBlockingInput_clearFlag;
    Stream* input;
};
#endif