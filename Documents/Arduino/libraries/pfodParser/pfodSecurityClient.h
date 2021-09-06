#ifndef pfodSecurityClient_h
#define pfodSecurityClient_h
/**
  pfodSecurityClient for Arduino
  Parses commands of the form { cmd | arg1 ` arg2 ... } hashCode
  Arguments are separated by `
  The | and the args are optional
  This is a complete paser for ALL commands a pfodApp will send to a pfodDevice
  see www.pfod.com.au  for more details.
  see http://www.forward.com.au/pfod/secureChallengeResponse/index.html for the details on the
  128bit security and how to generate a secure secret password

  pfodSecurityClient adds about 2100 bytes to the program and uses about 400 bytes RAM
   and upto 19 bytes of EEPROM starting from the address passed to init()

  The pfodSecurityClient parses messages of the form
  { cmd | arg1 ` arg2 ` ... } hashCode
  The hashCode is checked against the SipHash_2_4 using the 128bit secret key.
  The hash input is the message count, the challenge for this connection and the message
  If the hash fails the cmd is set to DisconnectNow (0xff)
  It is then the responsability of the calling program to close the connection.
  and to call pfodSecurityClient::disconnected() when the connection is closed.

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

#include <Arduino.h>
#include "pfodStream.h"
#include "pfodParser.h"
#include "pfodMACClient.h"
#include "pfodRingBuffer.h"
#include "pfod_Base.h"


// used to suppress warning
#define pfod_MAYBE_UNUSED(x) (void)(x)

class pfodSecurityClient : public Stream {
  public:
    pfodSecurityClient();
    Stream *getRawDataStream();

    // from Stream
    // These methods only access command responses i.e. { ... } msgs
    // to pickup any raw data use the Stream returned by getRawDataStream()
    int available();
    int peek();
    int read();
    void flush();
    size_t write(uint8_t b);
    size_t write(const uint8_t *buffer, size_t size);


    void setResponseTimeout(unsigned long timeout_mS); // time between sending command and getting respose else close connection
    void setKeepAliveInterval(uint16_t _interval_mS);  // defaults to 0 i.e. keepAlives not sent unless this method is called

    void setDebugStream(Print* debugOut);

    /**
       initialize the Security parser
       args
       io_arg the Stream pointer to read and write to for pfod messages

       eepromAddress the starting address in eeprom to save the key and power cycles
          amount of eeprom used is (2 bytes power cycles + 1 byte key length + key bytes) ==> 3 + (input hexKey length)/2,
          use -1 to disable use of EEPROM, pfod_EEPROM.h will also disable EEPROM use for those boards that do not have EEPROm

       hexKey  pointer to program memory F("hexString") holding the key
        if this key is different from the current one in eeprom the eeprom is updated and the
        power cycles are reset to 0xffff
        if changing the key suggest you add 2 to your eepromAddress to move on from the previous
        one.  The power cycle EEPROM addresses are are written to on each power up
        if hexKey is omitted or empty or blank then there is no sercurity used and EEPROM is not used.

    */
    void connect(Stream* io_arg);
    void connect(Stream* io_arg, const __FlashStringHelper *hexKeyPgr);
    void connect(pfod_Base* _pfodBase);
    void connect(pfod_Base* _pfodBase, const __FlashStringHelper *hexKeyPgr);
    Stream* getLinkStream(); // access to the underlying link stream

    void closeConnection(); // called when connection closed or DisconnectNow returned from parser
    //static const byte DisconnectNow = '!'; // this is returned if pfodDevice should drop the connection
    void setIdleTimeout(unsigned long timeout_in_seconds); // defaults to 10sec if pfodBase not used, else set from pfodBase.getDefaultTimeOut()
    bool isIdleTimeout();
    void init();
  protected:
    void pollIO();
    bool startConnection();
    void closeStream();

  private:
    void startAuthorization();
    void closeConnectionAddEOF();
    void resetKeepAliveTimer();
    const static size_t MAX_CLIENT_CMD_LEN = 256 + 8 + 2; // max cmd is 256 + 8 for hash + 2 padding (for nulls)
    const static size_t MAX_SERVER_RESPONSE_LEN = 1024 + 8 + 2; // max response is 1024 + 8 for hash + 2 padding;
    const static size_t TX_BUFFER_SIZE = MAX_CLIENT_CMD_LEN; // temp storage for incoming outgoing msgs
    const static size_t RX_BUFFER_SIZE = MAX_SERVER_RESPONSE_LEN; // temp storage for incoming msgs
    uint8_t tx_buffer[TX_BUFFER_SIZE]; // for now temp storage for outgoing msgs
    uint8_t rx_buffer[RX_BUFFER_SIZE]; // for now temp storage for incoming msgs
    uint8_t rx_temp_buffer[RX_BUFFER_SIZE]; // for now temp storage for incoming msgs
    uint8_t rx_raw_data_buffer[RX_BUFFER_SIZE]; // 1K ringbuffer for raw data
    pfodRingBuffer txBuf;
    pfodRingBuffer rxBuf;
    pfodRingBuffer rxTempBuf;
    pfodRingBuffer rxRawDataBuf; // incoming raw data is split off here
    void clearTxRxBuffers();
    uint16_t keepAliveIntervalValue; // keepAliveIntervalValue 0 means no keepAlives sent
    uint16_t keepAliveInterval;
    unsigned long keepAliveTimer;
    bool setKeepAliveIntervalCalled;

    size_t writeToIO(uint8_t b);
    void readFromIO();
    Stream *io;
    pfod_Base* pfod_Base_set; // null if not set
    Print *debugOut;
    pfodMACClient mac;
    byte authorizing;
    byte challenge[pfodMAC::challengeByteSize + 1]; // add one for hash identifier save this for calculating msg hash
    unsigned long responseTimer;
    unsigned long responseTimeout;
    unsigned long responseTimeoutValue;
    void restartResponseTimer();
    void stopResponseTimer();
    bool waitingForResponse; // true once } sent to block further commands until response received or link closed
    boolean noPassword;
    void setAuthorizeState(int auth);
    int msgHashCount;
    bool collectCheckHash(uint8_t b);
    void handleChallengeResponse();
    static const byte Msg_Hash_Size = pfodMAC::msgHashByteSize * 2; // number of hex digits for msg hash
    static const byte Msg_Hash_Size_Bytes = pfodMAC::msgHashByteSize; // number of hex bytes for msg hash i.e. 4
    byte msgHashBytes[Msg_Hash_Size + 1]; // allow for null outgoing
    byte incomingHashBytes[Msg_Hash_Size + 1];
    uint32_t inMsgCount;
    uint32_t outMsgCount;
    uint8_t outputParserState; // state of output parser
    boolean initialization;
    const __FlashStringHelper *hexKeyPgr;
    bool doFlush; // set to true for SMS /ESP-AT only, otherwise false
    bool connectionClosed; // set true by constructor and closeConnection(), set true after authorization succeeds
    bool closingConnectionOut; // sending {!}
    bool closingConnectionIn; // receiving {!}
    bool foundMsgStart; // true when find { set false on next char
    bool connecting; // set true is in the process of authorizing
    // this prevents call to closeConnection from connect provided last connection close cleanly.
    unsigned long timerDebug_ms; 
    unsigned long idleTimeout;
    unsigned long idleConnectionTimerStart; // holds the start millis()
    unsigned long idleConnectionTimer; // used for both authorizeation timeout and idletimeout
    bool setIdleTimeoutCalled; 
    void startIdleConnectionTimer();
    void stopIdleConnectionTimer();
};

#endif // pfodSecurityClient_h

