#ifndef TasUWB_h
#define TasUWB_h

#include <Arduino.h>

class TasUWB {
  public:
    TasUWB(int pin);
    void init();
    int read();

  private:
    int _pin;
};

#endif
