#include "MqttManager.h"

MqttManager::MqttManager() : _mqttClient(_wifiClient) {}

void MqttManager::begin(const char *server, uint16_t port, const char *clientId) {
  _server = server;
  _port = port;
  _clientId = clientId;
  _mqttClient.setServer(_server, _port);
  _mqttClient.setCallback(MqttManager::callback);
}

char *mqtt_username = nullptr;
char *mqtt_password = nullptr;
void MqttManager::setCredentials(const char *username, const char *password) {
  mqtt_username = strdup(username);
  mqtt_password = strdup(password);
}

unsigned long last_mqtt_reconnect_attempt = 0;
void MqttManager::reconnect() {
  if (millis() - last_mqtt_reconnect_attempt > 10000) {
    last_mqtt_reconnect_attempt = millis();
    if (_mqttClient.connect(_clientId, mqtt_username, mqtt_password)) {
      _mqttClient.publish("meshcore/status", "online");
      // Subscribe to mesh rx/tx topics upon connection
      ////_mqttClient.subscribe("meshcore/inbound/#");
    }
  }
}

void MqttManager::update() {
  if (!_mqttClient.connected()) {
    reconnect();
  }
  _mqttClient.loop();
}

bool MqttManager::publish(const char *topic, const char *payload) {
  if (_mqttClient.connected()) {
    return _mqttClient.publish(topic, payload);
  }
  return false;
}

void MqttManager::callback(char *topic, byte *payload, unsigned int length) {
  // Handle incoming MQTT messages and forward them into the MeshCore pipeline
}