/**
  pfodSecurityClient for Arduino
  Parses commands of the form { cmd | arg1 ` arg2 ... } hashCode
  Arguments are separated by `
  The | and the args are optional
  This is a complete paser for ALL commands a pfodApp will send to a pfodDevice

  pfodSecurityClient is also used to send pfod messages to the pfodApp and add a SipHash code to the end
  of each messge so that the authenticity and correctness of the message can be verified.

  see www.pfod.com.au  for more details and example applications
  see http://www.forward.com.au/pfod/secureChallengeResponse/index.html for the details on the
  128bit security and how to generate a secure secret password

  pfodSecurityClient adds about 2100 bytes to the program and uses about 400 bytes RAM
  and upto 19 bytes of EEPROM starting from the address passed to init()


  The pfodSecurityClient parses messages of the form
  { cmd | arg1 ` arg2 ` ... } hashCode
  The hashCode is checked against the SipHash_2_4 using the 128bit secret key.
  The hash input is the message count, the challenge for this connection and the message
  If the hash fails the cmd is set to DisconnectNow ('!')
  It is then the responsability of the calling program to close the connection so another user can connect.

  If the hash passes the message is parsed into the args array
  by replacing '|', '`' and '}' with '/0' (null)

  When the the end of message } is seen
  parse() returns the first byte of the cmd
  getCmd() returns a pointer to the null terminated cmd
  skipCmd() returns a pointer to the first arg (null terminated)
  or a pointer to null if there are no args
  getArgsCount() returns the number of args found.
  These calls are valid until the start of the next msg { is parsed.
  At which time they are reset to empty command and no args.
*/
/*
   (c)2014-2017 Forward Computing and Control Pty. Ltd.
   NSW Australia, www.forward.com.au
   This code is not warranted to be fit for any purpose. You may only use it at your own risk.
   This code may be freely used for both private and commercial use
   Provide this copyright is maintained.
*/

#include "pfodSecurity.h"
#include "pfodSecurityClient.h"
#include "pfodParser.h"
#include "SipHash_2_4.h"
#include "pfodParserUtils.h"
#include "HexConversionUtils.h"
#include <string.h>

#define CR 13
#define NL 10

//#define DEBUG_RAWIO
//#define DEBUG_AUTHORIZATION
//#define DEBUG_EEPROM_KEY_UPDATE
#define DEBUG_TIMER
#define DEBUG_CLOSE_CONNECTION
//#define DEBUG_PARSER
#define DEBUG_KEEP_ALIVE
#define DEBUG_ECHO_INPUT
// WARNING do not uncomment this define when connect to the network
// as it sends the expected response as well as the challenge
// ONLY use this via the serial monitor for debugging
//#define DEBUG_DISPLAY_RESPONSE

// used by authroizing var
#define NOT_AUTHORIZING 0
// set in disconnnected
#define AUTHORIZING_START (NOT_AUTHORIZING+1)
// set in connect or from loop if NOT_AUTHORIZING to go from disconnect to AUTHORIZING_START if not call to connect()
#define AUTHORIZING_SENT_CHALLENGE (AUTHORIZING_START+1)
#define AUTHORIZING_SUCCESS (AUTHORIZING_SENT_CHALLENGE+1)
#define AUTHORIZING_CMD '_'


pfodSecurityClient::pfodSecurityClient() {
  debugOut = NULL;
  pfod_Base_set = NULL;
  doFlush = false; // set to true for SMS only, otherwise false
  io = NULL;
  initialization = true;

  setKeepAliveIntervalCalled = false;
  keepAliveIntervalValue = 0;// default 0 for bridge work
  keepAliveInterval = 0; 
  keepAliveTimer = 0; 
  
  setIdleTimeoutCalled = false;
  idleTimeout = 10 * 1000; // default 10 sec idle timeout
  idleConnectionTimerStart = 0;
  idleConnectionTimer = 0;
  
  authorizing = NOT_AUTHORIZING;
  
  responseTimeoutValue = 10 * 1000; // 10 as per pfodApp
  responseTimeout = 0;
  waitingForResponse = false;
  closeConnection(); // sets   connectionClosed = true;   connecting = false;
  timerDebug_ms = millis();
  
}

void pfodSecurityClient::setKeepAliveInterval(uint16_t _interval_mS) {
  setKeepAliveIntervalCalled = true;
  keepAliveIntervalValue = _interval_mS;
#ifdef DEBUG_KEEP_ALIVE
  if (debugOut != NULL) {
    debugOut->print(F("setKeepAliveInterval() set to "));
    debugOut->print(keepAliveIntervalValue);
    debugOut->println(F("mS"));
  }
#endif
}

void pfodSecurityClient::setIdleTimeout(unsigned long timeout_in_seconds) {
  setIdleTimeoutCalled = true;
  idleTimeout = timeout_in_seconds * 1000;

#ifdef DEBUG_TIMER
  if (debugOut != NULL) {
    debugOut->print(F("setIdleTimeout() set idleTimeout to "));
    debugOut->println(idleTimeout);
  }
#endif
}

// true if no new cmd within idle timeout
bool pfodSecurityClient::isIdleTimeout() {
  if (idleConnectionTimer == 0) {
  	  return false;
  }

  if ((idleConnectionTimer != 0) && ((millis() - idleConnectionTimerStart) > idleConnectionTimer) && (authorizing == AUTHORIZING_SUCCESS)) {
    idleConnectionTimer = 0; // clear it now
#ifdef DEBUG_TIMER
    if (debugOut != NULL) {
      debugOut->println(F(" timer timed out, isIdleTimeout returns true."));
    }
#endif
    return true;
  } // else {
  return false;
} 

void pfodSecurityClient::startIdleConnectionTimer() {
	idleConnectionTimer = idleTimeout;
	idleConnectionTimerStart = millis();
}

void pfodSecurityClient::stopIdleConnectionTimer() {
	idleConnectionTimer = 0;
}

Stream *pfodSecurityClient::getRawDataStream() {
  return &rxRawDataBuf;
}

/**
   Call this before calling connect() if you want the debug output
   to go to some other Stream, other then the communication link

   debugOut is not written to unless you uncomment one of the #define DEBUG_ settings above
*/
void pfodSecurityClient::setDebugStream(Print* out) {
  debugOut = out;
  mac.setDebugStream(debugOut);
}




void pfodSecurityClient::connect(Stream* io_arg) {
  connect(io_arg, F(""));
}

void pfodSecurityClient::connect(pfod_Base* _pfod_Base_set) {
  connect(_pfod_Base_set, F(""));
}

void pfodSecurityClient::connect(pfod_Base* _pfod_Base_set, const __FlashStringHelper *hexKeyPgr) {
  pfod_Base_set = _pfod_Base_set;
  connect((Stream*)pfod_Base_set, hexKeyPgr);
}


void pfodSecurityClient::init() {
  setAuthorizeState(NOT_AUTHORIZING);
#ifdef DEBUG_PARSER
  if (debugOut != NULL) {
    debugOut->println(F(" pfodSecurityClient::init() parsing true"));
  }
#endif // DEBUG_PARSER

  inMsgCount = 0;
  outMsgCount = 0;
  msgHashCount = -1; // stop collecting hash
  txBuf.init(tx_buffer, TX_BUFFER_SIZE);
  rxBuf.init(rx_buffer, RX_BUFFER_SIZE);
  rxTempBuf.init(rx_temp_buffer, RX_BUFFER_SIZE);
  rxRawDataBuf.init(rx_raw_data_buffer, RX_BUFFER_SIZE);
  clearTxRxBuffers();
  outputParserState = pfodParser::pfodWaitingForStart;
  closingConnectionOut = false;
  closingConnectionIn = false;
  foundMsgStart = false;
  connectionClosed = true;
  connecting = false;
  stopResponseTimer(); // clears waitingForResponse
  stopIdleConnectionTimer();
  keepAliveInterval = 0;
}


void pfodSecurityClient::clearTxRxBuffers() {
  txBuf.clear();
  rxBuf.clear();
  rxTempBuf.clear();
}

/**
   Close connection and inject 0xff into rxBuf
   upper level parser can pick this up and clear any partial response
   This method is protected.
   Upper levels call the public closeConnection(), which does not inject 0xff
*/
void pfodSecurityClient::closeConnectionAddEOF() {
#ifdef DEBUG_CLOSE_CONNECTION
  if (debugOut != NULL) {
    debugOut->println(F(" pfodSecurityClient::closeConnectionAddEOF() called"));
  }
#endif // DEBUG_AUTHORIZATION
  closeConnection();
  rxBuf.write(0xff);
}

/**
   Call when the parser returns !
   any bytes in the input buffer after ! is returned are dropped
   any bytes recieved while not parsing are dropped
*/
void pfodSecurityClient::closeConnection() {
#if defined ( DEBUG_AUTHORIZATION) || defined ( DEBUG_CLOSE_CONNECTION)
  if (debugOut != NULL) {
    debugOut->println(F(" pfodSecurityClient::closeConnection() called"));
  }
#endif // DEBUG_AUTHORIZATION
  // close pfod_Base if set
  if (pfod_Base_set != NULL) {
    pfod_Base_set->_closeCurrentConnection();
  }
  init(); // clears connection timer and idleTimeout timer
  // also clears out cmd so need to add back ! if needed
}


// from Stream
// need to process data for hash first before making available to rxBuf
int pfodSecurityClient::available() {
  pollIO();
  return rxBuf.available();
}

int pfodSecurityClient::peek() {
  pollIO();
  return rxBuf.peek();
}

int pfodSecurityClient::read() {
  pollIO();
  int rtn = rxBuf.read();
  if ((closingConnectionIn) && (rxBuf.available() == 0)) {
    // get {! ... and have read this msg completely
    closingConnectionIn = false;
    closeConnection();
  }
  return rtn;
}

void pfodSecurityClient::flush() {
  pollIO();
  // nothing here
}

size_t pfodSecurityClient::write(uint8_t b) {
#ifdef DEBUG_ECHO_INPUT
  if (debugOut != NULL) {
    debugOut->print(F("write("));
    debugOut->print((char)b);
    debugOut->println(F(")"));
  }
#endif
  if (connectionClosed) {
    // wait for { to start new connection
    if (!connecting) {
      if (b == '{') {
        startConnection(); // sets connecting == true;
      } else {
        // connectionClosed and have not seen { to start connecting so just drop this byte
        // wait for write { to open new connection
        pollIO();
        return 1;
      }
    } // if !connecting
  }  // if connectionClosed
  txBuf.write(b);
  pollIO();
  return 1;
}

size_t pfodSecurityClient::write(const uint8_t *buffer, size_t size) {
  for (size_t i=0; i<size; i++) {
    write(buffer[i]);
  }
  return size;
}

bool pfodSecurityClient::startConnection() {
#ifdef DEBUG_ECHO_INPUT
  if (debugOut != NULL) {
    debugOut->println(" startConnection() called.");
  }
#endif
  if (!connectionClosed) {
#ifdef DEBUG_ECHO_INPUT
    if (debugOut != NULL) {
      debugOut->println(" connection open so just return.");
    }
#endif
    return true; // not closed so no need to start a new one
  }
  
  if (!setIdleTimeoutCalled) { // user did not call it
  	setIdleTimeoutCalled = true;
    if (pfod_Base_set != NULL) {
  	  	// override with base setting if base set
      setIdleTimeout(pfod_Base_set->getDefaultTimeOut());
    } // else { leave as default 10 sec
  } // else setIdleTimeout has been called by user so do not reset it
  
  if (pfod_Base_set != NULL) {
    // call base connect
    if (!(pfod_Base_set->connect())) {
      return false; // connect failed
    }
  }

  if (noPassword) {
    connecting = false;
    connectionClosed = false;
    setAuthorizeState(AUTHORIZING_SUCCESS);
    resetKeepAliveTimer();
    startIdleConnectionTimer();
  } else {
    if (connecting) {
      return true; // already set to connecting
    }
#ifdef DEBUG_ECHO_INPUT
    if (debugOut != NULL) {
      debugOut->println(" have password so set connecting == true.");
    }
#endif
    connecting = true;
    setAuthorizeState(NOT_AUTHORIZING);
  }
  return true;
}

void pfodSecurityClient::stopResponseTimer() {
  if (responseTimeout > 0) {
    responseTimeout = 0;
    waitingForResponse = false; // can send next cmd now
#ifdef DEBUG_TIMER
    if (debugOut != NULL) {
      debugOut->println("stopResponseTimer()");
    }
#endif
  }
}

// all the work is done here
void pfodSecurityClient::pollIO() {
  if (io == NULL) {
    return;
  }
#ifdef DEBUG_TIMER
  if (debugOut != NULL) {
    if (((millis() - timerDebug_ms) > 1000) ) {
      timerDebug_ms = millis();
      if (responseTimeout != 0) {
        debugOut->print(F("responseTimer Timer:"));
        debugOut->println((millis() - responseTimer));
      }
      if (idleConnectionTimer != 0) {
        debugOut->print(F("idleTimeout Timer:"));
        debugOut->println((millis() - idleConnectionTimerStart));
      }
    }
  }
#endif

  if ((responseTimeout > 0) && ((millis() - responseTimer) > responseTimeout) ) {
#ifdef DEBUG_TIMER
    if (debugOut != NULL) {
      debugOut->println("timed out waiting for response. close connection.");
    }
#endif
    closeConnectionAddEOF();
  } else { // not if ((responseTimeout > 0)
    if (connectionClosed) {
      if (!connecting) { // connecting set true by startConnection() ONLY and ONLY if connectionClosed == true
        // clear timers
        stopResponseTimer();
        keepAliveInterval = 0;
        return; // nothing to do here ?? what about time out??
      } // else
      startAuthorization();  // does nothing if not NOT_AUTHORIZING, i.e. only first call does anything
      // if no password then startAuthorization() sets connecting = false; and connectionClosed = false;

    } else { // !connectionClosed
      // check keep alive
      if ((keepAliveInterval > 0) && ((millis() - keepAliveTimer) > keepAliveInterval)  && (authorizing == AUTHORIZING_SUCCESS)) {
        // can only write this if parser is waiting for start
        // i.e. not in the middle of writing out a command.
        if (outputParserState == pfodParser::pfodWaitingForStart) {
          txBuf.write('{'); txBuf.write(' '); txBuf.write('}');
        } else {
#ifdef DEBUG_ECHO_INPUT
          if (debugOut != NULL) {
            debugOut->println("keep alive timed out but blocked by parser pfodWaitingForStart == true or not AUTHORIZING_SUCCESS.");
          }
#endif
        }
      }
      //if ((idleConnectionTimer > 0) && ((millis() - idleConnectionTimerStart) > idleConnectionTimer)  && (authorizing == AUTHORIZING_SUCCESS)) {
      if (isIdleTimeout()) {
        // no new cmd so close connection
        closeConnection();
      }

      // write to out via hash
      // if not waiting for response
      while (txBuf.available() && (!waitingForResponse)) {
        writeToIO(txBuf.read());
      }
    }
  } // if ((responseTimeout > 0)
  readFromIO();

}

void pfodSecurityClient::resetKeepAliveTimer() {
  keepAliveInterval = keepAliveIntervalValue;
  if (keepAliveInterval > 0) {
    keepAliveTimer = millis();
  } //else no keep alives
}

void pfodSecurityClient::restartResponseTimer() {
  responseTimer = millis();
  responseTimeout = responseTimeoutValue;
#ifdef DEBUG_TIMER
  if (debugOut != NULL) {
    debugOut->print("restartResponseTimer to :");
    debugOut->println(responseTimeout);
  }
#endif

}

/**
   startAuthorization, only runs once
*/
void pfodSecurityClient::startAuthorization() {
  if (authorizing != NOT_AUTHORIZING) {
    // only process this once
    return;
  } // else

  if (noPassword) {
    // this section duplicates the code in startConnection so not really need here
    connecting = false;
    connectionClosed = false;
    setAuthorizeState(AUTHORIZING_SUCCESS);
  } else {
    // have password
    io->write('{');
    io->write(AUTHORIZING_CMD);
    io->write('}');
    setAuthorizeState(AUTHORIZING_START);
    restartResponseTimer();
  }
}




bool pfodSecurityClient::collectCheckHash(uint8_t b) {
  if ((msgHashCount < 0)  || noPassword ) {
    return false; // hash not being collected yet
  }
  // (msgHashCount >= 0)) {
  msgHashBytes[msgHashCount++] = b;
#ifdef DEBUG_DISPLAY_RESPONSE
  if (debugOut != NULL) {
    debugOut->print(F(" hashHex:"));
    debugOut->println((char)b);
  }
#endif
  if (msgHashCount == Msg_Hash_Size) {
    // stop responseTimer
    stopResponseTimer();

    msgHashBytes[Msg_Hash_Size] = 0; //terminate
    // check hash
    msgHashCount = -1;
#ifdef DEBUG_DISPLAY_RESPONSE
    if (debugOut != NULL) {
      debugOut->print(F(" checkMsgHash: inMsgCount:"));
      debugOut->print(inMsgCount);
      debugOut->print(" ");
      mac.printBytesInHex(debugOut, mac.getHashedResult(), Msg_Hash_Size_Bytes);
      debugOut->println();
      debugOut->println((char*)msgHashBytes);
    }
#endif
    if (mac.checkMsgHash(msgHashBytes, Msg_Hash_Size_Bytes)) {
      while (rxTempBuf.available()) {
        rxBuf.write(rxTempBuf.read());
      }
      //  cmd = *parser.getCmd(); // reset cmd from parser
      inMsgCount++;
      if (closingConnectionOut) {
        closeConnection(); // do not add 0xff so parser buffers not cleared
        closingConnectionOut = false;
      }
    } else {
#ifdef DEBUG_ECHO_INPUT
      if (debugOut != NULL) {
        debugOut->println(F("Hash failed "));
      }
#endif
      closeConnectionAddEOF();
      return true;
    }
  }
  return true;
}

void pfodSecurityClient::handleChallengeResponse() {
#ifdef DEBUG_DISPLAY_RESPONSE
  if (debugOut != NULL) {
    debugOut->println(F(" handleChallengeResponse"));
  }
#endif
  if (authorizing == AUTHORIZING_START) {
#ifdef DEBUG_DISPLAY_RESPONSE
    if (debugOut != NULL) {
      debugOut->println(F(" authorizing == AUTHORIZING_START"));
    }
#endif
    // have sent {_}
    // pick up challenge from rxTempBuf {_xxxxxxxxxxxxxxxx}
    // calculate response and send
    //io->write(..)
    if (rxTempBuf.available() != (pfodMAC::pfodMAC::challengeByteSize * 2 + 3 + 2)) {
#ifdef DEBUG_DISPLAY_RESPONSE
      if (debugOut != NULL) {
        debugOut->println(F(" rxTempBuf.available() != 21"));
      }
#endif
      closeConnectionAddEOF();
      return;
    }
    rxTempBuf.read(); uint8_t b = rxTempBuf.read(); // skip {_
    if (b != AUTHORIZING_CMD) {
      closeConnectionAddEOF();
      return;
    }
    uint8_t challengeHashHex[pfodMAC::challengeHashByteSize * 2 + 1]; // for incoming hex and outgoing hex
    for (int i = 0; i < pfodMAC::challengeByteSize * 2; i++) {
      // convert hex to byte and store in challengeBytes
      b = rxTempBuf.read();
      challengeHashHex[i] = b;
    }
    challengeHashHex[pfodMAC::challengeHashByteSize * 2] = '\0'; // terminate
    asciiToHex((char*)challengeHashHex, challenge, pfodMAC::challengeByteSize);
    // save in challenge for later use in msg hash
    mac.hashChallenge(challenge); // sets result for getHashedResult()

#ifdef DEBUG_DISPLAY_RESPONSE
    if (debugOut != NULL) {
      debugOut->println(F(" challenge hashResult"));
    }
#endif
    // convert to hex
    hexToAscii((const unsigned char *) mac.getHashedResult(), pfodMAC::challengeHashByteSize, (char*) challengeHashHex,
               pfodMAC::challengeHashByteSize * 2 + 1);
#ifdef DEBUG_DISPLAY_RESPONSE
    for (int i = 0; i < pfodMAC::challengeHashByteSize * 2; i++) {
      if (debugOut != NULL) {
        debugOut->print((char)challengeHashHex[i]);
      }
    }
#endif
    //    // check the hash identifier should be 02
    if ((rxTempBuf.read() != '0') || (rxTempBuf.read() != '2')) {
#ifdef DEBUG_DISPLAY_RESPONSE
      if (debugOut != NULL) {
        debugOut->println(F(" hash identifier != \"02\""));
      }
#endif
      closeConnectionAddEOF();
      return;
    }
    // send it back
    io->write('{'); io->write('_'); io->write('|');
    for (int i = 0; i < pfodMAC::challengeHashByteSize * 2; i++) {
      io->write(challengeHashHex[i]);
    }
    io->write('}');
    setAuthorizeState(AUTHORIZING_SENT_CHALLENGE);
    restartResponseTimer();

  } else if (authorizing == AUTHORIZING_SENT_CHALLENGE) {
#ifdef DEBUG_DISPLAY_RESPONSE
    if (debugOut != NULL) {
      debugOut->println(F(" authorizing == AUTHORIZING_SENT_CHALLENGE"));
    }
#endif
    // have sent {_|xxxxxxxxxxxxxxxx}
    // expect rxTempBuf to be {_}
    if (rxTempBuf.available() != 3) {
#ifdef DEBUG_DISPLAY_RESPONSE
      if (debugOut != NULL) {
        debugOut->println(F(" rxTempBuf.available() != 3"));
      }
#endif
      closeConnectionAddEOF();
      return;
    }
    // else
    for (int i = 0; i < 3; i++) {
      incomingHashBytes[i] = rxTempBuf.read();
    }
    incomingHashBytes[3] = '\0';
    if ((strcmp("{_}", (const char*)incomingHashBytes) == 0)) {
      setAuthorizeState(AUTHORIZING_SUCCESS);
      resetKeepAliveTimer();
      startIdleConnectionTimer();
      connecting = false;
      connectionClosed = false;
#ifdef DEBUG_DISPLAY_RESPONSE
      if (debugOut != NULL) {
        debugOut->println(F(" AUTHORIZING_SUCCESS"));
      }
#endif
      // now
      pollIO(); // to write out txBuf
    } else {
      closeConnectionAddEOF();
    }
  }
  // else just return
}


void pfodSecurityClient::readFromIO() {
  if (!io) {
    return ;
  }

  while (io->available()) {
    uint8_t b = io->read();
#ifdef DEBUG_DISPLAY_RESPONSE
    if (debugOut != NULL) {
      debugOut->print(F(" readFromIO:"));  debugOut->println((char)b); debugOut->print(F(" noPassword:")); debugOut->print(noPassword);
      debugOut->print(F(" outputParserState:"));
      if (outputParserState == pfodParser::pfodWaitingForStart) {
        debugOut->println("pfodWaitingForStart");
      } else if (outputParserState == pfodParser::pfodMsgStarted) {
        debugOut->println("pfodMsgStarted");
      } else if (outputParserState == pfodParser::pfodInMsg) {
        debugOut->println("pfodInMsg");
      } else {
        debugOut->println((char)outputParserState);
      }
    }
#endif
    if (b == 0xff) {
      closeConnectionAddEOF();
      return;
    }
    if (collectCheckHash(b)) {
      return; // hash consumed and checked or continue if noPassword
    }

    // else
    // possible outputParserState values are:-
    // pfodWaitingForStart = 0xff;
    // pfodMsgStarted = '{';
    // pfodInMsg = 0;
    if (outputParserState == pfodParser::pfodWaitingForStart) {
      if ((msgHashCount < 0) || noPassword) {
        // finished collecting hash
        rxTempBuf.clear();
      }
#ifdef DEBUG_DISPLAY_RESPONSE
      if (debugOut != NULL) {
        debugOut->print(F(" pfodParser::pfodWaitingForStart cleared rxTempBuf  available:"));  debugOut->println(rxTempBuf.available());
      }
#endif

      if (b == pfodParser::pfodMsgStarted) {
        // found { while waiting for start
        outputParserState = pfodParser::pfodMsgStarted;
#ifdef DEBUG_DISPLAY_RESPONSE
        if (debugOut != NULL) {
          debugOut->print(F(" set pfodParser::pfodMsgStarted b:"));
          debugOut->println((char)b);
        }
#endif
        continue; //return; // nothing else to do
      } else {
        // NOT in msg started
        // is outside { }
        if (authorizing == AUTHORIZING_SUCCESS) {
          // this is raw data just copy to rxBuf
#ifdef DEBUG_DISPLAY_RESPONSE
          if (debugOut != NULL) {
            debugOut->print(F(" AUTHORIZING_SUCCESS write:"));  debugOut->println((char)b);
          }
#endif
          rxRawDataBuf.write(b);
#ifdef DEBUG_RAWIO
          if (debugOut != NULL) {
            debugOut->print(F(" raw_io:"));
            debugOut->println((char)b);
          }
#endif
          continue; //return; // nothing else to do
        }
      }

    } else if (outputParserState == pfodParser::pfodMsgStarted) {  // if (outputParserState == pfodParser::pfodWaitingForStart) {
      if (authorizing != AUTHORIZING_SUCCESS) {
        // expect next char to be _
        if (b != AUTHORIZING_CMD) {
          closeConnection();
          return;
        }
        // else
        outputParserState = pfodParser::pfodInMsg; // in msg
#ifdef DEBUG_DISPLAY_RESPONSE
        if (debugOut != NULL) {
          debugOut->println(F(" set pfodParser::pfodInMsg"));
        }
#endif

        rxTempBuf.write('{');
        //rxTempBuf.write('_'); handled below
#ifdef DEBUG_ECHO_INPUT
        if (debugOut != NULL) {
          debugOut->print("  pfodParser::pfodInMsg authorizing found ");  debugOut->println((char)b);
        }
#endif
      } else { // authorizing == AUTHORIZING_SUCCESS
        // {} Empty  {@ Language {. Menu {: Update menu/navigation items {^ Navigation input {= Streaming raw data
        // {' String input {# Numeric input {? Single Selection input {* Multiple  {! close connection
        // {+ dwg {- data

        if ((b == '}') || (b == '@') || (b == '.') || (b == ':') || (b == '^') || (b == '=') || (b == '\'') // '
            || (b == '#') || (b == '?') || (b == '*') || (b == '!') //
            || (b == '+') // dwg
            || (b == ',') || (b == ';')) { // new menus
          // this is real command
          // initial hash
#ifdef DEBUG_DISPLAY_RESPONSE
          if (debugOut != NULL) {
            debugOut->println(F(" init output hash"));
          }
#endif
          if (b == '!') {
            closingConnectionIn = true; // calles closeconnection when rxBuf is empty
          }
          if (!noPassword) {
            mac.initHash();
            mac.putByteToHash('{');
          }
          rxTempBuf.write(pfodParser::pfodMsgStarted); // response
          outputParserState = pfodParser::pfodInMsg; // in msg
#ifdef DEBUG_DISPLAY_RESPONSE
          if (debugOut != NULL) {
            debugOut->print(F(" set pfodParser::pfodInMsg b:"));
            debugOut->println((char)b);
          }
#endif
        } else {
          // not a real command response so all raw data
          // write out the held back {
          rxRawDataBuf.write(pfodParser::pfodMsgStarted);
#ifdef DEBUG_RAWIO
          if (debugOut != NULL) {
            debugOut->print(F(" raw_io:"));
            debugOut->print((char)pfodParser::pfodMsgStarted);
          }
#endif
          if (b == pfodParser::pfodMsgStarted) {
            // {{ go back to pfodMsgStarted
            // hold back second { until next char read
            outputParserState = pfodParser::pfodMsgStarted; // finished
#ifdef DEBUG_DISPLAY_RESPONSE
            if (debugOut != NULL) {
              debugOut->println(F(" set pfodParser::pfodMsgStarted"));
            }
#endif
            continue; //return; // nothing else to do
          } else {
            // not a real command just raw data
            outputParserState = pfodParser::pfodWaitingForStart; // finished
            rxRawDataBuf.write(b);
#ifdef DEBUG_RAWIO
            if (debugOut != NULL) {
              debugOut->print(F(" raw_io:"));
              debugOut->print((char)pfodParser::pfodMsgStarted);
            }
#endif
#ifdef DEBUG_DISPLAY_RESPONSE
            if (debugOut != NULL) {
              debugOut->println(F(" set pfodParser::pfodWaitingForStart b:"));
              debugOut->println((char)b);
            }
#endif
            continue; //return; // nothing else to do
          }
        }
      } // if (authorizing != AUTHORIZING_SUCCESS)
    } // else pfodMsgEnd but that should not happen for output

#ifdef DEBUG_DISPLAY_RESPONSE
    if (debugOut != NULL) {
      debugOut->write(b);
    }
#endif

    if (outputParserState == pfodParser::pfodInMsg) {
      rxTempBuf.write(b); // response
      // add it to the hash
      if (!noPassword) {
        mac.putByteToHash(b);
      }
      if (b == pfodParser::pfodMsgEnd) {
        outputParserState = pfodParser::pfodWaitingForStart; // finished
#ifdef DEBUG_DISPLAY_RESPONSE
        if (debugOut != NULL) {
          debugOut->print(F(" set pfodParser::pfodWaitingForStart b:"));
          debugOut->println((char)b);
        }
#endif

        // found closing }
        if (authorizing != AUTHORIZING_SUCCESS) {
          // send {_} expect {_challenge},
          // send {_|response} expect {_}
          // no password so stop responseTimer now
          stopResponseTimer();
          handleChallengeResponse();
          continue; //return; // nothing else to do
        } // if (authorizing != AUTHORIZING_SUCCESS) {

        if (!noPassword) {
          // have password
          // responseTimer will be stopped when incoming hash is recieved
          // create hash of incoming msg
          msgHashCount = 0; // start collecting hash
          mac.putLongToHash(inMsgCount);
          for (int i = 0; i < pfodMAC::challengeByteSize; i++) {
            mac.putByteToHash(challenge[i]);
          }
          mac.finishHash();
        } else {
          // no password so stop responseTimer now
          stopResponseTimer();
          // copy tempbuffer to rxBuf
          while (rxTempBuf.available()) {
            rxBuf.write(rxTempBuf.read());
          }

        }
      }
    }
  }
}

// time between sending command and getting respose else close connection
void pfodSecurityClient::setResponseTimeout(unsigned long timeout_mS) {
  responseTimeoutValue = timeout_mS;
}

/**
    write a byte to the connect stream
    quietly consumes all data not enclosed in { }
    <br>
    If the byte is part of a { } msg then add it to the hash
    and write the hash after the final }

  possible outputParserState values are:-
  pfodWaitingForStart = 0xff;
  pfodMsgStarted = '{';
  pfodInMsg = 0;

  Checks for {! and closes connection after send

*/
size_t pfodSecurityClient::writeToIO(uint8_t b) {  // write to Link
  if (!io) {
    return 1;
  }
#ifdef DEBUG_ECHO_INPUT
  if (debugOut != NULL) {
    debugOut->print(" writeToIO() called with ");
    debugOut->println((char)b);
  }
#endif

  //  quietly consumes all data not enclosed in { }
  if (outputParserState == pfodParser::pfodWaitingForStart) {
    if (b != pfodParser::pfodMsgStarted) {
#ifdef DEBUG_DISPLAY_RESPONSE
      if (debugOut != NULL) {
        debugOut->println(F(" Ignoring data outside { }"));
      }
#endif
      return 1; // consume all data outside { }
    } else {  // start msg {
      // found { while waiting for start
      foundMsgStart = true;
      outputParserState = pfodParser::pfodInMsg;
      // this is real command
      // initial hash
      if (!noPassword) {
#ifdef DEBUG_DISPLAY_RESPONSE
        if (debugOut != NULL) {
          debugOut->println(F(" init output hash"));
        }
#endif
        mac.initHash();
        mac.putByteToHash('{'); // for msg start
      } else {
        // no password skip hash
      }
      // write the { that started this command
      io->write(pfodParser::pfodMsgStarted); // start command
      // restart response timer
      restartResponseTimer(); //start response timer here so delays in writing whole command
      // ARE included in response timeout  so errors in code that do not write closing } will cause
      // link to timeout
#ifdef DEBUG_DISPLAY_RESPONSE
      if (debugOut != NULL) {
        debugOut->write((char)b);
      }
#endif
    }
  } else if ( outputParserState == pfodParser::pfodInMsg) {
    if (foundMsgStart) {
      foundMsgStart = false;
      // check for {! and set closing connection
      if (b == '!') {
        closingConnectionOut = true;
      }
    }
    io->write(b); // response
#ifdef DEBUG_DISPLAY_RESPONSE
    if (debugOut != NULL) {
      debugOut->write((char)b);
    }
#endif
    if (!noPassword) {
      // add it to the hash
      mac.putByteToHash(b);
    }
    if (b == pfodParser::pfodMsgEnd) {
      // found closing }
      resetKeepAliveTimer();
      startIdleConnectionTimer();
      waitingForResponse = true;
#ifdef DEBUG_TIMER
      if (debugOut != NULL) {
        debugOut->println(F("found } resetKeepAliveTimer "));
      }
#endif

      outputParserState = pfodParser::pfodWaitingForStart; // finished
      if (noPassword) {
        if (doFlush) {
          io->flush();
        }
      } else { // have password
        // create hash and send
#ifdef DEBUG_DISPLAY_RESPONSE
        if (debugOut != NULL) {
          debugOut->println(F(" send hash"));
          debugOut->print(F(" out msg Count "));
          debugOut->println(outMsgCount);
        }
#endif
        mac.putLongToHash(outMsgCount);
        outMsgCount++; // increment for next msg out
        for (int i = 0; i < pfodMAC::challengeByteSize; i++) {
          mac.putByteToHash(challenge[i]);
        }
        mac.finishHash();
#ifdef DEBUG_DISPLAY_RESPONSE
        if (debugOut != NULL) {
          debugOut->println(F(" add hashResult"));
        }
#endif
        // convert to hex
        hexToAscii((const unsigned char *) mac.getHashedResult(), pfodMAC::msgHashByteSize, (char*) msgHashBytes,
                   Msg_Hash_Size + 1);
        for (int i = 0; i < Msg_Hash_Size; i++) {
          io->write(msgHashBytes[i]);
          // note if output is full may lose byte here and hash at the other end will fail
          // so increment i from return from write so will loop here until write of all bytes is successful
        }
        if (doFlush) {
          io->flush();
        }
      }
      if (closingConnectionOut) {
#ifdef DEBUG_CLOSE_CONNECTION
        if (debugOut != NULL) {
          debugOut->println(F(" closingConnectionOut call closeConnectionAddEOF()"));
        }
#endif // DEBUG_CLOSE_CONNECTION
        io->flush(); // force send now
        // add EOF to rx to force close later
        closeConnectionAddEOF();
        closingConnectionOut = false;
      }
    }
    return 1;
  } else {
    // some other setting just reset
    outputParserState = pfodParser::pfodWaitingForStart;
    closingConnectionOut = false;
    foundMsgStart = false;
  }
#ifdef DEBUG_ECHO_INPUT
  if (debugOut != NULL) {
    debugOut->println(" writeToIO() end");
  }
#endif
  return 1;
}

/**
   initialize the Security parser
   args
   io_arg the Stream pointer to read and write to for pfod messages

   eepromAddress the starting address in eeprom to save the key and power cycles
      amount of eeprom used is (2 bytes power cycles + 1 byte key length + key bytes) ==> 3 + (input hexKey length)/2
      This is ignored if __no_pfodEEPROM__ defined i.e. NO EEPROM available for this board/uC

   hexKey  pointer to program memory F("hexString") holding the key
    if this key is different from the current one in eeprom the eeprom is updated and the
    power cycles are reset to 0xffff
    if changing the key suggest you add 2 to your eepromAddress to move on from the previous
    one.  The power cycle EEPROM addresses are are written to on each power up
    If __no_pfodEEPROM__ defined then power cycles set to a random number on power up

*/
void pfodSecurityClient::connect(Stream * io_arg, const __FlashStringHelper * hexKeyPgr_arg) {
  // max key size for this code which uses SipHash is 128 bits (16 bytes) (32 hex digits)

  noPassword = false;
  io = io_arg;
  hexKeyPgr = hexKeyPgr_arg;
  init();

  setAuthorizeState(NOT_AUTHORIZING);
#ifdef DEBUG_PARSER
  if (debugOut != NULL) {
    debugOut->println(F(" pfodSecurityClient::connect() parsing true"));
  }
#endif // DEBUG_PARSER

  // else create mac if null
  mac.setDebugStream(debugOut);
  char hexKey[pfodMAC::maxKeySize * 2 + 2]; // max  32 hexDigits for hex key
  const char *p = (const char *)hexKeyPgr;
  size_t n = 0;
  // start by trimming leading and trailing spaces
  int foundNonSpace = 0; // skip leading space
  // may need to rewrite pfodApp to check for utf8 'spaces' currently using java trim() fn.

  while (1) {
    unsigned char c = pgm_read_byte(p++);
    if (c == 0) {
      hexKey[n] = c;
      break; // store terminator
    };

    if ((c <= ' ')  && (!foundNonSpace)) {
      // skip
      continue;
    } else {
      foundNonSpace = 1; // found one
    }
    hexKey[n++] = c;
    if (n >= pfodMAC::maxKeySize * 2) {
      hexKey[n] = 0; // limit to pfodMAC::maxKeySize*2 hexDigits
      break;
    }
  }

  // now start at back and trim trailing spaces
  for (int i = n - 1; i >= 0; i--) {
    if (hexKey[i] <= ' ') {
      hexKey[i] = '\0';
      n--; // one less
    } else {
      break; // found non-space
    }
  }

#ifdef DEBUG_EEPROM_KEY_UPDATE
  if (debugOut != NULL) {
    debugOut->print(F(" Input Key of length "));
    debugOut->print(n);
    debugOut->println();
  }
#endif

  byte keyBytes[pfodMAC::maxKeySize];  // for input key
  int keyLen = 0;

  // ok get max 32 chars (bytes)
  // first check if all hex if so just pad with 0 and convert
  // this is for backward compatibility
  if (isAllHex(hexKey)) {
    keyLen = asciiToHex(hexKey, keyBytes, pfodMAC::maxKeySize); // note this handles odd number bytes implies trailing 0
#ifdef DEBUG_EEPROM_KEY_UPDATE
    if (debugOut != NULL) {
      debugOut->print(F(" New AllHex Key of length "));
      debugOut->print(keyLen);
      debugOut->print(F(" 0x"));
      mac.printBytesInHex(debugOut, keyBytes, keyLen);
      debugOut->println();
    }
#endif
  } else {
    // convert as base64 encoded bytes with encoding
    // a..zA..Z0..9() all other chars converted as (54 + (c - '0'))
    keyLen = pfodSecurity::getBytesFromPassword(hexKey, n, keyBytes, pfodMAC::maxKeySize);
#ifdef DEBUG_EEPROM_KEY_UPDATE
    if (debugOut != NULL) {
      debugOut->print(F(" New ascii Key of length "));
      debugOut->print(keyLen);
      debugOut->print(F(" 0x"));
      mac.printBytesInHex(debugOut, keyBytes, keyLen);
      debugOut->println();
    }
#endif
  }

#ifdef DEBUG_PARSER
  if (debugOut != NULL) {
    debugOut->println(F(" pfodSecurityClient::connect() start with connection closed"));
  }
#endif // DEBUG_PARSER
  if (!connectionClosed) {
    closeConnection(); // start with connection closed
  }
  if (keyLen == 0) {
    noPassword = true;
    setAuthorizeState(AUTHORIZING_SUCCESS);
#ifdef DEBUG_EEPROM_KEY_UPDATE
    if (debugOut != NULL) {
      debugOut->println(F(" No Password"));
    }
#endif
  } else {
    mac.setSecretKey(keyBytes, keyLen);
  }
  mac.init();
}


/**
   Return the stream pfodSecurity is writing its output to
   Don't write directly to this stream
   unless forceing a disconnect
*/
Stream* pfodSecurityClient::getLinkStream() {
  return io;
}



void pfodSecurityClient::setAuthorizeState(int authState) {
  if (noPassword) {
    authorizing = AUTHORIZING_SUCCESS;
  } else {
    authorizing = authState;
  }
}


