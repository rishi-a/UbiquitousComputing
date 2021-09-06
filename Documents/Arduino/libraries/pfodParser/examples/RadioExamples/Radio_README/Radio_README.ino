
  There are three pfodRadio examples
  see https://www.forward.com.au/pfod/pfodRadio/index.html for a detailed tutorial in these.

  Feather9x_Server_68 -- a pfodServer on NODE_ID 0x68

  Feather9x_Client_38 -- a pfodClient on NODE_ID 0x38 and programmed to connect to a pfodServer on NODE_ID 0x68
  This client takes commands from the Arduino IDE Serial Monitor and displays the reponses there.

  Feather9x_Client_38_bridge -- a pfodClient configured to connect commands/responses to a Hardware Serial.
  It NODE_ID is 0x38 programmed to connect to a pfodServer on NODE_ID 0x68.
  
  The three (3) ESP32 sketches are pfodServer bridges that can be connected to the Feather9x_Client_38_bridge via hardware uart
  to allow you to connect to your Radio/LoRa pfodDevice from pfodApp running on your Android mobile


