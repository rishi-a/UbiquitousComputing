#include <PeakDetection.h>
bool DEBUG=1;
int sensorPin = 0;
PeakDetection peakDetection;
unsigned long programStartTime, timeNow, firstPeakTime, secondPeakTime;
float BPM, data; //variable to store the analog input
double hb,hbo;
int countSamples=0;
bool firstPeak=0;


/*Define the Pins*/
int red=3, ir=10, select=8;


void setup() {
   Serial.begin(9600);
   peakDetection.begin(50, 3, 0.2);
   pinMode(select, OUTPUT);
   
}
void loop (){

  if(DEBUG){
    debughr();
    //hr();
    //delay(2000);
    //Serial.println("Calculating Hb:");
    //hb2rx();  
  }
  
  else{
  delay(2000);
  Serial.println("Put your finger");
  delay(3000);  //wait for some seconds for the user to stabilize
  programStartTime = millis();
  //hr();      //this function calculates the BPM and prints it in the Serial Monitor.  
  Serial.println("Calculating Hb. . .");
  delay(3000);
  hb2rx();
  Serial.println("\n***R Ratio***");
  rawhb();
  Serial.print("Done. Redoing now. . .");
  
  }
   
} 
