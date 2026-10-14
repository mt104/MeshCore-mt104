#ifdef ESP32

#include <stdint.h>

void syslogBegin();
bool syslogIsRunning();
void syslogSsend(uint8_t facility, uint8_t severity, const char *tag, const char *message);
void syslogDebug(const char *tag, const char *message);

#endif