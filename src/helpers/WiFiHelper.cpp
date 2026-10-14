#include "WiFiHelper.h"
#include "TagHelper.h"

#if defined(ESP32)

#include <WiFi.h>

static char _ssid[64] = "";
static char _passphrase[64] = "";

void WiFiHelperClass::updateSSID(const char *ssid) {
  strncpy(_ssid, ssid, sizeof(_ssid) - 1);
  _ssid[sizeof(_ssid) - 1] = '\0';
  tagHelper.setValue("wifi.ssid", _ssid);
  WiFi.disconnect();
  _maybeStartWiFi();
}

void WiFiHelperClass::updatePassphrase(const char *passphrase) {
  strncpy(_passphrase, passphrase, sizeof(_passphrase) - 1);
  _passphrase[sizeof(_passphrase) - 1] = '\0';
  tagHelper.setValue("wifi.passphrase", _passphrase);
  WiFi.disconnect();
  _maybeStartWiFi();
}

char *WiFiHelperClass::getConfiguredSSID() {
  if (_ssid[0] == '\0') {
    return "<not set>";
  }
  return _ssid;
}

char *WiFiHelperClass::getConfiguredPassphrase() {
  if (_passphrase[0] == '\0') {
    return "<not set>";
  }
  return _passphrase;
}

bool WiFiHelperClass::start() {
  strncpy(_ssid, tagHelper.getValue("wifi.ssid", ""), sizeof(_ssid) - 1);
  _ssid[sizeof(_ssid) - 1] = '\0';
  strncpy(_passphrase, tagHelper.getValue("wifi.passphrase", ""), sizeof(_passphrase) - 1);
  _passphrase[sizeof(_passphrase) - 1] = '\0';
  return _maybeStartWiFi();
}

bool WiFiHelperClass::_maybeStartWiFi() {
  if (_ssid[0] != '\0' && _passphrase[0] != '\0') {
    WiFi.begin(_ssid, _passphrase);
    WiFi.setAutoReconnect(true);
    return true;
  } else {
    WiFi.setAutoReconnect(false);
    return false;
  }
}

bool WiFiHelperClass::shouldInhibitSleep() {
  return (_ssid[0] != '\0' && _passphrase[0] != '\0');
}

void WiFiHelperClass::forget() {
  WiFi.disconnect();
  _ssid[0] = '\0';
  _passphrase[0] = '\0';
  tagHelper.forget("wifi.ssid");
  tagHelper.forget("wifi.passphrase");
}

WiFiClass WiFi;

#endif
