#pragma once

#include <Arduino.h>

class AutoPower {
public:
  void setup();
  void recalibrate();
  void loop();
  uint16_t getBattMilliVoltsSmoothed();
  uint16_t getLargestDropRecently();
  uint16_t getAverageDropRecently();
  uint16_t getLowestSafeVoltage();
  float hoursRemaining();
};

extern AutoPower autoPower;