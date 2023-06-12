#include "TasLAMP.h"


TasLAMP::TasLAMP() {
}

void TasLAMP::init() {
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, HIGH);
}


void TasLAMP::setLAMP(String state) {
    if (state == "on") {
        digitalWrite(_pin, HIGH);
        _state = 1;
    } else if (state == "off") {
        digitalWrite(_pin, LOW);
        _state = 0;
    }
}

uint8_t TasLAMP::getLAMP() {
    return _state;
}

/*
void TasLAMP::setLAMP(bool on) {
    digitalWrite(LAMP_PIN, on ? 1 : 0);
}

String TasLAMP::getLAMPstate() {
    return _state;
}

TasLAMP::TasLAMP(int LAMP_PIN) {
  pin = LAMP_PIN;
  pinMode(pin, OUTPUT);
}

void TasLAMP::setLAMP(bool state) {
  digitalWrite(pin, state ? HIGH : LOW);
}
*/