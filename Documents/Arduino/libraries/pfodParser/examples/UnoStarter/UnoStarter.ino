/* ===== pfod Command for Menu_1 ====
pfodApp msg {.} --> {,<bg n><b><+3><w>~Uno Starter`1000~V16|A<bg bl>`0~D4 output is ~~Low\High~|B<bg bl>`0~D5 PWM Setting ~%`255`0~100~0~|!F<-6>~|!C<bg bl>`1~D6 Input is ~~Low\High~t|!E<-6>~|!D<bg bl>`775~A0 ~V`1023`0~5~0~}
 */
// Using Serial and 9600 baud for send and receive
// Serial D0 (RX) and D1 (TX) on Arduino Uno, Micro, ProMicro, Due, Mega, Mini, Nano, Pro and Ethernet
// This code uses Serial so remove shield when programming the board
/*
 * (c)2014-2017 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provide this copyright is maintained.
 */
int swap01(int); // method prototype for slider end swaps

// ======================
// this is the pfodParser.h V2 file with the class renamed pfodParser_codeGenerated and with comments, constants and un-used methods removed
class pfodParser_codeGenerated: public Print { 
  public:
    pfodParser_codeGenerated(const char* version); pfodParser_codeGenerated(); void connect(Stream* ioPtr); void closeConnection(); byte parse(); bool isRefresh(); 
    const char* getVersion(); void setVersion(const char* version); void sendVersion(); byte* getCmd(); byte* getFirstArg();
    byte* getNextArg(byte *start); byte getArgsCount(); byte* parseLong(byte* idxPtr, long *result); byte getParserState();
    void setCmd(byte cmd); void setDebugStream(Print* debugOut); size_t write(uint8_t c); int available(); int read();
    int peek(); void flush(); void setIdleTimeout(unsigned long timeout); Stream* getPfodAppStream(); void init(); byte parse(byte in); 
  private:
    Stream* io; byte emptyVersion[1] = {0}; byte argsCount; byte argsIdx; byte parserState; byte args[255 + 1]; byte *versionStart;
    byte *cmdStart; bool refresh; const char *version;
};
//============= end of pfodParser_codeGenerated.h
pfodParser_codeGenerated parser("US17"); // create a parser to handle the pfod messages

// give the board pins names, if you change the pin number here you will change the pin controlled
int cmd_A_var; // name the variable for 'D4 output is'
const int cmd_A_pin = 4; // name the output pin for 'D4 output is'
int cmd_B_var; // name the variable for 'D5 PWM Setting'
const int cmd_B_pin = 5; // name the output pin for 'D5 PWM Setting'
int cmd_C_var; // name the variable for 'D6 Input is'
const int cmd_C_pin = 6; // name the input pin for 'D6 Input is'
int cmd_D_var; // name the variable for 'A0'
unsigned long cmd_D_adcStartTime=0; // ADC timer
unsigned long cmd_D_ADC_READ_INTERVAL = 1000;// 1sec, edit this to change adc read interval
const int cmd_D_pin = A0; // name the pin for 'A0'

// the setup routine runs once on reset:
void setup() {
  Serial.begin(9600);
  for (int i=3; i>0; i--) {
    // wait a few secs to see if we are being programmed
    delay(1000);
  }
  parser.connect(&Serial); // connect the parser to the i/o stream
  cmd_A_var = 0;
  //pinMode(cmd_A_pin, INPUT_PULLUP); 
  pinMode(cmd_A_pin, OUTPUT); // output for 'D4 output is' is initially LOW,
  //uncomment INPUT_PULLUP line above and set variable to 1, if you want it initially HIGH
  digitalWrite(cmd_A_pin,cmd_A_var); // set output
   cmd_B_var = 0;
  pinMode(cmd_B_pin, OUTPUT); // output for 'D5 PWM Setting' is initially LOW,
  digitalWrite(cmd_B_pin,cmd_B_var); // set output
   pinMode(cmd_C_pin, INPUT_PULLUP); // edit this to just pinMode(..,INPUT); if you don't want the internal pullup enabled

  // <<<<<<<<< Your extra setup code goes here
}

// the loop routine runs over and over again forever:
void loop() {
  byte cmd = parser.parse(); // parse incoming data from connection
  // parser returns non-zero when a pfod command is fully parsed
  if (cmd != 0) { // have parsed a complete msg { to }
    byte* pfodFirstArg = parser.getFirstArg(); // may point to \0 if no arguments in this msg.
    long pfodLongRtn; // used for parsing long return arguments, if any
    if ('.' == cmd) {
      // pfodApp has connected and sent {.} , it is asking for the main menu
      if (!parser.isRefresh()) {
        sendMainMenu(); // send back the menu designed
      } else {
        sendMainMenuUpdate(); // menu is cached just send update
      }

    // now handle commands returned from button/sliders
    } else if('A'==cmd) { // user moved slider -- 'D4 output is'
      // in the main Menu of Menu_1 
      // set output based on slider 0 == LOW, 1 == HIGH 
      parser.parseLong(pfodFirstArg,&pfodLongRtn); // parse first arg as a long
      cmd_A_var = (int)pfodLongRtn; // set variable
      digitalWrite(cmd_A_pin,cmd_A_var); // set output
      sendMainMenuUpdate(); // always send back a pfod msg otherwise pfodApp will disconnect.

    } else if('B'==cmd) { // user moved PWM slider -- 'D5 PWM Setting'
      // in the main Menu of Menu_1 
      parser.parseLong(pfodFirstArg,&pfodLongRtn); // parse first arg as a long
      cmd_B_var = (int)pfodLongRtn; // set variable
      analogWrite(cmd_B_pin,cmd_B_var); // set PWM output
      sendMainMenuUpdate(); // always send back a pfod msg otherwise pfodApp will disconnect.

//    } else if('F'==cmd) { // this is a label. pfodApp NEVER sends this cmd -- ''
//      // in the main Menu of Menu_1 

//    } else if('C'==cmd) { // this is a label. pfodApp NEVER sends this cmd -- 'D6 Input is'
//      // in the main Menu of Menu_1 

//    } else if('E'==cmd) { // this is a label. pfodApp NEVER sends this cmd -- ''
//      // in the main Menu of Menu_1 

//    } else if('D'==cmd) { // this is a label. pfodApp NEVER sends this cmd -- 'A0'
//      // in the main Menu of Menu_1 

    } else if ('!' == cmd) {
      // CloseConnection command
      closeConnection(parser.getPfodAppStream());
    } else {
      // unknown command
      parser.print(F("{}")); // always send back a pfod msg otherwise pfodApp will disconnect.
    }
  }
  cmd_C_var = digitalRead(cmd_C_pin);  // read digital input
  cmd_D_readADC(); 
  //  <<<<<<<<<<<  Your other loop() code goes here 
  
}

void closeConnection(Stream *io) {
  // add any special code here to force connection to be dropped
}
void cmd_D_readADC() {
  if ((millis() - cmd_D_adcStartTime) > cmd_D_ADC_READ_INTERVAL) {
    cmd_D_adcStartTime = millis(); // restart timer
    cmd_D_var = analogRead(cmd_D_pin);  // read ADC input
  }
}


void sendMainMenu() {
  // !! Remember to change the parser version string
  //    every time you edit this method
  parser.print(F("{,"));  // start a Menu screen pfod message
  // send menu background, format, prompt, refresh and version
  parser.print(F("<bg n><b><+3><w>~Uno Starter`1000"));
  parser.sendVersion(); // send the menu version 
  // send menu items
  parser.print(F("|A<bg bl>"));
  parser.print('`');
  parser.print(cmd_A_var); // output the current state 0 Low or 1 High 
  parser.print(F("~D4 output is ~~Low\\High~"));
 // Note the \\ inside the "'s to send \ ... 
  parser.print(F("|B<bg bl>"));
  parser.print('`');
  parser.print(cmd_B_var); // output the current PWM setting
  parser.print(F("~D5 PWM Setting ~%`255`0~100~0~"));
  parser.print(F("|!F<-6>"));
  parser.print(F("~"));
  parser.print(F("|!C<bg bl>"));
  parser.print('`');
  parser.print(cmd_C_var); // output the current state of the input
  parser.print(F("~D6 Input is ~~Low\\High~t"));
 // Note the \\ inside the "'s to send \ ... 
  parser.print(F("|!E<-6>"));
  parser.print(F("~"));
  parser.print(F("|!D<bg bl>"));
  parser.print('`');
  parser.print(cmd_D_var); // output the current ADC reading
  parser.print(F("~A0 ~V`1023`0~5~0~"));
  parser.print(F("}"));  // close pfod message
}

void sendMainMenuUpdate() {
  parser.print(F("{;"));  // start an Update Menu pfod message
  // send menu items
  parser.print(F("|A"));
  parser.print('`');
  parser.print(cmd_A_var); // output the current state 0 Low or 1 High 
  parser.print(F("|B"));
  parser.print('`');
  parser.print(cmd_B_var); // output the current PWM setting
  parser.print(F("|C"));
  parser.print('`');
  parser.print(cmd_C_var); // output the current state of the input
  parser.print(F("|D"));
  parser.print('`');
  parser.print(cmd_D_var); // output the current ADC reading
  parser.print(F("}"));  // close pfod message
  // ============ end of menu ===========
}
//=========================================================================
/* You can remove from here on if you have the pfodParser V2 library installed from
    http://www.forward.com.au/pfod/pfodParserLibraries/index.html
  * and add 
 #include <pfodEEPROM.h
 #include <pfodParser.h>
  * at the top of this file
  * and replace the line
 pfodParser_codeGenerated parser("V1"); // create a parser to handle the pfod messages
  * with
 pfodParser parser("V1");
*/
// this is the pfodParser.cpp V2 file with the class renamed pfodParser_codeGenerated and with comments, constants and un-used methods removed
pfodParser_codeGenerated::pfodParser_codeGenerated() { pfodParser_codeGenerated(""); }
pfodParser_codeGenerated::pfodParser_codeGenerated(const char *_version) { setVersion(_version); io = NULL; init(); }
void pfodParser_codeGenerated::init() {
  argsCount = 0; argsIdx = 0; args[0] = 0; args[1] = 0; cmdStart = args; versionStart = emptyVersion; parserState = ((byte)0xff); refresh = false;
}
void pfodParser_codeGenerated::connect(Stream* ioPtr) { init(); io = ioPtr; }
void pfodParser_codeGenerated::closeConnection() { init(); }
Stream* pfodParser_codeGenerated::getPfodAppStream() { return io; }
size_t pfodParser_codeGenerated::write(uint8_t c) {
  if (!io) {
    return 1;
  }
  return io->write(c);
}
int pfodParser_codeGenerated::available() { return 0; }
int pfodParser_codeGenerated::read() { return 0; }
int pfodParser_codeGenerated::peek() { return 0; }
void pfodParser_codeGenerated::flush() {
  if (!io) { return; }
  // nothing here for now
}
void pfodParser_codeGenerated::setIdleTimeout(unsigned long timeout) { }
void pfodParser_codeGenerated::setCmd(byte cmd) { init(); args[0] = cmd; args[1] = 0; cmdStart = args; versionStart = emptyVersion; }
byte* pfodParser_codeGenerated::getCmd() { return cmdStart; }
bool pfodParser_codeGenerated::isRefresh() { return refresh; } 
const char* pfodParser_codeGenerated::getVersion() { return version; }
void pfodParser_codeGenerated::setVersion(const char* _version) { version = _version; }
void pfodParser_codeGenerated::sendVersion() { print('~'); print(getVersion()); }
byte* pfodParser_codeGenerated::getFirstArg() {
  byte* idxPtr = cmdStart;
  while (*idxPtr != 0) { ++idxPtr; }
  if (argsCount > 0) { ++idxPtr; }
  return idxPtr;
}
byte* pfodParser_codeGenerated::getNextArg(byte *start) {
  byte* idxPtr = start;
  while ( *idxPtr != 0) { ++idxPtr; }
  if (idxPtr != start) { ++idxPtr; }
  return idxPtr;
}
byte pfodParser_codeGenerated::getArgsCount() { return argsCount; }
byte pfodParser_codeGenerated::getParserState() {
  if ((parserState == ((byte)0xff)) || (parserState == ((byte)'{')) || (parserState == 0) || (parserState == ((byte)'}')) ) {
    return parserState;
  } 
  return 0;
}
byte pfodParser_codeGenerated::parse() {
  byte rtn = 0;
  if (!io) { return rtn; }
  while (io->available()) {
    int in = io->read(); rtn = parse((byte)in);
    if (rtn != 0) {
      if (rtn == '!') { closeConnection(); }
      return rtn;
    }
  }
  return rtn;
}
byte pfodParser_codeGenerated::parse(byte in) {
  if (in == ((byte)0xff)) {
    // note 0xFF is not a valid utf-8 char
    // but is returned by underlying stream if start or end of connection
    // NOTE: Stream.read() is wrapped in while(Serial.available()) so should not get this
    // unless explicitlly added to stream buffer
    init(); // clean out last partial msg if any
    return 0;
  }
  if ((parserState == ((byte)0xff)) || (parserState == ((byte)'}'))) {
    parserState = ((byte)0xff);
    if (in == ((byte)'{')) { init(); parserState = ((byte)'{'); }
    return 0;
  }
  if ((argsIdx >= (255 - 2)) && (in != ((byte)'}'))) { init(); return 0; }
  if (parserState == ((byte)'{')) {
    parserState = 0;
    if (in == ((byte)':')) {
      refresh = true; return 0; 
    }
  }
  if ((in == ((byte)':')) && (versionStart != args)) {
    args[argsIdx++] = 0;
    versionStart = args; cmdStart = args+argsIdx; refresh = (strcmp((const char*)versionStart,version) == 0);
    return 0; 
  }
  if ((in == ((byte)'}')) || (in == ((byte)'|')) || (in == ((byte)'~')) || (in == ((byte)'`'))) {
    args[argsIdx++] = 0;
    if (parserState == ((byte)0xfe)) { argsCount++; }
    if (in == ((byte)'}')) {
      parserState = ((byte)'}'); args[argsIdx++] = 0; return cmdStart[0];
    } else {
      parserState = ((byte)0xfe);
    }
    return 0;
  }
  args[argsIdx++] = in; return 0;
}
byte* pfodParser_codeGenerated::parseLong(byte* idxPtr, long *result) {
  long rtn = 0; boolean neg = false;
  while ( *idxPtr != 0) {
    if (*idxPtr == ((byte)'-')) {
      neg = true;
    } else {
      rtn = (rtn << 3) + (rtn << 1); rtn = rtn +  (*idxPtr - '0');
    }
    ++idxPtr;
  }
  if (neg) { rtn = -rtn; }
  *result = rtn;
  return ++idxPtr;
}
void pfodParser_codeGenerated::setDebugStream(Print* debugOut) { }


int swap01(int in) {
  return (in==0)?1:0;
}
// ============= end generated code =========
 
