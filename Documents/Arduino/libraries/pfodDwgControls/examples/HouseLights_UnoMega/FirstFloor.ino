// draw and update the first floor

void firstFloorDraw() {
  drawFirstFloorPlan();
  drawFirstFloorControls();
}

void drawFirstFloorPlan() {
  dwgs.label().text(F("First Floor")).offset(30, 63).send();
  dwgs.rectangle().size(60, 58).send();
  dwgs.label().text(F("Balcony")).fontSize(+3).offset(25, 3).send();
  dwgs.line().offset(0, 22).size(32, 0).send();
  dwgs.line().size(0, -9).offset(32, 22).send();
  dwgs.line().size(60 - 32, 0).offset(32, 13).send();
  dwgs.label().text(F("Kitchen")).fontSize(+3).offset(60 - 10, 20).send();
  dwgs.label().text(F("Dining")).fontSize(+3).offset(28, 27).send();
  dwgs.label().text(F("Lounge")).fontSize(+3).offset(18, 52).send();
  dwgs.line().size(-18, 0).offset(60, 38).send();
  dwgs.line().size(0, -5).offset(60 - 18, 38).send();
  dwgs.line().size(0, -12).offset(60 - 18, 58).send();
  dwgs.label().text(F("Stairs")).fontSize(+3).offset(60 - 9, 38 + 3).send();
}

void drawFirstFloorControls() {
  dwgs.pushZero(25, 10);
  balconyLight.draw();
  dwgs.popZero();
  dwgs.pushZero(25, 35);
  diningLight.draw();
  dwgs.popZero();
  dwgs.pushZero(15, 45);
  loungeLight.draw();
  dwgs.popZero();
  dwgs.pushZero(55, 48);
  stairsLight.draw();
  dwgs.popZero();
  dwgs.pushZero(50, 27);
  kitchenLight.draw();
  dwgs.popZero();

  updateFirstFloorControls(); // update with current state
}

void updateFirstFloorControls() {
  balconyLight.update();//
  diningLight.update();//
  loungeLight.update();//
  kitchenLight.update();//
  stairsLight.update();//
}

