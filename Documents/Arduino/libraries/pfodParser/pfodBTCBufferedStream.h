#ifndef pfodBTCBufferedStream_h
#define pfodBTCBufferedStream_h
/**
 (c)2015 Forward Computing and Control Pty. Ltd.
 This code may be freely used for both private and commerical use.
 Provide this copyright is maintained.
*/

#include "pfodStream.h"

class pfodBTCBufferedStream : public Stream {

  public:
    pfodBTCBufferedStream(); // default 128 byte buffer and default send delay 10mS 
//    pfodBTCBufferedStream(size_t _bufferSize); // set buffer size (min size is 32) and use default send delay 200mS
    pfodBTCBufferedStream* connect(Stream* _stream);
    virtual size_t write(uint8_t);
    virtual size_t write(const uint8_t *buf, size_t size);
    virtual int available();
    virtual int read();
    virtual int peek();
    virtual void flush();   
    void setDebugStream(Print* out);
    size_t bytesToBeSent(); // bytes in buffer to be sent
    void clearTxBuffer();
  private:
    Stream* stream;
    void sendAfterDelay();
    unsigned long sendDelayTime;
    void forceSend();
    size_t _write(uint8_t c);
    static const unsigned long DEFAULT_SEND_DELAY_TIME = 10000; //in uS, 10mS delay before sending buffer
    static const size_t PFOD_DEFAULT_BTC_SEND_BUFFER_SIZE = 128; 
    uint8_t sendBuffer[PFOD_DEFAULT_BTC_SEND_BUFFER_SIZE]; //
    size_t sendBufferIdx = 0;
    size_t bufferSize;
    Print* debugOut;
    bool connectCalled;
    unsigned long sendTimerStart;
    bool timerRunning = false;
  //  void setBuffer(size_t _bufferSize);
};

#endif // pfodBTCBufferedStream_h