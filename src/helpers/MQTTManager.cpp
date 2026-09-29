#include "MqttManager.h"

//#include "helpers/Syslog.h"

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

char _topic_prefix[100] = "meshcore/";
void MqttManager::setTopicPrefix(const char *prefix) {
  strncpy(_topic_prefix, prefix, sizeof(_topic_prefix) - 1);
  _topic_prefix[sizeof(_topic_prefix) - 1] = '\0'; // Ensure null-termination
  //syslogDebug("MQTT topic prefix", _topic_prefix);
}

unsigned long last_mqtt_reconnect_attempt = 0;
void MqttManager::reconnect() {
  if (millis() - last_mqtt_reconnect_attempt > 10000) {
    last_mqtt_reconnect_attempt = millis();
    char lwt_topic[200];
    snprintf(lwt_topic, sizeof(lwt_topic), "%sstatus", _topic_prefix);
    if (_mqttClient.connect(_clientId, mqtt_username, mqtt_password, lwt_topic, 0, true, "offline")) {
      publish("status", "online");
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

char _publish_topic_buffer[200];
bool MqttManager::publish(const char *topic, const char *payload) {
  if (_mqttClient.connected()) {
    snprintf(_publish_topic_buffer, sizeof(_publish_topic_buffer), "%s%s", _topic_prefix, topic);
    return _mqttClient.publish(_publish_topic_buffer, payload);
  }
  return false;
}

void MqttManager::callback(char *topic, byte *payload, unsigned int length) {
  // Handle incoming MQTT messages and forward them into the MeshCore pipeline
}