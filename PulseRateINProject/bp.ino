
void  bp(){
  float pressureValue;
  float data;
  Serial.println("Computing Pressure. Start Pumping");
  while(analogRead(A5)<30){
    Serial.println(".");
  }
  Serial.println("Monitoring SYS pressure");

  //while loop exited. Now we will periodically look at PR value and decide if systolic value is reached

  data = (double)analogRead (sensorPin); // read the IR sensor that gives us PR
  peakDetection.add(data);
  int peak = peakDetection.getPeak();
  double filtered = peakDetection.getFilt();
  while(filtered<270){
    Serial.println(".");
    data = (double)analogRead (sensorPin); // read the IR sensor that gives us PR
    peakDetection.add(data);
    peak = peakDetection.getPeak();
    filtered = peakDetection.getFilt();
    delay(1000);
  }
  pressureValue = analogRead(A5);
  Serial.println("SYS: ");
  Serial.print(pressureValue);
}
  
