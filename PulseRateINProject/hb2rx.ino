double storeHb[3], averageHb;
int hbCount=0;

void hb2rx(){
  double ehb940=693.44,ehbo940=1214, hb, hbo;   //for Hemoglobin measurement
  double ehbo680=277.6,ehb680=2407.92,od680,od940;
  double num,den;
  analogWrite(red,255);delay(1500);
  od680 = nwod_red();
  analogWrite(red,0);delay(1500);
  
  
  analogWrite(ir,255);delay(1500);
  od940 = nwod_ir();
  analogWrite(ir,0);delay(1500);
 
  den=(ehbo940*ehb680)-(ehb940*ehbo680);
  den=abs(den);
  double p1=(ehb940*od680),p2=(od940*ehb680);
  num=(double)p1-p2;
  num=abs(num);
  hbo=(float)num/den;
  num=(ehbo940*od680)-(od940*ehbo680);
  num=abs(num);
  hb=(float)num/den;
  hb=hb+hbo;
  storeHb[hbCount]=hb;
  if(hbCount==2){
    averageHb = (storeHb[0]+storeHb[1]+storeHb[2])/3;
    Serial.print("Hb:");
    Serial.println(averageHb);
    hbCount=0;
  }
  else{
    hbCount++;
    Serial.println("Wait. .computing Hb");
    hb2rx();
  }
}


float nwod_red(){
  float odA_red;
  //digitalWrite(select, HIGH); //selects the RED coupled IR Receiver
  odA_red=analogRead(2);
  //Serial.print("I Red ");
  //Serial.println(odA_red);
  odA_red=(float)(2*log((float)1000/odA_red)/2.3);
  odA_red=odA_red*64500*0.13;  
  //Serial.println(odA_red);
  //odA_red=odA_red*64500*0.9;
  return  odA_red; 
}

float nwod_ir(){
  float odA_ir;
  //digitalWrite(select, LOW); //selects the IR coupled IR Receiver
  odA_ir=analogRead(1);
  //Serial.print("I0 IR ");
  //Serial.println(odA_ir);
  odA_ir=(float)(2*log((float)1000/odA_ir)/2.3);
  odA_ir=odA_ir*64500*0.13;
  //Serial.println(odA_ir);
  //odA_ir=odA_ir*64500*0.9; 
  return odA_ir; 
}
