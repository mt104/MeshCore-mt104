#pragma once

#if defined(ESP32)

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
  void sendSensorData(uint8_t batteryPercent, uint16_t batteryVoltage);
};

extern BTHomeService bthomeService;

#endif