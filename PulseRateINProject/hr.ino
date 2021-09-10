int thresholdCrossCount=0;

void debughr(){
  digitalWrite(select,LOW);
  analogWrite(ir,255);
  data = (double)analogRead (sensorPin); // read the sensor   
  //Serial.println(data);
  //Serial.print(",");
  peakDetection.add(data);
  int peak = peakDetection.getPeak();
  double filtered = peakDetection.getFilt();
  Serial.println(filtered);
  //Serial.println(peak);

  
}
void hr(){
     digitalWrite(select,LOW);
     analogWrite(ir,255);
     
     bool peakDetected = 0;

    //fingerPlacement();
    Serial.println("Sensing your heart rate now");
     

     do{
        data = (double)analogRead (sensorPin); // read the sensor
        peakDetection.add(data);
        int peak = peakDetection.getPeak();
        double filtered = peakDetection.getFilt();
        //Serial.println(filtered);

 
        if(filtered>=275){ //300 for filtered //280
          
          if(!peakDetected){
            thresholdCrossCount+=1;
            peakDetected = 1;
          }
        }
        if(filtered<275){ //150 for filtered
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
        programStartTime = millis();
        //Serial.println("Wait...");
        hr();
      }
     /*

    if(peak == 1){
      countSamples+=1;
      if(firstPeak == 0){
          firstPeakTime = millis();
          firstPeak = 1;
      }
      else{
          BPM = (60*70)/(countSamples);  //there are 70 samples per second
          secondPeakTime = millis();
          HRV = secondPeakTime-firstPeakTime; //we use HRV to check if the RR interval is correct
          
          if(BPM>=50 && BPM<=120 && HRV>=400 && HRV <=860){ //A human's HRV can be between 0.4s (infant) to 0.86s (adult)
            //Serial.print("BPM = ");
            //Serial.println(BPM);
         
          }
          BPM=0;
          countSamples=0;
          firstPeak=0;
      }  
    }
    else if(peak==0 || peak ==  -1){
      countSamples+=1;  
    } 
    */
     //delay(10);   
}

void fingerPlacement(){
   bool peakDetected = 0;
   data = (double)analogRead (sensorPin); // read the sensor
   peakDetection.add(data);
   int peak = peakDetection.getPeak();
   double filtered = peakDetection.getFilt();
   if(filtered>=300){
          if(!peakDetected){
            thresholdCrossCount+=1;
            peakDetected = 1;
          }
        }
    if(filtered<=150){
      peakDetected=0;
      return;  
    }
    fingerPlacement();
    Serial.println("Place the finger correctly");  
}
