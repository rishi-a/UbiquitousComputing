/*
   (c)2014-2018 Forward Computing and Control Pty. Ltd.
   NSW Australia, www.forward.com.au
   This code is not warranted to be fit for any purpose. You may only use it at your own risk.
   This code may be freely used for both private and commercial use
   Provide this copyright is maintained.
*/

#include "pfodRadioMsg.h"

pfodRadioMsg::pfodRadioMsg() {
  maxMsgLen = BUFFER_SIZE - 1;
  msgLen = 0; msgBuf[0] = '\0';
  addressedTo = 0;
  receivedFrom = 0;
  ackedMsgSeqNo = 0;
  msgSeqNo = 0;
}

void pfodRadioMsg::init(size_t _maxMsgLen) {
  maxMsgLen = _maxMsgLen;
  if (maxMsgLen >= BUFFER_SIZE) {
    maxMsgLen = BUFFER_SIZE - 1; // allow extra byte incase null gets copied on the end some where
  }
}

size_t pfodRadioMsg::getMaxMsgBufferSize() {
  return maxMsgLen;
}

uint8_t pfodRadioMsg::getLen() {
  return msgLen;
}
uint8_t* pfodRadioMsg::getBuf() {
  return msgBuf;
}

// copy raw msg into this object
// returns actual count of bytes copied
size_t pfodRadioMsg::saveMsg(uint8_t *_buf, uint8_t _len, uint8_t _addressedTo, uint8_t _receievedFrom, uint8_t _msgSeqNo, uint8_t _ackedMsgSeqNo) {
  addressedTo = _addressedTo;
  receivedFrom = _receievedFrom;
  msgSeqNo = _msgSeqNo;
  ackedMsgSeqNo = _ackedMsgSeqNo;  // may be 0 for ack to connect
  msgLen = _len;
  if (msgLen > maxMsgLen) {
    msgLen = maxMsgLen;
  }
  if (_buf == NULL) {
    msgLen = 0;
    msgBuf[0] = '\0';
    return 0; // nothing copied
  }
  memmove(msgBuf, _buf, msgLen); // use memmove here to allow copy of buffer to itself due to way RadioHead drivers' are written
  return msgLen;
}

// copy raw msg into this object
// returns actual count of bytes copied
size_t pfodRadioMsg::saveMsg(Stream *_txBufPtr, uint8_t _addressedTo, uint8_t _receievedFrom, uint8_t _msgSeqNo, uint8_t _ackedMsgSeqNo) {
  addressedTo = _addressedTo;
  receivedFrom = _receievedFrom;
  msgSeqNo = _msgSeqNo;
  ackedMsgSeqNo = _ackedMsgSeqNo; // may be 0 for ack to connect
  if (_txBufPtr == NULL) {
    msgLen = 0;
    msgBuf[0] = '\0';
    return 0; // nothing copied
  }
  msgLen = _txBufPtr->available();
  if (msgLen > maxMsgLen) {
    msgLen = maxMsgLen;
  }
  for (size_t i = 0; i < msgLen; i++) {
    msgBuf[i] = _txBufPtr->read();
  }
  return msgLen;
}

void pfodRadioMsg::setAckedMsgSeqNo(uint8_t _ackedMsgSeqNo) {
  ackedMsgSeqNo = _ackedMsgSeqNo;
}

uint8_t pfodRadioMsg::getAddressedTo() {
  return addressedTo;
}

uint8_t pfodRadioMsg::getReceivedFrom() {
  return receivedFrom;
}

uint8_t pfodRadioMsg::getMsgSeqNo() {
  return msgSeqNo;
}

// the msgSeqNo being acked in this msg (either TX or RX msg)
uint8_t pfodRadioMsg::getAckedMsgSeqNo() {
  return ackedMsgSeqNo;
}


bool pfodRadioMsg::isPureAckFor(uint8_t msgSeqNoWaitingForAck) {
  if ((msgSeqNoWaitingForAck == getAckedMsgSeqNo())
      && (getMsgSeqNo() == 0)
      && (getLen() == 1) && (getBuf()[0] == '\0')) {
    return true; // pure ack
  }
  return false;
}

bool pfodRadioMsg::isPureAck() {
  if ((getMsgSeqNo() == 0)
      && (getLen() == 1) && (getBuf()[0] == '\0')) {
    return true; // pure ack, note only connect has msgSeqNo == 0 but connects msgs are never have leading null.
  }
  return false;
}

bool pfodRadioMsg::isAckFor(pfodRadioMsg* lastRadioMsg) {
  if ((receivedFrom != lastRadioMsg->getAddressedTo())
      || (addressedTo != lastRadioMsg->getReceivedFrom()) ) {
    return false; // not ours
  }
  // test pure ack first
  if (isPureAckFor(lastRadioMsg->getMsgSeqNo())) {
    return true;
  }
  // test for ack in new msg
  if ((lastRadioMsg->getMsgSeqNo() == getAckedMsgSeqNo())
      && (getMsgSeqNo() != 0)) {
    return true;
  }
  return false;
}

// not lastRadioMsg is either an ack or newMsg with ack for last received msg
// so in all cases can just send last msg if re-requesing
bool pfodRadioMsg::isResendRequest(pfodRadioMsg* lastRadioMsg) {
  // i.e. is msgSeqNo < _expectedMsgSeqNo i.e. ((_expectedMsgSeqNo - msgSeqNo) > 128)
  if ((receivedFrom != lastRadioMsg->getAddressedTo())
      || (addressedTo != lastRadioMsg->getReceivedFrom())) {
    return false; // not ours
  }

  if (getMsgSeqNo() == lastRadioMsg->getAckedMsgSeqNo()) {
    return true; // requesting previous ack
  }
  return false;
}

bool pfodRadioMsg::isNewMsg(uint8_t _thisAddress, uint8_t _targetAddress, uint8_t _expectedMsgSeqNo, bool isServer) {
  // i.e. (msgSeqNo = _expectedMsgSeqNo) && (!( (len =1) && (buf[0] == '\0') && (msgSeqNo == 0)))  (note pure acks have msgSeqNo == 0)
  // OR (msgSeqNo == 0) && (! ((len =1) && buf[0] == '\0')))  => new connect request
  if (_targetAddress == 0) {
    return false;  // no connection yet
  }
  if ((receivedFrom != _targetAddress)
      || (addressedTo != _thisAddress)) {
    return false; // not ours
  }
  if (isPureAck()) {
    return false;
  }
  if ((!isServer) && (getMsgSeqNo() == 0)) {
    return false; // connection request sent to client
  }
  if (getMsgSeqNo() == _expectedMsgSeqNo) {
    return true;
  } //else
  return false;
}

/*
 * New connection request if toAddress matches ours AND msgSeqNo == 0 and not empty msg
 */
bool pfodRadioMsg::isNewConnectionRequest(uint8_t ourAddress) {
  if ((addressedTo != ourAddress)) {
    return false; // not ours
  }
  if (getMsgSeqNo() != 0) {
    return false;
  }
  if ( (getLen() == 0) || (getBuf()[0] == '\0') ) {
    return false; // new connection cannot be an empty msg or start with a null
  }
  return true;
}
