/*
 * (c)2014-2017 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provide this copyright is maintained.
 */
#include "pfodDwgsBase.h"

pfodDwgsBase::pfodDwgsBase() {
}

void pfodDwgsBase::send(char _startChar) {
	(void)(_startChar);
}

void pfodDwgsBase::initValues(struct VALUES* _valuesPtr) {
  valuesPtr = _valuesPtr;
  // valuesPtr->action = NULL;
  valuesPtr->startAngle = 0;
  valuesPtr->arcAngle = 180;
  valuesPtr->text = NULL;
  valuesPtr->textF = F("");
  valuesPtr->units = NULL;
  valuesPtr->unitsF = NULL;
  valuesPtr->reading = 0.0f;
  valuesPtr->haveReading = 0;
  valuesPtr->value = 0;
  valuesPtr->haveValue = 0;
  valuesPtr->displayMax = 1;
  valuesPtr->displayMin = 0;
  valuesPtr->max = 1;
  valuesPtr->min = 0;
  valuesPtr->decPlaces = 2;
  valuesPtr->fontSize = 0;
  valuesPtr->bold = 0;
  valuesPtr->italic = 0;
  valuesPtr->underline = 0;
  valuesPtr->color = BLACK_WHITE;
  valuesPtr->width = 1;
  valuesPtr->height = 1;
  valuesPtr->colOffset = 0;
  valuesPtr->rowOffset = 0;
  valuesPtr->idx = 0;
  valuesPtr->radius = 1;
  valuesPtr->filled = 0;
  valuesPtr->rounded = 0;
  valuesPtr->cmd = ' ';
  valuesPtr->cmdStr = NULL; 
  valuesPtr->loadCmd = ' '; // for loading and erase of insertDwgs
  valuesPtr->loadCmdStr = NULL; // for loading and erase of insertDwgs
  valuesPtr->filter = 0;
  valuesPtr->centered = 0; 
  valuesPtr->align = 'C'; // nothing defaults to centered
}

void pfodDwgsBase::startText() {
  struct VALUES* valPtr = valuesPtr;
  printIdx();
  printColor();
  out->print('~');
  if (valPtr->fontSize != 0) {
    out->print('<');
    if (valPtr->fontSize >= 0) {
      out->print('+');
    }
    out->print(valPtr->fontSize);
    out->print('>');
  }
  if (valPtr->italic != 0) {
    out->print('<'); out->print('i'); out->print('>');
  }
  if (valPtr->bold != 0) {
    out->print('<'); out->print('b'); out->print('>');
  }
  if (valPtr->underline != 0) {
    out->print('<'); out->print('u'); out->print('>');
  }
}

// prints ~ + number
// drops trailing zeros prints at most 3 decimal places
void pfodDwgsBase::printFloat(float f) {
  out->print('~');
  printFloatNumber(f);
}

// prints just number rounded to decPlaces, -ve decPlaces round to left of decimal point
void pfodDwgsBase::printFloatDecimals(float f, int decPlaces) {
  if (f < 0) {
    f = -f;
    out->print('-');
  }
  if (decPlaces <= 0) {
    unsigned long iValue = (unsigned long ) f;
    if ((f - iValue) != 0) {
      // round
      iValue = (unsigned long) (f + 0.5);
    }
    if (decPlaces == 0) {
      out->print(iValue);
    } else {
      // < 0
      int divider = 1;
      for (int i = 0; i < (-decPlaces); i++) {
        divider = divider * 10;
      }
      iValue = iValue / divider;
      int idValue = iValue * divider;
      if ((idValue - iValue) != 0) {
        // need to round
        iValue = iValue + (divider / 2);
        iValue = iValue / divider;
        idValue = iValue * divider;
      }
      out->print(idValue);
    }
  } else {
    // (decPlaces > 0) {
    out->print(f, decPlaces);
  }
}

// prints just number
// drops trailing zeros prints at most 3 decimal places
void pfodDwgsBase::printFloatNumber(float f) {
  if (f < 0) {
    f = -f;
    out->print('-');
  }
  // only print at most 3 decimals
  // round
  f = f + 0.0005;
  unsigned long intPart = (unsigned long)(f * 1000);
  // 1.234 => 1234
  int fraction = intPart % 1000; // 1234 -> 234
  // no decimals
  out->print((unsigned long)f);
  if (fraction != 0) {
    out->print('.');
    int tenths = (fraction / 100); // 234 -> 2
    out->print(tenths); 
    fraction =  fraction - tenths * 100; // -> 34
    int hundreths = (fraction / 10); // 34 -> 3
    if (hundreths != 0) {
      int thousandths =  fraction - hundreths * 10; // -> 4
      out->print(hundreths);
      if (thousandths != 0) {
        out->print(thousandths);
      }
    }
  }
}

// pfodApp defaults these to 0 if missing
void pfodDwgsBase::colRowOffset() {
  float colOffset = valuesPtr->colOffset;
  float rowOffset = valuesPtr->rowOffset;
  if (colOffset == 0) {
    out->print('~');
  } else {
    sendColRowVars(colOffset);
  }
  if (rowOffset == 0) {
    out->print('~');
  } else {
    sendColRowVars(rowOffset);
  }
}

void pfodDwgsBase::colWidthHeight() {
  sendColRowVars(valuesPtr->width);
  sendColRowVars(valuesPtr->height);
}

void pfodDwgsBase::sendColRowVars(float val) {
  out->print('~');
  long longVar = (long)val;
  if (longVar == TOUCHED_COL) {
    out->print('c');
  } else if (longVar == TOUCHED_ROW) {
    out->print('r');
  } else {
    printFloatNumber(val);
  }
}

void pfodDwgsBase::printColor() {
  int _colorValue = valuesPtr->color;
  out->print('~');
  if (_colorValue >= 0) {
    out->print(_colorValue);
  }
}

void pfodDwgsBase::printIdx() {
  unsigned int idx = valuesPtr->idx;
  if (idx > 0) {
    out->print('`');
    out->print(idx);
  }
}

