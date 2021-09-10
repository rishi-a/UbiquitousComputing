/*
This algorithm is based on the paper https://ieeexplore.ieee.org/stamp/stamp.jsp?tp=&arnumber=9063311

*/


float ac_r, dc_r, ac_ir, dc_ir,R,Hb;
void rawhb(){
  analogWrite(ir,0);
  analogWrite(red,255);
  delay(1500);
  digitalWrite(select,HIGH); //seelct the IR RX coupled with RED LED.
  ac_r = analogRead(0);
  dc_r = analogRead(2);
  analogWrite(red,0);
  analogWrite(ir,255);
  delay(1500);
  digitalWrite(select,LOW); //seelct the IR RX coupled with RED LED.
  ac_ir = analogRead(0);
  dc_ir = analogRead(1);

  R = (ac_r/dc_r)/(ac_ir/dc_ir);
  Hb = (-3.626)*R+15.838; //see Table III of the paper linked above
  Serial.print("R:");
  Serial.println(R);
  Serial.print("\n");

  //gracefully switchoff the IR and LED
  //analogWrite(ir,0);
  //analogWrite(red,0);
  
  
}
