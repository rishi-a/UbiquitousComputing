// draw and update the ground floor

void drawGroundFloorPlan() {
  dwgs.label().text(F("Ground Floor")).offset(30, 63).send();
  dwgs.rectangle().size(60, 53).offset(0, 5).send();
  dwgs.line().offset(0, 5).size(0, -5).send();
  dwgs.line().size(60, -5).send();
  dwgs.line().offset(60, -5).size(0, 10).send();

  dwgs.label().text(F("Balcony")).fontSize(+3).offset(20, 1).send();
  dwgs.line().offset(30, 5).size(0, 28).send();
  dwgs.line().offset(30, 33).size(12, 0).send();
  dwgs.line().offset(30, 30).size(-8, 0).send();
  dwgs.line().offset(22, 30).size(0, 12).send();
  dwgs.line().offset(0, 42).size(33, 0).send();
  dwgs.line().offset(33, 42).size(0, 16).send();
  dwgs.line().offset(12, 42).size(0, 16).send();
  dwgs.label().text(F("Master\nBed")).fontSize(+3).offset(12, 30).send();
  dwgs.label().text(F("Bed 2")).fontSize(+3).offset(45, 26).send();
  dwgs.label().text(F("Hall")).fontSize(+3).offset(30, 38).send();
  dwgs.label().text(F("Hall")).fontSize(+3).offset(30, 38).send();
  dwgs.label().text(F("Bath")).fontSize(+1).offset(22, 55).send();
  dwgs.label().text(F("Ensuite")).fontSize(+1).offset(6, 55).send();

  dwgs.line().size(-18, 0).offset(60, 38).send();
  dwgs.line().size(0, -5).offset(60 - 18, 38).send();
  dwgs.line().size(0, -12).offset(60 - 18, 50).send();
  dwgs.label().text(F("Stairs")).fontSize(+3).offset(60 - 9, 38 + 3).send();
}

void drawGroundFloorControls() {
  dwgs.pushZero(35, 1);
  gndBalconyLight.draw();
  dwgs.popZero();
  dwgs.pushZero(15, 20);
  masterBedLight.draw();
  dwgs.popZero();
  dwgs.pushZero(45, 20);
  bed2Light.draw();
  dwgs.popZero();
  dwgs.pushZero(55, 48);
  gndStairsLight.draw();
  dwgs.popZero();
  dwgs.pushZero(37, 45);
  hallLight.draw();
  dwgs.popZero();
  dwgs.pushZero(6, 50);
  ensuiteLight.draw();
  dwgs.popZero();
  dwgs.pushZero(22, 50);
  bathLight.draw();
  dwgs.popZero();

  updateGroundFloorControls(); // update with current state
}

void updateGroundFloorControls() {
  gndBalconyLight.update();
  masterBedLight.update();
  bed2Light.update();
  gndStairsLight.update();
  hallLight.update();
  bathLight.update();
  ensuiteLight.update();
}

