#include "BTHomeService.h"

BTHomeService bthomeService;

void BTHomeService::init(const char *deviceName) {
  if (isInitialized) return;
  BLEDevice::init(deviceName);

  // Disable address privacy so the MAC remains static across advert cycles
  esp_ble_gap_config_local_privacy(false);

  pAdvertising = BLEDevice::getAdvertising();

  // Ensure scan responses are completely disabled so the entire 13-byte array goes out in a single primary
  // advertisement frame
  pAdvertising->setScanResponse(false);
  
  isInitialized = true;
}

void BTHomeService::sendSensorData(float temperature, uint8_t batteryPercent) {
  if (!isInitialized) init("MeshCore-Sensor");

  int16_t tempScaled = static_cast<int16_t>(temperature * 100.0f);

  // Build complete BTHome V2 advertisement payload
  // Service Data (0x16) + UUID (0xF6D2) + BTHome Header (0x40) + Sensor Data
  std::string bthomePayload = "";
  //bthomePayload += (char)0xD2; // BTHome UUID Low Byte (0xF6D2)
  //bthomePayload += (char)0xF6; // BTHome UUID High Byte
  bthomePayload += (char)0x40; // BTHome V2, unencrypted

  // Battery Metric (0x01)
  bthomePayload += (char)0x01;
  bthomePayload += (char)batteryPercent;

  // Temperature Metric (0x02, int16_t little-endian)
  bthomePayload += (char)0x02;
  bthomePayload += (char)(tempScaled & 0xFF);
  bthomePayload += (char)((tempScaled >> 8) & 0xFF);

  BLEAdvertisementData advData;
  advData.setFlags(0x06); // General Discoverable + BR/EDR Not Supported
  advData.setServiceData(BLEUUID((uint16_t)0xF6D2), bthomePayload);

  // Stop and restart advertising to ensure fresh payload broadcast
  pAdvertising->stop();
  pAdvertising->setAdvertisementData(advData);
  pAdvertising->start();
}