#ifndef pfodSMS_SIM5320_h
#define pfodSMS_SIM5320_h

/*
   (c)2014-2018 Forward Computing and Control Pty. Ltd.
   NSW Australia, www.forward.com.au
   This code is not warranted to be fit for any purpose. You may only use it at your own risk.
   This code may be freely used for both private and commercial use
   Provide this copyright is maintained.
*/
/*
  This library uses about 22K of ROM (program space) and 3234 bytes of RAM
  and so is NOT suitable for use on an UNO
  Use a Mega2560 or similar with more then 4K of RAM
*/

#include "Arduino.h"

#include "pfodStream.h"
#include "pfod_Base.h"
#include "pfod_rawOutput.h"

#define SMS_MAX_SIZE 160
#define SMS_RX_BUFFER_SIZE 267
// max msg size after decode  256 + + 8(hash) + 2(extra) +null
// max pfod msg size
#define SMS_TX_MSG_SIZE 1026
// 1024 + 2
#define SMS_TX_BUFFER_SIZE (SMS_TX_MSG_SIZE+8+2)
// add a little 1026 + 8(hash) + 2(extra) + null

class pfodSMS_SIM5320: public pfod_Base {

  public:
    pfodSMS_SIM5320();
    void init(Stream *_gprs, int resetPin = -1, int powerOnOffPin = -1, int powerStatusPin = -1);  // use -1 if not used
    void setDebugStream(Stream* out);
    unsigned long getDefaultTimeOut();
    Print* getRawDataOutput();
    int available(void);
    int peek(void);
    int read(void);
    void flush(void);
    size_t write(uint8_t);
    size_t writeRawData(uint8_t c);
    Stream* getPfodAppStream(); // use pfodSecurity.getPfodAppStream() instead get the command response stream we are reading from and writing to
    void _closeCurrentConnection();

    
  private:
    int powerResetPin;
    int powerStatusPin;
    int resetPin;
    const static int MAX_PHNO_LEN = 20;
    const static int MAX_PHNO_CHECK = 6; // from end
    struct connection {
      int connectionNo; // = 0; is valid
      int msgNo_rx; // = 0; // first expected
      int msgNo_tx; // = 0; // next msgNo for next SMS being sent
      int msgNo_tx_newMsg; // = 0; next msgNo for start of new response
      // the msgNo_tx is set to this on a new msg
      int msgNo_tx_resend; // = 0; // starting msgNo for last response
      // the msgNo_tx is set to this on resend
      boolean timedOut;
      //knownConnection conPtr*;
      char returnPhNo[MAX_PHNO_LEN + 1]; // plus null  idx runs from 0 to MAX_PHNO_LEN
    };

    struct knownConnection {
      int connectionNo; // = 0; is valid
      char returnPhNo[MAX_PHNO_LEN + 1]; // plus null  idx runs from 0 to MAX_PHNO_LEN
    };

    static const int MAX_KNOWN_CONNECTION = 10;
    struct knownConnection knownConnectionsArray[MAX_KNOWN_CONNECTION]; // this allocate space for structs as well
    struct knownConnection* knownArrayPtrs[MAX_KNOWN_CONNECTION]; // allocate space for ptrs

    void init();
    void initPowerUp();
    int findEmptyKnownConnection();
    void updateMsgNoForStartNewCommand();
    void updateMsgNoForStartResend();

    void stopResponseSends();
    void stopAllSends();

    void clearKnownConnection(knownConnection * kCon);
    void clearKnownConnection(int idx);
    void printKnownConnections();
    void printKnownConnection(knownConnection * kCon);
    void swapKnownConnections();
    void setTopKnownConnectionFromCurrent();
    void moveKnownConnectionToTop(int idx);
    void updateKnownConnectionNo(connection * con); // always succeeds
    void updateKnownConnectionNoAndMoveToTop(connection * con); // always succeeds
    void setNextExpectedConnectionMsgNo(connection * con, boolean foundEndParens);

    typedef enum decode_sms_return_t {
      IGNORE, // invalid msg, wrong msgNo etc
      RESEND_LAST_RESPONSE, // ignore msg and resend last response
      CLOSE_CONNECTION, // close current connection no change in connectionNo
      START_NEW_COMMAND, // start a new command no new connection
      PROCESS_NEXT_PART_OF_COMMAND, // process continuation SMS of a command for connection expectedMsgNo has been updated
      CLOSE_CONNECTION_AND_UPDATE_CONNECTION, // close current and update for new connectionNo
      NEW_CONNECTION_SAME_PHONE_NO, // new connect but with same phoneNo
      IGNORE_CONNECTION,  // ignore connection attempt as current connection not timed out but let pfodApp retry
      START_NEW_CONNECTION, // start a new connection from a different phone no
      START_NEW_CONNECTION_AND_CLOSE // start new connection but msg is {!} so no reply but send alert to current and process msg
    } decode_sms_return_t;

    decode_sms_return_t decodeSMS(connection * current, connection * next, const char* smsChars, byte * decodedSMS,
                                  size_t* currentDecodedLen);

    bool havePowerResetPins;
    bool haveResetPin;
    void clearParser();
    void clearResponseSet();
    boolean decodeSMScmd(const char*smsChars, byte * decodedSMS, size_t* currentDecodedLen);
    int checkSMSmsgForValidNewConnection(int connectionNo, int msgNo, const char* decodedSMS, boolean foundCmdClose);
    size_t encodeSMS(int connectionNo, int msgNo, const byte * msgBytes, size_t msgBytesIdx, char* encodedSMS);
    //boolean isResendStoredSMS();

    const char* findStr(const char* msg, const __FlashStringHelper * ifsh);
    const char *findStr(const char* str, const char* target);

    const static size_t MAX_BYTES_TO_CONVERT = 117;

    const static int MAX_MSG_NO_LEN = 3;
    const static int MAX_MSG_LEN = 256; // max msg len then null
    char gprsMsg[MAX_MSG_LEN + 1]; // plus null  idx runs from 0 to MAX_MSG_LEN
    int gprsMsgIdx; // = 0;
    unsigned char gprsLastChar; // = '\0';
    // keep track of the next 10 incoming SMS sim card msg numbers are reported by +CMTI:
    const static int MAX_INCOMING_MSG_NOS = 10; // = 10+1 for head != tail

    uint8_t incomingMsgNos_head; // from 0 to MAX_INCOMING_MSG_NOS;
    uint8_t incomingMsgNos_tail; // from 0 to MAX_INCOMING_MSG_NOS;
    // add to head take from tail
    // stop adding when head+1%MAX_INCOMING_MSG_NOS+1 == tail
    // stop taking when tail == head
    // start tail==head == 0;
    // take (tail == head) so empty
    // add test (head+1%MAX_INCOMING_MSG_NOS != tail) add to [head] and head++
    // take tail!= head take[tail++]  now tail == head
    // add 3  [0],[1]
    char incomingMsgNos[MAX_INCOMING_MSG_NOS + 1][MAX_MSG_NO_LEN + 1];
    void requestIncomingSmsMsg(const char *msgNo);

    size_t rxBufferLen;  // runs from 0 to SMS_RX_BUFFER_SIZE
    size_t txBufferLen;  // runs from 0 to SMS_TX_BUFFER_SIZE
    size_t rxBufferIdx; // runs from 0 to rxBufferLen
    size_t txBufferIdx; // runs from 0 to txBufferLen
    byte rxBuffer[SMS_RX_BUFFER_SIZE + 1]; // allow for terminating null
    byte txBuffer[SMS_TX_BUFFER_SIZE + 1]; // allow for terminating null
    char responseSMSbuffer[SMS_MAX_SIZE + 1]; // only 160 + 1
    static const int SMS_RAWDATA_TX_BUFFER_SIZE = MAX_BYTES_TO_CONVERT - 1; //117
    byte rawdataTxBuffer[SMS_RAWDATA_TX_BUFFER_SIZE + 1]; // allow for terminating null and force null at end
    size_t rawdataTxBufferLen;  // runs from 0 to SMS_RAWDATA_TX_BUFFER_SIZE
    size_t rawdataTxBufferIdx; // runs from 0 to rawdataTxBufferLen
    char rawdataSMSbuffer[SMS_MAX_SIZE + 1]; // only 160 + 1
    void flushRawData();
    void clearRawdataTxBuffer();
    boolean isSendingResponse();
    boolean haveRequestedIncomingMsg;
    void requestNextIncomingMsg();

    boolean expectingMessageLines; // note msg from pfodApp may have embedded new line
    boolean sendingRawdata; // true if processing and sending SMS response to command
    boolean sendingResponse; // true if processing and sending SMS response to command
    boolean sendingResend; // true if processing and sending SMS resend of previous response to command
    boolean resendLastResponse;  // true if should start resend of last response
    // if sendingResponse then canSendRawData() return false and any write is just dropped
    boolean responseSet; // true if tx_buffer holds respons upto } and no new cmd recieved
    boolean deleteReadSmsMsgs; // true if should delete read msgs
    boolean foundOK; // true when OK was last gprs msg

    struct connection connection_A;
    struct connection connection_B;
    struct connection *currentConnection;
    struct connection *nextConnection;
    void pollSMSboard();
    void sendSMSrawData();

    const char* collectGprsMsgs();
    void clearGprsLine();
    void processGprsLine(const char* line);
    // maxLen includes null at end
    // void getProgStr(const __FlashStringHelper *ifsh, char*str, int maxLen);
    int decodeInt(const char*smsChars, size_t idx);
    size_t encodeInt(int number, char*rtn, size_t idx);
    size_t convertThree8Bytes(long three8Bytes, char*rtn, size_t idx);
    byte decodeSMSChar(byte encodedChar);
    byte encodeSMSChar(byte encodedChar);
    //Stream *console;
    Stream *gprs; // sms shield serial connection
    const static byte invalidSMSCharReturn = 0xff; // == 255,
    byte bytesToConvert[MAX_BYTES_TO_CONVERT + 1]; // max bytes to convert + null

    void closeConnection(connection * con);
    void clearConnection(connection * con);

    boolean newConnectionNumberGreater(int current, int next);
    boolean validSMSmsgLen(const char* smsChars);
    void printIgnoreOutOfSequence();
    //unsigned long timedOutTimer;
    //unsigned long start_mS;
    int findMatchingKnownPhoneNo(const char *phNo);
    boolean phoneNoMatchesCurrent(const char*newPhNo);
    boolean phoneNoMatches(const char*oldPhNo, const char*newPhNo);
    boolean phoneNoHas6Digits(const char *phNo);
    void checkGPRSpoweredUp();
    int powerCycleGPRS();
    boolean setupSMS();
    boolean gprsReady;
    boolean powerUpGPRS();
    void clearTxBuffer();

    void printConnection(connection * con);
    void swapConnections();
    void setPhoneNo(connection * con, const char* phNo);

    void printCurrentConnection();
    void printNextConnection();
  //  void startTimer();

    void startSMS(const char* phoneNo);
    void sendNextSMS(); // send next part of txbuffer
    void clearReadSmsMsgs();
    void clearAllSmsMsgs();
    bool clearAllSmsMsgsOnInit();
    void emptyIncomingMsgNo();

    void gprsPrint(const char* str);
    void gprsPrint(char c);
    void gprsPrint(const __FlashStringHelper * ifsh);

    boolean haveIncomingMsgNo();
    boolean addIncomingMsgNo(const char* msgNo);
    const char* getIncomingMsgNo();
    void deleteIncomingMsg(const char *newMsgNo);

    void turnEchoOn();
    void turnEchoOff();

    pfod_rawOutput raw_io;
    pfod_rawOutput* raw_io_ptr;


  private:
    Stream* debugOut;

};

#endif // pfodSMS_SIM5320_h

