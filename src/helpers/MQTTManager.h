#pragma once

#include <PubSubClient.h>
#include <WiFi.h>

class MqttManager {
public:
  MqttManager();
  void setCredentials(const char *username, const char *password);
  void setTopicPrefix(const char *prefix);
  void begin(const char *server, uint16_t port, const char *clientId);
  void update(); // Call in loop()
  bool publish(const char *topic, const char *payload);
  void subscribe(const char *topic);

private:
  WiFiClient _wifiClient;
  PubSubClient _mqttClient;
  const char *_server;
  uint16_t _port;
  const char *_clientId;

  void reconnect();
  static void callback(char *topic, byte *payload, unsigned int length);
};