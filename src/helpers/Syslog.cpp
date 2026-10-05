#include "Syslog.h"

#if defined(ESP32)
#include <WiFi.h>
#include <WiFiUdp.h>

WiFiUDP udpSyslog;

char syslogServer[64] = "";  // assuming a maximum length for the server hostname or IP
char syslogSource[64] = "";  // assuming a maximum length for the source hostname or IP

bool _doneBegin = false;
void syslogBegin(const char* server, const char* source) {
    // Silently fail if WiFi is not connected
    if (WiFi.status() != WL_CONNECTED) {
        return;
    }
    strncpy(syslogServer, server, sizeof(syslogServer) - 1);
    syslogServer[sizeof(syslogServer) - 1] = '\0';
    strncpy(syslogSource, source, sizeof(syslogSource) - 1);
    syslogSource[sizeof(syslogSource) - 1] = '\0';
    udpSyslog.begin(514);  // standard syslog port
    _doneBegin = true;
}

void syslogSsend(uint8_t facility, uint8_t severity, const char *tag, const char *message) {
  // Silently fail if WiFi is not connected or we didn't begin syslog
  if (WiFi.status() != WL_CONNECTED || !_doneBegin) {
      return;
  }
  // Calculate Syslog Priority (PRI)
  uint8_t pri = (facility * 8) + severity;

  // Format header: <PRI>HOSTNAME TAG: MESSAGE
  char buffer[256];
  snprintf(buffer, sizeof(buffer), "<%d>%s %s: %s", pri, syslogSource, tag, message);

  // Send UDP Packet
  udpSyslog.beginPacket(syslogServer, 514);  // standard syslog port
  udpSyslog.write((const uint8_t *)buffer, strlen(buffer));
  udpSyslog.endPacket();
}

void syslogDebug(const char *tag, const char *message) {
    syslogSsend(1, 7, tag, message);  // facility=1 (user-level), severity=7 (debug)
}

#endif

