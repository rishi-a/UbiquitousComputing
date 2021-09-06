/*
   (c)2014-2018 Forward Computing and Control Pty. Ltd.
   NSW Australia, www.forward.com.au
   This code is not warranted to be fit for any purpose. You may only use it at your own risk.
   This code may be freely used for both private and commercial use
   Provide this copyright is maintained.
*/
#include "pfodRadio.h"
#include "pfodRandom.h"


// to get debug out need to uncomment next line AND call setDebugStream
//#define DEBUG
//#define DEBUG_RAW_DATA
//#define DEBUG_TX_TIME
//#define DEBUG_RSSI

// send to debugOut if set and if #DEBUG defined
void pfodRadio::debugPfodRadioMsg(pfodRadioMsg* msg) {
#ifdef DEBUG
  if (debugOut != NULL) {
    debugOut->print("pfodRadioMsg ");
    debugOut->print(" To:");     debugOut->print(msg->getAddressedTo());
    debugOut->print(" From:");     debugOut->print(msg->getReceivedFrom());
    debugOut->print(" MsgSeqNo:");     debugOut->print(msg->getMsgSeqNo());
    debugOut->print(" AckedSeqNo:");     debugOut->print(msg->getAckedMsgSeqNo());
    debugOut->print(" Len:");     debugOut->print(msg->getLen());
    debugOut->println();
    for (uint8_t i=0; i<msg->getLen(); i++) {
      debugOut->print((char)((msg->getBuf())[i]));
    }
    debugOut->println();
    debugOut->println("---");
  }
#else
  (void)(msg);
#endif
}

/**
   set to (ackTimeout*1.5) * noOfRetries
*/
void pfodRadio::resetLinkConnectTimeout() {
  linkConnectionTimeout = (ackTimeout + (ackTimeout / 2)) * noOfRetries; // average random timeout == 1.5*ackTimeout
  linkConnectionTimeoutTimer = millis();
#ifdef DEBUG
  if (debugOut != NULL) {
    debugOut->print("resetLinkConnectTimeout called. reset linkConnectionTimeout:");
    debugOut->println(linkConnectionTimeout);
  }
#endif
}

// General process rule.  If incoming msg does not match what is expected just ignore it
// i.e. only closeConnection on timeout for ack to outgoing msg

void pfodRadio::pollRadio() {
  if ((millis() - debug_mS) > 1000) {
    debug_mS = millis();
#ifdef DEBUG
    if (debugOut != NULL) {
      if (linkConnectionTimeout > 0) {
        debugOut->print(" linkConnectionTimeout ");
        debugOut->print(millis() - linkConnectionTimeoutTimer);
        debugOut->print(F(" > "));
        debugOut->println(linkConnectionTimeout);
      }
    }
#endif
  }
  // see if the link has timed out
  if ((linkConnectionTimeout > 0) && ((millis() - linkConnectionTimeoutTimer) > linkConnectionTimeout)) {
#ifdef DEBUG
    if (debugOut != NULL) {
      debugOut->println(" linkConnection Timed out. Incoming Msg Seq No 0 will start new connection.");
    }
#endif
    linkConnectionTimeout = 0; //allow new connection with msgSecNo 0
  }
  if (thisAddress == 0xff) {
#ifdef DEBUG
    if (debugOut != NULL) {
      debugOut->println("Error: this address must be in the range 0 to 254 inclusive");
    }
#endif
    return;
  }

  if (!isServerNode) {
  } else {
    if (canAcceptNewConnections) { // skip this for clients
#ifdef DEBUG
      //if (debugOut != NULL) {
      //    debugOut->print(" canAcceptNewConnections, poll at ");debugOut->print(millis());
      //}
#endif
    }
  }
  if (recvMsg()) {
    lastRSSI = driver->lastRssi();
#if defined(DEBUG) || defined(DEBUG_RSSI)
    if (debugOut != NULL) {
      debugOut->print(" RSSI: ");
      debugOut->println(driver->lastRssi(), DEC);
    }
#endif
    if (isAckForLastMsgSent()) { // could be either pureAck or ack in newMsg
#ifdef DEBUG
      //if (debugOut != NULL) {
      //debugOut->println(" acked last msg");
      //}
#endif
    }
    if (!isPureAck()) { // acks handled above this filters
      // not ack for last msg sent
      // may still be ack for some old msg
      if (isServer()) {
        checkForAllowableNewConnection(); // only suceeds  if isServer AND new connect Request msg and not resend
      }
      // (i.e. expected incoming != 1)
      // a new connection above resets timeout and expected MsgSeqNos
      if (isNewMsg()) {  // returns false if !isServer() and newConnectionRequest
        // expected incoming seqNo increased AFTER sending ack  //note this only true if targetAddress matches
        // copy msg to rx buffer to make it available upstream
        if ((inComingSeqNo == 1) && (receivedMsg.getMsgSeqNo() == 1) && (linkConnectionTimeout > 0)) { // got next msg seqNo so clear linkConnectionTimer
          linkConnectionTimeout = 0;
#ifdef DEBUG
          if (debugOut != NULL) {
            debugOut->println(" received next seqNo, clear linkConnectionTimeout");
          }
#endif
        }
        uint8_t len = receivedMsg.getLen();
#ifdef DEBUG
        if (debugOut != NULL) {
         debugOut->print(" len:"); debugOut->print(len); debugOut->print(" rxBuf.availableForWrite():"); debugOut->println(rxBuf.availableForWrite());
        }
#endif
        if (rxBuf.availableForWrite() >= len) {
          // can fit this msg in rx buffer
          uint8_t *buf = receivedMsg.getBuf();
          for (uint8_t i = 0; i < len; i++) {
            // no need to check for overflow as already checked above
            rxBuf.write(buf[i]);
          }
          needToAckReceivedMsg = true;
        }
      } else {
        // else not new msg
        // DO NOT ack this msg
        // is it resend of previous msg??
        uint8_t receivedMsgSeqNoPlus1 = receivedMsg.getMsgSeqNo() + 1;
        if (receivedMsgSeqNoPlus1 == 0) {
          receivedMsgSeqNoPlus1 = 0xff;
        }
        // NOTE: for connect msgs inComingSeqNo == 1 -> previousMsgSeqNo == 0
        // so work on + 1 instead due to wrap from 0xff -> 1 normally
        if (receivedMsgSeqNoPlus1 == inComingSeqNo) {
          resendLastMsg(); // either pureAck or msg with ack
          // don't change the waitingForAckOfLastMsgSent state
        }
      }
      if (needToAckReceivedMsg) {
        // ALWAYS send ack for msg recieved before sending reply
        if (waitingForAckOfLastMsgSent) {
        	// update ackNo to ack this msg on resend
        	lastMsgSent.setAckedMsgSeqNo(receivedMsg.getMsgSeqNo());
        } else {
        	sendAck();
        }
        inComingSeqNo++; // have sent ack so expect new msg next time
        needToAckReceivedMsg = false;
      }
#ifdef DEBUG
      if (debugOut != NULL) {
        debugOut->println();
      }
#endif
    } // if (!isPureAck()) {
  }
  if (waitingForAckOfLastMsgSent) {
    // see if we are still transmitting
    if (inTxMode()) {
      // still sendi just reset timeLastMsSent
      timeLastMsgSent = millis();
#ifdef DEBUG_TX_TIME
      //      if ((debugOut != NULL) && (sendTimerRunning)) {
      //        debugOut->println(F("Still in Tx mode, reset ack timer"));
      //      }
#endif
    } else {
      // finished tx
#ifdef DEBUG_TX_TIME
      if ((debugOut != NULL) && (sendTimerRunning)) {
        sendTimerRunning = false; // only print this once
        debugOut->print(F("Tx time. Msg len:"));
        debugOut->print(lastMsgSent.getLen());
        debugOut->print(F(" send time:"));
        debugOut->print(millis() - sendTimeStart_mS);
        debugOut->println(F(" mS"));
      }
#endif
    }
    // see if have timed out waiting for ack
    if ((millis() - timeLastMsgSent) > randomTimeout) {
      // timed out waiting for ack
      if (retriesCount < noOfRetries) {
        retriesCount++;
#ifdef DEBUG_TX_TIME
        if (debugOut != NULL) {
          debugOut->print(F("Missing Ack. Randomized Ack timeout:"));
          debugOut->print(randomTimeout);
          debugOut->println(F(" mS"));
          debugOut->print(F("Resending last msg "));
          debugOut->print(millis() - sendTimeStart_mS);
          debugOut->println(F(" mS since start of previous Tx"));
        }
        sendTimeStart_mS = millis();
#endif
        resendLastMsg();
      } else {
        closeConnection(); //clears txbuffer, sets rxbuffer to 0xff
#ifdef DEBUG
        if (debugOut != NULL) {
          debugOut->println(" link Failed on retries");
        }
#endif
      }
    }
  } else {
    // have handled any incoming send data if any
    // send msg
    if (notInTxMode()) {
      sendMsg();
    }
  }
}

uint16_t pfodRadio::getLastRSSI() {
  return lastRSSI;
}

void pfodRadio::setDebugStream(Print* _debugOut) {
  debugOut = _debugOut;
}

unsigned long pfodRadio::getDefaultTimeOut() {
  return 10; // 10 sec
}


/**
   Called by pfodSecurity when parses {!} or get EOF in input stream
*/
void pfodRadio::_closeCurrentConnection() {
#ifdef DEBUG
  if (debugOut != NULL) {
    debugOut->print("_closeCurrentConnection ");
  }
#endif
  if (newConnection) {
    newConnection = false; // have processed this
#ifdef DEBUG
    if (debugOut != NULL) {
      debugOut->println(" newConnection");
    }
#endif
    // do not resetTargetAndSeqNos
    // do not clear Rxbuff
    // do not reset linkConnectionTimeout
    txBuf.clear();
    rawDataBuf.clear(); // clear raw data
    connectionClosed = false;
    return;
  } // else
#ifdef DEBUG
  if (debugOut != NULL) {
    debugOut->println(" NOT newConnection");
  }
#endif
  closeConnection();
  clearTxRxBuffers(); // clear 0xff from rxbuffer to prevent loop caller needs to clean itself up
}

bool pfodRadio::isPureAck() {
  return receivedMsg.isPureAck();
}

bool pfodRadio::isConnectionClosed() {
  return connectionClosed;
}

Print* pfodRadio::getRawDataOutput() {
  return &rawDataBuf; // rawdata sent to rawDataBuffer
}

size_t pfodRadio::writeRawData(uint8_t c) {
  return rawDataBuf.write(c);
}

// connection closed when we timeout on acks OR when pfodSecurity calls _closeCurrentConnection()
// server never sends new connection so if
// connectionClosed and isServer do NOT send msgs
void pfodRadio::closeConnection() {
  // clear target if server
  resetTargetAndSeqNos();
  if (isServer()) {
    // else if client leave targetAddress as set by connectTo(..)
    targetAddress = 0xff; // 255 == not set
  }
  connectionClosed = true; //
  linkConnectionTimeout = 0; // stop timeout
  clearTxRxBuffers();
  rxBuf.write(0xff);
}

bool pfodRadio::notInTxMode() {
  int currentMode = driver->getMode();
  return ((currentMode != pfodRadioDriver::Tx));
}

bool pfodRadio::inTxMode() {
  int currentMode = driver->getMode();
  return ((currentMode == pfodRadioDriver::Tx));
}

pfodRadio::pfodRadio(pfodRadioDriver * _driver, uint8_t _thisAddress) {
  if (_thisAddress == 0xff) {
    // Error: thisAddress must be in the range 0 to 254 inclusive
    // this will ignore received msgs
  }

  driver = _driver;
  thisAddress = _thisAddress;
  outGoingSeqNo = 0; // last outgoing seq no (expecting ack for this one
  inComingSeqNo = 0; // next expected incoming seqNo
  isServerNode = false;
  targetAddress = 0xff; // 255 == not set
  ackTimeout = 200; // in milliseconds
  receivedMsg.init(driver->getMaxMessageLength());
  receivedMsgPtr = &receivedMsg;
  lastMsgSent.init(driver->getMaxMessageLength());
  lastMsgSentPtr = &lastMsgSent;
  maxWriteBufLen = 0; // not set
  //writeBuf = NULL; // not created yet
  needToAckReceivedMsg = false;
  noOfRetries = 5;
  waitingForAckOfLastMsgSent = false;
  retriesCount = 0;
  connectionClosed = true;
  newConnection = false;
  linkConnectionTimeoutTimer = 0;
  linkConnectionTimeout = 0;
  debug_mS = millis();
  sendTimerRunning = false;
}

// sets new randomTimeout
void pfodRadio::setTimeLastMsgSent() {
  uint8_t r8 = pfodRandom8();
  uint16_t r = ((((uint32_t)ackTimeout) * r8) >> 8); // (t * (r&255)) / 256
  randomTimeout = ackTimeout + r; // new random timeout
  //#ifdef DEBUG
  //  if (debugOut != NULL) {
  //    debugOut->print(" r8:"); debugOut->println(r8);
  //    debugOut->print(" r:"); debugOut->println(r);
  //    debugOut->print(" ackTimeout:"); debugOut->println(ackTimeout);
  //    debugOut->print(" randomTimeout:"); debugOut->println(randomTimeout);
  //  }
  //#endif
  timeLastMsgSent = millis();
}

void pfodRadio::clearTxRxBuffers() {
  txBuf.clear();
  rxBuf.clear();
  rawDataBuf.clear();
}


bool pfodRadio::init() {
  if (!driver->init()) {
#ifdef DEBUG
    if (debugOut != NULL) {
      debugOut->println("init() driver init() failed!"); 
    }
#endif  
    return false;
  }
  waitingForAckOfLastMsgSent = false;
  retriesCount = 0;
  // initize buffers for client use i.e. rx 1024 and tx 256
  // if listen() called to make this a server then re-initialize them for server use
  txBuf.init(buffer_256, BUFFER_SIZE_256); 
  rxBuf.init(buffer_1024, BUFFER_SIZE_1024);
  rawDataBuf.init(NULL,0); // not needed for client, server will re-initialize
  clearTxRxBuffers();
  lastWriteTime = millis();
  driver->setThisAddress(thisAddress);
#ifdef DEBUG
    if (debugOut != NULL) {
      debugOut->print("init() This node address:"); debugOut->println(thisAddress);
    }
#endif  
  connectionClosed = true;
  return true;
}

/**
    the _to address must be in the range 1 to 254 inclusive
*/
void pfodRadio::connectTo(uint8_t _to) { // seting up as client
  if (_to == 0xff) {
#ifdef DEBUG
    if (debugOut != NULL) {
      debugOut->println("ERROR: connectTo() destination address must be in the range 0 to 254 inclusive!!");
    }
#endif
  }
  retriesCount = 0;
  lastMsgSent.saveMsg(NULL, 0, _to, thisAddress, 0, 0);
  targetAddress = _to;
#ifdef DEBUG
    if (debugOut != NULL) {
      debugOut->print("connectTo() address:"); debugOut->println(targetAddress);
    }
#endif  
  isServerNode = false;
  canAcceptNewConnections = false; // clients can not accept connections
}

// called from sendMsg() only
// if server just do nothing and return
// if client, if the connection is closed reset counts / msgNo for a new connection
void pfodRadio::checkIfNeedToConnect() {
  if (isServerNode) {
    return; // servers never start connections
  }
  if (!isConnectionClosed()) {
    return;
  }
#ifdef DEBUG
  //if (debugOut != NULL) {
  // debugOut->println("checkIfNeedToConnect");
  //}
#endif
  resetTargetAndSeqNos();
  connectionClosed = false; // assume ack works
}

void pfodRadio::resetTargetAndSeqNos() {
  waitingForAckOfLastMsgSent = false;
  outGoingSeqNo = 0;
  inComingSeqNo = 1; // expected seq for reply to connect
  retriesCount = 0;
}

uint8_t pfodRadio::getThisAddress() {
  return thisAddress;
}

bool pfodRadio::isServer() {
  return isServerNode;
}

void pfodRadio::setMaxWriteBufLen(size_t _maxLen) {
  maxWriteBufLen = _maxLen + 8 + 2; // this is ignored for the moment
}

void pfodRadio::listen() { // setting up as server
  // restore tx,rx buffers to 1024 and 256
  // set rawDataBuf to 1024
  rxBuf.init(buffer_256, BUFFER_SIZE_256); 
  txBuf.init(buffer_1024, BUFFER_SIZE_1024);
  rawDataBuf.init(rawDataBuffer_1024,BUFFER_SIZE_1024); 
  lastMsgSent.saveMsg(NULL, 0, 0, thisAddress, 0, 0);
  outGoingSeqNo = 1;
  inComingSeqNo = 0; // expected seq for connect
  targetAddress = 0xff; // 255 == not set // none yet
  isServerNode = true;
  canAcceptNewConnections = true;
}

// max timeout is 32.7sec (32767mS)
void pfodRadio::setAckTimeout(uint16_t _timeout_mS) {
  ackTimeout = _timeout_mS;
  if (ackTimeout > 0x7fff) {
    ackTimeout = 0x7fff;
  }
  if (ackTimeout == 0) {
    ackTimeout = 1;
  }
#ifdef DEBUG
  if (debugOut != NULL) {
    debugOut->print("setTimeout called:");
    debugOut->println(ackTimeout);
  }
#endif

  randomTimeout = ackTimeout;
}


// data available to read
int pfodRadio::available() {
  pollRadio(); // do the main work here
  return rxBuf.available();
}

int pfodRadio::peek() {
  pollRadio(); // do the main work here
  return rxBuf.peek();
}

int pfodRadio::read() {
  pollRadio(); // do the main work here
  return rxBuf.read();
}

void pfodRadio::flush() {
  lastWriteTime = millis() - (SEND_MSG_DELAY_TIME + 1); // force send NOW if anything to send
  pollRadio(); // do the main work here
  //  rxBuf.flush(); // actually empty
  //  txBuf.flush();  // actually empty
}


size_t pfodRadio::write(uint8_t b) {
  pollRadio(); // do the main work here
  lastWriteTime = millis();
  txBuf.write(b);
  return 1; // mark as successfull even if buffer full.  Other end will sort it out.
}



void pfodRadio::setNoOfRetries(uint8_t _noOfRetries) {
  noOfRetries = _noOfRetries; // zero means never retry
}

pfodRadioMsg* pfodRadio::getReceivedMsg() {
  return &receivedMsg;
}

/**
   New connection request if toAddress matches ours AND msgSeqNo == 0 and not empty msg
*/
bool pfodRadio::isNewConnectionRequest() {
  return receivedMsg.isNewConnectionRequest(thisAddress);
}

// this is server code ONLY
// if this is a new connection then reset the seqNos and target
// else leave as is
void pfodRadio::checkForAllowableNewConnection() {
#ifdef DEBUG
  if (debugOut != NULL) {
    debugOut->print(" called checkForAllowableNewConnection() targetAddress:"); debugOut->print(targetAddress); debugOut->print(" receivedFrom:"); debugOut->println(receivedMsg.getReceivedFrom());
  }
#endif
  if (!isServer()) {
    return; // only servers can accept connections
  }
  // this code only executed on server

  if (!isNewConnectionRequest()) { // New connection request if toAddress matches ours AND msgSeqNo == 0 and not empty msg
#ifdef DEBUG
    if (debugOut != NULL) {
      debugOut->print(" !isNewConnectionRequest() return receivedFrom:"); debugOut->println(receivedMsg.getReceivedFrom());
    }
#endif
    return; // not a new connection
  }
  if ((targetAddress != 0xff) && (targetAddress == receivedMsg.getReceivedFrom() )) {
    // reconnection from existing clientset new connection  OR resend of connection request missed ack
    // OK
    // existing client and msgSeqNo == 0
    if ((inComingSeqNo == 1) && (receivedMsg.getMsgSeqNo() == 0) && (linkConnectionTimeout > 0)) { // expected next msg seq No
      // the very last msg from this client was a connection msg so assume this is a resend of that msg
      // so don't reset outgoing/incomming seqNo
      // later code will resend previous response
      // UNLESS linkConnectionTimeout has passed in which case accept this as a new connection
      return;
    } // else
#ifdef DEBUG
    if (debugOut != NULL) {
      debugOut->print(" Accepting new connection from existing client "); debugOut->println(receivedMsg.getReceivedFrom());
    }
#endif

  } else { // not same target
    if (!connectionClosed) {
#ifdef DEBUG
      if (debugOut != NULL) {
        debugOut->print(" Cannot accept new connection from "); debugOut->print(receivedMsg.getReceivedFrom());
        debugOut->print(" because existing connection from "); debugOut->print(targetAddress); debugOut->println(" not timed out or closed.");
      }
#endif
      return; // ignore if not closed
    } else {
#ifdef DEBUG
      if (debugOut != NULL) {
        debugOut->print(" ConnectionClosed. Accepting new connection from client "); debugOut->println(receivedMsg.getReceivedFrom());
      }
#endif
    }
  } // else
  if (receivedMsg.getReceivedFrom() == 0xff) {
    return; // do not accept msgs from 255
  }
  // new connection
  // push 0xff on to rxbuf
  rxBuf.write(0xff);
  newConnection = true;
  targetAddress = receivedMsg.getReceivedFrom(); // set new connection
  // reset seq no
  connectionClosed = false; // new connection
  outGoingSeqNo = 1; // next outgoing seq no
  inComingSeqNo = 0; // next expected incoming seqNo
  // start linkConnection timeout
  resetLinkConnectTimeout(); // reset linkConnectionTimeout
#ifdef DEBUG
  if (debugOut != NULL) {
    debugOut->print(" reset linkTimer to :"); debugOut->println(linkConnectionTimeout);
  }
#endif

}

bool pfodRadio::isNewMsg() {
  return receivedMsg.isNewMsg(thisAddress, targetAddress, inComingSeqNo, isServer());
}


bool pfodRadio::recvMsg() {
  uint8_t len = (uint8_t)receivedMsg.getMaxMsgBufferSize(); // RadioHead len limited to uint8_t
  if (driver->receive(receivedMsg.getBuf(), &len))   {
    receivedMsg.saveMsg(receivedMsg.getBuf(), len, driver->headerTo(), driver->headerFrom(), driver->headerId(), driver->headerFlags());
#ifdef DEBUG
    if (debugOut != NULL) {
      debugOut->print("Received msg receivedFrom:"); debugOut->print(receivedMsg.getReceivedFrom());  debugOut->print(" addressedTo:"); debugOut->print(receivedMsg.getAddressedTo());
    }
#endif

    if (receivedMsg.isPureAck()) {
#ifdef DEBUG
      if (debugOut != NULL) {
        debugOut->print(" Pure Ack for : "); debugOut->println(receivedMsg.getAckedMsgSeqNo());
      }
#endif

    } else {
#ifdef DEBUG
      if (debugOut != NULL) {
        debugOut->print(" incomingMsgSeqNo:"); debugOut->print(receivedMsg.getMsgSeqNo());
        debugOut->print(" ackMsgSeqNo:"); debugOut->print(receivedMsg.getAckedMsgSeqNo());
        debugOut->print(" len:"); debugOut->print(receivedMsg.getLen()); debugOut->println(" data:");
        char *buf = (char*)receivedMsg.getBuf();
        for (int i = 0; i < receivedMsg.getLen(); i++) {
          debugOut->print(buf[i]);
        }
        debugOut->println();
      }
#endif

    }
    return true;
  } // else
  return false;
}

bool pfodRadio::isAckForLastMsgSent() {
  if (!waitingForAckOfLastMsgSent) {
    return false; // not needed
  }
  if (!receivedMsg.isAckFor(&lastMsgSent)) {
    return false;
  } // else
  waitingForAckOfLastMsgSent = false; // got ack
  retriesCount = 0;
#ifdef DEBUG
  if (debugOut != NULL) {
    debugOut->print("Got ack to : "); debugOut->println((uint8_t)(outGoingSeqNo - 1));
  }
#endif

  return true;
}

bool pfodRadio::sendTo(uint8_t* _buf, uint8_t _len, uint8_t _address) {
  if (_address == 0xff) {
#ifdef DEBUG
    if (debugOut != NULL) {
      debugOut->println("Error!! while trying to send message! destination address must be in the range 0 to 254 inclusive");
    }
#endif
    return false;
  }

  driver->setHeaderTo(_address);
  driver->setHeaderFrom(thisAddress);
  return driver->send(_buf, _len);
}

void pfodRadio::sendAck() {
  uint8_t ack = '\0';
  lastMsgSent.saveMsg(&ack, 1, receivedMsg.getReceivedFrom(), receivedMsg.getAddressedTo(), 0, receivedMsg.getMsgSeqNo());
#ifdef DEBUG
  if (debugOut != NULL) {
    debugOut->print("Send Pure Ack for "); debugOut->print(receivedMsg.getMsgSeqNo());
    debugOut->print(" addressedTo:"); debugOut->print(lastMsgSent.getAddressedTo()); debugOut->print(" receivedFrom:"); debugOut->println(lastMsgSent.getReceivedFrom());
  }
#endif

  sendMsg(&lastMsgSent);
}

void pfodRadio::resendLastMsg() {
#ifdef DEBUG
  if (debugOut != NULL) {
    debugOut->print("Resend last msg ");
  }
#endif

  if (lastMsgSent.isPureAck()) {
#ifdef DEBUG
    if (debugOut != NULL) {
      debugOut->print(" Pure Ack for :"); debugOut->print(lastMsgSent.getAckedMsgSeqNo());
    }
#endif

  } else {
#ifdef DEBUG
    if (debugOut != NULL) {
      debugOut->print(" no:"); debugOut->print(lastMsgSent.getMsgSeqNo()); debugOut->print(" with ack for:"); debugOut->print(lastMsgSent.getAckedMsgSeqNo());
    }
#endif

  }
#ifdef DEBUG
  if (debugOut != NULL) {
    debugOut->print(" addressedTo:"); debugOut->print(lastMsgSent.getAddressedTo()); debugOut->print(" receivedFrom:"); debugOut->println(lastMsgSent.getReceivedFrom());
  }
#endif

  setTimeLastMsgSent();
  sendMsg(&lastMsgSent);
}


// sends from rawDataBuf or txBuf if any available
size_t pfodRadio::sendMsg() {
	if (isServer() && isConnectionClosed()) {
#ifdef DEBUG
    //if (debugOut != NULL) {
    //   debugOut->println("sendMsg() isServer and isConnectionClosed, return");
    //}
#endif
#ifdef DEBUG_RAW_DATA
    if (debugOut != NULL) {
      if (rawDataBuf.available() > 0) {
        debugOut->print("sendMsg() isServer and isConnectionClosed clear rawDataBuf available():");
        debugOut->println(rawDataBuf.available());
      }
    }
#endif
       rawDataBuf.clear();
	   return 0;
	}
  if (waitingForAckOfLastMsgSent) {
#ifdef DEBUG
    //if (debugOut != NULL) {
    //   debugOut->println("sendMsg() Still waitingForAckOfLastMsgSent, return");
    //}
#endif
    return 0;
  }
  if ((txBuf.available() == 0) && (rawDataBuf.available() == 0)) {
    return 0;
  }
  
  if ((rawDataBuf.available() >= driver->getMaxMessageLength()) ||
  	  (txBuf.available() >= driver->getMaxMessageLength()) ||  
     ((millis() -  lastWriteTime) > SEND_MSG_DELAY_TIME) ) {
     // continue
  } else {
  	  return 0; // try later
  }

#ifdef DEBUG_TX_TIME
  if (rawDataBuf.available() >= driver->getMaxMessageLength()) {
    if (debugOut != NULL) {
       debugOut->print("sendMsg() rawDataBuf.available() >=  ");
       debugOut->println(driver->getMaxMessageLength());
    }
  }
  if (txBuf.available() >= driver->getMaxMessageLength()) {
    if (debugOut != NULL) {
       debugOut->print("sendMsg() txBuf.available() >=  ");
       debugOut->println(driver->getMaxMessageLength());
    }
  }
  if ((millis() -  lastWriteTime) > SEND_MSG_DELAY_TIME) {
    if (debugOut != NULL) {
       debugOut->print("sendMsg() ((millis() -  lastWriteTime) > ");
       debugOut->println(SEND_MSG_DELAY_TIME);
    }
  }
  if (debugOut != NULL) {
     debugOut->print("sendMsg() millis() = ");
     debugOut->print(millis());
     debugOut->print(" lastWriteTime = ");
     debugOut->println(lastWriteTime);
  }
#endif

  checkIfNeedToConnect(); // resets outgoing and expected incoming
  // only if !isServer() && isConnectionClosed()
  waitingForAckOfLastMsgSent = true;
#ifdef DEBUG_TX_TIME
  sendTimeStart_mS = millis();
  sendTimerRunning = true;
#endif
  setTimeLastMsgSent();
  retriesCount = 0;
  // only send ack msgseqNo if last msg sent was a pureAck
  // note ALWAYS send an ack first before sending next msg.
  // only end msg with ack for first msg AFTER pureAck
  uint8_t ackMsgSeqNo = 0;
  if (lastMsgSent.isPureAck()) { // duplicate ack msg seq No on next msg sent
    ackMsgSeqNo = receivedMsg.getMsgSeqNo();
  }
  size_t lenSent = 0;
  if (rawDataBuf.available() ) {
  	  // send raw data first 
  	  // this assumes raw data buffer ONLY written to when cmd received and waiting to send response!!
    lenSent = lastMsgSent.saveMsg(&rawDataBuf, targetAddress, thisAddress, outGoingSeqNo++, ackMsgSeqNo);
#ifdef DEBUG_RAW_DATA
    if (debugOut != NULL) {
      debugOut->print("RawData msg len:"); debugOut->print(lastMsgSent.getLen()); debugOut->println(" data:");
      char *buf = (char*)lastMsgSent.getBuf();
      for (int i = 0; i < lastMsgSent.getLen(); i++) {
        debugOut->print(buf[i]);
      }
      debugOut->println();
    }
#endif
      
  } else { // send txBuf
    lenSent = lastMsgSent.saveMsg(&txBuf, targetAddress, thisAddress, outGoingSeqNo++, ackMsgSeqNo);
#ifdef DEBUG_RAW_DATA
    if (debugOut != NULL) {
      debugOut->print("pfodMsg msg len:"); debugOut->print(lastMsgSent.getLen()); debugOut->println(" data:");
      char *buf = (char*)lastMsgSent.getBuf();
      for (int i = 0; i < lastMsgSent.getLen(); i++) {
        debugOut->print(buf[i]);
      }
      debugOut->println();
    }
#endif
  }
  if (outGoingSeqNo == 0) {
    outGoingSeqNo = 1; // only 0 for connects
  }
#ifdef DEBUG
  if (debugOut != NULL) {
    debugOut->print("Sent msg ");  debugOut->print(" addressedTo:"); debugOut->print(lastMsgSent.getAddressedTo()); debugOut->print(" receivedFrom:"); debugOut->print(lastMsgSent.getReceivedFrom());
    debugOut->print(" outGoingMsgSeqNo:"); debugOut->print(lastMsgSent.getMsgSeqNo());
    debugOut->print(" ackMsgSeqNo:"); debugOut->println(lastMsgSent.getAckedMsgSeqNo());
    debugOut->print(" len:"); debugOut->print(lastMsgSent.getLen()); debugOut->println(" data:");
    char *buf = (char*)lastMsgSent.getBuf();
    for (int i = 0; i < lastMsgSent.getLen(); i++) {
      debugOut->print(buf[i]);
    }
    debugOut->println();
  }
#endif

  sendMsg(&lastMsgSent);
  return lenSent;
}

void pfodRadio::sendMsg(pfodRadioMsg * radioMsg) {
  driver->setHeaderId(radioMsg->getMsgSeqNo());
  driver->setHeaderFlags(radioMsg->getAckedMsgSeqNo()); //set ack id for this msg zero if nothing to be acked, i.e. first connect msg or start of next cmd from client
  sendTo(radioMsg->getBuf(), radioMsg->getLen(), radioMsg->getAddressedTo());
}



