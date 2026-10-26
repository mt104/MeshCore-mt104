#include <Arduino.h>

#if defined(ESP32)
#include <BLEAdvertising.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#endif

#if defined(NRF52_PLATFORM)
#include <bluefruit.h>
#endif

class BTHomeWatcher {
public:
  void begin();
  void end();
//  static void parseBTHomePayload(const uint8_t *payload, uint16_t length);
  int getDebugFuncLen();
  static void scan_callback(ble_gap_evt_adv_report_t *report);
};
