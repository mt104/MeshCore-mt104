#include "BTHomeWatcher.h"

#define BTHOME_UUID 0xfcd2 // v2

bool _done_first_begin = false;
char _result[256];
unsigned long _num_received = 0;

#ifdef ESP32
class BTHomeAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
private:
  BLEScan *_scanner;

public:
  BTHomeAdvertisedDeviceCallbacks(BLEScan *scanner) : _scanner(scanner) {}

  void onResult(BLEAdvertisedDevice advertisedDevice) {
    // Look for BTHome service UUID in the advertised device
    if (advertisedDevice.haveServiceUUID()) {
      // Get payload from the advertised device
      BLEUUID serviceDataUUID = advertisedDevice.getServiceDataUUID();
      if (true or serviceDataUUID.equals(BLEUUID((uint16_t)BTHOME_UUID))) {
        _num_received++;
        std::string serviceData = advertisedDevice.getServiceData();
        uint16_t func_len = serviceData.length();

        if (func_len > 0) {
          const uint8_t *data = (const uint8_t *)serviceData.data();

          sprintf(_result, "Num Received: %lu\nRSSI: %d dBm\nPayload len: %d", _num_received,
                  advertisedDevice.getRSSI(), serviceData.length());

          // Pass the payload directly (ESP32 already strips the 2-byte UUID prefix)
          // parseBTHomePayload(data, func_len);
        }
      }
    }

    // Clear previous scan results using the stored scanner reference
    if (_scanner != nullptr) {
      _scanner->clearResults();
    }
  }
};

BLEScan *pBLEScan = nullptr;

#endif

void BTHomeWatcher::begin() {
  Serial.println("Starting BTHomeWatcher scan");
  sprintf(_result, "Scanning...");
  _num_received = 0;

  if (!_done_first_begin) {
    Serial.println("Performing first-time initialization for BTHomeWatcher");
#ifdef NRF52
    Bluefruit.Scanner.setRxCallback(scan_callback);
#endif
#ifdef ESP32
    pBLEScan = BLEDevice::getScan();
    BTHomeAdvertisedDeviceCallbacks *callbacks = new BTHomeAdvertisedDeviceCallbacks(pBLEScan);
    pBLEScan->setAdvertisedDeviceCallbacks(callbacks);
    pBLEScan->setActiveScan(false);
#endif
    _done_first_begin = true;
  }

#ifdef NRF52
  Bluefruit.Scanner.restartOnDisconnect(true);
  Bluefruit.Scanner.setInterval(160, 80);
  Bluefruit.Scanner.useActiveScan(false); // Passive scanning saves power
  Bluefruit.Scanner.start(0);             // 0 = Scan continuously without timeout
#endif
#ifdef ESP32
  pBLEScan->start(0, nullptr, false);
#endif
}

void BTHomeWatcher::end() {
  Serial.println("Ending BTHomeWatcher scan");
#ifdef NRF52
  Bluefruit.Scanner.stop();
#endif
#ifdef ESP32
  if (pBLEScan) {
    pBLEScan->clearResults();
    pBLEScan->stop();
  }
#endif
}

#ifdef NRF52
#include <bluefruit.h>
#endif

char *BTHomeWatcher::getResult() {
  return _result;
}

#ifdef NRF52
void BTHomeWatcher::scan_callback(ble_gap_evt_adv_report_t *report) {
  uint8_t buffer[32];
  memset(buffer, 0, sizeof(buffer));

  // Search the advertisement report for Service Data (Type 0x16)
  uint16_t func_len =
      Bluefruit.Scanner.parseReportByType(report, BLE_GAP_AD_TYPE_SERVICE_DATA, buffer, sizeof(buffer));
  _debug_func_len = func_len;

  // Service data includes the 2-byte UUID prefix little-endian followed by the BTHome payload
  if (func_len > 2) {
    uint16_t extracted_uuid = (buffer[1] << 8) | buffer[0];

    if (extracted_uuid == BTHOME_UUID) {
      // Pass the payload starting right after the 2-byte UUID
      // parseBTHomePayload(&buffer[2], func_len - 2);
    }
  }

  Bluefruit.Scanner.resume();
}
#endif