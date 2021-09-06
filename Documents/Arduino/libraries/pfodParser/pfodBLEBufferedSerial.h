#ifndef pfodBLEBufferedSerial_h
#define pfodBLEBufferedSerial_h
/**
  (c)2015 Forward Computing and Control Pty. Ltd.
  This code may be freely used for both private and commerical use.
  Provide this copyright is maintained.
*/

#include "pfodStream.h"

// This class will block if buffer is full
// because there is a 0.2sec delay between sending each 20 byte block
// a full buffer can result in noticable delays in processing the loop() method

class pfodBLEBufferedSerial : public Stream {

  public:
  	  // pfodBLEBufferedSerial use malloc to allocate buffer, if malloc fails then default static 32byte buffer is used.
  	  // malloc uses dynamic memory not already used by variables, i.e. bytes free for local variables
  	  // but need to leave some bytes for local variables, say 300 or so, after malloc
  	  // for example if compiler shows "leaving 1135 bytes for local variables"  then use pfodBLEBufferedSerial(800) or less
  	  // pfod messages are ALWAYS less then 1024 bytes so the buffer size NEVER needs to be larger then 1024
  	  // the sample pfod messages at the top of the pfodDesigner generated code give you an idea of the length of the messages being sent.
  	  
    pfodBLEBufferedSerial(); // default 1024 byte buffer and default send delay 200mS
    pfodBLEBufferedSerial(size_t _bufferSize); // set buffer size (min size is 32) and use default send delay 200mS
    pfodBLEBufferedSerial* connect(Stream* _stream);
    virtual size_t write(uint8_t);  // this blocks if buffer full
    virtual size_t write(const uint8_t *buf, size_t size); // this blocks if buffer full
    virtual int available();
    virtual int read();
    virtual int peek();
    virtual void flush();
    void setDebugStream(Print* out);
    size_t bytesToBeSent(); // bytes in buffer to be sent
    static const size_t BLE_SEND_BLOCK_SIZE = 20; // BLE msg size
    void clearTxBuffer();
    void setBLEBlockSendDelay(uint16_t count); // NOTE: count is number of 1.25mS to delay between send blocks of BLE data
    // this number should be > maxConnection interval number so previous block is picked up prior to sending next one.
    // must be call BEFORE connect() is called, otherwise ignored
protected:
    Stream* stream;
    void sendAfterDelay();
    size_t _write(uint8_t c);
    size_t bufferSize;
    size_t sendBufferIdxHead;
    size_t sendBufferIdxTail;
    uint8_t sendBlock[BLE_SEND_BLOCK_SIZE];
    uint8_t* sendBuffer; // allow for terminating null
    Print* debugOut;
    unsigned long sendDelay_uS;

  private:
    static const unsigned long DEFAULT_BLE_SEND_DELAY_TIME = 200; // 200mS delay between 20byte msgs
    bool connectCalled;
    static const size_t PFOD_DEFAULT_SEND_BUFFER_SIZE = 1024; // Max data size pfodApp msg
    static const size_t defaultBufferSize = 32; // BLE msg size    
    uint8_t defaultBuffer[defaultBufferSize]; // if malloc fails
    unsigned long sendTimerStart;
    bool timerRunning = false;
    void setBuffer(size_t _bufferSize);
};

#endif // pfodBLEBufferedSerial_h
