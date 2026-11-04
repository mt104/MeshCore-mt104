#pragma once

#include <Arduino.h>

class AutoPower {
public:
  void setup();
  void loop();
  uint16_t getBattMilliVoltsSmoothed();
};
