#include "TasUWB.h"

TasUWB::TasUWB(int pin) {
  _pin = pin;
}

void TasUWB::init() {
  pinMode(_pin, INPUT);
}

int TasUWB::read() {
  return digitalRead(_pin);
}
