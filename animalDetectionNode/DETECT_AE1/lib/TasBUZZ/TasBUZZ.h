#ifndef TasBUZZ_h
#define TasBUZZ_h

#include <Arduino.h>

class TasBUZZ {
  public:
    TasBUZZ(int pin);
    void init();
    void on();
    void off();
    void setBUZZ(String state);
    uint8_t getBUZZ();

  private:
    int _pin;
    uint8_t _state;
};

#endif