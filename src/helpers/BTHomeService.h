#pragma once

#include <Arduino.h>

#if defined(ESP32)
#include <BLEAdvertising.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#endif

#if defined(NRF52_PLATFORM)
#include <bluefruit.h>
#endif

#if defined(ESP32) or defined(NRF52_PLATFORM)

class BTHomeService {
private:
  BLEAdvertising *pAdvertising;
  bool isInitialized = false;

public:
  void init(const char *deviceName);
  void sendSensorData(uint8_t batteryPercent, uint16_t batteryVoltage, int8_t noiseFloor);
};

extern BTHomeService bthomeService;

#endif