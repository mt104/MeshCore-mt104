#pragma once

#include <Arduino.h>

class AutoPower {
public:
  void setup();
  void loop();
  uint16_t getBattMilliVoltsSmoothed();
  uint16_t getLargestDropRecently();
  uint16_t getLowestSafeVoltage();
};

extern AutoPower autoPower;