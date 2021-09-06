#include <PeakDetection.h>

int sensorPin = 0;
PeakDetection peakDetection;
unsigned long programStartTime, timeNow, firstPeakTime, secondPeakTime;
float BPM, HRV;
double hb,hbo;

float data;  //variable to store the analog input
int countSamples=0;
bool firstPeak=0;


/*Define the Pins*/
int red=3, ir=10;


void setup() {
   Serial.begin(9600);
   peakDetection.begin(30, 3.5, 0.2);
   
}
void loop (){

  // Use the below code to debig only.
  /*
  while(1){
    debughr();  
  }
  */
 
  delay(2000);
  Serial.println("Put your finger");
  delay(3000);  //wait for some seconds for the user to stabilize
  programStartTime = millis();
  Serial.println("Calulating Heart Rate");
  hr();      //this function calculates the BPM and prints it in the Serial Monitor.  
  Serial.println("Calculating Hb. . .");
  delay(3000);
  rawhb();
  Serial.print("Done. Redoing now. . ."); 
  
   
} 
