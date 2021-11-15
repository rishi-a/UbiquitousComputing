

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


void debugbp(){
  digitalWrite(select,LOW);
  analogWrite(ir,255);
  double filtered;
  int i;
  float data = (double)analogRead (sensorPin); // read the sensor   
  //Serial.println(data);
  //Serial.print(",");
  double small1 = 1000;
  double large1 = 0;
  double small2 = 1000;
  double large2 = 0;
  Serial.println("removing initial data") ;
  for(i = 0;i<1000;i++){
    data = (double)analogRead (sensorPin); 
    peakDetection.add(data);
  filtered = peakDetection.getFilt();
    }
  Serial.println("done");
  for(i = 0;i<window_size;i++){
  data = (double)analogRead (sensorPin); 
  
  peakDetection.add(data);
  filtered = peakDetection.getFilt();
  small1 = min(filtered,small1);
  large1 = max(filtered,large1);
  Serial.println(filtered); 
  
  //Serial.println(peak);

  
  }
  int count=0;
  for(i = 0;i<window_size;i++){
  data = (double)analogRead (sensorPin); 

  peakDetection.add(data);
  filtered = peakDetection.getFilt();
  window[count] = filtered;
  count = count+1;
  small2 = min(filtered,small2);
  large2 = max(filtered,large2);
  
  
  //Serial.println(peak);

  
  }
  count = 0;
  //double small = (small1+small2)/2.0;
  //double large = (large1+large2)/2.0;
//Serial.println(small); 
//Serial.println(large); 
  double thresh= ((large2-small2)+(large1-small1))/2.0;
Serial.println("pump air"); 
Serial.println(thresh);
//thresh = 4;
double smallc=0;
double largec=1000;
int index;
  while(largec-smallc>thresh*0.5){
    data = (double)analogRead (sensorPin); 
    peakDetection.add(data);
    filtered = peakDetection.getFilt();
    smallc = 1000;
    largec = 0;
    for(index = 0;index<window_size-1;index++){
      window[index] = window[index+1];
      
      smallc = min(window[index],smallc);
      largec = max(window[index],largec);
      }
    window[149] = filtered;
    smallc = min(window[window_size-1],smallc);
    largec = max(window[window_size-1],largec);
    
     //Serial.print(largec-smallc); 
    // Serial.print("\t");
     //Serial.println(thresh); 
    }
  //Serial.println(largec-smallc); 
  Serial.println("systole ");
  Serial.println("remove air");
  syst1 = analogRead(A5)*1.131-16.76 ;
 delay(2000);
  while(largec-smallc<thresh*0.8){
    data = (double)analogRead (sensorPin); 
    peakDetection.add(data);
    filtered = peakDetection.getFilt();
    smallc = 1000;
    largec = 0;
    for(index = 0;index<window_size-1;index++){
      window[index] = window[index+1];
      
      smallc = min(window[index],smallc);
      largec = max(window[index],largec);
      }
    window[149] = filtered;
    smallc = min(window[window_size-1],smallc);
    largec = max(window[window_size-1],largec);
    
    // Serial.println(filtered); 
    }
    Serial.println("thresh2 over"); 
    syst2 = analogRead(A5)*1.131-16.76 ;
delay(2000);
double maxdiff = largec-smallc  ;
  while(largec-smallc>=maxdiff){
    maxdiff = largec-smallc;
    data = (double)analogRead (sensorPin); 
    peakDetection.add(data);
    filtered = peakDetection.getFilt();
    smallc = 1000;
    largec = 0;
    for(index = 0;index<window_size-1;index++){
      window[index] = window[index+1];
      
      smallc = min(window[index],smallc);
      largec = max(window[index],largec);
      }
    window[149] = filtered;
    smallc = min(window[window_size-1],smallc);
    largec = max(window[window_size-1],largec);
    
     //Serial.println(filtered); 
    }
  
 Serial.println("diastole");
diast=analogRead(A5)*1.131-16.76 ;
Serial.println(syst1);
Serial.println(syst2);
Serial.println(diast);
delay(3000);

 
}


void hr(){
    int thresholdCrossCount=0;
    float data, BPM;
    digitalWrite(select,LOW);
    analogWrite(ir,255);
     
    bool peakDetected = 0;

    //fingerPlacement();
    //Serial.println("Be steady. Sensing your heart rate now");
    
    unsigned long programStartTime = millis();

     do{
        data = (double)analogRead (sensorPin); // read the sensor
        peakDetection.add(data);
        int peak = peakDetection.getPeak();
        double filtered = peakDetection.getFilt();
        //Serial.println(filtered);

 
        if(filtered>=(350)){ //280
          
          if(!peakDetected){
            thresholdCrossCount+=1;
            peakDetected = 1;
          }
        }
        if(filtered<(200)){ //265 for filtered /277
          peakDetected=0;  
        }

        
      }while((millis()-programStartTime)<15000);

      Serial.println(thresholdCrossCount);
      
      BPM=(thresholdCrossCount*4);
      thresholdCrossCount=0; //reset after recording every BPM
      if(BPM >=60 && BPM <= 120){
        Serial.print("Heart Rate:");
        EEBlue.write("Heart Rate: ");

        dtostrf(BPM, 6, 2, sBPM); // Leave room for too large numbers!
        Serial.println(BPM);
        EEBlue.write(sBPM);
        EEBlue.write("\n");
        programStartTime = millis(); 
      }
      else{
        Serial.println("Did you put your finger properly? Trying again.");
        delay(2000); //put a delay as user will move her finger after seeing the above message
        programStartTime = millis();
        Serial.println("Repeating...");
        EEBlue.write("Placement Incorrect. Trying again\n");
        delay(2000);
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
     threshold = threshold+2; //Anything above 4% of this threshold is a peak;
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
