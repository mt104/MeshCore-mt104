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

#define BTHOME_MAX_NEIGHBOURS 4
#define BTHOME_INTERVAL_MS 20000

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
  Neighbour neighbours[BTHOME_MAX_NEIGHBOURS];

public:
  void on();
  void off();
  bool isEnabled();
  void init(const char *deviceName);
  void deinit();
  void resetPacketCounter();
  void incrementPacketCounter();
  void resetNeighboursSNR();
  void removeFirstNeighbour();
  void updateNeighboursSNR(uint8_t from, float snr);
  void sendSensorData(uint8_t batteryPercent, uint16_t batteryVoltage, int8_t noiseFloor);
};

extern BTHomeService bthomeService;

#endif