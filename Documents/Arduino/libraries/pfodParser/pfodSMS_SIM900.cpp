/**
TODO  
Alerts
Note: Connection timeout here only allows another pfodApp to connect. 
It does not close the connection or prevent raw data being sent
*/ 
/*
 * (c)2014-2017 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provide this copyright is maintained.
 */
 
 
#include "pfodSMS_SIM900.h"
#include "pfodParserUtils.h"
#include "pfodWaitForUtils.h"

//#define DEBUG
//#define DEBUG_INCOMING
// uncomment next line to show timeout time every 20 sec
//#define SHOW_TIMEOUT_TIMER
//#define DEBUG_TIMEOUT_TIMER
//#define DEBUG_WRITE
//#define DEBUG_FLUSH
//#define DEBUG_DECODED_MSG
//#define DEBUG_SETUP
//#define DEBUG_RAWDATA

pfodSMS_SIM900::pfodSMS_SIM900() {
  raw_io.set_pfod_Base((pfod_Base*)this);
  raw_io_ptr = &raw_io;
  gprs = NULL; // not set yet
  powerResetPin = -1; // not set yet
}

Print* pfodSMS_SIM900::getRawDataOutput() {
  return raw_io_ptr;
}

/**
 * return default time out for SMS of 600sec (10min)
 */
unsigned long pfodSMS_SIM900::getDefaultTimeOut() {
	return 600;
}

/**
 * Call this before calling init(..) if you want the debug output
 *
 * debugOut is not written to unless you uncomment one of the #define DEBUG_ settings above
 */
void pfodSMS_SIM900::setDebugStream(Stream* out) {
  debugOut = out;
}

/**
  Initialize the stream to be used to send and receive SMS msgs
  Use -1 for _powerOnOffPin if not used, but usually is used.
*/
void pfodSMS_SIM900::init(Stream *_gprs, int _powerOnOffPin) {
  gprs = _gprs;
  powerResetPin = _powerOnOffPin; // -1 if not used
  init();
}

/**
 called from pfodSecurity::connect()
 */
void pfodSMS_SIM900::init() {
#ifdef DEBUG
  if (debugOut != NULL) {
    debugOut->print(F("NOTE: \\r \\n are replaced by [ ] for printout as in smsLine:'OK[]'\n"));
  }
#endif // DEBUG
  resendLastResponse = false;
  responseSet = true; // wait for first command
  sendingResponse = false;
  sendingResend = false;
  sendingRawdata = false;
  deleteReadSmsMsgs = false;
  haveRequestedIncomingMsg = false;
  bytesToConvert[0] = '\0';
  timedOutTimer = 0; // timed out;
  rxBuffer[SMS_RX_BUFFER_SIZE] = '\0';
  rxBuffer[0] = '\0';
  rxBufferLen = 0;
  rxBufferIdx = 0;
  rawdataTxBuffer[SMS_RAWDATA_TX_BUFFER_SIZE] = '\0';
  rawdataTxBufferLen = 0;
  rawdataTxBufferIdx = 0;
  rawdataSMSbuffer[0] = '\0';
  clearTxBuffer();
  currentConnection = &connection_A;
  clearConnection (currentConnection);
  nextConnection = &connection_B;
  clearConnection (nextConnection);
  for (int i = 0; i < MAX_KNOWN_CONNECTION; i++) {
    clearKnownConnection(knownConnectionsArray + i);
    knownArrayPtrs[i] = knownConnectionsArray + i;
  }
  //printCurrentConnection();
  //printNextConnection();
  for (int i = 0; i < MAX_INCOMING_MSG_NOS + 1; i++) {
    incomingMsgNos[i][0] = '\0';
  }
  incomingMsgNos_head = 0;
  emptyIncomingMsgNo(); // sets tail == head so empty
  clearGprsLine();
  expectingMessageLines = false; // note msg from pfodApp may have embedded new line
  gprsReady = false;
  // setup pin for powerup

#ifdef DEBUG_SETUP
  if (debugOut != NULL) {
    debugOut->println(F("Waiting for \\r from console before continuing."));
    while (!pfodWaitForUtils::waitFor("\r", debugOut)) {
      // return to continue
    }
    debugOut->println(F("Got \\r continuing with setup, checkGPRSpoweredUp()."));
  }
#endif // DEBUG_SETUP
  checkGPRSpoweredUp();  
  foundOK = true;
}

Stream* pfodSMS_SIM900::getPfodAppStream() {
  // get the command response print we are writing to
  return gprs;
}

void pfodSMS_SIM900::printKnownConnections() {
#ifdef DEBUG
  if (debugOut != NULL) {
    debugOut->print(F("KnownConnections:-\n"));
  }
#endif // DEBUG
  for (int i = 0; i < MAX_KNOWN_CONNECTION; i++) {
    printKnownConnection (knownArrayPtrs[i]);
  }
}

/**
 * moves idx knownConnection to 0 and pushes the others down
 */
void pfodSMS_SIM900::moveKnownConnectionToTop(int idx) {
  if ((idx < 0) || (idx >= MAX_KNOWN_CONNECTION)) {
#ifdef DEBUG
    if (debugOut != NULL) {
      debugOut->print(F("ERROR: moveKnownConnectionToTop index out of range:"));
      debugOut->print(idx);
      debugOut->println();
    }
#endif // DEBUG
    return; // error
  }
  knownConnection *tmp = knownArrayPtrs[idx];
  for (int i = idx; i > 0; i--) {
    knownArrayPtrs[i] = knownArrayPtrs[i - 1];
  }
  knownArrayPtrs[0] = tmp;
}

/**
 * updates knownConnection array with this connection details or adds them
 * DOES NOT MOVE THIS connection to top
 */
void pfodSMS_SIM900::updateKnownConnectionNo(connection *con) {
  int idx = findMatchingKnownPhoneNo(con->returnPhNo);
  if (idx < 0) {
    // find an empty one and set the phoneNo
    idx = findEmptyKnownConnection();
    strncpy_safe(knownArrayPtrs[idx]->returnPhNo, con->returnPhNo, MAX_PHNO_LEN);
  }
  knownArrayPtrs[idx]->connectionNo = con->connectionNo;
}

void pfodSMS_SIM900::updateKnownConnectionNoAndMoveToTop(connection *con) {
  updateKnownConnectionNo(con); // never fails
  int idx = findMatchingKnownPhoneNo(con->returnPhNo); // never <0 since previous line adds it
  moveKnownConnectionToTop(idx); //now top
}

/**
 * returns -1 if none found, else index of match
 */
int pfodSMS_SIM900::findMatchingKnownPhoneNo(const char *phNo) {
  int rtn = -1;
  for (int i = 0; i < MAX_KNOWN_CONNECTION; i++) {
    if (phoneNoMatches(knownArrayPtrs[i]->returnPhNo, phNo)) {
      rtn = i;
      break;
    }
  }
  return rtn;
}

/**
 * sets top known connection from current connection
 * Top connection must be either same phoneNo or empty
 */
void pfodSMS_SIM900::setTopKnownConnectionFromCurrent() {
  if ((knownArrayPtrs[0]->returnPhNo[0]) // not empty
      && (!phoneNoMatchesCurrent(knownArrayPtrs[0]->returnPhNo))) {
    // error
#ifdef DEBUG
    if (debugOut != NULL) {
      debugOut->print(
        F(
          "ERROR: setTopKnownConnectionFromCurrent. Top knownConnecton not empty and does not match current phoneNo\n"));
      debugOut->print(F(" Top knownConnection is "));
    }
#endif // DEBUG
    printKnownConnection (knownArrayPtrs[0]);
    printCurrentConnection();
    return;
  }
  // else
  strncpy_safe(knownArrayPtrs[0]->returnPhNo, currentConnection->returnPhNo, MAX_PHNO_LEN);
  knownArrayPtrs[0]->connectionNo = currentConnection->connectionNo;
}

/**
 * returns MAX_KNOWN_CONNECTION-1 if none found, else index of first empty entry
 * NOTE: always clear the found connection, i.e. will clear last one if none empty
 */
int pfodSMS_SIM900::findEmptyKnownConnection() {
  int rtn = MAX_KNOWN_CONNECTION - 1;
  for (int i = 0; i < MAX_KNOWN_CONNECTION; i++) {
    if (!(knownArrayPtrs[i]->returnPhNo[0])) {
      rtn = i;
      break;
    }
  }
  clearKnownConnection(rtn);
  return rtn;
}

void pfodSMS_SIM900::printKnownConnection(knownConnection *kCon) {
#ifdef DEBUG
  if (debugOut != NULL) {
    debugOut->print(F("ConnectionNo:"));
    debugOut->print(kCon->connectionNo);
    debugOut->print(F(" Return PhNo:"));
    debugOut->print(kCon->returnPhNo);
    debugOut->println();
  }
#else
  (void)(kCon);  
#endif // DEBUG
}

void pfodSMS_SIM900::clearKnownConnection(int idx) {
  if ((idx < 0) || (idx >= MAX_KNOWN_CONNECTION)) {
#ifdef DEBUG
    if (debugOut != NULL) {
      debugOut->print(F("ERROR: clearKnownConnection index out of range:"));
      debugOut->print(idx);
      debugOut->println();
    }
#endif // DEBUG
    return; // error
  }
  clearKnownConnection (knownArrayPtrs[idx]);
}

void pfodSMS_SIM900::clearKnownConnection(knownConnection *kCon) {
  kCon->connectionNo = 0;
  kCon->returnPhNo[0] = '\0';
}

void pfodSMS_SIM900::clearTxBuffer() {
  txBuffer[SMS_TX_BUFFER_SIZE] = '\0';
  txBuffer[0] = '\0';
  txBufferLen = 0;
  txBufferIdx = 0;
}


/**
 * closes the connection but keeps the previous connection number and phone no
 * so that a re-connection from this phone has to have a larger connection number
 * this protects against old msgs being acted on after the connnection is closed.
 */
void pfodSMS_SIM900::closeConnection(connection* con) {
  con->msgNo_rx = 0;
  con->msgNo_tx = 1;
  con->msgNo_tx_resend = 1; // set by flush
  con->msgNo_tx_newMsg = 1; // set by end of send msg after flush
  //  con->connectionNo = 0; // = 0; // not set
  con->timedOut = true;
  //  con->returnPhNo[0] = '\0';
}

void pfodSMS_SIM900::clearConnection(connection* con) {
  closeConnection(con);
  con->connectionNo = 0; // = 0; // not set
  con->returnPhNo[0] = '\0';
}

/**
 *  Priority for actions are
 *  Get incomming SMS msgNo_rx and then get message and delete incomming
 *    this may initiate a rawData or Alert but only if one is not waiting to be sent
 *  otherwise fills incoming stream for processing
 *  Sends of responses 0 to end  (from processing incoming stream)  drops other responses until sent.  Should not happen.
 *  Resends 0 to end   terminated by start of send of responses due to processing incoming stream
 *  send rawData or Alert if any full rawData drops other data until sent (canSendRawData == false)
 // state controls
 boolean expectingMessageLines; // note msg from pfodApp cannot have \r\n as are encoded
 boolean sendingResponse; // true if processing and sending SMS response to command
 boolean sendingResend; // true if resending previous response to command
 // this takes txBuffer and encodes upto 117 bytes and sends msg
 // continues until all of txBuffer sent

 responseSMSbuffer[0] not null => have SMS msg to send to returnConnection phno

 *
 */


void pfodSMS_SIM900::_closeCurrentConnection() {
    currentConnection->timedOut = true;
#ifdef DEBUG_TIMEOUT_TIMER
      if (debugOut != NULL) {
        debugOut->print(F("!!! --- close current Connection  --- !!!\n"));
      }
#endif // DEBUG
}

void pfodSMS_SIM900::clearResponseSet() {
  responseSet = false; // started new cmd can send response now Note: not cleared on close connection {!}
  clearTxBuffer();
}

void pfodSMS_SIM900::clearParser() {
  // put 0xff into rxBuffer before current msg
  // this will reset upstream parser like connection closed
  for (int i = SMS_RX_BUFFER_SIZE - 1; i > 0; i--) {
    rxBuffer[i] = rxBuffer[i - 1];
  }
  rxBuffer[0] = 0xff;
  rxBufferLen++; // add one to length
#ifdef DEBUG_DECODED_MSG
  if (debugOut != NULL) {
    debugOut->print(F("rxBuffer:'"));
    debugOut->write((const uint8_t*) (rxBuffer + rxBufferIdx), (rxBufferLen - rxBufferIdx));
    debugOut->println(F("'"));
  }
#endif // DEBUG_DECODED_MSG

}

/**
 * got new connection so stop sending responses or resends
 * if same phone number continue sending raw data
 */
void pfodSMS_SIM900::stopResponseSends() {
  // i.e prevent next call to sendNextSMS
  resendLastResponse = false;
  sendingResponse = false;
  sendingResend = false;
  // clear tx buffer
  clearTxBuffer();
}

/**
 * updates currentConnection for start of new command
 * set msgNo for sending new msg
 * newMsg no updated after finish sending the response if any
 * This is called by flush to send new command response.
 */
void pfodSMS_SIM900::updateMsgNoForStartNewCommand() {
  currentConnection->msgNo_tx = currentConnection->msgNo_tx_newMsg;
  currentConnection->msgNo_tx_resend = currentConnection->msgNo_tx_newMsg;
}
/**
 * updates currentConnection for start of resend of previous command
 */
void pfodSMS_SIM900::updateMsgNoForStartResend() {
  currentConnection->msgNo_tx = currentConnection->msgNo_tx_resend;
}

/**
 * got new connection from different phone so stop sending responses or resends
 * AND stop sending raw data
 */
void pfodSMS_SIM900::stopAllSends() {
  stopResponseSends(); // stop responses and resends
  // also stop sending rawdata TODO
}

/**
 *  If getting incomming SMS msg then incomingMsgNo is not empty
 *
 * // may need AT timer as well?? to force OK each 30secs just in case
 */
void pfodSMS_SIM900::processGprsLine(const char* smsLine) {
  const char *result = NULL;
#ifdef DEBUG
  if (debugOut != NULL) {
    // these msg need Serial set to 57600 or they interfer with gprs smsLine
    //debugOut->print(F("expMsgLn:"));  debugOut->print(expectingMessageLines?'T':'F');
    //debugOut->println();
    debugOut->print(F("smsLine:'"));
    debugOut->print(smsLine);
    debugOut->print(F("'"));
    debugOut->println();
  }
#endif // DEBUG
  if (expectingMessageLines) { // since /r/n not in encoding only ever get one line for msg
    expectingMessageLines = false;
    haveRequestedIncomingMsg = false; //got it now
#ifdef DEBUG
    if (debugOut != NULL) {
      //debugOut->print(F("msg '"));
      //debugOut->print(smsLine);
      //debugOut->print(F("'\n"));
    }
#endif // DEBUG
    // delete this incomming msg
    deleteReadSmsMsgs = true;
    // decode it
    // Returns: 0 if same connection No, may have empty msg if msg being ignored or invalid
    // else returns non-zero if new connection No.  msgNo == 0 in which case need to test current->timedOut
    // or newConnectionPhNo == current
    // if timedOut the start a new connection else send back {!...} to close
    // else return -ve if not a new connection in which case need to test { } and send back alert
    printCurrentConnection();
    int initialRxBufferLen = rxBufferLen;
    // updates nextConnection with connectionNo
    // have already set phoneNo
    decode_sms_return_t decode_return = decodeSMS(currentConnection, nextConnection, smsLine, rxBuffer, &rxBufferLen);
    switch (decode_return) {
      case RESEND_LAST_RESPONSE:
#ifdef DEBUG
        if (debugOut != NULL) {
          debugOut->print(F("\n>> RESEND_LAST_RESPONSE\n"));
        }
#endif // DEBUG
        printCurrentConnection();
        // resend last response
        // clear any msg added do not swap connections
        // do not reset timer
        rxBufferLen = initialRxBufferLen;
        rxBuffer[rxBufferLen] = '\0';
        clearConnection (nextConnection);
        // if sendingResend will finish that before doing this new resend
        resendLastResponse = true;
        break;
      case CLOSE_CONNECTION: // same phoneNo and connectionNo
#ifdef DEBUG
        if (debugOut != NULL) {
          debugOut->print(F("\n>> CLOSE_CONNECTION\n"));
        }
#endif // DEBUG
        stopAllSends();
        clearConnection(nextConnection);
        closeConnection (currentConnection); // leave connection No set  reset msgNo
        // process {!} msg
        // but leave responseSet true as no response expected
        //clearParser(); // {!} will do this but just to be sure NOTE: hash not checked before acting on close {!} !!!!
        // should not be a problem as does not allow control only disconnect
        break;
      case START_NEW_COMMAND:  // normal start of msg for current connection
#ifdef DEBUG
        if (debugOut != NULL) {
          debugOut->print(F("\n>> START_NEW_COMMAND\n"));
        }
#endif // DEBUG
        printCurrentConnection();
        stopResponseSends();
        clearConnection(nextConnection);
        // process the msg
        clearResponseSet();
        currentConnection->timedOut = false;
        break;
      case PROCESS_NEXT_PART_OF_COMMAND:  // normal next part of msg for current connection
#ifdef DEBUG
        if (debugOut != NULL) {
          debugOut->print(F("\n>> PROCESS_NEXT_PART_OF_COMMAND\n"));
        }
#endif // DEBUG
        clearConnection(nextConnection);
        // process the msg
        // do not reset time only do this for start of commands
        break;
      case CLOSE_CONNECTION_AND_UPDATE_CONNECTION: // phoneNo must be same as current here so must be top one
#ifdef DEBUG
        if (debugOut != NULL) {
          debugOut->print(F("\n>> CLOSE_CONNECTION_AND_UPDATE_CONNECTION\n"));
        }
#endif // DEBUG
        stopAllSends();
        printNextConnection();
        swapConnections();
        setTopKnownConnectionFromCurrent(); // updates connectionNo
        closeConnection(currentConnection); // leave connection No set  reset msgNo
        clearConnection(nextConnection); // old connection storage
        //clearParser(); // should have {!} in input but just to be sure
        break;
      case NEW_CONNECTION_SAME_PHONE_NO: // phoneNo must be same as current here so must be top one
#ifdef DEBUG
        if (debugOut != NULL) {
          debugOut->print(F("\n>> NEW_CONNECTION_SAME_PHONE_NO\n"));
        }
#endif // DEBUG
        printCurrentConnection();
        stopResponseSends();
        // process the msg
        printNextConnection();
        swapConnections();
        setTopKnownConnectionFromCurrent();
        clearResponseSet();
        currentConnection->timedOut = false;
        clearConnection(nextConnection); // old connection storage
        clearParser();  // need to inject 0xff here as no !
        break;
      case IGNORE_CONNECTION: // tried to start new connection but current not timed out
#ifdef DEBUG
        if (debugOut != NULL) {
          debugOut->print(F("\n>> IGNORE_CONNECTION\n"));
        }
#endif // DEBUG
        rxBufferLen = initialRxBufferLen;
        rxBuffer[rxBufferLen] = '\0';
        // SMS msg No == 0; for this case
        // do NOT update connection number as this is a new connection attempt which did not suceed
        // the pfodApp will try again with same connection no or a new one
        //updateKnownConnectionNo(nextConnection); // will add phoneNo if not there and update connectionNo
        clearConnection(nextConnection); // old connection storage
        break;
      case START_NEW_CONNECTION: // start a new connecton from new phone no
#ifdef DEBUG
        if (debugOut != NULL) {
          debugOut->print(F("\n>> START_NEW_CONNECTION\n"));
        }
#endif // DEBUG
        stopAllSends();
        printNextConnection();
        swapConnections(); // next is now current
        clearResponseSet();
        currentConnection->timedOut = false;
        updateKnownConnectionNoAndMoveToTop(currentConnection); // will add phoneNo if not there next was swapped to current
        clearConnection(nextConnection); // old connection storage
        clearParser(); // close any current timed out connection
        // process msg
        break;
      case START_NEW_CONNECTION_AND_CLOSE: // start a new connecton from new phone no and close immediately
#ifdef DEBUG
        if (debugOut != NULL) {
          debugOut->print(F("\n>> START_NEW_CONNECTION_AND_CLOSE\n"));
        }
#endif // DEBUG
        stopAllSends();
        printNextConnection();
        swapConnections();
        updateKnownConnectionNoAndMoveToTop(currentConnection); // will add phoneNo if not there next was swapped to current
        closeConnection(currentConnection); // leave connection No set  reset msgNo marks as timedout
        clearConnection(nextConnection); // old connection storage
        clearParser(); // close any current timed out connection
        // process msg
        break;
      default:
#ifdef DEBUG
        if (debugOut != NULL) {
          debugOut->print(F("\n>> IGNORE\n"));
        }
#endif // DEBUG
        // IGNORE
        // clear any msg added do not swap connections
        // do not reset timer
        rxBufferLen = initialRxBufferLen;
        rxBuffer[rxBufferLen] = '\0';
        clearConnection(nextConnection);
        break;
    }

  } else if ((result = findStr(smsLine, F("> ")))) { // F("+CMGS"))) { // reply from gprs
    if (responseSMSbuffer[0]) { // sending response do this in preference to raw data
      // but may be already in the process of sendingRawdata
      gprsPrint((char*) responseSMSbuffer);
      gprsPrint(26);
      gprsPrint("\r");
      responseSMSbuffer[0] = '\0'; // have sent this now
      if (txBufferIdx >= txBufferLen) {
        if (sendingResponse) {
          // update next to send
          currentConnection->msgNo_tx_newMsg = currentConnection->msgNo_tx;
        }
        sendingResponse = false; // finished send response now
        // else resending or raw data
        sendingResend = false;
      }
    } else if (rawdataSMSbuffer[0]) { // set in foundOK if  rawdataSMSbuffer[0] != '\0'
      gprsPrint((char*) rawdataSMSbuffer);
      gprsPrint(26);
      gprsPrint("\r");
      rawdataSMSbuffer[0] = '\0'; // have sent this now
      sendingRawdata = false;
    } else {
      gprsPrint(26); // nothing to send so send nothing  this should not happen
      gprsPrint("\r");
    }
  } else if ((result = findStr(smsLine, F("+CMTI:")))) {
#ifdef DEBUG
    if (debugOut != NULL) {
      debugOut->print(F("new msg:"));
    }
#endif // DEBUG
    const char *newMsgNo = findStr(result, F(","));
    if (newMsgNo) {
      const char *newMsgNoEnd = findStr(newMsgNo, F("["));
      if (newMsgNoEnd) {
        *(((char *) newMsgNoEnd) - 1) = '\0';
      }
#ifdef DEBUG
      if (debugOut != NULL) {
        debugOut->print(newMsgNo); // save a 3 msg nos for complete 256 input
      }
#endif // DEBUG
      if (!addIncomingMsgNo(newMsgNo)) {
        // just delete this msg
        deleteIncomingMsg(newMsgNo);
      }
    }
#ifdef DEBUG
    if (debugOut != NULL) {
      debugOut->println();
    }
#endif // DEBUG
    //requestIncomingSmsMsg(newMsgNo);  do this on OK

  } else if ((result = findStr(smsLine, F("+CMGR:")))) {
    //+CMGR: "REC UNREAD","+61419226364","","14/11/03,12:10:02+44"[]
    // get phNo
    const char *phNo = findStr(result, F(","));
    nextConnection->returnPhNo[0] = '\0'; // clear phNo
    if (phNo) { // points to next char after ,
      if (*phNo == '"') {
        phNo++; // step over " if any
      }
      const char *phNoEnd = findStr(phNo, F(","));
      if (phNoEnd) {
        // phNoEnd points to next char after ,
        phNoEnd--; // set back one to point to ,
        if (*(phNoEnd - 1) == '"') {
          phNoEnd--; // step back to closing " if any
        }
        *((char *) phNoEnd) = '\0'; // drop "
#ifdef DEBUG
        if (debugOut != NULL) {
          debugOut->print(F(" this message's phoneNo:"));
          debugOut->print(phNo); // have removed trailing "
          debugOut->print(F("\n"));
        }
#endif // DEBUG
        setPhoneNo(nextConnection, phNo); // only set if phNo found
      } // end if phNoEnd
    }
    // next line is a line of message
    expectingMessageLines = true;
#ifdef DEBUG
    if (debugOut != NULL) {
      debugOut->print(F("set messageLine true\n"));
    }
#endif // DEBUG
  } else if ((result = findStr(smsLine, F("ERROR")))) {
    // send CLEAR ALL MSGS to restart
    clearAllSmsMsgs();
  } else if ((result = findStr(smsLine, F("OK")))) {
#ifdef DEBUG
    if (debugOut != NULL) {
      debugOut->print(F("found OK -- "));
      debugOut->print(F(" haveRequestedIncomingMsg:"));  debugOut->print(haveRequestedIncomingMsg ? 'T' : 'F');
      debugOut->print(F(" sendingResp:"));  debugOut->print(sendingResponse ? 'T' : 'F');
      debugOut->print(F(" sendingResend:"));  debugOut->print(sendingResend ? 'T' : 'F');
      debugOut->print(F(" resendLastResp:"));  debugOut->print(resendLastResponse ? 'T' : 'F');
      debugOut->print(F(" rSMS[0] Null:"));  debugOut->print((responseSMSbuffer[0] == '\0') ? 'T' : 'F');
      debugOut->print(F(" sendingRawdata:"));  debugOut->print(sendingRawdata ? 'T' : 'F');
      debugOut->print(F(" rdSMS[0] Null:"));  debugOut->print((rawdataSMSbuffer[0] == '\0') ? 'T' : 'F');
      debugOut->print(F("\n\n"));
    }
#endif // DEBUG

    // !!!! -------
    // add timer to send \r and then AT\r every 10 sec or so to force found OK incase of errors
    foundOK = true; // set to false by any gprsPrint call so have to wait for call to compete to set true again
    // can do next action if any
    if (deleteReadSmsMsgs) {
      deleteReadSmsMsgs = false;
      //if next expected msgNo is %3==0 then this last msg read was end of a command
      // delete all unread msgs as will be sending response to this one
      if ((currentConnection->msgNo_rx % 3) == 0) { // if end of cmd need to delete ALL incoming msgs
        clearAllSmsMsgs();
      } else { // just clear this one
        clearReadSmsMsgs();
      }
    } else if (haveRequestedIncomingMsg) {
#ifdef DEBUG_INCOMING
      if (debugOut != NULL) {
        debugOut->println(F("haveRequestedIncomingMsg"));
      }
#endif
      // wait for message
    } else if (haveIncomingMsgNo()) {
#ifdef DEBUG_INCOMING
      if (debugOut != NULL) {
        debugOut->println(F("haveIncomingMsgNo so request it"));
      }
#endif
      requestNextIncomingMsg(); // process the next incoming msg
    } else if (responseSMSbuffer[0]) {
#ifdef DEBUG
      if (debugOut != NULL) {
        debugOut->print(F("responseSMSbuffer[0] not null\n"));
      }
#endif // DEBUG
      // encodeSMS has filled output buffer again so send it.
      // this could be sending response, resending response or sending raw data
      startSMS(currentConnection->returnPhNo);

    } else if (sendingResponse) { //returnMsg[0]) {
#ifdef DEBUG
      if (debugOut != NULL) {
        debugOut->print(F("sendingResponse true\n"));
      }
#endif // DEBUG
      resendLastResponse = false; // always
      sendingResend = false; // always
      sendNextSMS(); // send next part

    } else if (sendingResend) { //returnMsg[0]) {
      // finish doing resend before doing next one
#ifdef DEBUG
      if (debugOut != NULL) {
        debugOut->print(F("sendingResend true\n"));
      }
#endif // DEBUG
      resendLastResponse = false; // always
      sendNextSMS(); // send next part

    } else if (resendLastResponse) {
      // note check sendingResend first so that we finish sending last resend before starting another.
#ifdef DEBUG
      if (debugOut != NULL) {
        debugOut->print(F("resendLastResponse true\n"));
      }
#endif // DEBUG
      resendLastResponse = false;
      updateMsgNoForStartResend();
      sendingResend = true; // prevent buffer being changed by write while resending
      txBufferIdx = 0; // start from begining of buffer again
      sendNextSMS(); // resend the last response

    } else if (rawdataSMSbuffer[0]) {
#ifdef DEBUG
      if (debugOut != NULL) {
        debugOut->print(F("responseSMSbuffer[0] not null\n"));
      }
#endif // DEBUG
      // encodeSMS has filled output buffer again so send it.
      // this could be sending response, resending response or sending raw data
      startSMS(currentConnection->returnPhNo);
      sendingRawdata = true; // for > if statement
    }
  } else {  // end of found OK
  }
}

/**
 * true if handling incoming msg or sending respose
 */
boolean pfodSMS_SIM900::isSendingResponse() {
  return (sendingResponse || sendingResend || resendLastResponse || responseSMSbuffer[0]
          || haveRequestedIncomingMsg);
}

void pfodSMS_SIM900::requestIncomingSmsMsg(const char *newMsgNo) {
  // ask of this msg
  if (newMsgNo && *newMsgNo) { // skip nulls and empty msg Nos
    gprsPrint(F("AT+CMGR="));  // <<<<<<< AT_cmd
    gprsPrint(newMsgNo);
    gprsPrint('\r');
    haveRequestedIncomingMsg = true;
  }
}

// SMS encoding
// The actual encoding used is
// if (sixBitSlice == 0) {
// encodedChar = ' ';
// } else if (sixBitSlice == 1) {
// encodedChar = '+';
// } else if (sixBitSlice < (12)) {
// encodedChar = (sixBitSlice-2) + '0'; // 0..9
// } else if (sixBitSlice < (12+26)) {
// encodedChar = (sixBitSlice-12) + 'A';
// } else {
// encodedChar = (sixBitSlice-12-26) + 'a';
// }
// This uses ' ', '+' and 0 to 9 A to Z, a to z,
//
// The reverse decoding is
// if ((encodeChar >= 'a')&& (encodedChar <= 'z')) {
// sixBixSlice = encodedChar - 'a' + 38;
// } else if ((encodedChar >= 'A') && encodedChar <= 'Z')) {
// sixBitSlice = encodedChar - 'A' + 12;
// } else if ((encodedChar >= '0') && encodedChar <= '9')) {
// sixBitSlice = encodedChar - '0' + 2;
// } else if (encodedChar == '+') {
// sixBitSlice = 1;
// } else if (encodedChar == ' ') {
// sixBitSlice = 0;
// } else {
//  invalid return 0xff
// }
// sixBitSlice = 0x3F & sixBitSlice; // clear upper bits
//
// 117 UTF-8 bytes + 4 chars for connectionNo and seqNo.
// This implies 117*8/6 = 156 bytes in Base64 encoding
// + 4 bytes for seqNo and block No = 160 bytes max per message

/**
 * decodes encoded char to 6 bit byte
 * Returns 0xff if encode char is invalid
 * else returns (0x3f & decoded char)
 */
byte pfodSMS_SIM900::encodeSMSChar(byte c) {
  c &= 0x3f; // only handle lower 6 bits
  if (c == 0) {
    return (' ');
  } else if (c == 1) {
    return ('+');
  }
  if (c < 12) {
    return ('0' + (c - 2));
  } else if (c >= 12 && c < 38) {
    return ('A' + (c - 12));
  } //else {
  return ('a' + (c - 38));
}

/**
 * encode upto 117 UTF-8 bytes into 156 bytes + 4 byte seqNo + blockNo
 * encodedSMS must be at least 161 long to allow for null
 *
 * @param connectionNo
 * @param msgNo   tx msg no
 * @param msgBytes
 * @return updated index into output msg
 *   encodedBytes are padded with zero bits to make up 4 6bit blocks
 */
size_t pfodSMS_SIM900::encodeSMS(int connectionNo, int msgNo, const byte* msgBytes, size_t msgBytesIdx, char* encodedSMS) {
#ifdef DEBUG
  if (debugOut != NULL) {
    debugOut->print(F(" encodeSMS ---------------------  "));
    debugOut->print(F(" connectionNo:"));  debugOut->print(connectionNo);
    debugOut->print(F(" msgNo:"));  debugOut->print(msgNo);
    debugOut->print(F(" msgBytesIdx:"));  debugOut->print(msgBytesIdx);
  }
#endif // DEBUG
  size_t bytesToConvertLen = strlen(((const char*) msgBytes) + msgBytesIdx);
#ifdef DEBUG
  if (debugOut != NULL) {
    debugOut->print(F(" strlen:"));  debugOut->print(bytesToConvertLen);
  }
#endif // DEBUG
  size_t returnIdx = msgBytesIdx;
  encodedSMS[0] = '\0'; // terminate converted chars
  if (bytesToConvertLen == 0) {
#ifdef DEBUG
    if (debugOut != NULL) {
      debugOut->print(F(" Nothing to encode\n"));
      debugOut->println();
    }
#endif // DEBUG
    return returnIdx;
  }
  int padding = 0;
  if (bytesToConvertLen > MAX_BYTES_TO_CONVERT) {
    bytesToConvertLen = MAX_BYTES_TO_CONVERT; // full msg
  } else if (bytesToConvertLen == MAX_BYTES_TO_CONVERT) {
    // note should limit raw data to 116 so get a null
    // last SMS is completely full AND not raw data msg,  move 3 bytes to next SMS
    bytesToConvertLen = MAX_BYTES_TO_CONVERT - 3;
    // NOTE have to move exactly 3 bytes to next msg so that this msg is NOT padded with nulls
  } else {
    // last msg not full
    // bytesToConvertLen < MAX_BYTES_TO_CONVERT
    padding = 3 - (bytesToConvertLen % 3); // remainder 0,1 or 2 -> 3,2,1 nulls to pad
  }
  strncpy_safe((char*) bytesToConvert, ((const char*) msgBytes) + msgBytesIdx, bytesToConvertLen);
  returnIdx += bytesToConvertLen; // set up return idx

  // now padd with nulls if needed
  for (int i = 0; i < padding; i++) {
    bytesToConvert[bytesToConvertLen++] = '\0';
  }

#ifdef DEBUG
  if (debugOut != NULL) {
    debugOut->print(F(" bytesToConvertLen:"));
    debugOut->print(bytesToConvertLen);
    //debugOut->println();
  }
#endif // DEBUG
  // bytesToConvertLen is now length to be converted
  size_t idx = 0;
  idx = encodeInt(msgNo, encodedSMS, idx);
  idx = encodeInt(connectionNo, encodedSMS, idx);

  // take the bytes in sets of 3 and convert to 4 chars
  size_t bytesToConvertIdx = 0;
  while (bytesToConvertIdx < bytesToConvertLen) {
    // use 0xff to clear any extended sign
    long three8Bytes = 0;
    three8Bytes += 0xff0000 & (((long) (bytesToConvert[bytesToConvertIdx++])) << 16);
    three8Bytes += 0xff00 & (((long) (bytesToConvert[bytesToConvertIdx++])) << 8);
    three8Bytes += 0xff & ((long) (bytesToConvert[bytesToConvertIdx++]));
    // now fill in the 4 6bit bytes
    idx = convertThree8Bytes(three8Bytes, encodedSMS, idx);
  }
  encodedSMS[idx] = '\0'; // terminate converted chars
#ifdef DEBUG
  if (debugOut != NULL) {
    debugOut->print(F("  encodedSMS msg size:"));
    debugOut->print(idx);
    debugOut->println();
  }
#endif // DEBUG
  return returnIdx;
}

/**
 * encode an two byte int as two chars
 */
size_t pfodSMS_SIM900::encodeInt(int number, char*rtn, size_t idx) {
  rtn[idx++] = encodeSMSChar(number >> 6);
  rtn[idx++] = encodeSMSChar(number);
  return idx;
}

/**
 * convert at most 24 bits into 4 6bit bytes starting at idx
 *
 * @param number
 * @param rtn
 * @param idx
 *            start idx for placing converted number
 * @return next char idx
 */
size_t pfodSMS_SIM900::convertThree8Bytes(long three8Bytes, char*rtn, size_t idx) {
  rtn[idx++] = encodeSMSChar(three8Bytes >> 18);
  rtn[idx++] = encodeSMSChar(three8Bytes >> 12);
  rtn[idx++] = encodeSMSChar(three8Bytes >> 6);
  rtn[idx++] = encodeSMSChar(three8Bytes);
  return idx;
}

/**
 * decodes encoded char to 6 bit byte
 * Returns 0xff if encode char is invalid
 * else returns (0x3f & decoded char)
 */
byte pfodSMS_SIM900::decodeSMSChar(byte encodedChar) {
  byte rtn = 0;
  if (encodedChar & 0x80) {
    return 0xff; // error -ve byte
  }
  if ((encodedChar >= (byte) 'a') && (encodedChar <= (byte) 'z')) {
    rtn = (encodedChar - (byte) 'a') + 38; // 12+26
  } else if ((encodedChar >= (byte) 'A') && (encodedChar <= (byte) 'Z')) {
    rtn = (encodedChar - (byte) 'A') + 12;
  } else if ((encodedChar >= (byte) '0') && (encodedChar <= (byte) '9')) {
    rtn = (encodedChar - (byte) '0') + 2;
  } else if (encodedChar == (byte) '+') {
    rtn = 1;
  } else if (encodedChar == (byte) ' ') {
    rtn = 0;
  } else {
    // invalid
    return 0xff;
  }
  return (0x3F & rtn); // clear upper bits
}

/**
 *  decode the next two chars as an int
 *  return 0 if invalid char
 */
int pfodSMS_SIM900::decodeInt(const char* smsChars, size_t idx) {
  int rtn = 0;
  byte dc = decodeSMSChar(smsChars[idx++]);
  if (dc == 0xff) {
    return 0; // invalid
  } // else leading bit is always 0
  rtn += (((int) dc) << 6);
  dc = decodeSMSChar(smsChars[idx++]);
  if (dc == 0xff) {
    return 0; // invalid
  }
  rtn += dc;
  return rtn;
}

/**
 * Compare current to new connection number (mod 4095), only call this for same phone numbers
 * Returns: true if new connection number is greater (mod 4095), else false for == or <=
 *
 * if the SMS connection number is not equal to the current connection number and both have same phone number,
 *   the following algorithm used to compare them:-
 * If the SMS connection number less than current connection number
 * then the Adjusted SMS connection number == SMS connection number + 4095
 * else the Adjusted SMS connection number == SMS connection number
 * Then if 0 < (Adjusted SMS connection number - current connection number) <= 2048,
 *   then this SMS connection number is taken as being greater then the current connection number,
 *   otherwise it is taken as being less then the current connection number.
 * This ensures that any messages from the previous 2047 connections will be ignored.
 */
boolean pfodSMS_SIM900::newConnectionNumberGreater(int current, int next) {
  if ((next == 0) && (current != 0)) {
    return true; // zero connectionNo is always greater if current is != 0
  }
  // else
  if (next < current) { // current == new does not add 4095
    next += 4095;
  }
  if (next <= current) { // this handles current == new
    return false;
  }
  // else
  if ((next - current) <= 2048) {
    return true;
  } // else
  return false;
}

boolean pfodSMS_SIM900::validSMSmsgLen(const char* smsChars) {
  size_t smsCharLen = strlen(smsChars);
  if ((smsCharLen == 0) || (smsCharLen % 4 != 0) || (smsCharLen > 160)) {
    // not a valid msg
#ifdef DEBUG
    if (debugOut != NULL) {
      debugOut->print(F("Invalid SMS msg length:"));
      debugOut->print((int) smsCharLen);
      debugOut->println();
    }
#endif // DEBUG
    return false; // empty decode
  }  // else
  return true;
}

void pfodSMS_SIM900::printIgnoreOutOfSequence() {
#ifdef DEBUG
  if (debugOut != NULL) {
    debugOut->print(F(" Ignore out-of-seq\n"));
  }
#endif // DEBUG
}

void pfodSMS_SIM900::setNextExpectedConnectionMsgNo(connection *con, boolean foundEndParens) {
  // set rx msg no for nextConnection.
  con->msgNo_rx += 1; // next expected
  if (foundEndParens) {
#ifdef DEBUG
    if (debugOut != NULL) {
      // debugOut->print(F(" incrementMsgNo to % 3 == 0\n"));
    }
#endif // DEBUG
    if (con->msgNo_rx % 3 != 0) {
      // make it so for this msg
      con->msgNo_rx = ((con->msgNo_rx / 3) + 1) * 3;
    }
  }
}

/**
 * this is only called after setting nextConnection phoneNo to number of incoming SMS
 *
 * decode sms into decodedSMS byte array
 * pointer to current length
 * On entry msgNo_rx == expected msgNo (msgNo_rx%3 == 0 if last sms contained } )
 *
 * Return:
 *
 * decodedSMS is update with msg, if any
 * and currentDecodedLen updated to reflect chars added, if any
 * Globals Updated:
 * resendLastResponse set to true if last response to be sent again
 * ...
 *
 * Note: decodeSMS buffer must be at least SMS_RX_BUFFER_SIZE+1 long
 */
pfodSMS_SIM900::decode_sms_return_t pfodSMS_SIM900::decodeSMS(connection *current, connection *next, const char* smsChars,
    byte* decodedSMS, size_t* currentDecodedLen) {
#ifdef DEBUG
  if (debugOut != NULL) {
    debugOut->print(F(" decodeSMS ---------------------  "));
  }
#endif // DEBUG
  //size_t initialDecodeLen = *currentDecodedLen;
  decodedSMS[*currentDecodedLen] = '\0';

  if (!validSMSmsgLen(smsChars)) { // checked
    return IGNORE; // ignore this msg completly nothing added to buffer
  }
  if (next->returnPhNo[0] == '\0') {
    // no return phone number, blocked??
#ifdef DEBUG
    if (debugOut != NULL) {
      debugOut->print(F("Return PhNo empty. Ignoring SMS msg\n"));
    }
#endif // DEBUG
    return IGNORE; // ignore this msg completly nothing added to buffer
  }
  if (!phoneNoHas6Digits(next->returnPhNo)) {
#ifdef DEBUG
    if (debugOut != NULL) {
      debugOut->print(F("Return PhNo does not have at least MAX_PHNO_CHECK digits. Ignoring SMS msg\n"));
    }
#endif // DEBUG
    return IGNORE; // ignore this msg completly nothing added to buffer
  }


  int expectedMsgNo = current->msgNo_rx;
  int currentConnectionNo = current->connectionNo; // may be 0 if current phoneNo empty

  // On entry msgNo_rx == expected msgNo and msgNo_rx%3 == 0 if last sms contained }
  // in which case first char must be {

  // get here if have atleast 4 chars in sms msg
  size_t smsCharIdx = 0;
  int smsMsgNo = decodeInt(smsChars, smsCharIdx); // new connection must start with space space
  smsCharIdx += 2;
  int smsConnectionNo = decodeInt(smsChars, smsCharIdx);
  smsCharIdx += 2;
#ifdef DEBUG
  if (debugOut != NULL) {
    debugOut->print(F(" smsConnectionNo:"));
    debugOut->print(smsConnectionNo);
    debugOut->print(F(" smsMsgNo: "));
    debugOut->print(smsMsgNo);
    debugOut->println();
  }
#endif // DEBUG
  // set nextConnection with this connectionNo and next expected msgNo , will check for } later
  next->connectionNo = smsConnectionNo; // for the return
  next->msgNo_rx = 0; // will be incremented below if new connection

  boolean samePhoneNo = phoneNoMatchesCurrent(next->returnPhNo);
  int knownIdx = findMatchingKnownPhoneNo(next->returnPhNo);  // -1 if not found

  //now parse
  size_t initialDecodeLen = *currentDecodedLen; // default value if returning empty msg
  boolean foundEndParens = decodeSMScmd(smsChars, decodedSMS, currentDecodedLen);
#ifdef DEBUG
  if (debugOut != NULL) {
    debugOut->print(F("decodeSMScmd new Len:"));
    debugOut->println((*currentDecodedLen) - initialDecodeLen);
    debugOut->print(F("decodeSMScmd '"));
    debugOut->print((const char*)decodedSMS + initialDecodeLen);
    debugOut->print(F("'"));
  }
#endif // DEBUG
  if (initialDecodeLen == *currentDecodedLen) { // checked
    // empty msg i.e. invalid // do not update
#ifdef DEBUG
    if (debugOut != NULL) {
      debugOut->print(F("Ignoring invalid SMS msg\n"));
    }
#endif // DEBUG
    return IGNORE;
  }

  if (knownIdx >= 0) { // found a know connection
#ifdef DEBUG
    if (debugOut != NULL) {
      debugOut->print(F("Known Connection: "));
    }
#endif // DEBUG
    printKnownConnection (knownArrayPtrs[knownIdx]);
    // check special case of first connection
    if ((smsConnectionNo == 0) && (smsMsgNo == 0)) {
      // clear the connection to let a new one start from this phone
#ifdef DEBUG
      if (debugOut != NULL) {
        debugOut->print(F("Erasing ..."));
      }
#endif // DEBUG
      clearKnownConnection(knownIdx);
      knownIdx = -1;
      if (samePhoneNo) {
        // phoneNo matches current
        clearConnection(current); // clear it // sets connectionNo to 0
        samePhoneNo = false; // start again from beginning
      }
    }
  } else {
#ifdef DEBUG
    if (debugOut != NULL) {
      debugOut->print(F("Unknown PhoneNo: "));
    }
#endif // DEBUG
  }

  // note if samePhoneNo then must have knownIdx ==0
  if (samePhoneNo && (knownIdx != 0)) {
#ifdef DEBUG
    if (debugOut != NULL) {
      debugOut->print(F("PhoneNo matches current but not found in knownConnections\n"));
    }
#endif // DEBUG
    // fix this
    int emptyIdx = findEmptyKnownConnection(); // always works
    moveKnownConnectionToTop(emptyIdx);
    setTopKnownConnectionFromCurrent(); // fill in from current
  }

  // note if currentPhNo matches we are just clearing the previous knownConnectionInfo
  // connectionNo 0 will ALWAYS be greater then any non-zero connectionNo

  // else
#ifdef DEBUG_DECODED_MSG
  if (debugOut != NULL) {
    debugOut->print(F("DecodedSMS:'"));
    debugOut->print((char*) (decodedSMS + initialDecodeLen));
    debugOut->print(F("'"));
    debugOut->println();
  }
#endif // DEBUG_DECODED_MSG
  // NOTE: cannot check for } in short SMS as may be breaking up to get } and hash in same msg

  boolean foundStartParens = (decodedSMS[initialDecodeLen] == '{');
  boolean foundCloseMsg = false;
  if (((*currentDecodedLen - initialDecodeLen) >= 3) && foundStartParens) {
    if ((decodedSMS[initialDecodeLen + 1] == '!') && (decodedSMS[initialDecodeLen + 2] == '}')) {
      foundCloseMsg = true;
    }
  }

#ifdef DEBUG
  if (debugOut != NULL) {
    debugOut->print(F(" foundEndParens:"));  debugOut->print(foundEndParens ? 'T' : 'F');
    debugOut->print(F(" foundStartParens: "));  debugOut->print(foundStartParens ? 'T' : 'F');
    debugOut->print(F(" foundCloseMsg: "));  debugOut->print(foundCloseMsg ? 'T' : 'F');
    debugOut->print(F(" samePhoneNo: "));  debugOut->print(samePhoneNo);
    debugOut->print(F(" knownConnectionIdx: "));  debugOut->print(knownIdx);
    debugOut->println();
  }
#endif // DEBUG
  // ok now have expectedMsgNo, smsMsgNo, currentConnectionNo, smsConnectionNo
  // and start and end paren booleans and closeMsg boolean
  // and samePhoneNo boolean;
  if ((smsMsgNo % 3 == 0) && (!foundStartParens)) { // checked
#ifdef DEBUG
    if (debugOut != NULL) {
      debugOut->print(F(" Ignore StartNewMsg but not starting with {\n"));
    }
#endif // DEBUG
    return IGNORE;
  }

  // common case is same phone and connection No
  // NOTE if connectionNo == 0 and msgNo == 0 then will NOT come through here
  //
  // will go to PhoneNo does not match and Did Not find in KnownConnections
  //   if phoneNo matched current, current was cleared so TIMED_OUT and phoneNo empty
  //  then will goto connection timed out else
  // else depends on current connection

  if (samePhoneNo) {
    // here knownIdx == 0 from above
    if (currentConnectionNo == smsConnectionNo) {
      if (smsMsgNo == 0) { // if currentConnectNo matches then still have existing connection so == 0 is not valid new msg
        if ((smsMsgNo + 3) == expectedMsgNo) {
          return RESEND_LAST_RESPONSE;
        } //else {  (smsMsgNo + 3) != expectedMsgNo)
        // ignore this msg
        printIgnoreOutOfSequence();
        return IGNORE;
      } //  else
      if ((smsMsgNo % 3) == 0) { // smsMsgNo != 0
        if (smsMsgNo == expectedMsgNo) {
          // will process this msg
          // update expectedMsgNo
          setNextExpectedConnectionMsgNo(current, foundEndParens);
          if (foundCloseMsg) {
#ifdef DEBUG
            if (debugOut != NULL) {
              debugOut->print(F(" closeConnection\n"));
            }
#endif // DEBUG
            return CLOSE_CONNECTION;
          }
          // else
          return START_NEW_COMMAND;
        }
        // else { // smsMsgNo %3 ==0 and  != expected
        if ((smsMsgNo + 3) == expectedMsgNo) {
          return RESEND_LAST_RESPONSE;
        } //else {  (smsMsgNo + 3) != expectedMsgNo)
        // ignore this msg
        printIgnoreOutOfSequence();
        return IGNORE;
      }
      //else { // smsMsgNo % 3 != 0
      if (smsMsgNo == expectedMsgNo) {
        setNextExpectedConnectionMsgNo(current, foundEndParens);
        return PROCESS_NEXT_PART_OF_COMMAND;
      } //else {  // != expected
      printIgnoreOutOfSequence();
      return IGNORE;
    }

    if (!newConnectionNumberGreater(currentConnectionNo, smsConnectionNo)) {
#ifdef DEBUG
      if (debugOut != NULL) {
        debugOut->print(F(" ConnectionNo less then current (same phoneNo)\n"));
      }
#endif // DEBUG
      return IGNORE;
    }

    // else { // sms connectionNo is greater
    if (smsMsgNo == 0) {
      setNextExpectedConnectionMsgNo(next, foundEndParens); // new connection
      if (foundCloseMsg) {
#ifdef DEBUG
        if (debugOut != NULL) {
          debugOut->print(F(" close new Connection from same phoneNo\n"));
        }
#endif // DEBUG
        return CLOSE_CONNECTION_AND_UPDATE_CONNECTION; // close current connection and update connectionNo and process {!} msg
      }
      //else { // start new connection with same number
#ifdef DEBUG
      if (debugOut != NULL) {
        debugOut->print(F(" startNewConnection (same phoneNo)\n"));
      }
#endif // DEBUG
      return NEW_CONNECTION_SAME_PHONE_NO; // trigger swap in connections

    }
    //else { // != 0 ignore
    printIgnoreOutOfSequence();
    return IGNORE;
  }
  // end same phoneNo

  //else { // different phoneNo
  // if current cleared above knownIdx set to -1 so this test not called
  if ((knownIdx >= 0) && (!newConnectionNumberGreater(knownArrayPtrs[knownIdx]->connectionNo, smsConnectionNo))) {
    // known and not > so ignore
    printIgnoreOutOfSequence();
    return IGNORE;
  }
  // either unknown or >= known
  if (!(current->timedOut)) {
    if (smsMsgNo == 0) {
#ifdef DEBUG
      if (debugOut != NULL) {
        debugOut->print(F(" Ignore new connection from different phone as current not timed out\n"));
      }
#endif // DEBUG
      return IGNORE_CONNECTION;  // ignore this attempted connection but allow it to try again later
      // so do not update connection No.
    } // else
    printIgnoreOutOfSequence();
    return IGNORE;
  }
  // else current timed out  // get here is current connection cleared due to 0,0 msg from same phoneNo
  if (smsMsgNo == 0) {
    setNextExpectedConnectionMsgNo(next, foundEndParens); // new connection
    if (foundCloseMsg) {
#ifdef DEBUG
      if (debugOut != NULL) {
        debugOut->print(F(" New connection closeConnection request from different PhoneNo\n"));
      }
#endif // DEBUG
      return START_NEW_CONNECTION_AND_CLOSE; // close current connection and process {!} msg
      // very unlikely to happen, normal new connection {.} will be first msg
    }
    //else {
    // start a new connection
#ifdef DEBUG
    if (debugOut != NULL) {
      debugOut->print(F(" Start new connection from different PhoneNo\n"));
    }
#endif // DEBUG
    return START_NEW_CONNECTION; // trigger swap in connections
    // need to send alert to current phoneNo
  } // else
  printIgnoreOutOfSequence();
  return IGNORE;
}

//* Note: decodeSMS buffer must be at least SMS_RX_BUFFER_SIZE+1 long
// returns true if found '}' can check first char is '{' on return
boolean pfodSMS_SIM900::decodeSMScmd(const char*smsChars, byte*decodedSMS, size_t* currentDecodedLen) {
  // start at index 4 skipping connectionNo and msgNo
  size_t initialLength = *currentDecodedLen;
  boolean rtn = false; // '}' not found
  decodedSMS[initialLength] = '\0';
  size_t decodedSMSidx = 0;
  size_t smsCharLen = strlen(smsChars);
  if ((smsCharLen == 0) || (smsCharLen % 4 != 0) || (smsCharLen > 160)) {
    // not a valid msg pfodApp does not send empty cmds
#ifdef DEBUG
    if (debugOut != NULL) {
      debugOut->print(F("Invalid SMS msg length:"));
      debugOut->print((int) smsCharLen);
      debugOut->println();
    }
#endif // DEBUG
    return false; // empty decode
  }
  // get here if have atleast 4 chars in sms msg
  size_t smsCharIdx = 4;

  // get here if msgNo is the expected one  do not increment msg no until parse valid msg
  // now convert the rest of the sms msg
  size_t decodeLen = *currentDecodedLen;
  while (smsCharIdx < smsCharLen) {
    long four6bitBytes = 0; // 4 bytes for a long in arduino
    byte dc = decodeSMSChar(smsChars[smsCharIdx++]);
    if (dc == 0xff) {
      decodedSMS[initialLength] = '\0';
      return false; // invalid
    } // else leading bit is always 0
    four6bitBytes += 0xfc0000 & (((long) dc) << 18);

    dc = decodeSMSChar(smsChars[smsCharIdx++]);
    if (dc == 0xff) {
      decodedSMS[initialLength] = '\0';
      return false; // invalid
    } // else leading bit is always 0
    four6bitBytes += 0x3f000 & (((long) dc) << 12);

    dc = decodeSMSChar(smsChars[smsCharIdx++]);
    if (dc == 0xff) {
      decodedSMS[initialLength] = '\0';
      return false; // invalid
    } // else leading bit is always 0
    four6bitBytes += 0xfc0 & (((long) dc) << 6);

    dc = decodeSMSChar(smsChars[smsCharIdx++]);
    if (dc == 0xff) {
      decodedSMS[initialLength] = '\0';
      return false; // invalid
    } // else leading bit is always 0
    four6bitBytes += (0x3f) & ((long) dc);
    byte b = (byte)(0xff & (four6bitBytes >> 16));
    if ((b == '\0') || ((++decodeLen) >= SMS_RX_BUFFER_SIZE)) {
      break; // stop on first null
    }
    rtn |= (b == (byte) '}');
    decodedSMS[decodedSMSidx++] = b;
    b = (byte)(0xff & (four6bitBytes >> 8));
    if ((b == '\0') || ((++decodeLen) >= SMS_RX_BUFFER_SIZE)) {
      break; // stop on first null
    }
    rtn |= (b == (byte) '}');
    decodedSMS[decodedSMSidx++] = b;
    b = (byte)(0xff & (four6bitBytes));
    if ((b == '\0') || ((++decodeLen) >= SMS_RX_BUFFER_SIZE)) {
      break; // stop on first null
    }
    rtn |= (b == (byte) '}');
    decodedSMS[decodedSMSidx++] = b;
  }
  // finished decoding
  // update current length
  *currentDecodedLen = decodeLen;
  decodedSMS[decodedSMSidx++] = '\0';  // terminate  Note UTF-8 does not have nulls in extended char points
  return rtn;  // true if found closing '}' else false;
}

/**
 * must process last msg BEFORE calling this again
 */
void pfodSMS_SIM900::pollSMSboard() {
  // update timer
  unsigned long mS = millis();
#ifdef SHOW_TIMEOUT_TIMER
  if (debugOut != NULL) {
    unsigned long msSec = mS % 20000;
    if (msSec == 0) {
      debugOut->print(F("timedOutTimer:"));
      debugOut->print(timedOutTimer);
      debugOut->print(F(" start_mS:"));
      debugOut->print(start_mS);
      debugOut->print(F(" mS:"));
      debugOut->print(mS);
      debugOut->print(F(" mS - start_mS:"));
      debugOut->print((mS - start_mS));
      debugOut->println();
    }
  }
#endif
  if (timedOutTimer == 0) {
    currentConnection->timedOut = true;
  } else {
    if ((mS - start_mS) > timedOutTimer) {
      timedOutTimer = 0;
      currentConnection->timedOut = true;
#ifdef DEBUG_TIMEOUT_TIMER
      if (debugOut != NULL) {
        debugOut->print(F("!!! --- current Connection timedOut --- !!!\n"));
      }
#endif // DEBUG
    }
  }

  const char* gprsLine = collectGprsMsgs();
  if (foundOK) {
    requestNextIncomingMsg(); // if any
  }
  if (gprsLine) {
#ifdef DEBUG
    if (debugOut != NULL) {
      // print line
      // debugOut->print(gprsLine);
      // debugOut->println();
    }
#endif // DEBUG
    processGprsLine(gprsLine);
    // finished with line
    clearGprsLine();
  }
}

void pfodSMS_SIM900::requestNextIncomingMsg() {
  if (haveRequestedIncomingMsg) {
    return; // already processing one
  }
  if (haveIncomingMsgNo()) {
    const char* newMsgNo = getIncomingMsgNo();
#ifdef DEBUG_INCOMING
    if (debugOut != NULL) {
      debugOut->print(F("request next incomingMsgNo:"));
      debugOut->print(newMsgNo);
      debugOut->println();
    }
#endif // DEBUG
    requestIncomingSmsMsg(newMsgNo);
  }
}

const char* pfodSMS_SIM900::collectGprsMsgs() {
  boolean lineEnd = false;
  while ((!lineEnd) && (gprs->available())) {
    int c = gprs->read();
    if (c == '\n') {
      lineEnd = true;
      if (expectingMessageLines) { // never have /r /n in message lines
        break;
      }
      c = ']';
    }
    if (c == '\r') {
      if (expectingMessageLines) { // never have /r /n in message lines
        lineEnd = true;
        break;
      }
      c = '[';
    }
    gprsMsg[gprsMsgIdx++] = c;   // writing data into array
    if ((gprsMsgIdx == MAX_MSG_LEN) || (gprsLastChar == '>' && c == ' ' && !expectingMessageLines)) {
      lineEnd = true;
    }
    gprsLastChar = c;
  }
  // end of while either no more avail or lineEnd
  if (lineEnd) {
#ifdef DEBUG
    if (debugOut != NULL) {
      //  debugOut->print(F("line end "));
      //  debugOut->print(gprsMsgIdx);
      //  debugOut->println();
    }
#endif // DEBUG
    gprsMsg[gprsMsgIdx] = '\0';   // terminate string
    return gprsMsg;
  } //else {
  return NULL;
}

void pfodSMS_SIM900::clearGprsLine() {
#ifdef DEBUG
  if (debugOut != NULL) {
    // consolePrint(F("clearGprsLine"));
    // consolePrintln();
  }
#endif // DEBUG
  gprsLastChar = '\0';
  gprsMsgIdx = 0;
  gprsMsg[0] = '\0';
}


/**
 * call this if phoneNo does not match currentConnection
 * on return if currentConnection not timedout replace with this connection
 * returns -1 for invalid
 * 0 for valid but not new connection (i.e. msgNo not zero) // send raw data msg if alreay connected
 * new connectionNo if valid and msgNo is zero  // send close connection if already connected
 */
int pfodSMS_SIM900::checkSMSmsgForValidNewConnection(int connectionNo, int msgNo, const char* decodedSMS,
    boolean foundCmdClose) {
  if ((connectionNo > 0) && foundCmdClose && (decodedSMS[0] == (byte) '{')) {
    if (msgNo == 0) {
      return connectionNo;
    } else {
      return 0; // not a new connection
    }
  } //else {
  return -1;
}


// to send responses NOT raw data
void pfodSMS_SIM900::sendNextSMS() {
#ifdef DEBUG
  if (debugOut != NULL) {
    debugOut->print(F("sendNextSMS : "));
    debugOut->print(F(" txBufferIdx:"));  debugOut->print(txBufferIdx);
    debugOut->print(F(" txBufferLen:"));  debugOut->print(txBufferLen);
    debugOut->println();
  }
#endif // DEBUG
  if (txBufferIdx >= txBufferLen) {
#ifdef DEBUG
    if (debugOut != NULL) {
      debugOut->print(F("  sendNextSMS nothing to do \n"));
    }
#endif // DEBUG
    return;  // nothing to do
  }
#ifdef DEBUG
  if (debugOut != NULL) {
    //  debugOut->print(F("txBufferIdx:"));
    //  debugOut->print(txBufferIdx);
    //  debugOut->println();
  }
#endif // DEBUG
  txBufferIdx = encodeSMS(currentConnection->connectionNo, currentConnection->msgNo_tx++, (const byte*) txBuffer,
                          txBufferIdx, (char *) responseSMSbuffer);
#ifdef DEBUG
  if (debugOut != NULL) {
    //  debugOut->print(F("  after encodeSMS txBufferIdx:"));
    //  debugOut->print(txBufferIdx);
    //  debugOut->println();
  }
#endif // DEBUG
  if (foundOK) {
    // gprs not processing something else
    processGprsLine("OK");  // force start of send by responseSMSbuffer[0] != '\0'
  }
}


/**
 * call this often
 * returns number of bytes left to read from SMS receive buffer
 */
int pfodSMS_SIM900::available(void) {
  pollSMSboard();
  return (rxBufferLen - rxBufferIdx);
}

int pfodSMS_SIM900::peek(void) {
  if (available()) {
    return (0xff & ((int) rxBuffer[rxBufferIdx]));
  } else {
    return -1;
  }
}

int pfodSMS_SIM900::read(void) {
  // if the head isn't ahead of the tail, we don't have any characters
  if (available()) {
    int b = ((int) rxBuffer[rxBufferIdx++]);
    if (rxBufferIdx == rxBufferLen) {
      // buffer emptied
      rxBufferIdx = 0;
      rxBufferLen = 0;
    }
    return (0xff & b);
  }
  return -1; // nothing to return this will reset parser!!
}

// make this private
void pfodSMS_SIM900::flush() {
#ifdef DEBUG_FLUSH
  if (debugOut != NULL) {
    debugOut->print(F("txBufferIdx:"));  debugOut->print(txBufferIdx);
    debugOut->print(F(" txBufferLen:"));  debugOut->print(txBufferLen);
    debugOut->println();
  }
#endif // DEBUG_FLUSH

  // also send sms if have 117 8bit bytes
  // or starting reply i.e. {
  // or ending reply }
  // generally only use flush to send rawData msgs
  txBuffer[txBufferIdx] = '\0';
#ifdef DEBUG_FLUSH
  if (debugOut != NULL) {
    debugOut->print(F("flush :'"));
    debugOut->print((char*) txBuffer);
    debugOut->print(F("'\n"));
  }
#endif // DEBUG_FLUSH
  txBufferLen = txBufferIdx;
  txBufferIdx = 0;
  sendingResponse = true;
  sendingResend = false;
  responseSet = true;  // prevent another response until new cmd received
  updateMsgNoForStartNewCommand();
  sendNextSMS();
}

void pfodSMS_SIM900::clearRawdataTxBuffer() {
  rawdataTxBufferLen = 0;
  rawdataTxBufferIdx = 0;
}

/**
 * This depends on all the raw data being written in one loop
 * otherwise get a bit of the last one
 * if no newLine then just send when buffer full.
 */
size_t pfodSMS_SIM900::writeRawData(uint8_t c) {
  if (currentConnection->returnPhNo[0] == '\0') {
    // clear all buffers and do not write
    clearRawdataTxBuffer();
    return 1; // skip this one
  }
  if ((rawdataTxBufferLen != 0) || rawdataSMSbuffer[0] || isSendingResponse()) {
#ifdef DEBUG_RAWDATA
    if (debugOut != NULL) {
      if (c == '\n') {
        debugOut->println(F(" writeRawData waiting to send last buffer"));
      }
    }
#endif // DEBUG
    return 1;
  } // else
#ifdef DEBUG_RAWDATA
  if (debugOut != NULL) {
    // debugOut->print(F("writeRawData("));
    // debugOut->print((char)c);
    // debugOut->print(F(")"));
  }
#endif // DEBUG
  rawdataTxBuffer[rawdataTxBufferIdx++] = c;
  if ((c == '\n') || (rawdataTxBufferIdx >= SMS_RAWDATA_TX_BUFFER_SIZE)) {
    // end of line flush this data
    flushRawData(); // this sets rawdataTxBufferLen and encodes result
    // seting rawdataTxBufferLen prevents any furthe raw data being written
#ifdef DEBUG_RAWDATA
    if (debugOut != NULL) {
      debugOut->println(F("\nflush RawData"));
    }
#endif // DEBUG
  }
  return 1;
}

void pfodSMS_SIM900::flushRawData() {
  rawdataTxBufferLen = rawdataTxBufferIdx; // this triggers send on OK if rawdataTxBufferLen > 0
  rawdataTxBufferIdx = 0;
#ifdef DEBUG_RAWDATA
  if (debugOut != NULL) {
    debugOut->write((const char*)rawdataTxBuffer, rawdataTxBufferLen);
    debugOut->println();
  }
#endif // DEBUG
  rawdataTxBufferIdx = encodeSMS(0, 0, (const byte*) rawdataTxBuffer, 0, (char *) rawdataSMSbuffer);
  // non null rawdataSMSbuffer[0] triggers send
#ifdef DEBUG_RAWDATA
  if (debugOut != NULL) {
    debugOut->print(F("Encoded RawData:"));
    debugOut->print((char *) rawdataSMSbuffer);
    debugOut->println(F("'"));
  }
#endif
  rawdataTxBufferLen = 0;
  rawdataTxBufferIdx = 0;
  if (foundOK) {
    // gprs not processing something else
    processGprsLine("OK");  // force start of send
  }
}


size_t pfodSMS_SIM900::write(uint8_t c) {
#ifdef DEBUG_WRITE
  if (debugOut != NULL) {
    debugOut->print(F("write("));
    debugOut->print((char)c);
    debugOut->print(F(")"));
  }
#endif // DEBUG

  // check for start of reply if so drop any other data in buffer
  // check for end of reply if so send
  // check for len == 117 and if rawData if so send
  // else just add to buffer
  if ((!sendingResponse) && (currentConnection->returnPhNo[0] == '\0')
      && (!sendingResend)) {
    // clear all buffers and do not write
    clearTxBuffer();
    return 1; // skip this one
  }

  if (sendingResponse || sendingResend || resendLastResponse) {
    // do not save this c
    return 1;
  }
  if (responseSet) {
#ifdef DEBUG
    if (debugOut != NULL) {
      debugOut->print(F("responseSet return"));
    }
#endif // DEBUG
    return 1; // waiting for next command
  }

  txBuffer[txBufferIdx++] = (byte) c;
  // flush on  buffer full
  if ((txBufferIdx >= SMS_TX_MSG_SIZE)) { // || ((byte) c == '}')) {  do not flush on } let parser do this
    flush();
  }
  return 1;
}

// will return pointer to null if match at end of string
// otherwise returns pointer to next char AFTER the match!!
const char* pfodSMS_SIM900::findStr(const char* msg, const __FlashStringHelper *ifsh) {
  const int MAX_LEN = 16;
  char str[MAX_LEN]; // allow for null
  getProgStr(ifsh, str, MAX_LEN);
  return findStr(msg, (char*) str);
}

// will return pointer to null if match at end of string
// otherwise returns pointer to next char AFTER the match!! if found
// else return NULL if NOT found
const char * pfodSMS_SIM900::findStr(const char* str, const char* target) {
  if (str == NULL) {
    return NULL;
  }
  if (target == NULL) {
    return NULL;
  }
  size_t targetLen = 0;
  size_t index = 0;
  size_t strIdx = 0;
  targetLen = strlen(target);
  if (targetLen == 0) {
    return str;
  }
  index = 0;
  char c;
  while ((c = str[strIdx++])) { // not end of null
    if (c != target[index]) {
      index = 0; // reset index if any char does not match
    } // no else here
    if (c == target[index]) {
      if (++index >= targetLen) { // return true if all chars in the target match
        return (str + strIdx);
      }
    }
  }
  return NULL;
}

/**
 * check last 6 digits of phone no  with the currentConnection
 */
boolean pfodSMS_SIM900::phoneNoMatchesCurrent(const char*newPhNo) {
  return phoneNoMatches(currentConnection->returnPhNo, newPhNo);
}

boolean pfodSMS_SIM900::phoneNoHas6Digits(const char *phNo) {
  if ((phNo == NULL) || (!*phNo)) {
    return false;
  }
  int i = strlen(phNo) - 1;
  int k = 0;
  while ((i >= 0) && (k < MAX_PHNO_CHECK)) {
    if ((phNo[i] < '0') || (phNo[i] > '9')) {
      break;
    }
    // else
    i--;
    k++;
  }
  return (k == MAX_PHNO_CHECK); // found MAX_PHNO_CHECK digits
}

/**
 * check last 6 digits of phone no  with the currentConnection
 */
boolean pfodSMS_SIM900::phoneNoMatches(const char* oldPhNo, const char*newPhNo) {
  if ((newPhNo == NULL) || (oldPhNo == NULL) || (!*newPhNo) || (!*oldPhNo)) {
    return false;
  }
  int i = strlen(oldPhNo) - 1;
  int j = strlen(newPhNo) - 1;
  int k = 0;
  while ((i >= 0) && (j >= 0) && (k < MAX_PHNO_CHECK)) {
    if (oldPhNo[i] != newPhNo[j]) {
      break;
    }
    // else
    i--;
    j--;
    k++;
  }
  return (k == MAX_PHNO_CHECK); // found MAX_PHNO_CHECK matches
  // else false
}

/**
 * Checks if shield is responding to commands
 * if not power it up
 * Does NOT check if ready to send msgs i.e. connected to network
 * because it will recieve a message first when pfodApp connects
 * and we cannot do any thing to make it connect to the network sooner than it does
 */
void pfodSMS_SIM900::checkGPRSpoweredUp() {
  if (powerResetPin >= 0) {
    pinMode(powerResetPin, INPUT); // no pullup this the default
    pinMode(powerResetPin, OUTPUT); // low
  }

  boolean callReady = false;
  while (!callReady) {
    delay(2000); //wait 2sec
#ifdef DEBUG_SETUP
    if (debugOut != NULL) {
      debugOut->println(F("checkGPRSpoweredUp"));
    }
#endif // DEBUG_SETUP
    // gprsPrint('\r'); // terminate any unterminated previous command
    pfodWaitForUtils::dumpReply(gprs, debugOut);
    delay(1500);
    gprsPrint(F("AT\r"));  // <<<<<<< AT_cmd
    // check for OK to see if turned on or not
    if (pfodWaitForUtils::waitFor(F("OK"), 3000, gprs, debugOut)) {
      pfodWaitForUtils::dumpReply(gprs, debugOut);
#ifdef DEBUG_SETUP
      if (debugOut != NULL) {
        debugOut->print(F("GPRS already running\n"));
      }
#endif // DEBUG_SETUP
      gprsReady = true; // shield is responding to commands
    } else {
      pfodWaitForUtils::dumpReply(gprs, debugOut);
#ifdef DEBUG_SETUP
      if (debugOut != NULL) {
        debugOut->print(F(" GPRS needs to be powered up.\n"));
      }
#endif // DEBUG_SETUP
      while (!gprsReady) {
        gprsReady = powerUpGPRS();
        pfodWaitForUtils::dumpReply(gprs, debugOut);
#ifdef DEBUG_SETUP
        if (debugOut != NULL) {
          if (!gprsReady) {
            debugOut->println(F("\n Try again to power up GPRS"));
          }
        }
#endif // DEBUG_SETUP
      }
    }

    int count = 0;
    while ((!callReady) && (count < 300)) { // try for 30min = 300
      count++;
      gprsPrint(F("AT+CCALR?\r"));  // <<<<<<< AT_cmd
      if (!pfodWaitForUtils::waitFor(F("+CCALR: 1"), 6000, gprs, debugOut)) {
        // wait for 6sec for OK  else try again
      } else {
        callReady = true;
      }
    }
    if (!callReady) {
      while (!powerUpGPRS()) {
        pfodWaitForUtils::dumpReply(gprs, debugOut);
#ifdef DEBUG_SETUP
        if (debugOut != NULL) {
          if (!gprsReady) {
            debugOut->println(F("\n Try again to power up GPRS"));
          }
        }
#endif // DEBUG_SETUP
      }
    }
    // turn off and try again
  }

  pfodWaitForUtils::dumpReply(gprs, debugOut, 5000); // dump responses until get 5sec with nothing
  if (gprsReady) {
    setupSMS();
#ifdef DEBUG_SETUP
    if (debugOut != NULL) {
      debugOut->print(F(" GPRS ready\n"));
    }
#endif // DEBUG_SETUP
  }
}

void pfodSMS_SIM900::setupSMS() {
  // set unsolicited result code enable (tested this way)
  gprsPrint(F("AT+CIURC=1\r")); // set unsolicited result code enable  // <<<<<<< AT_cmd
  pfodWaitForUtils::waitForOK(gprs, debugOut);
  gprsPrint(F("AT+CMGF=1\r")); // set text mode  // <<<<<<< AT_cmd
  pfodWaitForUtils::waitForOK(gprs, debugOut);
  pfodWaitForUtils::dumpReply(gprs, debugOut);
  gprsPrint(F("AT+CSCS=\"GSM\"\r")); // set GSM chars set  // <<<<<<< AT_cmd
  pfodWaitForUtils::waitForOK(gprs, debugOut);
  pfodWaitForUtils::dumpReply(gprs, debugOut);
  gprsPrint(F("AT+CSMP=17,167,0,0\r")); // set default text mode (not 8bit)  // <<<<<<< AT_cmd
  pfodWaitForUtils::waitForOK(gprs, debugOut);
  pfodWaitForUtils::dumpReply(gprs, debugOut);
  // delete all on startup
#ifdef DEBUG_SETUP
  if (debugOut != NULL) {
    gprsPrint(F("AT+CMGL=\"ALL\"\r")); // print them all out first // <<<<<<< AT_cmd
    pfodWaitForUtils::waitFor(F("OK"), 20000, gprs, debugOut); // wait for 20sec as may be printing out lots of old msgs
    pfodWaitForUtils::dumpReply(gprs, debugOut);
  }
#endif //DEBUG_SETUP
  // now delete them
  clearAllSmsMsgs();
  pfodWaitForUtils::waitForOK(gprs, debugOut);
  pfodWaitForUtils::dumpReply(gprs, debugOut);
}

/**
 * note pin 9 must be set as output
 * by caller
 */
boolean pfodSMS_SIM900::powerUpGPRS() {
  // power up GPRS
  if (powerResetPin < 0) {
#ifdef DEBUG_SETUP
    if (debugOut != NULL) {
      debugOut->print(F(" power pin not set cannot power cycle GPRS\n"));
    }
#endif // DEBUG_SETUP
    return true;
  } // else
#ifdef DEBUG_SETUP
  if (debugOut != NULL) {
    debugOut->print(F(" powerUpDownGPRS\n"));
  }
#endif // DEBUG_SETUP
  digitalWrite(powerResetPin, HIGH);
  delay(1500);  // PULSE HIGH FOR 1.5 SEC.
  digitalWrite(powerResetPin, LOW);
  // after 3.2sec GPRS serial is active will send RDY if not autobauding
  delay(4000); // > 3.2 sec for Serial port to be active
  int count = 0;
  while (count < 2) {
    count++;
    gprsPrint(F("AT\r")); // send AT  // <<<<<<< AT_cmd
    if (!pfodWaitForUtils::waitFor(F("OK"), 6000, gprs, debugOut)) {
      // wait for 6sec for OK  return false if not found
      delay(2000);  // wait for 2 sec and try again.
    }	else {
      return true;
    }
  }
  // did not get OK after two attempts
  return false;
}


void pfodSMS_SIM900::startSMS(const char* phoneNo) {
  if (!(*phoneNo)) {
#ifdef DEBUG
    if (debugOut != NULL) {
      debugOut->print(F("return PhNo empty do not start SMS msg"));
      debugOut->println();
    }
#endif // DEBUG
    return;
  }
  gprsPrint(F("AT+CMGS=\"")); // send direct add " " back in  // <<<<<<< AT_cmd
  gprsPrint(phoneNo);
  gprsPrint(F("\"\r"));
}

void pfodSMS_SIM900::printCurrentConnection() {
#ifdef DEBUG
  if (debugOut != NULL) {
    debugOut->print(F(" currentConnection -- "));
  }
#endif // DEBUG
  printConnection (currentConnection);
}
void pfodSMS_SIM900::printNextConnection() {
#ifdef DEBUG
  if (debugOut != NULL) {
    debugOut->print(F(" nextConnection -- "));
  }
#endif // DEBUG
  printConnection (nextConnection);
}
void pfodSMS_SIM900::printConnection(connection* con) {
#ifdef DEBUG
  if (debugOut != NULL) {
    debugOut->print(con->returnPhNo);
    debugOut->print(F(" ConNo:"));  debugOut->print(con->connectionNo);
    debugOut->print(F(" TimedOut:"));  debugOut->print(con->timedOut ? 'T' : 'F');
    debugOut->print(F("    MsgNo_tx:"));  debugOut->print(con->msgNo_tx);
    debugOut->print(F(" msgNo_tx_newMsg:"));  debugOut->print(con->msgNo_tx_newMsg);
    debugOut->print(F(" msgNo_tx_resend:"));  debugOut->print(con->msgNo_tx_resend);
    debugOut->print(F("    MsgNo_rx:"));  debugOut->print(con->msgNo_rx);
    debugOut->println();
  }
#else
  (void)(con);
#endif // DEBUG
}

void pfodSMS_SIM900::swapConnections() {
  connection* tmpConnection = nextConnection;
  nextConnection = currentConnection;
  currentConnection = tmpConnection;
}

void pfodSMS_SIM900::setPhoneNo(connection *con, const char* phNo) {
  strncpy_safe(con->returnPhNo, phNo, MAX_PHNO_LEN);
}

void pfodSMS_SIM900::clearAllSmsMsgs() {
#ifdef DEBUG
  if (debugOut != NULL) {
    debugOut->print(F("clearing ALL msgs \n"));
  }
#endif // DEBUG
  gprsPrint(F("AT+CMGDA=\"DEL ALL\"\r")); // delete all msgs out and in on  // <<<<<<< AT_cmd
  emptyIncomingMsgNo();
  // and clearout incoming msgNo storage
}

void pfodSMS_SIM900::clearReadSmsMsgs() {
#ifdef DEBUG
  if (debugOut != NULL) {
    debugOut->print(F("clearing ALL receieved READ msgS \n"));
  }
#endif // DEBUG
  gprsPrint(F("AT+CMGDA=\"DEL READ\"\r"));  // <<<<<<< AT_cmd
}

void pfodSMS_SIM900::gprsPrint(const char* str) {
  if (gprs) {
    foundOK = false; // wait for OK
    gprs->print(str);
  }
}

void pfodSMS_SIM900::gprsPrint(char c) {
  if (gprs) {
    foundOK = false; // wait for OK
    gprs->print(c);
  }
}

void pfodSMS_SIM900::gprsPrint(const __FlashStringHelper *ifsh) {
  if (gprs) {
    foundOK = false; // wait for OK
    gprs->print(ifsh);
  }
}

boolean pfodSMS_SIM900::haveIncomingMsgNo() {
  return incomingMsgNos_head != incomingMsgNos_tail;
}

void pfodSMS_SIM900::emptyIncomingMsgNo() {
  incomingMsgNos_tail = incomingMsgNos_head;
}

/**
 * returns true if added else false
 */
boolean pfodSMS_SIM900::addIncomingMsgNo(const char* msgNo) {
  uint8_t newHead = (incomingMsgNos_head + 1) % (MAX_INCOMING_MSG_NOS + 1);
  if (newHead == incomingMsgNos_tail) {
#ifdef DEBUG
    if (debugOut != NULL) {
      debugOut->print(F(" incoming msgNo queue full\n"));
    }
#endif // DEBUG
    // queue full just delete this msg
    return false;
  } else {
    strncpy_safe(incomingMsgNos[incomingMsgNos_head], msgNo, MAX_MSG_NO_LEN);
#ifdef DEBUG
    if (debugOut != NULL) {
      //    debugOut->print(F(" added msgNo "));
      //    debugOut->print(incomingMsgNos[incomingMsgNos_head]);
      //    debugOut->print(F(" at:"));
      //    debugOut->print(incomingMsgNos_head);
      //    debugOut->println();
    }
#endif // DEBUG
    incomingMsgNos_head = newHead;
    return true;
  }
}

const char* pfodSMS_SIM900::getIncomingMsgNo() {
  if (incomingMsgNos_head == incomingMsgNos_tail) {
    // queue empty
#ifdef DEBUG
    if (debugOut != NULL) {
      debugOut->print(F(" incoming msgNo queue empty\n"));
    }
#endif // DEBUG
    return NULL;
  } else {
    const char* rtn = incomingMsgNos[incomingMsgNos_tail];
#ifdef DEBUG
    if (debugOut != NULL) {
      //        debugOut->print(F(" removed msgNo "));
      //        debugOut->print(rtn);
      //        debugOut->print(F(" from:"));
      //        debugOut->print(incomingMsgNos_tail);
      //        debugOut->println();
    }
#endif // DEBUG
    incomingMsgNos_tail = (incomingMsgNos_tail + 1) % (MAX_INCOMING_MSG_NOS + 1);
    return rtn;
  }
}

void pfodSMS_SIM900::deleteIncomingMsg(const char *newMsgNo) {
  // ask of this msg
  if (newMsgNo && *newMsgNo) { // skip nulls and empty msg Nos
    gprsPrint(F("AT+CMGD="));  // <<<<<<< AT_cmd
    gprsPrint(newMsgNo);
    gprsPrint('\r');
  }
}

