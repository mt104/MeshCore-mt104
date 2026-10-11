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

#define BTHOME_MAX_NEIGHBOURS 10

class BTHomeService {
private:
  BLEAdvertising *pAdvertising;
  bool isInitialized = false;
  uint8_t packetsSinceReset = 0;
  unsigned long lastResetTime = 0;
  // Struct to hold neighbour information
  struct Neighbour {
    uint8_t id;
    float snr;
  };
  Neighbour neighbours[BTHOME_MAX_NEIGHBOURS];  // Array to hold up to 10 neighbours

public:
  void init(const char *deviceName);
  void resetPacketCounter();
  void incrementPacketCounter();
  void resetNeighboursSNR();
  void updateNeighboursSNR(uint8_t from, float snr);
  void sendSensorData(uint8_t batteryPercent, uint16_t batteryVoltage, int8_t noiseFloor);
};

extern BTHomeService bthomeService;

#endif