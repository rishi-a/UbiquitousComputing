
#ifndef pfodRadioMsg_h
#define pfodRadioMsg_h
/**
  pfodRadioMsg for Arduino
  Holds a radio msg
*/
/*
   (c)2014-2018 Forward Computing and Control Pty. Ltd.
   NSW Australia, www.forward.com.au
   This code is not warranted to be fit for any purpose. You may only use it at your own risk.
   This code may be freely used for both private and commercial use
   Provide this copyright is maintained.
*/

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "pfodStream.h"


class pfodRadioMsg {
  public:
    pfodRadioMsg();
    void init(size_t _maxMsgLen); // actual datasize i.e. excludes header
    size_t saveMsg(uint8_t *_buf, uint8_t _len, uint8_t _addressedTo, uint8_t _receievedFrom, uint8_t _msgSeqNo, uint8_t _ackedMsgSeqNo);
    size_t saveMsg(Stream* _txBufPtr, uint8_t _addressedTo, uint8_t _receievedFrom, uint8_t _msgSeqNo, uint8_t _ackedMsgSeqNo);
    uint8_t getLen();
    uint8_t* getBuf();
    uint8_t getAddressedTo();
    uint8_t getReceivedFrom();
    uint8_t getMsgSeqNo();
    uint8_t getAckedMsgSeqNo();
    void setAckedMsgSeqNo(uint8_t _ackedMsgSeqNo); 
    size_t getMaxMsgBufferSize();
    bool isPureAck();
    bool isAckFor(pfodRadioMsg* lastRadioMsg); // i.e. ackedMsgSeqNo == msgSeqNoAwaitinAck
    bool isResendRequest(pfodRadioMsg* lastRadioMsg); // i.e. is msgSeqNo < _expectedMsgSeqNo i.e. ((_expectedMsgSeqNo - msgSeqNo) > 128)
    bool isNewMsg(uint8_t _thisAddress, uint8_t _targetAddress, uint8_t _expectedMsgSeqNo, bool isServer); // i.e. (msgSeqNo = _expectedMsgSeqNo) && (!( (len =1) && (buf[0] == '\0') && (msgSeqNo == 0)))  (note pure acks have msgSeqNo == 0)
    // OR (msgSeqNo == 0) && (! ((len =1) && buf[0] == '\0')))  => new connect request    
    bool isNewConnectionRequest(uint8_t ourAddress); //
  protected:

    static const size_t BUFFER_SIZE = 255;
    size_t maxMsgLen; // set by driver and limited to BUFFER_SIZE above
    size_t msgLen; // includes null if any
    uint8_t msgBuf[BUFFER_SIZE];
    uint8_t addressedTo;
    uint8_t receivedFrom;
    uint8_t ackedMsgSeqNo;
    uint8_t msgSeqNo;

    bool isPureAckFor(uint8_t msgSeqNoWaitingForAck);
};

#endif // pfodRadioMsg_h
