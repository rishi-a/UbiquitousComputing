/*
This algorithm is based on the paper https://ieeexplore.ieee.org/stamp/stamp.jsp?tp=&arnumber=9063311

*/

/*
float ac_r, dc_r, ac_ir, dc_ir,R,Hb;
void rawhb(){
  analogWrite(ir,0);
  analogWrite(red,255);
  delay(1500);
  ac_r = analogRead(0);
  dc_r = analogRead(1);
  analogWrite(red,0);
  analogWrite(ir,255);
  delay(1500);
  ac_ir = analogRead(0);
  dc_ir = analogRead(1);

  R = (ac_r/dc_r)/(ac_ir/dc_ir);
  Hb = (-3.626)*R+15.838; //see Table III of the paper linked above
  Serial.print("Hb:");
  Serial.println(Hb);

  //gracefully switchoff the IR and LED
  analogWrite(ir,0);
  analogWrite(red,0);
  
  
}
*/
