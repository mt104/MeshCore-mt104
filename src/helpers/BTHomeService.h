#pragma once

#include <Arduino.h>
#include <BLEAdvertising.h>
#include <BLEDevice.h>
#include <BLEUtils.h>

class BTHomeService {
private:
  BLEAdvertising *pAdvertising;
  bool isInitialized = false;

public:
  void init(const char *deviceName);
  void sendSensorData(float temperature, uint8_t batteryPercent);
};

extern BTHomeService bthomeService;