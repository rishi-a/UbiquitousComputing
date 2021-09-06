#ifndef pfodParser_h
#define pfodParser_h
/**
  pfodParser for Arduino
  Parses commands of the form { cmd | arg1 ` arg2 ... }
  Arguments are separated by `
  The | and the args are optional
  This is a complete paser for ALL commands a pfodApp will send to a pfodDevice
  see www.pfod.com.au  for more details.

  pfodParser adds about 482 bytes to the program and uses about 260 bytes RAM

  The pfodParser parses messages of the form
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

#include <Arduino.h>
#include "pfodStream.h"
#include "pfodParserUtils.h"
#include "pfodDelay.h"
#include "pfodDwgs.h"
#include "pfodControl.h"

// used to suppress warning
#define pfod_MAYBE_UNUSED(x) (void)(x)

class pfodParser: public Print {
  public:
    // you can reduce this value if you are sending shorter commands.  Most pfod commands are very short <20 bytes, but depends on the pfod menu items you serve
    // but never increase it.
    static const byte pfodMaxMsgLen = 0xff; // == 255, if no closing } by now ignore msg

    pfodParser(const char* version);
    pfodParser();
    void connect(Stream* ioPtr);
    void closeConnection();

    byte parse();
    bool isRefresh(); // starts with {version: and the version matches this parser's version
    const char *getVersionRequested(); // the version asked for in the command i.e. {versionRequested:...}
    const char* getVersion();
    void setVersion(const char* version); // no usually needed
    void sendVersion(); // send ~ version to parser.print
    void sendRefreshAndVersion(unsigned long refresh_mS); // send `refresh_mS ~ version to parser.print
    byte* getCmd();
    byte* getFirstArg();
    byte* getNextArg(byte *start);
    byte getArgsCount();
    byte* parseLong(byte* idxPtr, long *result);
    /**
       pfodWaitingForStart if outside msg
       pfodMsgStarted if just seen opening {
       pfodInMsg in msg after {
       pfodMsgEnd if just seen closing }
    */
    byte getParserState();
    void setCmd(byte cmd);
    static const byte pfodWaitingForStart = 0xff;
    static const byte pfodMsgStarted = '{';
    static const byte pfodRefresh = ':';
    static const byte pfodInMsg = 0;
    static const byte pfodMsgEnd = '}';
    void setDebugStream(Print* debugOut); // does nothing
    size_t write(uint8_t c);
    int available();
    int read();
    int peek();
    void flush();
    void setIdleTimeout(unsigned long timeout); // does nothing in parser
    Stream* getPfodAppStream(); // get the command response stream we are writing to
    // for pfodParser this is also the rawData stream

    // this is returned if pfodDevice should drop the connection
    // only returned by pfodParser in read() returns -1
    void init();  // for now
    byte parse(byte in); // for now
    void ignoreSeqNum(); // for pfodSecurity so hash does not accidently drop a command.
    byte parseDwgCmd();
    const byte* getDwgCmd(); // valid only after parseDwgCmd() called on image cmd
    bool isTouch(); // default TOUCH even if not parsed
    bool isClick();
    bool isDown();
    bool isDrag();
    bool isUp();
    bool isPress();
    //    bool isEntry();
    //    bool isExit();

    uint8_t getTouchType();
    int getTouchedCol(); // default 0
    int getTouchedRow(); // default 0
    const static int TOUCH = 0;
    const static int DOWN = 1;
    const static int DRAG = 2;
    const static int UP = 4;
    const static int CLICK = 8;
    const static int PRESS = 16;
    //    const static int ENTRY = 32;
    //    const static int EXIT = 64;
    const static int DOWN_UP = 256; // only for touchZone filter send, never recieved by parser
    const static int TOUCH_DISABLED = 512; // only for touchZone filter send, never recieved by parser

  private:
     void constructInit();
    //static const byte DisconnectNow = '!';
    Stream* io;
    char emptyVersion[1];
    byte emptyBytes[1];
    byte argsCount;  // no of arguments found in msg
    byte argsIdx;
    byte parserState;
    byte args[pfodMaxMsgLen + 1]; // allow for trailing null
    byte *versionStart;
    const byte *activeCmdStart;
    uint8_t seqNum; // 0 if not set else last char before leading {
    uint8_t lastSeqNum; // 0 if not set else last char before leading {
    byte ignoreCmdSeqNum; // != 0 if should ignore them
    uint8_t touchType;
    int col;
    int row;
    byte *cmdStart;
    bool refresh;
    const char *version;
    static const byte pfodBar = (byte)'|';
    static const byte pfodTilda = (byte)'~';
    static const byte pfodAccent = (byte)'`';
    static const byte pfodArgStarted = 0xfe;
};

#endif // pfodParser_h

