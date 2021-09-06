// NonBlockingInputTest.ino
// these next two includes come from the pfodParser library V3.36+.  https://www.forward.com.au/pfod/pfodParserLibraries/index.html
#include <pfodNonBlockingInput.h>
#include <pfodBufferedStream.h>
/*
   (c)2019 Forward Computing and Control Pty. Ltd.
   NSW Australia, www.forward.com.au
   This code is not warranted to be fit for any purpose. You may only use it at your own risk.
   This generated code may be freely used for both private and commercial use
   provided this copyright is maintained.
*/

// ========== SERIAL setting ================
// change this to match your board,  UNO/Mega use Serial, Redboard Turbo USB (slow) use SerialUSB, Redboard Turbo D0/D1 (fast) use Serial1
#define SERIAL Serial
const uint32_t SERIAL_BAUD_RATE = 115200;

Stream* serialIO;
pfodNonBlockingInput nonBlocking;

const size_t OUTPUT_BUFFER_SIZE = 256;
uint8_t outputBuffer[OUTPUT_BUFFER_SIZE];
pfodBufferedStream bufferedStream(SERIAL_BAUD_RATE, outputBuffer, OUTPUT_BUFFER_SIZE, false); // do not block, just drop chars if buffer full

// for readInputLine()
const size_t lineBufferSize = 81; // 80 + terminating null for line upto 80 char long
char lineBuffer[lineBufferSize] = ""; // start empty

void setup() {

  // change the #define SERIAL at the top of this sketch to use other Serial connections
  SERIAL.begin(SERIAL_BAUD_RATE);
  // ------------------------------
  for (int i = 10; i > 0; i--) {
    SERIAL.println(i);
    delay(500);
  }
  serialIO = bufferedStream.connect(&SERIAL); // buffer the output so it does not block the loop if printing loooong messages
  nonBlocking.connect(serialIO);

  serialIO->println(F("NonBlockingInputTest"));
  serialIO->println(F("Reads one line at a time and either prints the first char, if any, or the whole line."));
  serialIO->println(F("After each input line the processing switches between returning the first char or whole line."));
  serialIO->println();
  serialIO->println(F(" -- Return whole line --"));
}

bool clearAfterOneChar = false;
size_t lineSize = lineBufferSize;
void toggleInputProcessing() {
  clearAfterOneChar = !clearAfterOneChar;
  if (clearAfterOneChar) {
    serialIO->println(F(" -- Return just first char --"));
    lineSize = 2; // one char + terminating null
  } else {
    serialIO->println(F(" -- Return whole line --"));
    lineSize = lineBufferSize;
  }
}

void loop() {
  int inputLen = nonBlocking.readInputLine(lineBuffer, lineSize, true); // echo input
  if (inputLen >= 0) {
    // lineBuffer has input
    serialIO->println(); // terminate output line using our idea of end of line
    serialIO->print("lineBuffer contains "); serialIO->print(strlen(lineBuffer)); serialIO->println(" chars");
    nonBlocking.clearInput(); // skip any following chars
    toggleInputProcessing(); // toggle input processing to return just one char
    // process linebuffer here
    lineBuffer[0] = '\0'; // then clear it to read next line
  }
}
