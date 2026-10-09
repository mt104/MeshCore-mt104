#include "BTHomeService.h"

#if defined(ESP32) or defined(NRF52_PLATFORM)

BTHomeService bthomeService;

char *_name = nullptr;

#if defined(NRF52_PLATFORM)
bool _bluefruitAdvertising = false;
#endif

void BTHomeService::init(const char *deviceName) {
  if (isInitialized) return;

  _name = strdup(deviceName);

#if defined(ESP32)
  BLEDevice::init(_name);

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
#endif

#if defined(NRF52_PLATFORM)
  // Initialize Bluefruit
  Bluefruit.begin();
  Bluefruit.setTxPower(4); // Set transmit power (dBm)
#endif

  isInitialized = true;
}

void BTHomeService::sendSensorData(uint8_t batteryPercent, uint16_t batteryVoltage, int8_t noiseFloor) {
#if defined(ESP32)
  if (!isInitialized) init("MeshCore-Sensor");
#endif

  uint8_t payload[100]; // Should really keep track of how much we need rather than having an arbitrary large buffer

  int pos = 0;

#if defined(NRF52_PLATFORM)
  payload[pos++] = 0xd2;
  payload[pos++] = 0xfc;
#endif

  payload[pos++] = 0x40;

  payload[pos++] = 0x01;
  payload[pos++] = batteryPercent;
  payload[pos++] = 0x0c;
  payload[pos++] = (uint8_t)(batteryVoltage & 0xFF);
  payload[pos++] = (uint8_t)((batteryVoltage >> 8) & 0xFF);
  // Generic 8 bit signed integer sensor (noise floor)
  payload[pos++] = 0x59;
  payload[pos++] = (int8_t)(noiseFloor & 0xFF);

#if defined(ESP32)
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
#endif

#if defined(NRF52_PLATFORM)
  Bluefruit.Advertising.clearData();

  // Flags: General Discoverable, BR/EDR Not Supported
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);

  // Add Service Data (0x16) containing the BTHome payload
  Bluefruit.Advertising.addData(BLE_GAP_AD_TYPE_SERVICE_DATA, payload, pos);

  // Device Name in scan response to save payload space
  Bluefruit.ScanResponse.addName();

  /*
   * Advertising interval:
   * Fast Advertising: 100ms for quick discovery
   * Slow Advertising: 1000ms for battery savings
   */
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(160, 160); // units of 0.625ms -> 1600 * 0.625ms = 1000ms (1 second)
  Bluefruit.Advertising.setFastTimeout(0);

  if (!_bluefruitAdvertising) {
    Bluefruit.Advertising.start(0);
    _bluefruitAdvertising = true;
  }
#endif

}
#endif