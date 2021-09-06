
/* ===== pfod Command for Menu_1 ====
pfodApp msg {.} --> {,<bg n><b><+3><w>~ESP8266 Starter`1000~V15|A<bg bl>`0~D4 output is ~~Low\High~|B<bg bl>`0~D5 PWM Setting ~%`255`0~100~0~|!F<-6>~|!C<bg bl>`1~D14 Input is ~~Low\High~t|!E<-6>~|!D<bg bl>`775~A0 ~V`1023`0~5~0~|G<bg bl>~Plot A0}
 */
// Using ESP8266 based board programmed via Arduino IDE
// follow the steps given on https://github.com/esp8266/arduino under Installing With Boards Manager

// You need to modify the WLAN_SSID, WLAN_PASS settings below 
// to match your network settings 

/*
 * (c)2014-2017 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provide this copyright is maintained.
 */
#include <EEPROM.h>
  
// Download pfodESP8266BufferedClient library from http://www.forward.com.au/pfod/pfodParserLibraries/index.html
#include <ESP8266WiFi.h>
#include <pfodESP8266Utils.h>
#include <pfodESP8266BufferedClient.h>
//#define DEBUG  
// Download pfodParser library from http://www.forward.com.au/pfod/pfodParserLibraries/index.html
#include <pfodSecurity.h>  // V2.31 or higher
int swap01(int); // method prototype for slider end swaps

pfodSecurity parser("ESP16"); // create a parser to handle the pfod messages
pfodESP8266BufferedClient bufferedClient;


#define WLAN_SSID       "*******"        // cannot be longer than 32 characters!
#define WLAN_PASS       "*******"

const int portNo = 4989; // What TCP port to listen on for connections.
const char staticIP[] = "";  // set this the static IP you want, e.g. "10.1.1.200" or leave it as "" for DHCP. DHCP is not recommended.

// add your pfod Password here for 128bit security
// eg "b0Ux9akSiwKkwCtcnjTnpWp" but generate your own key, "" means no pfod password
#define pfodSecurityCode ""
// see http://www.forward.com.au/pfod/ArduinoWiFi_simple_pfodDevice/index.html for more information and an example
// and QR image key generator.

WiFiServer server(portNo);
WiFiClient client;
boolean alreadyConnected = false; // whether or not the client was connected previously

// give the board pins names, if you change the pin number here you will change the pin controlled
int cmd_A_var; // name the variable for 'D4 output is'
const int cmd_A_pin = 4; // name the output pin for 'D4 output is'
int cmd_B_var; // name the variable for 'D5 PWM Setting'
const int cmd_B_pin = 5; // name the output pin for 'D5 PWM Setting'
int cmd_C_var; // name the variable for 'D14 Input is'
const int cmd_C_pin = 14; // name the input pin for 'D14 Input is'
int cmd_D_var; // name the variable for 'A0'
unsigned long cmd_D_adcStartTime=0; // ADC timer
unsigned long cmd_D_ADC_READ_INTERVAL = 1000;// 1sec, edit this to change adc read interval
const int cmd_D_pin = A0; // name the pin for 'A0'
int cmd_G_var; // name the variable for 'Plot A0'

// the setup routine runs once on reset:
void setup() {
  EEPROM.begin(512);  // only use 20bytes for pfodSecurity but reserve 512 (pfodWifiConfig uses more)
  cmd_A_var = 0;
  //pinMode(cmd_A_pin, INPUT_PULLUP); 
  pinMode(cmd_A_pin, OUTPUT); // output for 'D4 output is' is initially LOW,
  //uncomment INPUT_PULLUP line above and set variable to 1, if you want it initially HIGH
  digitalWrite(cmd_A_pin,cmd_A_var); // set output
   cmd_B_var = 0;
  pinMode(cmd_B_pin, OUTPUT); // output for 'D5 PWM Setting' is initially LOW,
  digitalWrite(cmd_B_pin,cmd_B_var); // set output
   pinMode(cmd_C_pin, INPUT_PULLUP); // edit this to just pinMode(..,INPUT); if you don't want the internal pullup enabled
  cmd_G_var = 0;
  #ifdef DEBUG
    Serial.begin(115200);
  #endif
  #ifdef DEBUG
    for (int i = 10; i > 0; i--) {
      Serial.print(i);
      Serial.print(' ');
      delay(500);
    }
    Serial.println();
  #endif
    /* Initialise wifi module */
    if (*staticIP != '\0') {
      IPAddress ip(pfodESP8266Utils::ipStrToNum(staticIP));
      IPAddress gateway(ip[0], ip[1], ip[2], 1); // set gatway to ... 1
  #ifdef DEBUG
      Serial.print(F("Setting gateway to: "));
      Serial.println(gateway);
  #endif
      IPAddress subnet(255, 255, 255, 0);
      WiFi.config(ip, gateway, subnet);
    }
    WiFi.begin(WLAN_SSID, WLAN_PASS);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
  #ifdef DEBUG
      Serial.print(".");
  #endif
    }
  #ifdef DEBUG
    Serial.println();
    Serial.println("WiFi connected");
  #endif
  
    // Start the server
    server.begin();
  #ifdef DEBUG
    Serial.println("Server started");
  #endif
  
    // Print the IP address
  #ifdef DEBUG
    Serial.println(WiFi.localIP());
  #endif
  
    // initialize client
    client = server.available(); // evaluates to false if no connection
    

  // <<<<<<<<< Your extra setup code goes here
}

// the loop routine runs over and over again forever:
void loop() {
  if (!client) { // see if a client is available
    client = server.available(); // evaluates to false if no connection
  } else {
    // have client
    if (!client.connected()) {
      if (alreadyConnected) {
      // client closed so clean up
      closeConnection(parser.getPfodAppStream());
      }
    } else {
      // have connected client
      if (!alreadyConnected) {
        parser.connect(bufferedClient.connect(&client),F(pfodSecurityCode)); // sets new io stream to read from and write to
        EEPROM.commit(); // does nothing if nothing to do
        alreadyConnected = true;
      }

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

    //    } else if('C'==cmd) { // this is a label. pfodApp NEVER sends this cmd -- 'D14 Input is'
    //      // in the main Menu of Menu_1 

    //    } else if('E'==cmd) { // this is a label. pfodApp NEVER sends this cmd -- ''
    //      // in the main Menu of Menu_1 

    //    } else if('D'==cmd) { // this is a label. pfodApp NEVER sends this cmd -- 'A0'
    //      // in the main Menu of Menu_1 

        } else if('G'==cmd) { // user pressed -- 'Plot A0'
          // in the main Menu of Menu_1 
          // << add your action code here for this button
      // open a plot screen set scale to 0 t 1023 for ADC raw counts
      //              screen title | xAxis  | Yaxis ~max ~ min
      parser.print(F("{=A0 ADC Plot|time (s)|Raw Counts~1023~0}"));

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
    }
  }
  
  //  <<<<<<<<<<<  Your other loop() code goes here 
  
}

void closeConnection(Stream *io) {
  // add any special code here to force connection to be dropped
  parser.closeConnection(); // nulls io stream
  alreadyConnected = false;
  bufferedClient.stop(); // clears client reference
  WiFiClient::stopAll(); // cleans up memory
  client = server.available(); // evaluates to false if no connection
}
void cmd_D_readADC() {
  if ((millis() - cmd_D_adcStartTime) > cmd_D_ADC_READ_INTERVAL) {
    cmd_D_adcStartTime = millis(); // restart timer
    cmd_D_var = analogRead(cmd_D_pin);  // read ADC input
    // send a line the CSV data for plotting
    parser.print(millis()/1000.0); // time in sec
    parser.print(',');
    parser.print(cmd_D_var); // raw adc reading
    parser.println(); // terminate data with newline
  }
}


void sendMainMenu() {
  // !! Remember to change the parser version string
  //    every time you edit this method
  parser.print(F("{,"));  // start a Menu screen pfod message
  // send menu background, format, prompt, refresh and version
  parser.print(F("<bg n><b><+3><w>~ESP8266 Starter`1000"));
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
  parser.print(F("~D5 PWM Setting ~%`1023`0~100~0~"));
  parser.print(F("|!F<-6>"));
  parser.print(F("~"));
  parser.print(F("|!C<bg bl>"));
  parser.print('`');
  parser.print(cmd_C_var); // output the current state of the input
  parser.print(F("~D14 Input is ~~Low\\High~t"));
 // Note the \\ inside the "'s to send \ ... 
  parser.print(F("|!E<-6>"));
  parser.print(F("~"));
  parser.print(F("|!D<bg bl>"));
  parser.print('`');
  parser.print(cmd_D_var); // output the current ADC reading
  parser.print(F("~A0 ~V`1023`0~1~0~"));
  parser.print(F("|G<bg bl>"));
  parser.print(F("~Plot A0"));
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
 
