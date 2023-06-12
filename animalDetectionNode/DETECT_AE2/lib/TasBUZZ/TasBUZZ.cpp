#include "TasBUZZ.h"

//#define BUZZ_PIN A0


TasBUZZ::TasBUZZ(int BUZZ_PIN) {
  _pin = BUZZ_PIN;
}

void TasBUZZ::init() {
  pinMode(_pin, OUTPUT);
}

void TasBUZZ::on() {
  digitalWrite(_pin, HIGH);  
}

void TasBUZZ::off() {
  digitalWrite(_pin, LOW);
}

void TasBUZZ::setBUZZ(String state) {
  if(String(state) == "on"){
    digitalWrite(_pin, HIGH);
    _state = 1;
  }
  else if (String(state) == "off") {
    digitalWrite(_pin, LOW);
    _state = 0;
  }
}

uint8_t TasBUZZ::getBUZZ() {
    return _state;
}
