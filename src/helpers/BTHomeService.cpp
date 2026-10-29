#include "BTHomeService.h"

#if defined(ESP32) or defined(NRF52_PLATFORM)

#include "TagHelper.h"

BTHomeService bthomeService;

char *_name = nullptr;
bool _isEnabled = false;
uint32_t _interval_seconds = 20;

static char *keyIsEnabled = "bthome.enabled";
static char *keyInterval = "bthome.interval";

#if defined(NRF52_PLATFORM)
bool _bluefruitAdvertising = false;
#endif

void BTHomeService::on() {
  if (isEnabled()) {
    return;
  }
  tagHelper.setValue(keyIsEnabled, "1");
  resetPacketCounter();
  resetNeighboursSNR();
  init(_name);
}

void BTHomeService::off() {
  if (!isEnabled()) {
    return;
  }
  tagHelper.setValue(keyIsEnabled, "0");
  deinit();
}

bool BTHomeService::isEnabled() {
  return _isEnabled;
}

uint32_t BTHomeService::getIntervalSeconds() {
  return _interval_seconds;
}

void BTHomeService::setIntervalSeconds(uint32_t interval) {
  _interval_seconds = interval;
  char buffer[10];
  sprintf(buffer, "%u", interval);
  tagHelper.setValue(keyInterval, buffer);
}

void BTHomeService::init(const char *deviceName) {
  // Store the device name for later use
  if (_name != nullptr) {
    free(_name);
  }
  _name = strdup(deviceName);

  // Load the interval from tagHelper
  _interval_seconds = atoi(tagHelper.getValue(keyInterval, "20"));

  // Initialise _isEnabled based on the value stored in tagHelper, needed for first call
  _isEnabled = strcmp(tagHelper.getValue(keyIsEnabled, "1"), "1") == 0;

  // If the service is not enabled, we should not proceed with initialization
  if (!isEnabled()) {
    return;
  }

  if (isInitialized) return;

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
  Bluefruit.setTxPower(8); // Set transmit power (dBm)
#endif

  resetPacketCounter();
  resetNeighboursSNR();
  isInitialized = true;
}

// Opposite of init
void BTHomeService::deinit() {
  if (!isEnabled()) return;

#if defined(ESP32)
  BLEDevice::deinit();
#endif

#if defined(NRF52_PLATFORM)
  Bluefruit.Advertising.stop();
#endif

  isInitialized = false;
  _isEnabled = false;
}

void BTHomeService::resetPacketCounter() {
  packetsSinceReset = 0;
  lastResetTime = millis();
}

void BTHomeService::incrementPacketCounter() {
  packetsSinceReset++;
}

void BTHomeService::resetNeighboursSNR() {
  // Zero out id and SNR for all neighbours
  for (int i = 0; i < BTHOME_MAX_NEIGHBOURS; i++) {
    neighbours[i].id = 0;
    neighbours[i].snr = 0.0f;
  }
}

void BTHomeService::removeFirstNeighbour() {
  for (int i = 0; i < BTHOME_MAX_NEIGHBOURS - 1; i++) {
    neighbours[i] = neighbours[i + 1];
  }
  neighbours[BTHOME_MAX_NEIGHBOURS - 1].id = 0;
  neighbours[BTHOME_MAX_NEIGHBOURS - 1].snr = 0.0f;
}

void BTHomeService::updateNeighboursSNR(uint8_t from, float snr) {
  bool handled = false;
  // If we already know about this neighbour, update its SNR; otherwise, add it to the list
  for (int i = 0; i < BTHOME_MAX_NEIGHBOURS; i++) {
    if (neighbours[i].id == from) {
      // Only update if higher SNR than previously recorded
      if (snr > neighbours[i].snr) {
        neighbours[i].snr = snr;
      }
      handled = true;
      break;
    }
  }
  if (handled) return;
  // If we didn't find the neighbour, add it to the first empty slot
  for (int i = 0; i < BTHOME_MAX_NEIGHBOURS; i++) {
    if (neighbours[i].id == 0) {
      neighbours[i].id = from;
      neighbours[i].snr = snr;
      break;
    }
  }
}

void BTHomeService::sendSensorData(uint8_t batteryPercent, uint16_t batteryVoltage, int8_t noiseFloor) {

  if (!isEnabled()) return;

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

  // Packets per minute
  float minutesSinceReset = (millis() - lastResetTime) / 60000.0f;
  uint8_t packetsPerMinute = (uint8_t)(packetsSinceReset / minutesSinceReset);
  resetPacketCounter();
  // Generic 8 bit unsigned integer sensor (packets per minute)
  payload[pos++] = 0x09;
  payload[pos++] = packetsPerMinute;

  // Neighbours' SNR to serial port for debugging, converting the 3 bytes in id to a string in hex
  for (int i = 0; i < BTHOME_MAX_NEIGHBOURS; i++) {
    if (neighbours[i].id != 0) {
      char idStr[3];
      snprintf(idStr, sizeof(idStr), "%02X", neighbours[i].id);
      Serial.print("Neighbour ID: ");
      Serial.print(idStr);
      Serial.print(" SNR: ");
      Serial.println(neighbours[i].snr);
    }
  }
  Serial.println("");

  // Neighbours' SNR data as text, one at once using first in array
  char neighbourData[10];
  payload[pos++] = 0x53;
  sprintf(neighbourData, "%02X %0.2f", neighbours[0].id, neighbours[0].snr);
  payload[pos++] = strlen(neighbourData);
  for (int i = 0; i < strlen(neighbourData); i++) {
    payload[pos++] = neighbourData[i];
  }
  removeFirstNeighbour();

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