
/* ===== pfod Command for Navigation Example ====
 */
// Using Serial and 9600 for send and receive
// Serial D0 (RX) and D1 (TX) on Arduino Uno, Micro, ProMicro, Due, Mega, Mini, Nano, Pro
// This code uses Serial so remove Bluetooth shield when programming the board
/*
 * (c)2014-2017 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provide this copyright is maintained.
 */
// ======================

#include <EEPROM.h>
#include <pfodParser.h>
pfodParser parser("Nav1");  // need latest pfodParser library V2.31+
// set menu and image versions to "Nav1"
// if version is not set then will never ask for a refresh of image or menu as neither will be cached.

const int LEFT = 0;
const int RIGHT = 1;
const int UP = 2;
const int DOWN = 3;
const int HOME = 4;

// this var updates the label at the top of the screen
int buttonPressed = HOME; // 0 = left, 1 = right, 2= up, 3= down, 4 = home

// the setup routine runs once on reset:
void setup() {
  Serial.begin(9600);
  for (int i = 3; i > 0; i--) {
    // wait a few secs to see if we are being programmed
    delay(1000);
  }
  parser.connect(&Serial); // connect the parser to the i/o stream
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
      // send back the menu designed
      if (!parser.isRefresh()) {
        sendMainMenu();
      } else {
        sendMainMenuUpdate();
      }

      // now handle commands returned from buttons
      // 'A' is the Label cmd and is never sent
      //   } else if ('A' == cmd) { // label, no input
      //    never get A cmd from the label but if we do just return {}from else below

    } else if (('B' == cmd) || ('G' == cmd)) { // left
      buttonPressed = LEFT;
      sendMainMenuUpdate(); // sent update

    } else if (('C' == cmd)  || ('H' == cmd)) { // right
      buttonPressed = RIGHT;
      sendMainMenuUpdate(); // sent update

    } else if (('D' == cmd) || ('I' == cmd)) { // up
      buttonPressed = UP;
      sendMainMenuUpdate(); // sent update

    } else if (('E' == cmd) || ('J' == cmd)) { // down
      buttonPressed = DOWN;
      sendMainMenuUpdate(); // sent update

    } else if (('F' == cmd) || ('K' == cmd))  { // home
      buttonPressed = HOME;
      sendMainMenuUpdate(); // sent update

    } else if ('!' == cmd) {
      // CloseConnection command
      closeConnection(parser.getPfodAppStream());
    } else {
      // unknown command
      parser.print(F("{}")); // always send back a pfod msg otherwise pfodApp will disconnect.
    }
  }
}

void closeConnection(Stream *io) {
  // add any special code here to force connection to be dropped
}

void sendMainMenu() {
  parser.print(F("{,"));  // start a V2 Menu screen pfod message
  parser.print(F("<bg 000040>~")); // set background dark blue and empty screen prompt
  parser.print(F("~")); // send ~ before the menu version string
  parser.print(parser.getVersion());  // send version for this menu
  parser.print(F("|!A")); // set menu item cmd to A and background to blue
  parser.print(F("`")); // send ` before buttonPressed value
  parser.print(buttonPressed);  // send the current setting
  parser.print(F("~Pressed ~ Button~Left\\Right\\Up\\Down\\Home~t"));
  addSpacer(0);
  parser.print(F("|^G<bg bl>~Text for Left Button|^H<bg y>~Text for Right Button"));
  addSpacer(1);
  parser.print(F("|^d1-|^d2-|^I<bg g>~Text for Up Button|^J<bg r>~Text for Down Button"));  // hide Left and Right
  // use dummy cmds for them d1 for the hidden Left, d2 for the hidden Right
  addSpacer(2);
  parser.print(F("|^B<bg bl>~Left|^C<bg y>~Right|^D<bg g>~Up|^E<bg r>~Down|^F<bg w>~Home"));
  addSpacer(3);
  parser.print(F("|^d3-|^d4-|^d5-|^d6-|^K<bg w>~Text for\nHome Button"));  // hide Left and Right. Up and Down
  // use dummy cmds for them d3, d4, d5, d6 for the hidden buttons
  addSpacer(4);
  parser.print(F("}"));  // close pfod message
}

void sendMainMenuUpdate() {
  parser.print(F("{;"));  // start an V2 Update Menu pfod message
  parser.print(F("|A")); // update the menu item with cmd A
  parser.print(F("`")); // send ` before the current updated sliderSetting value
  parser.print(buttonPressed);  // send the current setting
  parser.print(F("}"));  // close pfod message
}

// adds a white spacer  <bg w>
// the spacer's cmd is s<number> but is never sent because this is a label
// eg addSpacer(5) sends
// |!s5<bg w><-18>
// change the -18 to change the size of the spacer
// more negative gives smaller spacer
// more positive gives larger spacer
// remove the <bg w> to get an invisible spacer (same colour as the menu background)
void addSpacer(int number) {
  parser.print(F("|!s"));
  parser.print(number); // to make cmd unique in this menu
  parser.print(F("<bg w><-18>"));
}
// ============ end of menu item ===========


