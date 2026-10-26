#include "BTHomeWatcher.h"

bool _done_first_begin = false;
int _debug_func_len = -1;

#ifdef ESP32
class BTHomeAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    //BTHomeWatcher::scan_callback(advertisedDevice);
    _debug_func_len++;
  }
};

BLEScan *pBLEScan = nullptr;
#endif

void BTHomeWatcher::begin() {
  Serial.println("Starting BTHomeWatcher scan");

  if (!_done_first_begin) {
    Serial.println("Performing first-time initialization for BTHomeWatcher");
#ifdef NRF52
    Bluefruit.Scanner.setRxCallback(scan_callback);
#endif
#ifdef ESP32
    pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new BTHomeAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(false);
#endif
    _done_first_begin = true;
  }

#ifdef NRF52
  Bluefruit.Scanner.restartOnDisconnect(true);
  Bluefruit.Scanner.setInterval(160, 80);
  Bluefruit.Scanner.useActiveScan(false); // Passive scanning saves power
  Bluefruit.Scanner.start(0); // 0 = Scan continuously without timeout
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
    }
#endif
}

#ifdef NRF52
#include <bluefruit.h>
#endif

// BTHome v2 16-bit Service UUID
const uint16_t BTHOME_UUID = 0x181C;

/*
void BTHomeWatcher::parseBTHomePayload(const uint8_t *payload, uint16_t length) {
  if (length < 1) return;

  uint8_t bthome_info = payload[0];
  bool is_encrypted = (bthome_info & 0x01);

  Serial.printf("  - BTHome Info: 0x%02X (Encrypted: %s)\n", bthome_info, is_encrypted ? "YES" : "NO");

  if (is_encrypted) {
    Serial.println("  - Payload is encrypted.");
    return;
  }

  uint16_t index = 1;
  while (index < length) {
    uint8_t obj_type = payload[index++];

    switch (obj_type) {
    case 0x00: // Packet ID
      if (index < length) {
        Serial.printf("    * Packet ID: %d\n", payload[index]);
        index += 1;
      }
      break;
    case 0x01: // Battery
      if (index < length) {
        Serial.printf("    * Battery: %d%%\n", payload[index]);
        index += 1;
      }
      break;
    case 0x02: // Temperature
      if (index + 1 < length) {
        int16_t raw_temp = (payload[index + 1] << 8) | payload[index];
        Serial.printf("    * Temperature: %.2f °C\n", raw_temp * 0.01);
        index += 2;
      }
      break;
    case 0x03: // Humidity
      if (index < length) {
        Serial.printf("    * Humidity: %d%%\n", payload[index]);
        index += 1;
      }
      break;
    default:
      Serial.printf("    * Unknown Object Type: 0x%02X\n", obj_type);
      return;
    }
  }
}
*/

int BTHomeWatcher::getDebugFuncLen() {
    return _debug_func_len;
}

#ifdef NRF52
void BTHomeWatcher::scan_callback(ble_gap_evt_adv_report_t* report) {
    uint8_t buffer[32];
    memset(buffer, 0, sizeof(buffer));

    // Search the advertisement report for Service Data (Type 0x16)
    uint16_t func_len = Bluefruit.Scanner.parseReportByType(report, BLE_GAP_AD_TYPE_SERVICE_DATA, buffer, sizeof(buffer));
    _debug_func_len = func_len;

    // Service data includes the 2-byte UUID prefix little-endian followed by the BTHome payload
    if (func_len > 2) {
        uint16_t extracted_uuid = (buffer[1] << 8) | buffer[0];

        if (extracted_uuid == BTHOME_UUID) {
            /*
            char mac_str[18];
            sprintf(mac_str, "%02X:%02X:%02X:%02X:%02X:%02X",
                    report->peer_addr.addr[5], report->peer_addr.addr[4], report->peer_addr.addr[3],
                    report->peer_addr.addr[2], report->peer_addr.addr[1], report->peer_addr.addr[0]);

            Serial.printf("Found BTHome Device! MAC: %s, RSSI: %d dBm\n", mac_str, report->rssi);
            */
            
            // Pass the payload starting right after the 2-byte UUID
            //parseBTHomePayload(&buffer[2], func_len - 2);
        }
    }

    //Bluefruit.Scanner.start(0); // 0 = Scan continuously without timeout
    Bluefruit.Scanner.resume();
}
#endif
