

int threshold; //this is the threshold to detect peaks. This will be determined in the fignerplacement test and used further to detect heart rate

void debughr(){
  digitalWrite(select,LOW);
  analogWrite(ir,255);
  float data = (double)analogRead (sensorPin); // read the sensor   
  //Serial.println(data);
  //Serial.print(",");
  peakDetection.add(data);
  int peak = peakDetection.getPeak();
  double filtered = peakDetection.getFilt();
  Serial.println(filtered);
  //Serial.println(peak);

  
}
void hr(){
    int thresholdCrossCount=0;
    float data, BPM;
    digitalWrite(select,LOW);
    analogWrite(ir,255);
     
    bool peakDetected = 0;

    fingerPlacement();
    Serial.println("Be steady. Sensing your heart rate now");

    unsigned long programStartTime = millis();

     do{
        data = (double)analogRead (sensorPin); // read the sensor
        peakDetection.add(data);
        int peak = peakDetection.getPeak();
        double filtered = peakDetection.getFilt();
        //Serial.println(filtered);

 
        if(filtered>=280){ //300 for filtered //280
          
          if(!peakDetected){
            thresholdCrossCount+=1;
            peakDetected = 1;
          }
        }
        if(filtered<265){ //150 for filtered
          peakDetected=0;  
        }

        
      }while((millis()-programStartTime)<15000);

      Serial.println(thresholdCrossCount);
      
      BPM=(thresholdCrossCount*4);
      thresholdCrossCount=0; //reset after recording every BPM
      if(BPM >=60 && BPM <= 120){
        Serial.print("Heart Rate:");
        Serial.println(BPM); 
        programStartTime = millis(); 
      }
      else{
        Serial.println("Did you put your finger properly? Trying again.");
        delay(2000); //put a delay as user will move her finger after seeing the above message
        programStartTime = millis();
        Serial.println("Repeating...");
        hr();
      }
    

    
}

void fingerPlacement(){
      int i = 0;
      float data; int peak; double filtered;
      double storeAnalog[100];
      int storeAnaloglength = sizeof(storeAnalog)/sizeof(storeAnalog[0]);

     Serial.println("Warming Up. . Remove any finger");
     delay(5000);
     //store some analog values to determine the threshold
     while(i<storeAnaloglength){
       delay(100);
       data = (double)analogRead (sensorPin); // read the sensor
       //Serial.println(i);
       peakDetection.add(data);
       peak = peakDetection.getPeak();
       filtered = peakDetection.getFilt();
       //erial.println(filtered);
       storeAnalog[i++] = filtered; 
     }
     threshold = sum(storeAnalog+25, storeAnaloglength-30)/(storeAnaloglength-30); //we are ignoring first 25 values of the sensor
     threshold = threshold+5; //Anything above 4% of this threshold is a peak;
     Serial.println("Threshold is: ");
     Serial.println(threshold);
     Serial.println("Keep your finger");
     delay(5000);

  
     unsigned long startFP=millis();


     while(filtered<threshold || ((millis()-startFP)<5000)){
        //Serial.println(filtered);
        data = (double)analogRead (sensorPin); // read the sensor
        peakDetection.add(data);
        peak = peakDetection.getPeak();
        filtered = peakDetection.getFilt();
        Serial.println(".");
        delay(50);
     }
     delay(1500);
     Serial.println("\n");
}

int sum(double arr[], int n)
{
    int sum = 0; // initialize sum
 
    // Iterate through all elements
    // and add them to sum
    for (int i = 0; i < n; i++){
    //Serial.println(arr[i]);
    sum += arr[i];
    }
 
    return sum;
}
