#include <PeakDetection.h>
bool DEBUG=0;
int sensorPin = 0;
PeakDetection peakDetection;


/*Define the Pins*/
int red=3, ir=10, select=8;


void setup() {
   Serial.begin(9600);
   peakDetection.begin(10, 3, 0.2);
   pinMode(select, OUTPUT);
   
}
void loop (){

  if(DEBUG){
    //measure_prsr();
    debughr();
    //hr();
    //delay(2000);
    //Serial.println("Calculating Hb:");
    //hb2rx();  
  }
  
  else{
  delay(3000);  //wait for some seconds for the user to stabilize
  hr();      //this function calculates the BPM and prints it in the Serial Monitor.  
  delay(1000);
  hb2rx();
  spo2();
  pi();
  //bp();
  Serial.print("*****\n");
  
  }
   
} 
