// TasLAMP.h

#ifndef TASLAMP_H
#define TAsLAMP_H

#include <Arduino.h>

//#define LAMP_PIN    6

//void setLAMP(bool on);


class TasLAMP {
public:
    TasLAMP();
    //TasLAMP(uint8_t pin);
    void init();
    void setLAMP(String state);
    uint8_t getLAMP();

private:
    uint8_t _state;
    uint8_t _pin;
};



#endif // TASLAMP_H
