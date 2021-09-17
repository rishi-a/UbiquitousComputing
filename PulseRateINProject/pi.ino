


void pi(){
  unsigned long startPI = millis();
  float data, mindata = 1023, maxdata = 0, pindex;
  

  do{
    data = (double)analogRead (1); // read the sensor
    
    if(data<mindata){
      mindata = data;  
    }
    
   }while((millis()-startPI)<5000);

   startPI = millis(); //reset to calulte max

   do{
    data = (double)analogRead (1); // read the sensor
    
    if(data>maxdata){
      maxdata = data;  
    }
    
   }while((millis()-startPI)<5000);
   

   pindex = ((maxdata-mindata)/maxdata)*100;
   Serial.println("PI: ");
   Serial.print(pindex);
  
}
