#include "pfodWaitForUtils.h"
/*
   For Mega2560 or other board which has Serial connected to USB and has another hardware Serial1
   on Mega2560 Serial1 is D18(TX1) / D19(RX1)
   This sketch starts with Serial1 set to fromBaud and attempts to fix the baud rate of the SIM5320 to the toBaud value
   If connection at the fromBaud fails, it just tries to connect at the toBaud rate to check that has been previously set.

   Status and error msgs are output to Serial (USB) connection at 19200 baud
*/
/*
 * (c)2014-2018 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This generated code may be freely used for both private and commercial use
 * provide this copyright is maintained.
 */

#define fromBaud 115200
// the current baud rate

#define toBaud 19200
// the desired baud rate


void setup() {
  Serial.begin(19200);  // << this is Arduino IDE Serial Monitor baud rate!!
  // wait 10sec for you to open the serial monitor at 19200
  for (int i = 10; i > 0; i--) {
    Serial.print(i); Serial.print(' ');
    delay(1000);
  }
  Serial.println();

  // start Serial1 at fromBaud
  Serial1.begin(fromBaud);

  pfodWaitForUtils::dumpReply(&Serial);
  Serial.print(F("Attempting to change SIM5320 baud rate from ")); Serial.print(fromBaud); Serial.print(F(" to ")); Serial.println(toBaud);
  Serial.println(F("Power cycle the SIM5320 module and then enter any character to continue..."));
  while (Serial.available() == 0) {
    pfodWaitForUtils::dumpReply(&Serial1, &Serial); // dump any SIM5320 startup msgs
  }
  pfodWaitForUtils::dumpReply(&Serial);
  Serial.print(F("Try connecting to SIM5320 at ")); Serial.println(fromBaud);

  pfodWaitForUtils::dumpReply(&Serial1, &Serial, 2000); // dump any SIM5320 startup msgs

  // try and connect at fromBaud
  Serial1.print("AT\r");
  if (pfodWaitForUtils::waitFor(F("OK"), 3000, &Serial1, &Serial)) {
    pfodWaitForUtils::dumpReply(&Serial1, &Serial);
    Serial.print(F("Connected to SIM5320 at (fromBaud) :")); Serial.println(fromBaud);
    delay(2000);
    // set baud to toBaud
    Serial1.print(F("AT+IPREX=")); Serial1.print(toBaud); Serial1.print('\r');
    if (pfodWaitForUtils::waitFor(F("OK"), 3000, &Serial1, &Serial)) {
      pfodWaitForUtils::dumpReply(&Serial1, &Serial);    
      Serial.print(F("SIM5320 baud set to :")); Serial.println(toBaud);
    } else {
      Serial.print(F("ERROR: Could not set SIM5320 baud set to :")); Serial.println(toBaud);
      Serial.print(F("Program Finished !!"));
      while (1) {
        delay(10);
      }
    }
  } else {
    pfodWaitForUtils::dumpReply(&Serial1, &Serial);
    Serial.print(F("Could not connected to SIM5320 at (fromBaud) :")); Serial.println(fromBaud);
  }

  delay(1000);
  Serial1.begin(toBaud);
  Serial.print(F("Try to connect at (toBaud) :")); Serial.println(toBaud);
  Serial1.print("AT\r");
  if (pfodWaitForUtils::waitFor(F("OK"), 3000, &Serial1, &Serial)) {
    pfodWaitForUtils::dumpReply(&Serial1, &Serial);
    Serial.print(F("Connected to SIM5320 at (toBaud) :")); Serial.println(toBaud);

  } else {
    pfodWaitForUtils::dumpReply(&Serial1, &Serial);
    Serial.print(F("ERROR!! Could not connected to SIM5320 at (toBaud) :")); Serial.println(toBaud);
    Serial.print(F("SIM5320 may be set to some other baud other than (fromBaud) :")); Serial.println(fromBaud);
  }

  Serial.print(F("Program Finished !!"));

}



void loop() {
  // nothing here
}
