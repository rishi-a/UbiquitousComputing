#ifndef pfodRadio_h
#define pfodRadio_h
/**
  pfodRadio for Arduino
  Handles send and receive of radio msgs with acks and retries
*/
/*
   (c)2014-2018 Forward Computing and Control Pty. Ltd.
   NSW Australia, www.forward.com.au
   This code is not warranted to be fit for any purpose. You may only use it at your own risk.
   This code may be freely used for both private and commercial use
   Provide this copyright is maintained.
*/
#include <Arduino.h>
#include "pfodStream.h"
#include "pfod_Base.h"
#include "pfodRadioMsg.h"
#include "pfodRingBuffer.h"
#include "pfodRadioDriver.h"

/// the default retry timeout in milliseconds  change this with setAckTimeout(mS)
#define RH_DEFAULT_TIMEOUT 500

/// The default number of retries  change this with setNoOfRetries(noRetries)
#define RH_DEFAULT_RETRIES 5

///
/// An ack consists of a message with:
/// - TO set to the from address of the original message
/// - FROM set to this node address
/// - ID set to the ID of the original message
/// - FLAGS with the RH_FLAGS_ACK bit set
/// - 1 octet of payload containing ASCII '!' (since some drivers cannot handle 0 length payloads)
///

class pfodRadio: public pfod_Base {
  public:

    pfodRadio(pfodRadioDriver *_driver, uint8_t _thisAddress);
    void setDebugStream(Print* _debugOut);
    void debugPfodRadioMsg(pfodRadioMsg* msg);

    void connectTo(uint8_t _to); // seting up as client and connect

    void listen(); // setting up as server

    // max timeout is 32.7sec (32767mS)
    void setAckTimeout(uint16_t _timeout_mS);

    void setNoOfRetries(uint8_t _noOfRetries);

    void setMaxWriteBufLen(size_t _maxLen); // NOTE: this setting is ignored for the moment
    // max len of data written to this radio in one block.
    // this data block is stored and then sent in smaller radio sized chunks
    // defaults to 255 for Clients
    // and 1023 for Server (pfodDevice)
    // you can set a smaller value based on your menus and commands.  If using pfodDwgs leave server setting at 1023.
    // code adds 8 for security has and 2 for padding

    bool recvMsg(); //uint8_t* _buf, uint8_t* _len, uint8_t* _receivedFrom, uint8_t* _addressedTo, uint8_t* _incomingMsgSeqNo, uint8_t* _ackMsgSeqNo);
    bool isNewMsg();
    bool isAckForLastMsgSent();
    // if sendMsg with ack use last arg
    // else ommit last arg to just send message without an ack included
    //  size_t sendMsg(uint8_t *_buf, uint8_t _len);
    size_t sendMsg();  // send from txBuf is any data waiting

    bool isConnectionClosed();
    uint16_t getLastRSSI();

    bool notInTxMode();
    bool inTxMode();
    pfodRadioMsg* getReceivedMsg();
    void sendAck();
    uint8_t getThisAddress();
    bool isServer(); // true is running as server else false
    bool init(); // calls driver init
    // from Stream
    int available();
    int peek();
    int read();
    void flush();
    size_t write(uint8_t b);
    unsigned long getDefaultTimeOut();
    void _closeCurrentConnection(); // called from pfodSecurity/pfodSecurityClient does NOT push 0xff into rx buffer after clearing buffers
    size_t writeRawData(uint8_t c);
    Print* getRawDataOutput();

  protected:
    bool sendTo(uint8_t* _buf, uint8_t _len, uint8_t _address);
    pfodRadioMsg receivedMsg;
    pfodRadioMsg lastMsgSent;
    void sendMsg(pfodRadioMsg * radioMsg); // returns
    void pollRadio();
    void clearTxRxBuffers();
    bool waitingForAckOfLastMsgSent;
    unsigned long timeLastMsgSent;
    bool connectionClosed;
    void checkIfNeedToConnect();
    void resendLastMsg();
    bool isNewConnectionRequest();
    uint16_t getRandomTimeout();
    void setTimeLastMsgSent();
    Print* debugOut;
    void checkForAllowableNewConnection();
    void closeConnection();



  private:
    void resetTargetAndSeqNos();
    bool isPureAck();
    void resetLinkConnectTimeout(); // set to (ackTimeout*1.5) * noOfRetries, timesout resends of msgSeqNo 0 ack

    pfodRingBuffer rawDataBuf;
    pfodRingBuffer txBuf;
    pfodRingBuffer rxBuf;
    unsigned long lastWriteTime; // delay send for 50mS to let multiple writes accumulate.
    static const unsigned long SEND_MSG_DELAY_TIME = 50; // ms
    static const uint8_t sizeOfLong = (uint8_t) (sizeof(long) * 8); //bits per byte

    bool canAcceptNewConnections;
    bool newConnection; // set true on accepting new connection msgSeqNo 0
    uint8_t retryCount; // retry send for current msg waiting to be acked
    uint8_t thisAddress; // address of this node
    uint8_t targetAddress; // where to send the to the other connection,
    bool isServerNode;
    /// The last sequence number to be used
    /// Defaults to 0
    uint16_t ackTimeout; // in milliseconds
    uint16_t randomTimeout; // == timeout + random()*timeout
    uint8_t outGoingSeqNo; // last outgoing seq no (expecting ack for this one
    uint8_t inComingSeqNo; // next expected incoming seqNo
    size_t maxWriteBufLen;
    const static size_t MAX_CLIENT_CMD_LEN = 255 + 8 + 2; // max cmd is 255 + 8 for hash + 2 padding (for nulls)
    const static size_t MAX_SERVER_RESPONSE_LEN = 1023 + 8 + 2; // max response is 1023 + 8 for hash + 2 padding;
    const static size_t BUFFER_SIZE_256 = MAX_CLIENT_CMD_LEN; // temp storage for incoming msgs
    const static size_t BUFFER_SIZE_1024 = MAX_SERVER_RESPONSE_LEN; // temp storage for incoming msgs
    uint8_t rawDataBuffer_1024[BUFFER_SIZE_1024]; // for now temp storage for outgoing msgs
    uint8_t buffer_1024[BUFFER_SIZE_1024]; // for now temp storage for outgoing msgs
    uint8_t buffer_256[BUFFER_SIZE_256]; // for now temp storage for incoming msgs
    bool needToAckReceivedMsg;
    uint8_t noOfRetries; // how many retrys to do before link broken
    uint8_t retriesCount;
    pfodRadioMsg* receivedMsgPtr;
    pfodRadioMsg* lastMsgSentPtr;
    pfodRadioDriver* driver;
    uint16_t lastRSSI;
    unsigned long linkConnectionTimeoutTimer;
    unsigned long linkConnectionTimeout; // 0 if not running
    unsigned long debug_mS;
    unsigned long sendTimeStart_mS; // time to tx msg
    bool sendTimerRunning; // true if timing send time

};

#endif

