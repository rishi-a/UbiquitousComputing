
/* ===== pfod Command for Menu_1 ====
pfodApp msg {.} --> {,<bg n><b><+3><w>~Uno Starter`0~V19|A<bg bl>`0~D4 output is ~~Low\High~|B<bg bl>`0~D5 PWM Setting ~%`255`0~100~0~|!F<-6>~|!C<bg bl>`1~D6 Input is ~~Low\High~t|!E<-6>~|!D<bg bl>`775~A0 ~V`1023`0~5~0~}
 */
// Using Serial and 19200 baud for send and receive, via Serial
// GPRS shield V2 from SeeedStudio
// This code uses Serial so remove shield when programming the mega
// NOTE: Because of the RAM requirements, this code needs to be loaded onto a Arduino Mega 2560 or similar
// It is NOT suitable for use on an Arduino UNO

/*
 * (c)2014-2017 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provide this copyright is maintained.
 */
#include <EEPROM.h>
// Download library from http://www.forward.com.au/pfod/pfodParserLibraries/index.html
#include <pfodSMS_SIM900.h>
#include <pfodSecurity.h>
int swap01(int); // method prototype for slider end swaps

pfodSMS_SIM900 pfodSMS; // create object to handle SMS messages
pfodSecurity parser("SMS20"); // create a parser to handle the pfod messages

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
  Serial.begin(19200);
  for (int i=3; i>0; i--) {
    // wait a few secs to see if we are being programmed
    delay(1000);
  }
  cmd_A_var = 0;
  //pinMode(cmd_A_pin, INPUT_PULLUP); 
  pinMode(cmd_A_pin, OUTPUT); // output for 'D4 output is' is initially LOW,
  //uncomment INPUT_PULLUP line above and set variable to 1, if you want it initially HIGH
  digitalWrite(cmd_A_pin,cmd_A_var); // set output
   cmd_B_var = 0;
  pinMode(cmd_B_pin, OUTPUT); // output for 'D5 PWM Setting' is initially LOW,
  digitalWrite(cmd_B_pin,cmd_B_var); // set output
   pinMode(cmd_C_pin, INPUT_PULLUP); // edit this to just pinMode(..,INPUT); if you don't want the internal pullup enabled

// initialize the SMS and connect the parser
  pfodSMS.init(&Serial,9); //  uses pin 9 for powerup/reset
  parser.connect(&pfodSMS); // connect parser to SMS stream
  // To set a password use next line instead, password uses 19 bytes of EEPROM starting from 0
  // parser.connect(&pfodSMS, F("173057F7A706AF9BBE65D51122A14CEE"));
  // The password is upto 32 hex digits, 0..9 A..F
  // passwords shorter then 32 hex digits are padded with 0's

  // <<<<<<<<< Your extra setup code goes here
}

// the loop routine runs over and over again forever:
void loop() {
  byte cmd = parser.parse(); // pass it to the parser
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
  // nothing special here for SMS
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
  parser.print(F("<bg n><b><+3><w>~Uno Starter`0"));
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

int swap01(int in) {
  return (in==0)?1:0;
}
// ============= end generated code =========
 