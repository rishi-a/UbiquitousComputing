
/* ===== Example of using pfodBLEBufferedSerial ====
*/
// Using Serial and 9600 baud for send and receive
// Bluefruit LE UART Friend  and Arduino Mega2560
/*
   (c)2014-2017 Forward Computing and Control Pty. Ltd.
   NSW Australia, www.forward.com.au
   This code is not warranted to be fit for any purpose. You may only use it at your own risk.
   This generated code may be freely used for both private and commercial use
   provide this copyright is maintained.
*/

// download the libraries from http://www.forward.com.au/pfod/pfodParserLibraries/index.html
// pfodParser V3.13+, pfodDwgControls V1.2+
#include <pfodParser.h>
#include <pfodDwgControls.h>
// pfodDwgControls.h includes DesignerSwitch.h
#include <pfodBLEBufferedSerial.h>

int swap01(int); // method prototype for slider end swaps
int reading = 100; // value for gauge in Tonne

pfodParser parser("B1"); // create a parser to handle the pfod messages
pfodBLEBufferedSerial bufferedSerial;   // 1024 buffer size
pfodDwgs dwgs(&parser);  // drawing support

DesignerSwitch switch_z('a', &dwgs); // an example switch for drawing z
// Commands for drawing controls only need to unique per drawing, you can re-use cmd 'a' in another drawing.

unsigned long gaugeTimer;
const unsigned long GUAGE_TIMER_INTERVAL = 1000; // 1000mS
unsigned int gaugeIncrement = 5;

// the setup routine runs once on reset:
void setup() {
  Serial.begin(9600); // <<<<<<<<<<<<<< connect Adafruit BLE UART to Serial of Mega
  for (int i = 3; i > 0; i--) {
    // wait a few secs to see if we are being programmed
    delay(1000);
  }
  //parser.connect((&Serial1)); // connect the parser to the i/o stream NO buffer
  // replace this with connection via bufferedSerial
  parser.connect(bufferedSerial.connect(&Serial)); // connect the parser to the i/o stream via buffer
  // <<<<<<<<< Your extra setup code goes here

  switch_z.setLabel(F("Light"));
  gaugeSetup(); // set up gauge
  gaugeSetValue(reading);  // set intial reading
  gaugeTimer = millis(); // initialze timer
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
    } else if ('C' == cmd) { // user pressed menu item that loaded drawing with load cmd 'z'
      // in the main Menu of Second Drawing Attempt
      byte dwgCmd = parser.parseDwgCmd();  // parse rest of dwgCmd, return first char of active cmd
      // can be \0 if no dwg or no active areas defined
      // << Modify this code for your own controls
      if (switch_z.getCmd() == dwgCmd) { // check drawing control cmd to see what the user pressed in this drawing
        switch_z.setValue(!switch_z.getValue()); // modify this to take action based on this control
        sendDrawingUpdates_z(); // update the drawing
      } else { // no change
        parser.print(F("{}")); // always send back a pfod msg otherwise pfodApp will disconnect.
      }

    } else if ('z' == cmd) { // pfodApp is asking to load dwg 'z'
      if (!parser.isRefresh()) { // not refresh, send whole dwg
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
  unsigned long nowMs = millis();
  if (nowMs - gaugeTimer >= GUAGE_TIMER_INTERVAL) {
    gaugeTimer = nowMs;
    reading = reading + gaugeIncrement;
    if (reading >= 1000) {
      reading == 1000;
      gaugeIncrement = -gaugeIncrement;
    }
    if (reading <= 0) {
      reading = 0;
      gaugeIncrement = -gaugeIncrement;
    }
    gaugeSetValue(reading);
  }
}

void closeConnection(Stream *io) {
  // add any special code here to force connection to be dropped
}

void sendMainMenu() {
  // !! Remember to change the parser version string
  //    every time you edit this method
  parser.print(F("{,"));  // start a Menu screen pfod message
  // send menu background, format, prompt, refresh and version
  parser.print(F("~`2000"));
  parser.sendVersion(); // send the menu version
  // send menu items
  parser.print(F("|+C~z"));
  parser.print(F("}"));  // close pfod message
}

void sendMainMenuUpdate() {
  parser.print(F("{;"));  // start an Update Menu pfod message
  // send menu items
  parser.print(F("|C"));
  parser.print(F("}"));  // close pfod message
  // ============ end of menu ===========
}

void sendDrawing_z() {
  //dwgs.start(25, 15, dwgs.WHITE); // background defaults to WHITE if omitted i.e. dwgs.start(25, 15);  //covers all wide, 1/2 down
  dwgs.start(50, 30, dwgs.WHITE); // background defaults to WHITE if omitted i.e. dwgs.start(50,30);  //covers all wide, 1/2 down
  parser.sendVersion(); // send the parser version to cache this image
  //dwgs.pushZero(5, 5, 0.5f); // scale by 1.0f,  scale defaults to 1.0f if omitted, i.e. pushZero(10,15);  //switch smaller 0.5f
  dwgs.pushZero(10, 15, 1.0f); // scale by 1.0f,  scale defaults to 1.0f if omitted, i.e. pushZero(10,15);
  switch_z.draw();  // button drawn
  dwgs.popZero();
  dwgs.pushZero(35, 20, 1.5); // move zero to centre of dwg to 25,12 and scale by 1.5 times
  gaugeDraw();
  dwgs.end();
}

void sendDrawingUpdates_z() {
  dwgs.startUpdate();
  switch_z.update();  // update existing button with current state
  gaugeUpdate(); // update with current state
  dwgs.end();
}


int swap01(int in) {
  return (in == 0) ? 1 : 0;
}
// ============= end generated code =========

// ====================== this is the gauge dwg code =================
// modified from the gauge example in pfodDwgControls

int  gauge_z_idx = pfodDwgs::reserveIdx(3);
const __FlashStringHelper* gaugeLabel;
int gaugeValue = 0; // 0 to 1000

void gaugeSetup() {
  gaugeLabel = F("Weight\n");
}

// input value is 0 to 1000
void gaugeSetValue(int value) {
  gaugeValue = value;
}

void gaugeDraw() {
  dwgs.arc().filled().color(dwgs.GREY).radius(5).start(217.5).angle(-255).send();
  dwgs.arc().filled().color(dwgs.WHITE).radius(4).idx(gauge_z_idx + 1).start(217.5).angle(-255).send();

  dwgs.arc().filled().color(pfodDwgs::BLACK).radius(5.5).start(217.5).angle(0).send();
  dwgs.arc().filled().color(pfodDwgs::BLACK).radius(5.5).start(90).angle(0).send();
  dwgs.arc().filled().color(pfodDwgs::BLACK).radius(5.5).start(-37.5).angle(0).send();

  dwgs.label().offset(-5.5, 3).fontSize(-6).text(F("0")).send();
  dwgs.label().offset(0, -6.5).fontSize(-6).text(F("500")).send();
  dwgs.label().offset(6.5, 3).fontSize(-6).text(F("1000")).send();
  gaugeUpdate(); // update with current state
}

void gaugeUpdate() {
  float gaugeAngle = gaugeValue / 1000.0 * 255.0; // scaled to 0 to 255 for display angle
  dwgs.arc().filled().color(dwgs.RED).radius(5).idx(gauge_z_idx).start(217.5).angle(-gaugeAngle).send();
  dwgs.label().color(dwgs.RED).idx(gauge_z_idx + 2).fontSize(-3).bold().text(gaugeLabel).intValue(gaugeValue).maxValue(1000).displayMax(1000).decimals(0).units(F("t")).send();
}

