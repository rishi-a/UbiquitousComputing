/**
  pfodRawCmdParser for Arduino
  The pfodRawCmdParser parses messages of the form
  { cmd ` arg1 ` arg2 ` ... }
  or
  { cmd ~ arg1}
  or
  { cmd }
  The args are optional
  This is a complete paser for ALL commands a pfodApp will send to a pfodDevice
  see www.pfod.com.au  for more details.

  pfodRawCmdParser adds about 482 bytes to the program and uses about 260 bytes RAM

  The pfodRawCmdParser parses messages of the form
  { cmd ` arg1 ` arg2 ` ... }
  or
  { cmd ~ arg1}
  or
  { cmd }
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

#include "pfodRawCmdParser.h"

pfodRawCmdParser::pfodRawCmdParser() {
  io = NULL;
  emptyVersion[0] = 0;
  init();
}

/**
   Note: there must NOT be null the io stream
*/
void pfodRawCmdParser::init() {
  argsIdx = 0;
  readIdx = 0;
  args[0] = 0; // no cmd
  args[1] = 0; //
  args[pfodMaxMsgLen] = 0; // double terminate
  args[pfodMaxMsgLen - 1] = 0;
  cmdStart = args; // if no version
  parserState = pfodWaitingForStart; // not started yet pfodInMsg when have seen {
  refresh = false;

}

void pfodRawCmdParser::connect(Stream* ioPtr) {
  init();
  io = ioPtr;
}

void pfodRawCmdParser::closeConnection() {
  //io = NULL; cannot clear this here
  init();
}

void pfodRawCmdParser::setDisconnect() {
  readIdx = 3;
  argsIdx = 0;
  args[0] = '{'; args[1] = '!'; args[2] = '}'; args[3] = 0; args[4] = 0;
}

Stream* pfodRawCmdParser::getPfodAppStream() {
  return io;
}

size_t pfodRawCmdParser::write(uint8_t c) {
  if (!io) {
    return 1; // cannot write if io null but just pretend to
  }
  return io->write(c);
}

int pfodRawCmdParser::available() {
  return argsIdx - readIdx;
}

int pfodRawCmdParser::read() {
  int rtn = peek();
  if (rtn != 0) {
    readIdx++;
  }
  return rtn;
}
int pfodRawCmdParser::peek() {
  int rtn = 0;
  if (readIdx < argsIdx) {
    rtn = args[readIdx];
  }
  return rtn;
}

void pfodRawCmdParser::flush() {
  if (!io) {
    return ; // cannot write if io null but just pretend to
  }
  //io->flush();
}


//void pfodRawCmdParser::setCmd(byte cmd) {
//  init();
//  args[0] = cmd;
//  args[1] = 0;
//  cmdStart = args;
//}

/**
   Return pointer to start of raw cmd i.e. { ... } null terminated
*/
byte* pfodRawCmdParser::getRawCmd() {
  return args;
}


/**
   pfodWaitingForStart if outside msg
   pfodMsgStarted if just seen opening {
   pfodInMsg in msg after {
   pfodMsgEnd if just seen closing }
*/
byte pfodRawCmdParser::getParserState() {
  if ((parserState == pfodWaitingForStart) ||
      (parserState == pfodMsgStarted) ||
      (parserState == pfodInMsg) ||
      (parserState == pfodMsgEnd) ) {
    return parserState;
  } //else {
  return pfodInMsg;
}

byte pfodRawCmdParser::parse() {
  byte rtn = 0;
  if (!io) {
    return rtn;
  }
  while (io->available()) {
    int in = io->read();
    rtn = parse((byte)in);
    if (rtn != 0) {
      // found msg
      return rtn;
    }
  }
  return rtn;
}

/**
   parse
   Inputs:
   byte in -- byte read from Serial port
   Return:
   return 0 if complete message not found yet
   else return first char of cmd when see closing }
   or ignore msg if pfodMaxCmdLen bytes after {
   On non-zero return args[] contains
   the cmd null terminated followed by the args null terminated
   argsCount is the number of args

   parses
   { cmd | arg1 ` arg2 ... }
   { cmd ` arg1 ` arg2 ... }
   { cmd ~ arg1 ~ arg2 ... }
   save the cmd in args[] replace |, ~ and ` with null (\0)
   then save arg1,arg2 etc in args[]
   count of args saved in argCount
   on seeing } return first char of cmd
   if no } seen for pfodMaxCmdLen bytes after starting { then
   ignore msg and start looking for { again

   States:
   before {   parserState == pfodWaitingForStart
   when   { seen parserState == pfodInMsg
*/
byte pfodRawCmdParser::parse(byte in) {
  if (in == 0xff) {
    // note 0xFF is not a valid utf-8 char
    // but is returned by underlying stream if start or end of connection
    // NOTE: Stream.read() is wrapped in while(Serial.available()) so should not get this
    // unless explicitlly added to stream buffer
    init(); // clean out last partial msg if any
    return 0;
  }
  if ((parserState == pfodWaitingForStart) || (parserState == pfodMsgEnd)) {
    parserState = pfodWaitingForStart;
    if (in == pfodMsgStarted) { // found {
      init(); // clean out last cmd
      parserState = pfodMsgStarted;
      args[argsIdx++] = pfodMsgStarted;
    }
    // else ignore this char as waiting for start {
    // always reset counter if waiting for {
    return 0;
  }

  // else have seen {
  if ((argsIdx >= (pfodMaxMsgLen - 1)) && // -1 since { is stored in rawCmd // was -2 since { never stored
      (in != pfodMsgEnd)) {
    // ignore this msg and reset
    // should not happen as pfodApp should limit
    // msgs sent to pfodDevice to <=255 bytes
    init();
    return 0;
  }
  // first char after opening {
  if (parserState == pfodMsgStarted) {
    parserState = pfodInMsg;
    if (in == pfodRefresh) {
      refresh = true;
      args[argsIdx++] = in;
      return 0; // skip this byte if get {:
    }
  }
  // else continue. Check for version:
  if ((in == pfodRefresh) && (versionStart != args)) {
    // found first : set version pointer
    args[argsIdx++] = in;
    versionStart = args;
    cmdStart = args + argsIdx; // next byte after :
    refresh = true; // (strcmp((const char*)versionStart, (const char*)version) == 0);
    return 0;
  }

  // else process this msg char
  // look for special chars | ' }
  if ((in == pfodMsgEnd) || (in == pfodBar) || (in == pfodTilda) || (in == pfodAccent)) {
    args[argsIdx++] = in;
    if (parserState == pfodArgStarted) {
      // increase count if was parsing arg
      argsCount++;
    }
    if (in == pfodMsgEnd) {
      parserState = pfodMsgEnd; // reset state
      args[argsIdx] = 0; // terminate cmd
      readIdx = 0;
      // return command byte found
      // this will return 0 when parsing {} msg
      return cmdStart[0]; // just return all cmds parsed
    } else {
      parserState = pfodArgStarted;
    }
    return 0;
  }
  // else normal byte
  args[argsIdx++] = in;
  return 0;
}


