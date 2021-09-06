#ifndef pfodBufferedSerial_h
#define pfodBufferedSerial_h
/**
  (c)2018 Forward Computing and Control Pty. Ltd.
  This code may be freely used for both private and commerical use.
  Provide this copyright is maintained.
*/

#include "pfodStream.h"
#include "pfodRingBuffer.h"

class pfodBufferedStream : public Stream {

  public:
    /**
       baudRate -- the maximum rate at which the bytes are to be released.  Bytes will be relased slower depending on how long your loop() method takes to execute
       bufferSize -- number of bytes to buffer,max bufferSize is limited to 32766
       buf -- the user allocated buffer to store the bytes, must be at least bufferSize long.
       blocking -- default true,  excess bytes block loop() until there is free space in the buffer.  false drops excess bytes
    */
    pfodBufferedStream(const uint32_t baudRate, uint8_t *_buf, size_t _bufferSize, bool blocking = true);
    pfodBufferedStream* connect(Stream* _stream); // where to read and write to
    virtual size_t write(uint8_t);
    virtual size_t write(const uint8_t *buf, size_t size);
    virtual int available();
    virtual int read();
    virtual int peek();
    virtual void flush(); // this blocks until write buffer empty
    virtual int availableForWrite();
    void setDebugStream(Print* out);
    size_t bytesToBeSent(); // bytes in buffer to be sent
    size_t getSize(); // check on allocation
    void clearTxBuffer(); // clears outgoing (write) buffer
    uint32_t getBaudRate();
  private:
    unsigned long uS_perByte; // == 1000000 / (baudRate/10) == 10000000 / baudRate
    Stream* stream;
    pfodRingBuffer ringBuffer;
    uint32_t baudRate;
    void sendNextByte();
    unsigned long sendTimerStart;
    bool blocking; // defaults to true;
    Print* debugOut;
};

#endif // pfodBufferedSerial_h
