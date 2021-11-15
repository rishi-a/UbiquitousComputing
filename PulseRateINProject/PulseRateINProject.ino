#include <PeakDetection.h>
#include <SoftwareSerial.h>

//for bluetooth module
SoftwareSerial EEBlue(6, 5); // RX | TX

bool DEBUG=0;
int sensorPin = 0;
PeakDetection peakDetection;


/*Define the Pins*/
int red=3, ir=10, select=8;
int i;
int window_size = 120;
double window[120];

/*This variable will store the string datatype of pulse rate, which is originally float*/
char sBPM[8]; // Buffer big enough for 7-character float
char saverageHb[8];
char sSpO2[8]; 
char spindex[8];

void setup() {
   Serial.begin(9600);
   EEBlue.begin(38400);  
   peakDetection.begin(30, 3, 0.2);
   pinMode(select, OUTPUT);
   
}
void loop (){

  if(DEBUG){
    //measure_prsr();
    debughr();
    //hr();
    //Serial.println("Calculating Hb in 4 seconds:");
    //delay(5000);
    //Serial.println("Calculating Hb:");
    //hb2rx();  
  }
  
  else{
    if (EEBlue.available()){
      int ble_input = EEBlue.read();
      Serial.println(":Welcome To Two Touch Doctor:\n");
      EEBlue.write(":Welcome To Two Touch Doctor:\n");
      delay(5000);  //wait for some seconds for the user to stabilize
      hr();      //this function calculates the BPM and prints it in the Serial Monitor.
      Serial.println("Calculating Hb in 4 seconds:");
      EEBlue.write(":Calculating Hb, SpO2 and PI in 4 seconds:\n");  
      delay(4000);
      hb2rx();
      spo2();
      pi();
      //bp();
      Serial.print("*****\n");
      EEBlue.write(":Thank you. Bye Bye:\n");
      
    }
  
  }
   
} 
