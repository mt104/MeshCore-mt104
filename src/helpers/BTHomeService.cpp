#include "BTHomeService.h"

BTHomeService bthomeService;

char *_name = nullptr;

void BTHomeService::init(const char *deviceName) {
  if (isInitialized) return;
  BLEDevice::init(deviceName);
  _name = strdup(deviceName);

  // Disable address privacy so the MAC remains static across advert cycles
  esp_ble_gap_config_local_privacy(false);

  pAdvertising = BLEDevice::getAdvertising();

  // Configure advertising parameters
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinInterval(0x20); // 20ms advertising interval
  pAdvertising->setMaxInterval(0x40); // 40ms advertising interval

  // Ensure scan responses are completely disabled so the entire 13-byte array goes out in a single primary
  // advertisement frame
  pAdvertising->setScanResponse(false);
  
  isInitialized = true;
}

void BTHomeService::sendSensorData(uint8_t batteryPercent) {
  if (!isInitialized) init("MeshCore-Sensor");

  uint8_t payload[8];

  int pos = 0;

  // BTHome Device Info Header
  payload[pos++] = 0x40;

  payload[pos++] = 0x01;
  payload[pos++] = batteryPercent;

  // int16_t temp_raw = (int16_t)(temperature * 100.0f);
  // payload[pos++] = 0x02;
  // payload[pos++] = (uint8_t)(temp_raw & 0xFF);        // Temperature Low Byte
  // payload[pos++] = (uint8_t)((temp_raw >> 8) & 0xFF); // Temperature High Byte

  BLEAdvertisementData advData;
  advData.setFlags(0x06); // General Discoverable + BR/EDR Not Supported

  String payloadStr = "";
  for (int i = 0; i < pos; i++) {
    payloadStr += (char)payload[i];
  }

  advData.setServiceData(BLEUUID((uint16_t)0xFCD2), payloadStr.c_str());
  advData.setName(_name);

  pAdvertising->stop();
  pAdvertising->setAdvertisementData(advData);
  pAdvertising->start();
}