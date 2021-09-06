double ehb940=693.44,ehbo940=1214;   //for Hemoglobin measurement
double ehbo680=277.6,ehb680=2407.92,od680,od940,odA;
void gethb(){
  double num,den;
  analogWrite(red,255);delay(1500);
  nwod();
  analogWrite(red,0);delay(1500);
  od680=odA;
  
  analogWrite(ir,255);delay(1500);
  nwod();
  analogWrite(ir,0);delay(1500);
  od940=odA;

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
  Serial.print("HB:");
  Serial.println(hb);
}


void nwod(){
  odA=analogRead(1);
 // Serial.print("odA");
 // Serial.println(odA);
  odA=odA;
  odA=(float)(2*log((float)1000/odA)/2.3);
  odA=odA*64500*0.9;  
}
