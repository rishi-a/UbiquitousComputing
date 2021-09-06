#ifndef pfodRawCmdParser_h
#define pfodRawCmdParser_h
/**
  pfodRawCmdParser for Arduino
  Parses commands of the form { cmd | arg1 ` arg2 ... }
  Arguments are separated by `
  The | and the args are optional
  This is a complete paser for ALL commands a pfodApp will send to a pfodDevice
  see www.pfod.com.au  for more details.

  pfodRawCmdParser adds about 482 bytes to the program and uses about 260 bytes RAM

  The pfodRawCmdParser parses messages of the form
  { cmd | arg1 ` arg2 ` ... }
  The message is parsed into the args array by replacing '|', '`' and '}' with '/0' (null)
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

#include "Arduino.h"

#include "pfodStream.h"
#include "pfodParser.h"

// used to suppress warning
#define pfod_MAYBE_UNUSED(x) (void)(x)

class pfodRawCmdParser: public Stream {
  public:
    pfodRawCmdParser();
    void connect(Stream* ioPtr);

    byte parse();
    byte* getRawCmd();
    /**
       pfodWaitingForStart if outside msg
       pfodMsgStarted if just seen opening {
       pfodInMsg in msg after {
       pfodMsgEnd if just seen closing }
    */
    byte getParserState();
    //void setCmd(byte cmd);
    //void setDebugStream(Print* debugOut); // does nothing
    size_t write(uint8_t c);
    int available();
    int read();
    int peek();
    void flush();
    Stream* getPfodAppStream(); // get the command response stream we are writing to
    // for pfodRawCmdParser this is also the rawData stream

    // this is returned if pfodDevice should drop the connection
    // only returned by pfodRawCmdParser in read() returns -1
    void init();
    byte parse(byte in);
    void setDisconnect(); // sets parser to {!}
    void closeConnection();
  private:
    //static const byte DisconnectNow = '!';
    Stream* io;
    // you can reduce this value in pfodParser.h if you are sending shorter commands.  Most pfod commands are very short <20 bytes, but depends on the pfod menu items you serve.
    // but never increase it.
    static const byte pfodMaxMsgLen = pfodParser::pfodMaxMsgLen; //0xff; // == 255, if no closing } by now ignore msg
    byte emptyVersion[1];
    byte readIdx;
    byte argsIdx;
    byte argsCount;  // no of arguments found in msg
    byte parserState;
    byte args[pfodMaxMsgLen + 1]; // allow for trailing null
    byte *versionStart;
    byte *cmdStart;
    bool refresh;
    static const byte pfodWaitingForStart = 0xff;
    static const byte pfodMsgStarted = '{';
    static const byte pfodRefresh = ':';
    static const byte pfodInMsg = 0;
    static const byte pfodMsgEnd = '}';
    static const byte pfodBar = (byte)'|';
    static const byte pfodTilda = (byte)'~';
    static const byte pfodAccent = (byte)'`';
    static const byte pfodArgStarted = 0xfe;
};

#endif // pfodParser_h

