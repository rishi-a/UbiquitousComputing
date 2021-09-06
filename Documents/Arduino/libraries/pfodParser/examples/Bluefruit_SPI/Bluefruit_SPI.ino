/* ===== pfod Command for Adafruit LE SPI ====
pfodApp msg {.} --> {,<bg n><b><+4>~Adafruit Bluefruit LE SPI`1000~V7|A<+5>`0~LED is ~~Off\On~|C`0~D3 PWM ~%`255`0~100~0~|B<+2>~A0 Plot}
 */
// Using Adafruit Bluefruit LE SPI Board 
//   i.e.  Bluefruit LE Shield, Bluefruit LE Micro, Feather 32u4 Bluefruit LE, Feather M0 Bluefruit LE or Bluefruit LE SPI Friend 
// Use Adafruit_BluefruitLE_nRF51-master V1.9.2 
// Use Arduino V1.6.8 IDE
/*
 * (c)2014-2017 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provide this copyright is maintained.
 */

#include <Arduino.h>
#include <SPI.h>
// download Adafruit_BluefruitLE_nRF51-master from https://github.com/adafruit/Adafruit_BluefruitLE_nRF51
#include "Adafruit_BluefruitLE_SPI.h"
// download the pfodParser library V2.37+ from http://www.forward.com.au/pfod/pfodParserLibraries/index.html
#include <pfodEEPROM.h>
#include <pfodParser.h>
int swap01(int); // method prototype for slider end swaps
float getPlotVarScaling(long varMax, long varMin, float displayMax, float displayMin);

pfodParser parser("V8"); // create a parser to handle the pfod messages
Adafruit_BluefruitLE_SPI ble(8, 7, 4); // create a BLE SPI connection

boolean alreadyConnected = false; // whether or not the client was connected previously

// give the board pins names, if you change the pin number here you will change the pin controlled
int cmd_A_var; // name the variable for 'LED is'  0=Off 1=On 
const int cmd_A_pin = 13; // name the output pin for 'LED is'
int cmd_C_var; // name the variable for 'D3 PWM'
const int cmd_C_pin = 3; // name the output pin for 'D3 PWM'

// plotting data variables
int plot_1_varMin = 0;
int plot_1_var = plot_1_varMin;
float plot_1_scaling;
float plot_1_varDisplayMin = 0.0;
// plot 2 is hidden
// plot 3 is hidden
unsigned long plotDataTimer = 0; // plot data timer
unsigned long PLOT_DATA_INTERVAL = 1000;// mS == 1 sec, edit this to change the plot data interval

// the setup routine runs once on reset:
void setup() {
  // while (!Serial);  // required for Flora & Micro
  delay(500);
  // Serial.begin(115200);
  cmd_A_var = 0;
  //pinMode(cmd_A_pin, INPUT_PULLUP); 
  pinMode(cmd_A_pin, OUTPUT); // output for 'LED is' is initially LOW,
  //uncomment INPUT_PULLUP line above and set variable to 1, if you want it initially HIGH
  digitalWrite(cmd_A_pin,cmd_A_var); // set output
   cmd_C_var = 0;
  pinMode(cmd_C_pin, OUTPUT); // output for 'D3 PWM' is initially LOW,
  analogWrite(cmd_C_pin,cmd_C_var); // set output
  // calculate the plot vars scaling here once to reduce computation
  plot_1_scaling = getPlotVarScaling(1023,plot_1_varMin,3.3,plot_1_varDisplayMin);

  if ( !ble.begin(0) ) {
    // Serial.println(F("Could not find Bluefruit SPI, make sure it's in CoMmanD mode & check wiring!"));
    while(1);
  }
  ble.verbose(false);

  /* Perform a factory reset to make sure everything is in a known state */
  if ( ! ble.factoryReset() ) {
   // Serial.println(F("Could not preform factory reset"));
    while(1);
  }
  ble.echo(false);
  // ble.info();
  ble.setMode(BLUEFRUIT_MODE_DATA);
  ble.flush();

  // <<<<<<<<< Your extra setup code goes here
}

// the loop routine runs over and over again forever:
void loop() {
  if (!ble.isConnected()) {
    if (alreadyConnected) {
      // client closed so clean up
      closeConnection(parser.getPfodAppStream());
    }
  } else {
    // connected
    if (!alreadyConnected) {
      parser.connect(&ble); // sets new io stream to read from and write to
      alreadyConnected = true;
    }

    uint8_t cmd = parser.parse(); // parse incoming data from connection
    // parser returns non-zero when a pfod command is fully parsed
    if (cmd != 0) { // have parsed a complete msg { to }
      uint8_t* pfodFirstArg = parser.getFirstArg(); // may point to \0 if no arguments in this msg.
      long pfodLongRtn; // used for parsing long return arguments, if any
      if ('.' == cmd) {
        // pfodApp has connected and sent {.} , it is asking for the main menu
        if (!parser.isRefresh()) {
          sendMainMenu(); // send back the menu designed
        } else {
          sendMainMenuUpdate(); // menu is cached just send update
        }

      // now handle commands returned from button/sliders
      } else if('A'==cmd) { // user moved slider -- 'LED is'
        // in the main Menu of Adafruit LE SPI 
        // set output based on slider 0=Off 1=On 
        parser.parseLong(pfodFirstArg,&pfodLongRtn); // parse first arg as a long
        cmd_A_var = (int)pfodLongRtn; // set variable
        digitalWrite(cmd_A_pin,cmd_A_var); // set output
        sendMainMenuUpdate(); // always send back a pfod msg otherwise pfodApp will disconnect.

      } else if('C'==cmd) { // user moved PWM slider -- 'D3 PWM'
        // in the main Menu of Adafruit LE SPI 
        parser.parseLong(pfodFirstArg,&pfodLongRtn); // parse first arg as a long
        cmd_C_var = (int)pfodLongRtn; // set variable
        analogWrite(cmd_C_pin,cmd_C_var); // set PWM output
        sendMainMenuUpdate(); // always send back a pfod msg otherwise pfodApp will disconnect.

      } else if('B'==cmd) { // user pressed -- 'A0 Plot'
        // in the main Menu of Adafruit LE SPI 
        // return plotting msg.
        parser.print(F("{=Plot of A0|time (secs)|A0~~~Volts||}"));

      } else if ('!' == cmd) {
        // CloseConnection command
        closeConnection(parser.getPfodAppStream());
      } else {
        // unknown command
        parser.print(F("{}")); // always send back a pfod msg otherwise pfodApp will disconnect.
      }
    }
  sendData();
  }
  
  //  <<<<<<<<<<<  Your other loop() code goes here 
  
}

void closeConnection(Stream *io) {
  // add any special code here to force connection to be dropped
  ble.disconnect();
  alreadyConnected = false;
}

void sendData() {
  if ((millis() - plotDataTimer) > PLOT_DATA_INTERVAL) {
    plotDataTimer = millis(); // restart plot data timer
    // assign values to plot variables from your loop variables or read ADC inputs
    plot_1_var = analogRead(A0); // read input to plot 
    // plot_2_var plot Hidden so no data assigned here
    // plot_3_var plot Hidden so no data assigned here
    // send plot data in CSV format
    parser.print(((float)plotDataTimer)/10000.0); // time in secs
    parser.print(','); parser.print(((float)(plot_1_var-plot_1_varMin)) * plot_1_scaling + plot_1_varDisplayMin);
    parser.print(','); // Plot 2 is hidden. No data sent.
    parser.print(','); // Plot 3 is hidden. No data sent.
    parser.println(); // end of CSV data record
  }
}


float getPlotVarScaling(long varMax, long varMin, float displayMax, float displayMin) {
    long varRange = varMax-varMin;
    if (varRange == 0) { varRange = 1; } // prevent divide by zero
    return (displayMax-displayMin)/((float)varRange);
}


void sendMainMenu() {
  // !! Remember to change the parser version string
  //    every time you edit this method
  parser.print(F("{,"));  // start a Menu screen pfod message
  // send menu background, format, prompt, refresh and version
  parser.print(F("<bg n><b><+4>~Adafruit Bluefruit LE SPI`1000"));
  parser.sendVersion(); // send the menu version 
  // send menu items
  parser.print(F("|A<+5>"));
  parser.print('`');
  parser.print(cmd_A_var); // output the current state 0 Low or 1 High 
  parser.print(F("~LED is ~~Off\\On~"));
 // Note the \\ inside the "'s to send \ ... 
  parser.print(F("|C"));
  parser.print('`');
  parser.print(cmd_C_var); // output the current PWM setting
  parser.print(F("~D3 PWM ~%`255`0~100~0~"));
  parser.print(F("|B<+2>"));
  parser.print(F("~A0 Plot"));
  parser.print(F("}"));  // close pfod message
}

void sendMainMenuUpdate() {
  parser.print(F("{;"));  // start an Update Menu pfod message
  // send menu items
  parser.print(F("|A"));
  parser.print('`');
  parser.print(cmd_A_var); // output the current state 0 Low or 1 High 
  parser.print(F("|C"));
  parser.print('`');
  parser.print(cmd_C_var); // output the current PWM setting
  parser.print(F("}"));  // close pfod message
  // ============ end of menu ===========
}

int swap01(int in) {
  return (in==0)?1:0;
}
// ============= end generated code =========
 
