/* ===== pfod Command for Menu_1 ====
  pfodApp msg {.} --> {,~`0~V2|+A~z}
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

// download the libraries from http://www.forward.com.au/pfod/pfodParserLibraries/index.html
// pfodParser V3.1+, pfodDwgControls V1.0+
#include <pfodParser.h>
#include <pfodDwgControls.h>

int swap01(int); // method prototype for slider end swaps

pfodParser parser(""); // create a parser to handle the pfod messages
pfodDwgs dwgs(&parser);  // drawing support
// Commands for drawing controls only need to unique per drawing, you can re-use cmds 'a' in another drawing.



// the setup routine runs once on reset:
void setup() {
  Serial.begin(9600);
  for (int i = 3; i > 0; i--) {
    // wait a few secs to see if we are being programmed
    delay(1000);
  }
  parser.connect(&Serial); // connect the parser to the i/o stream

  // <<<<<<<<< Your extra setup code goes here
}

// the loop routine runs over and over again forever:
void loop() {
  uint8_t cmd = parser.parse(); // parse incoming data from connection
  // parser returns non-zero when a pfod command is fully parsed
  if (cmd != 0) { // have parsed a complete msg { to }
    uint8_t* pfodFirstArg = parser.getFirstArg(); // may point to \0 if no arguments in this msg.
    pfod_MAYBE_UNUSED(pfodFirstArg); // may not be used, just suppress warning
    long pfodLongRtn; // used for parsing long return arguments, if any
    pfod_MAYBE_UNUSED(pfodLongRtn); // may not be used, just suppress warning
    if ('.' == cmd) {
      // pfodApp has connected and sent {.} , it is asking for the main menu
      if (!parser.isRefresh()) {
        sendMainMenu(); // send back the menu designed
      } else {
        sendMainMenuUpdate(); // menu is cached just send update
      }

      // now handle commands returned from button/sliders
    } else if ('A' == cmd) { // user pressed drawing loaded with load cmd 'z'
      // in the main Menu of Menu_1
      byte dwgCmd = parser.parseDwgCmd();  // parse rest of dwgCmd, return first char of active cmd
      pfod_MAYBE_UNUSED(dwgCmd); // may not be used, just suppress warning
      sendDrawingUpdates_z(); // update the drawing
      // always send back a response or pfodApp will timeout

    } else if ('z' == cmd) { // pfodApp is asking to load dwg 'z'
      if (!parser.isRefresh()) { // not refresh send whole dwg
        sendDrawing_z();
      } else { // refresh just update drawing state
        sendDrawingUpdates_z();
      }

    } else if ('!' == cmd) {
      // CloseConnection command
      closeConnection(parser.getPfodAppStream());
    } else {
      // unknown command
      parser.print(F("{}")); // always send back a pfod msg otherwise pfodApp will disconnect.
    }
  }
  //  <<<<<<<<<<<  Your other loop() code goes here

}

void closeConnection(Stream *io) {
  // add any special code here to force connection to be dropped
}

void sendMainMenu() {
  // !! Remember to change the parser version string
  //    every time you edit this method
  parser.print(F("{,"));  // start a Menu screen pfod message
  // send menu background, format, prompt, refresh and version
  parser.print(F("~`0"));
  parser.sendVersion(); // send the menu version
  // send menu items
  parser.print(F("|+A~z"));
  parser.print(F("}"));  // close pfod message
}

void sendMainMenuUpdate() {
  parser.print(F("{;"));  // start an Update Menu pfod message
  // send menu items
  parser.print(F("|A"));
  parser.print(F("}"));  // close pfod message
  // ============ end of menu ===========
}

void sendDrawing_z() {
  dwgs.start(50, 40, dwgs.WHITE); // background defaults to WHITE if omitted i.e. dwgs.start(50,30);
  parser.sendVersion(); // send the parser version to cache this image
  dwgs.pushZero(25, 3);
  dwgs.label().text("Drawing Primitives").fontSize(+3).bold().color(dwgs.NAVY).send();
  dwgs.popZero();
  dwgs.pushZero(0, 6, 1.5);
  dwgs.line().color(dwgs.NAVY).size(9.1, 1.8).offset(3.3, 1.05).send();
  dwgs.rectangle().color(dwgs.BLUE).size(4.2, 2.9).offset(2.02, 4.1).send();
  dwgs.rectangle().color(dwgs.BLUE).filled().size(4.2, 2.9).offset(9.03, 4.1).send();
  dwgs.rectangle().color(dwgs.BLACK).rounded().size(4.2, 2.9).offset(2.03, 8.1).send();
  dwgs.rectangle().color(dwgs.BLACK).filled().rounded().size(4.2, 2.9).offset(9.04, 8.1).send();
  dwgs.arc().color(dwgs.GREEN).start(200).angle(220).radius(2.1).offset(4.2, 14.12).send();
  dwgs.arc().color(dwgs.GREEN).filled().start(-220).angle(-220).radius(2.1).offset(11.2, 14.12).send();
  dwgs.circle().color(dwgs.RED).radius(2).offset(17.5, 5).send();
  dwgs.circle().filled().color(dwgs.RED).filled().radius(2).offset(17.5, 10).send();
  dwgs.popZero();
  dwgs.pushZero(40, 15);
  dwgs.touchZone().cmd('a').size(1, 1).send();
  dwgs.label().fontSize(-1).bold().text(F("touchZone\nSends cmd\nwhen touched.")).offset(0, 10).send();
  dwgs.label().fontSize(-1).italic().text(F("Normally invisible except\nwhen in Debug mode.")).offset(-5, 17).send();
  dwgs.popZero();
  dwgs.end();
}

void sendDrawingUpdates_z() {
  dwgs.startUpdate();
  // nothing to update here
  dwgs.end();
}

int swap01(int in) {
  return (in == 0) ? 1 : 0;
}
// ============= end generated code =========

