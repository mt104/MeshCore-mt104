#include "BTHomeWatcher.h"

#define BTHOME_UUID 0xfcd2 // v2

bool _is_scanning = false;
bool _done_first_begin = false;
char _result[1024];
unsigned long _num_received = 0;

#ifdef ESP32
class BTHomeAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
private:
  BLEScan *_scanner;

public:
  BTHomeAdvertisedDeviceCallbacks(BLEScan *scanner) : _scanner(scanner) {}

  void onResult(BLEAdvertisedDevice advertisedDevice) {
    // Look for BTHome service UUID in the advertised device
    if (advertisedDevice.haveServiceData()) {
      // Get payload from the advertised device
      BLEUUID serviceDataUUID = advertisedDevice.getServiceDataUUID();
      if (serviceDataUUID.equals(BLEUUID((uint16_t)BTHOME_UUID))) {
        _num_received++;
        std::string serviceData = advertisedDevice.getServiceData();
        uint16_t func_len = serviceData.length();

        if (func_len > 0) {
          const uint8_t *data = (const uint8_t *)serviceData.data();

          // Skip over the 0x40 prefix if present
          if (data[0] == 0x40) {
            data++;
            func_len--;
          }

          // Pass the payload directly (ESP32 already strips the 2-byte UUID prefix)
          BTHomeWatcher::parseBTHomePayload(data, func_len);
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
#ifdef NRF52_PLATFORM
    Bluefruit.Scanner.setRxCallback(scan_callback);
#endif
#ifdef ESP32
    pBLEScan = BLEDevice::getScan();
    BTHomeAdvertisedDeviceCallbacks *callbacks = new BTHomeAdvertisedDeviceCallbacks(pBLEScan);
    pBLEScan->setAdvertisedDeviceCallbacks(callbacks);
    pBLEScan->setActiveScan(false);
#endif
    _done_first_begin = true;
    _is_scanning = true;
  }

#ifdef NRF52_PLATFORM
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
#ifdef NRF52_PLATFORM
  Bluefruit.Scanner.stop();
#endif
#ifdef ESP32
  if (pBLEScan) {
    pBLEScan->clearResults();
    pBLEScan->stop();
  }
#endif
  _is_scanning = false;
}

#ifdef NRF52_PLATFORM
#include <bluefruit.h>
#endif

char *BTHomeWatcher::getResult() {
  return _result;
}

bool BTHomeWatcher::isScanning() {
  return _is_scanning;
}

#ifdef NRF52_PLATFORM
void BTHomeWatcher::scan_callback(ble_gap_evt_adv_report_t *report) {
  uint8_t buffer[32];
  memset(buffer, 0, sizeof(buffer));

  // Search the advertisement report for Service Data (Type 0x16)
  uint16_t func_len = Bluefruit.Scanner.parseReportByType(report, BLE_GAP_AD_TYPE_SERVICE_DATA, buffer, sizeof(buffer));

  // Service data includes the 2-byte UUID prefix little-endian followed by the BTHome payload
  if (func_len > 2) {
    uint16_t extracted_uuid = (buffer[1] << 8) | buffer[0];

    if (extracted_uuid == BTHOME_UUID) {
      // Pass the payload starting right after the 2-byte UUID
      parseBTHomePayload(&buffer[2], (uint16_t)(func_len - 2));
    }
  }

  Bluefruit.Scanner.resume();
}
#endif

void BTHomeWatcher::parseBTHomePayload(const uint8_t *data, uint16_t len) {
  sprintf(_result, "");

  // Step through the payload and parse individual fields as needed
  char buffer[256];
  uint16_t index = 0;

  while (index < (len - 1)) { // -1 to skip packet id
    uint8_t field_type = data[index++];
    switch (field_type) {
      // Example case for a hypothetical field type
      case 0x01: {
        // Parse field of type 0x01 - Battery percentage
        uint8_t battery_percentage = data[index++];
        sprintf(buffer, "%d%% ", battery_percentage);
        strcat(_result, buffer);
        break;
      }
      case 0x02: {
        // Parse field of type 0x02 - Temperature
        int16_t temperature = (data[index] << 8) | data[index + 1];
        index += 2;
        sprintf(buffer, "%0.2fC ", temperature / 100.0);
        strcat(_result, buffer);
        break;
      }
      case 0x0c: {
        // Parse field of type 0x0c - Voltage
        uint16_t voltage = (data[index]) | (data[index + 1] << 8);
        index += 2;
        sprintf(buffer, "%dmV ", voltage);
        strcat(_result, buffer);
        break;
      }
      case 0x59: {
        // Parse field of type 0x59 - Count
        int8_t count = data[index++];
        sprintf(buffer, "%d ", count);
        strcat(_result, buffer);
        break;
      }
      case 0x09: {
        // Parse field of type 0x09 - Count
        uint8_t count = data[index++];
        sprintf(buffer, "%d ", count);
        strcat(_result, buffer);
        break;
      }
      case 0x53: {
        // Parse field of type 0x53 - Text
        uint8_t text_length = data[index++];
        if (text_length > 30) text_length = 30; // Ensure we don't overflow the buffer
        memcpy(buffer, &data[index], text_length);
        buffer[text_length] = '\0';
        index += text_length;
        strcat(_result, "\"");
        strcat(_result, buffer);
        strcat(_result, "\" ");
        break;
      }
      default:
        // If we get here we can't safely handle the rest of the payload because we don't know how big this field is
        sprintf(buffer, "\n?? 0x%02X, len %d, index %d\n", field_type, len, index);
        strcat(_result, buffer);
        return;
    }
  }
}