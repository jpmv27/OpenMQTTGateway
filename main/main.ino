/*
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation

   Act as a gateway between your 433mhz, infrared IR, BLE, LoRa signal and one interface like an MQTT broker
   Send and receiving command by MQTT

  This program enables to:
 - receive MQTT data from a topic and send signal (RF, IR, BLE, GSM)  corresponding to the received MQTT data
 - publish MQTT data to a different topic related to received signals (RF, IR, BLE, GSM)

  Copyright: (c)Florian ROBERT

    This file is part of OpenMQTTGateway.

    OpenMQTTGateway is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenMQTTGateway is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "User_config.h"

enum GatewayState {
  WAITING_ONBOARDING,
  ONBOARDING,
  OFFLINE,
  NTWK_CONNECTED,
  BROKER_CONNECTED,
  PROCESSING,
  NTWK_DISCONNECTED,
  BROKER_DISCONNECTED,
  LOCAL_OTA_IN_PROGRESS,
  REMOTE_OTA_IN_PROGRESS,
  SLEEPING,
  ERROR
};
GatewayState gatewayState = GatewayState::WAITING_ONBOARDING;

// Macros and structure to enable the duplicates removing on the following gateways
#if defined(OMG_GATEWAY_RF) || defined(OMG_GATEWAY_IR) || defined(OMG_GATEWAY_SRFB) || defined(OMG_GATEWAY_WEATHERSTATION) || defined(OMG_GATEWAY_RTL_433)
// array to store previous received RFs, IRs codes and their timestamps
struct ReceivedSignal {
  uint64_t value;
  uint32_t time;
};

ReceivedSignal receivedSignal[] = {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}};

#  define struct_size (sizeof(receivedSignal) / sizeof(ReceivedSignal))
#endif

//Time used to wait for an interval before measures
unsigned long timer_sys_measures = 0;

// Time used to wait before system checkings
unsigned long timer_sys_checks = 0;

// First Start of offline mode modules
bool firstStart = true;

#define ARDUINOJSON_USE_LONG_LONG     1
#define ARDUINOJSON_ENABLE_STD_STRING 1

#include <queue>
int queueLength = 0;
unsigned long queueLengthSum = 0;
unsigned long blockedMessages = 0;
unsigned long receivedMessages = 0;
int maxQueueLength = 0;
#ifndef QueueSize
#  define QueueSize 18
#endif

/**
 * Deep-sleep for the ESP8266 & ESP32 we need some form of indicator that we have posted the measurements and am ready to deep sleep.
 * Set this to true in the sensor code after publishing the measurement.
 */
bool ready_to_sleep = false;

#include <ArduinoJson.h>
#include <Elog.h>
#include <PicoMQTT.h>

#include <memory>

#include "LEDManager.h"
#include "TheengsUtils.h"

LEDManager ledManager;

struct JsonBundle {
  StaticJsonDocument<JSON_MSG_BUFFER> doc;
};

std::queue<std::string> jsonQueue;

#ifdef ESP32
#  include <driver/adc.h>
// Mutex  to protect the queue
SemaphoreHandle_t xQueueMutex;
// Mutex to protect mqtt publish
SemaphoreHandle_t xMqttMutex;
#endif

StaticJsonDocument<JSON_MSG_BUFFER> modulesBuffer;
JsonArray modules = modulesBuffer.to<JsonArray>();
bool ethConnected = false;

#ifndef OMG_GATEWAY_GFSUNINVERTER
// Arduino IDE compiles, it automatically creates all the header declarations for all the functions you have in your *.ino file.
// Unfortunately it ignores #if directives.
// This is a simple workaround for this problem.
struct GfSun2000Data {};
#endif

// Modules config inclusion
#if defined(ZwebUI) && defined(ESP32)
#  include "config_WebUI.h"
#endif
#if defined(OMG_GATEWAY_RF) || defined(OMG_GATEWAY_RF2) || defined(OMG_GATEWAY_PILIGHT) || defined(OMG_ACTUATOR_SOMFY) || defined(OMG_GATEWAY_RTL_433)
#  include "config_RF.h"
#endif
#ifdef OMG_GATEWAY_WEATHERSTATION
#  include "config_WeatherStation.h"
#endif
#ifdef OMG_GATEWAY_GFSUNINVERTER
#  include "config_GFSunInverter.h"
#endif
#ifdef OMG_GATEWAY_LORA
#  include "config_LORA.h"
#endif
#ifdef OMG_GATEWAY_SRFB
#  include "config_SRFB.h"
#endif
#ifdef OMG_GATEWAY_BT
#  include "config_BT.h"
#endif
#ifdef OMG_GATEWAY_IR
#  include "config_IR.h"
#endif
#ifdef OMG_GATEWAY_2G
#  include "config_2G.h"
#endif
#ifdef OMG_ACTUATOR_ONOFF
#  include "config_ONOFF.h"
#endif
#ifdef OMG_SENSOR_INA226
#  include "config_INA226.h"
#endif
#ifdef OMG_SENSOR_HCSR501
#  include "config_HCSR501.h"
#endif
#ifdef OMG_SENSOR_ADC
#  include "config_ADC.h"
#endif
#ifdef OMG_SENSOR_BH1750
#  include "config_BH1750.h"
#endif
#ifdef OMG_SENSOR_MQ2
#  include "config_MQ2.h"
#endif
#ifdef OMG_SENSOR_TEMT6000
#  include "config_TEMT6000.h"
#endif
#ifdef OMG_SENSOR_TSL2561
#  include "config_TSL2561.h"
#endif
#ifdef OMG_SENSOR_BME280
#  include "config_BME280.h"
#endif
#ifdef OMG_SENSOR_HTU21
#  include "config_HTU21.h"
#endif
#ifdef OMG_SENSOR_LM75
#  include "config_LM75.h"
#endif
#ifdef OMG_SENSOR_AHTX0
#  include "config_AHTx0.h"
#endif
#ifdef OMG_SENSOR_RN8209
#  include "config_RN8209.h"
#endif
#ifdef OMG_SENSOR_HCSR04
#  include "config_HCSR04.h"
#endif
#ifdef OMG_SENSOR_C37_YL83_HMRD
#  include "config_C37_YL83_HMRD.h"
#endif
#ifdef OMG_SENSOR_DHT
#  include "config_DHT.h"
#endif
#ifdef OMG_SENSOR_SHTC3
#  include "config_SHTC3.h"
#endif
#ifdef OMG_SENSOR_DS1820
#  include "config_DS1820.h"
#endif
#ifdef OMG_GATEWAY_RFM69
#  include "config_RFM69.h"
#endif
#ifdef OMG_SENSOR_GPIOINPUT
#  include "config_GPIOInput.h"
#endif
#ifdef OMG_SENSOR_GPIOKEYCODE
#  include "config_GPIOKeyCode.h"
#endif
#ifdef OMG_SENSOR_TOUCH
#  include "config_Touch.h"
#endif
#ifdef OMG_MQTT_DISCOVERY
#  include "config_mqttDiscovery.h"
#endif
#ifdef OMG_ACTUATOR_FASTLED
#  include "config_FASTLED.h"
#endif
#ifdef OMG_ACTUATOR_PWM
#  include "config_PWM.h"
#endif
#ifdef OMG_ACTUATOR_SOMFY
#  include "config_Somfy.h"
#endif
#if defined(OMG_BOARD_M5STICKC) || defined(OMG_BOARD_M5STICKCP) || defined(OMG_BOARD_M5STACK) || defined(OMG_BOARD_M5TOUGH)
#  include "config_M5.h"
#endif
#if defined(OMG_DISPLAY_SSD1306)
#  include "config_SSD1306.h"
#endif
#if defined(OMG_GATEWAY_SERIAL)
#  include "config_SERIAL.h"
#endif
/*------------------------------------------------------------------------*/

void setupTLS(int index = CNT_DEFAULT_INDEX);

char g_ota_pass[parameters_size] = OMG_GW_PASSWORD;
#  define MAC_NAME_MAX_LEN 30
char g_WifiManager_ssid[MAC_NAME_MAX_LEN] = OMG_WIFIMANAGER_SSID;
int failure_number_ntwk = 0; // number of failure connecting to network
int failure_number_mqtt = 0; // number of failure connecting to MQTT

static unsigned long last_ota_activity_millis = 0;
// Global struct to store live SYS configuration data
SYSConfig_s SYSConfig;

bool failSafeMode = false;
bool ProcessLock = true; // Process lock when we want to use a critical function like OTA for example
static bool mqttSetupPending = true;
static int cnt_index = CNT_DEFAULT_INDEX;

#ifdef ESP32
#  include <ArduinoOTA.h>
#  include <FS.h>
#  include <SPIFFS.h>
#  include <esp_task_wdt.h>
#  include <nvs.h>
#  include <nvs_flash.h>

bool BTProcessLock = true; // Process lock when we want to use a critical function like OTA for example, at start to true so as to wait for critical functions to be performed before BLE start

#  if !defined(NO_INT_TEMP_READING)
// ESP32 internal temperature reading
#    include <stdio.h>

#    include "rom/ets_sys.h"
#    include "soc/rtc_cntl_reg.h"
#    include "soc/sens_reg.h"
#  endif
#  ifdef ESP32_ETHERNET
#    include <ETH.h>
void WiFiEvent(WiFiEvent_t event);
#  endif

#  include <WiFiClientSecure.h>
#  include <WiFiMulti.h>
WiFiMulti wifiMulti;
#  include <WiFiManager.h>
#  ifdef MDNS_SD
#    include <ESPmDNS.h>
#  endif

#elif defined(ESP8266)
#  include <ArduinoOTA.h>
#  include <DNSServer.h>
#  include <ESP8266WebServer.h>
#  include <ESP8266WiFi.h>
#  include <ESP8266WiFiMulti.h>
#  include <FS.h>
#  include <WiFiManager.h>
X509List caCert;
#  if OMG_MQTT_SECURE_SIGNED_CLIENT
X509List* pClCert = nullptr;
PrivateKey* pClKey = nullptr;
#  endif
ESP8266WiFiMulti wifiMulti;
#  ifdef MDNS_SD
#    include <ESP8266mDNS.h>
#  endif

#else
#  include <Ethernet.h>
#endif

void handle_autodiscovery() {
#ifdef OMG_MQTT_DISCOVERY
  static bool connectedOnce = false;
  const unsigned long now = millis();

  // at first connection we publish the discovery payloads
  // or, when we have just re-connected (only when discovery_republish_on_reconnect is enabled)
  const bool publishDiscovery = SYSConfig.discovery && (!connectedOnce || discovery_republish_on_reconnect);

  if (publishDiscovery) {
    pubMqttDiscovery();
#  ifdef OMG_GATEWAY_LORA
    launchLORADiscovery(true);
#  endif
#  ifdef OMG_GATEWAY_BT
    launchBTDiscovery(true);
#  endif
#  ifdef OMG_GATEWAY_RTL_433
    launchRTL_433Discovery(true);
#  endif
  }

  connectedOnce = true;
#endif
}

#if OMG_MQTT_BROKER_MODE

class MQTTServer : public PicoMQTT::Server {
public:
  size_t get_client_count() const {
    return clients.size();
  }

  bool connected() const {
    return !clients.empty();
  }

protected:
  virtual void on_subscribe(const char* client_id, const char* topic) override {
    // Whenever a client subscribes successfully to some topic, see if this is likely a subscription to a
    // autodiscovery topic.  If it is, fire handle_autodiscovery().
    const String pattern(topic);
    const bool is_autodiscovery_subscription = (pattern == "#") || (pattern.startsWith(String(discovery_prefix) + "/"));
    if (is_autodiscovery_subscription)
      handle_autodiscovery();
  }
};

std::unique_ptr<MQTTServer> mqtt;
#else
std::unique_ptr< ::Client> eClient;
std::unique_ptr<PicoMQTT::Client> mqtt;
#endif

template <typename T> // Declared here to avoid pre-compilation issue (missing "template" in auto declaration by pio)
void Config_update(JsonObject& data, const char* key, T& var);
template <typename T>
void Config_update(JsonObject& data, const char* key, T& var) {
  if (data.containsKey(key)) {
    if (var != data[key].as<T>()) {
      var = data[key].as<T>();
      Logger.notice(OMG_LOGID, F("Config %s changed to: %T" CR), key, data[key].as<T>());
    } else {
      Logger.notice(OMG_LOGID, F("Config %s unchanged, currently: %T" CR), key, data[key].as<T>());
    }
  }
}

/*
 * Dispatch json messages towards the communication layer
 *
*/
bool jsonDispatch(JsonObject& data) {
  bool res = false;
  if (data.containsKey("origin") || data.containsKey("topic")) {
    GatewayState previousGatewayState = gatewayState;
    gatewayState = GatewayState::PROCESSING;
#if OMG_MQTT_MESSAGE_UTC_TIMESTAMP == true
    data["UTCtime"] = TheengsUtils::UTCtimestamp();
#endif
#if OMG_MQTT_MESSAGE_LOCAL_TIMESTAMP == true
    data["time"] = TheengsUtils::localtimestamp();
#endif
#if OMG_MQTT_MESSAGE_UNIX_TIMESTAMP == true
    data["unixtime"] = TheengsUtils::unixtimestamp();
#endif
    if (data.containsKey("origin")) {
      pubWebUI((char*)data["origin"].as<const char*>(), data);
    }
    if (SYSConfig.mqtt && !SYSConfig.offline) {
      res = pub(data);
    }
#ifdef OMG_GATEWAY_SERIAL
    if (SYSConfig.serial) {
      char jsonStr[JSON_MSG_BUFFER_MAX];
      serializeJson(data, jsonStr);
      receivingDATA("", jsonStr);
      res = true; // Return the state from receivingDATA
    }
#endif
    gatewayState = previousGatewayState; // restore the previous state
  } else {
    Logger.error(OMG_LOGID, F("No origin or topic in JSON filtered" CR));
    gatewayState = GatewayState::ERROR;
  }
  return res;
}

// Add a document to the queue
boolean enqueueJsonObject(const StaticJsonDocument<JSON_MSG_BUFFER>& jsonDoc, int timeout) {
  receivedMessages++;
  if (jsonDoc.size() == 0) {
    Logger.error(OMG_LOGID, F("Empty JSON, skipping" CR));
    gatewayState = GatewayState::ERROR;
    return true;
  }
  if (queueLength >= QueueSize) {
    Logger.warning(OMG_LOGID, F("%d Doc(s) in queue, doc blocked" CR), queueLength);
    blockedMessages++;
    return false;
  }
  Logger.debug(OMG_LOGID, F("Enqueue JSON" CR));
  std::string jsonString;
  serializeJson(jsonDoc, jsonString);
#ifdef ESP32
  // Semaphore check before enqueueing a document
  if (xSemaphoreTake(xQueueMutex, pdMS_TO_TICKS(timeout)) == pdFALSE) {
    Logger.error(OMG_LOGID, F("xQueueMutex not taken" CR));
    gatewayState = GatewayState::ERROR;
    blockedMessages++;
    return false;
  }
#endif
  jsonQueue.push(jsonString);
#ifdef ESP32
  xSemaphoreGive(xQueueMutex);
#endif
  Logger.debug(OMG_LOGID, F("Queue length: %d" CR), jsonQueue.size());
  return true;
}

// Semaphore check before enqueueing a document with default timeout QueueSemaphoreTimeOutLoop
bool enqueueJsonObject(const StaticJsonDocument<JSON_MSG_BUFFER>& jsonDoc) {
  return enqueueJsonObject(jsonDoc, QueueSemaphoreTimeOutLoop);
}

#ifdef ESP32
#  include "mbedtls/sha256.h"

std::string generateHash(const std::string& input) {
  unsigned char hash[32];
  mbedtls_sha256((unsigned char*)input.c_str(), input.length(), hash, 0);

  char hashString[65]; // Room for null terminator
  for (int i = 0; i < 32; ++i) {
    sprintf(&hashString[i * 2], "%02x", hash[i]);
  }

  return std::string(hashString);
}
#else
std::string generateHash(const std::string& input) {
  return "Not implemented for ESP8266";
}
#endif

/*
 * Add the jsonObject id as a topic to the jsonObject origin
 *
*/
void buildTopicFromId(JsonObject& Jsondata, const char* origin) {
  if (!Jsondata.containsKey("id")) {
    Logger.error(OMG_LOGID, F("No id in Jsondata" CR));
    gatewayState = GatewayState::ERROR;
    return;
  }

  std::string topic = Jsondata["id"].as<std::string>();

  // Replace ":" in topic
  size_t pos = topic.find(":");
  while (pos != std::string::npos) {
    topic.erase(pos, 1);
    pos = topic.find(":", pos);
  }
#ifdef OMG_GATEWAY_BT
  if (BTConfig.pubBeaconUuidForTopic && !BTConfig.extDecoderEnable && Jsondata.containsKey("model_id") && Jsondata["model_id"].as<std::string>() == "IBEACON") {
    if (Jsondata.containsKey("uuid")) {
      topic = Jsondata["uuid"].as<std::string>();
    } else {
      Logger.error(OMG_LOGID, F("No uuid in Jsondata" CR));
      gatewayState = GatewayState::ERROR;
    }
  }

  if (BTConfig.extDecoderEnable && !Jsondata.containsKey("model"))
    topic = BTConfig.extDecoderTopic.c_str();
#endif
  std::string subjectStr(origin);
  topic = subjectStr + "/" + topic;

  Jsondata["origin"] = topic;

  Logger.debug(OMG_LOGID, F("Origin: %s" CR), Jsondata["origin"].as<const char*>());
}

// Empty the documents queue
void emptyQueue() {
  queueLength = jsonQueue.size();
  if (queueLength > maxQueueLength) {
    maxQueueLength = queueLength;
  }
  if (queueLength == 0) {
    return;
  }
  Logger.debug(OMG_LOGID, F("Dequeue JSON" CR));
  DynamicJsonDocument jsonBuffer(JSON_MSG_BUFFER);
  JsonObject obj = jsonBuffer.to<JsonObject>();
#ifdef ESP32
  if (xSemaphoreTake(xQueueMutex, pdMS_TO_TICKS(QueueSemaphoreTimeOutTask)) == pdFALSE) {
    Logger.error(OMG_LOGID, F("xQueueMutex not taken" CR));
    gatewayState = GatewayState::ERROR;
    return;
  }
#endif
  auto error = deserializeJson(jsonBuffer, jsonQueue.front());
  jsonQueue.pop();
#ifdef ESP32
  xSemaphoreGive(xQueueMutex);
#endif
  if (error) {
    Logger.error(OMG_LOGID, F("deserialize jsonQueue.front() failed: %s, buffer capacity: %u" CR), error.c_str(), jsonBuffer.capacity());
    gatewayState = GatewayState::ERROR;
  } else {
    if (jsonDispatch(obj))
      queueLengthSum++;
  }
}

/**
 * @brief Publish the payload on default MQTT topic.
 *
 * @param topicori suffix to add on default MQTT Topic
 * @param payload  the message to sends
 * @param retainFlag true if you what a retain
 */
bool pub(const char* topicori, const char* payload, bool retainFlag) {
  String topic = String(mqtt_topic) + String(g_gateway_name) + String(topicori);
  return pubMQTT(topic.c_str(), payload, retainFlag);
}

/**
 * @brief Publish the payload on default MQTT topic
 *
 * @param topicori suffix to add on default MQTT Topic
 * @param data The Json Object that represents the message
 */
bool pub(JsonObject& data) {
  bool res = false;
  bool ret = sensor_Retain;
  if (data.containsKey("retain") && data["retain"].is<bool>()) {
    ret = data["retain"];
    data.remove("retain");
  }
  if (data.size() == 0) {
    Logger.error(OMG_LOGID, F("Empty JSON, not published" CR));
    gatewayState = GatewayState::ERROR;
    return res;
  }
  String topic;
  if (data.containsKey("origin") && data["origin"].is<const char*>()) {
    topic = String(mqtt_topic) + String(g_gateway_name) + String(data["origin"].as<const char*>());
    data.remove("origin");
  } else if (data.containsKey("topic") && data["topic"].is<const char*>()) {
    topic = data["topic"].as<const char*>();
    data.remove("topic");
  } else {
    Logger.error(OMG_LOGID, F("No topic or origin in JSON, not published" CR));
    gatewayState = GatewayState::ERROR;
    return res;
  }

#if OMG_MQTT_VALUE_AS_A_TOPIC
#  ifdef OMG_GATEWAY_PILIGHT
  String value = data["value"];
  String protocol = data["protocol"];
  if (value != "null" && value != 0) {
    topic = topic + "/" + protocol + "/" + value;
  }
#  else
  uint64_t value = data["value"];
  if (value != 0) {
    topic = topic + "/" + TheengsUtils::toString(value);
  }
#  endif
#endif

#if OMG_MQTT_JSON_PUBLISHING
  String dataAsString = "";
  serializeJson(data, dataAsString);
  res = pubMQTT(topic.c_str(), dataAsString.c_str(), ret);
#endif

#if OMG_MQTT_SIMPLE_PUBLISHING
  Logger.debug(OMG_LOGID, F("simplePub - ON" CR));
  // Loop through all the key-value pairs in obj
  for (JsonPair p : data) {
#  if defined(ESP8266)
    yield();
#  endif
    if (p.value().is<uint64_t>() && strcmp(p.key().c_str(), "rssi") != 0) { //test rssi , bypass solution due to the fact that a int is considered as an uint64_t
      if (strcmp(p.key().c_str(), "value") == 0) { // if data is a value we don't integrate the name into the topic
        res = pubMQTT(topic, p.value().as<uint64_t>());
      } else { // if data is not a value we integrate the name into the topic
        res = pubMQTT(topic + "/" + String(p.key().c_str()), p.value().as<uint64_t>());
      }
    } else if (p.value().is<int>()) {
      res = pubMQTT(topic + "/" + String(p.key().c_str()), p.value().as<int>());
    } else if (p.value().is<float>()) {
      res = pubMQTT(topic + "/" + String(p.key().c_str()), p.value().as<float>());
    } else if (p.value().is<char*>()) {
      res = pubMQTT(topic + "/" + String(p.key().c_str()), p.value().as<const char*>());
    }
  }
#endif
  return res;
}

/**
 * @brief Publish the payload on default MQTT topic
 *
 * @param topicori suffix to add on default MQTT Topic
 * @param payload the message to sends
 */
bool pub(const char* topicori, const char* payload) {
  String topic = String(mqtt_topic) + String(g_gateway_name) + String(topicori);
  return pubMQTT(topic, payload);
}

/**
 * @brief Low level MQTT functions without retain
 *
 * @param topic  the topic
 * @param payload  the payload
 */
bool pubMQTT(const char* topic, const char* payload) {
  return pubMQTT(topic, payload, sensor_Retain);
}

/**
 * @brief Very Low level MQTT functions with retain Flag
 *
 * @param topic the topic
 * @param payload the payload
 * @param retainFlag  true if retain the retain Flag
 */
bool pubMQTT(const char* topic, const char* payload, bool retainFlag) {
  bool res = false;
  if (SYSConfig.mqtt && !SYSConfig.offline) {
#ifdef ESP32
    if (xSemaphoreTake(xMqttMutex, pdMS_TO_TICKS(QueueSemaphoreTimeOutTask)) == pdFALSE) {
      Logger.error(OMG_LOGID, F("xMqttMutex not taken" CR));
      gatewayState = GatewayState::ERROR;
      return res;
    }
#endif
    if (mqtt && mqtt->connected()) {
      Logger.notice(OMG_LOGID, F("[ OMG->MQTT ] topic: %s msg: %s " CR), topic, payload);
      res = mqtt->publish(topic, payload, 0, retainFlag);
    } else {
      Logger.warning(OMG_LOGID, F("MQTT not connected, aborting the publication" CR));
    }
#ifdef ESP32
    xSemaphoreGive(xMqttMutex);
#endif
  } else {
    Logger.notice(OMG_LOGID, F("[ OMG->MQTT deactivated or offline] topic: %s msg: %s " CR), topic, payload);
  }
  return res;
}

bool pubMQTT(String topic, const char* payload) {
  return pubMQTT(topic.c_str(), payload);
}

bool pubMQTT(const char* topic, unsigned long payload) {
  char val[11];
  sprintf(val, "%lu", payload);
  return pubMQTT(topic, val);
}

bool pubMQTT(const char* topic, unsigned long long payload) {
  char val[21];
  sprintf(val, "%llu", payload);
  return pubMQTT(topic, val);
}

bool pubMQTT(const char* topic, String payload) {
  return pubMQTT(topic, payload.c_str());
}

bool pubMQTT(String topic, String payload) {
  return pubMQTT(topic.c_str(), payload.c_str());
}

bool pubMQTT(String topic, int payload) {
  char val[12];
  sprintf(val, "%d", payload);
  return pubMQTT(topic.c_str(), val);
}

bool pubMQTT(String topic, unsigned long long payload) {
  char val[21];
  sprintf(val, "%llu", payload);
  return pubMQTT(topic.c_str(), val);
}

bool pubMQTT(String topic, float payload) {
  char val[12];
  dtostrf(payload, 3, 1, val);
  return pubMQTT(topic.c_str(), val);
}

bool pubMQTT(const char* topic, float payload) {
  char val[12];
  dtostrf(payload, 3, 1, val);
  return pubMQTT(topic, val);
}

bool pubMQTT(const char* topic, int payload) {
  char val[12];
  sprintf(val, "%d", payload);
  return pubMQTT(topic, val);
}

bool pubMQTT(const char* topic, unsigned int payload) {
  char val[12];
  sprintf(val, "%u", payload);
  return pubMQTT(topic, val);
}

bool pubMQTT(const char* topic, long payload) {
  char val[11];
  sprintf(val, "%ld", payload);
  return pubMQTT(topic, val);
}

bool pubMQTT(const char* topic, double payload) {
  char val[16];
  sprintf(val, "%f", payload);
  return pubMQTT(topic, val);
}

bool pubMQTT(String topic, unsigned long payload) {
  char val[11];
  sprintf(val, "%lu", payload);
  return pubMQTT(topic.c_str(), val);
}

void delayWithOTA(long waitMillis) {
  long waitStep = 100;
  for (long waitedMillis = 0; waitedMillis < waitMillis; waitedMillis += waitStep) {
#ifndef ESPWifiManualSetup
    checkButton(); // check if a reset of wifi/mqtt settings is asked
#endif
    ArduinoOTA.handle();
#if defined(ZwebUI) && defined(ESP32)
    WebUILoop();
#endif
#ifdef ESP32
    //esp_task_wdt_reset();
#endif
    delay(waitStep);
  }
}

void SYSConfig_init() {
  SYSConfig.mqtt = DEFAULT_MQTT;
  SYSConfig.serial = DEFAULT_SERIAL;
  SYSConfig.offline = DEFAULT_OFFLINE;
#if USE_BLUFI
  SYSConfig.blufi = DEFAULT_BLUFI;
#endif
#ifdef OMG_MQTT_DISCOVERY
  SYSConfig.discovery = DEFAULT_DISCOVERY;
  SYSConfig.ohdiscovery = OpenHABDiscovery;
#endif
#ifdef LED_ADDRESSABLE
  SYSConfig.rgbbrightness = DEFAULT_ADJ_BRIGHTNESS;
#endif
  SYSConfig.powerMode = DEFAULT_LOW_POWER_MODE;
}

void SYSConfig_fromJson(JsonObject& SYSdata) {
  Config_update(SYSdata, "mqtt", SYSConfig.mqtt);
  Config_update(SYSdata, "serial", SYSConfig.serial);
  Config_update(SYSdata, "offline", SYSConfig.offline);
#if USE_BLUFI
  Config_update(SYSdata, "blufi", SYSConfig.blufi);
#endif
#ifdef OMG_MQTT_DISCOVERY
  Config_update(SYSdata, "disc", SYSConfig.discovery);
  Config_update(SYSdata, "ohdisc", SYSConfig.ohdiscovery);
#endif
#ifdef LED_ADDRESSABLE
  Config_update(SYSdata, "rgbb", SYSConfig.rgbbrightness);
#endif
  Config_update(SYSdata, "powermode", SYSConfig.powerMode);
}

#ifdef ESP32
void SYSConfig_save() {
  StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject SYSdata = jsonBuffer.to<JsonObject>();
  SYSdata["mqtt"] = SYSConfig.mqtt;
  SYSdata["serial"] = SYSConfig.serial;
  SYSdata["offline"] = SYSConfig.offline;
  SYSdata["powermode"] = SYSConfig.powerMode;
#  if USE_BLUFI
  SYSdata["blufi"] = SYSConfig.blufi;
#  endif
#  ifdef OMG_MQTT_DISCOVERY
  SYSdata["disc"] = SYSConfig.discovery;
  SYSdata["ohdisc"] = SYSConfig.ohdiscovery;
#  endif
#  ifdef LED_ADDRESSABLE
  SYSdata["rgbb"] = SYSConfig.rgbbrightness;
#  endif
  String conf = "";
  serializeJson(jsonBuffer, conf);
  preferences.begin(Gateway_Short_Name, false);
  int result = preferences.putString("SYSConfig", conf);
  preferences.end();
  Logger.notice(OMG_LOGID, F("SYS Config_save: %s, result: %d" CR), conf.c_str(), result);
}
#else // Function not available for ESP8266
void SYSConfig_save() {}
#endif

bool cmpToMainTopic(const char* topicOri, const char* toAdd) {
  if (strcmp(topicOri, toAdd) == 0)
    return true;
  // Is string "<mqtt_topic><gateway_name><toAdd>" equal to "<topicOri>"?
  // Compare first part with first chunk
  if (strncmp(topicOri, mqtt_topic, strlen(mqtt_topic)) != 0)
    return false;
  // Move pointer of sizeof chunk
  topicOri += strlen(mqtt_topic);
  // And so on...
  if (strncmp(topicOri, g_gateway_name, strlen(g_gateway_name)) != 0)
    return false;
  topicOri += strlen(g_gateway_name);
  if (strncmp(topicOri, toAdd, strlen(toAdd)) != 0)
    return false;
  return true;
}

#if defined(ESP32)
void SYSConfig_load() {
  StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
  preferences.begin(Gateway_Short_Name, true);
  if (preferences.isKey("SYSConfig")) {
    auto error = deserializeJson(jsonBuffer, preferences.getString("SYSConfig", "{}"));
    preferences.end();
    if (error) {
      Logger.error(OMG_LOGID, F("SYS config deserialization failed: %s, buffer capacity: %u" CR), error.c_str(), jsonBuffer.capacity());
      gatewayState = GatewayState::ERROR;
      return;
    }
    if (jsonBuffer.isNull()) {
      Logger.warning(OMG_LOGID, F("SYS config is null" CR));
      return;
    }
    JsonObject jo = jsonBuffer.as<JsonObject>();
    SYSConfig_fromJson(jo);
    Logger.notice(OMG_LOGID, F("SYS config loaded" CR));
  } else {
    preferences.end();
    Logger.notice(OMG_LOGID, F("SYS config not found" CR));
  }
}
#else // Function not available for ESP8266
void SYSConfig_load() {}
#endif

#if defined(MDNS_SD)
std::pair<String, uint16_t> discoverMQTTbroker() {
  Logger.debug(OMG_LOGID, F("Browsing for MQTT service" CR));
  int n = MDNS.queryService("mqtt", "tcp");
  if (n == 0) {
    Logger.warning(OMG_LOGID, F("no services found" CR));
  } else {
    Logger.debug(OMG_LOGID, F("%d service(s) found" CR), n);
    for (int i = 0; i < n; ++i) {
      Logger.debug(OMG_LOGID, F("Service %d %s found" CR), i, MDNS.hostname(i).c_str());
      Logger.debug(OMG_LOGID, F("IP %s Port %d" CR), MDNS.IP(i).toString().c_str(), MDNS.port(i));
    }
    if (n == 1) {
      Logger.debug(OMG_LOGID, F("One MQTT server found setting parameters" CR));
      return {MDNS.IP(0).toString(), uint16_t(MDNS.port(0))};
    } else {
      Logger.warning(OMG_LOGID, F("Several MQTT servers found, please deactivate mDNS and set your default server" CR));
    }
  }
  return {"", 0};
}
#endif

#if OMG_MQTT_BROKER_MODE
void setupMQTT() {
  Logger.notice(OMG_LOGID, F("Reconfiguring MQTT broker..." CR));

  mqtt.reset(new MQTTServer());
  mqtt->begin();
}
#else

struct ss_cnt_parameters_backup {
  ss_cnt_parameters parameters;
  int cnt_index;
  bool saveOnSuccess;
};

static std::unique_ptr<ss_cnt_parameters_backup> cnt_parameters_backup;

void setupMQTT() {
  Logger.notice(OMG_LOGID, F("Reconfiguring MQTT client..." CR));

  const auto& parameters = cnt_parameters_array[cnt_index];

  // free the old MQTT client and underlying connection
  mqtt.reset();
  eClient.reset();
  failure_number_mqtt = 0;

  // configure socket
  if (parameters.isConnectionSecure) {
    eClient.reset(new WiFiClientSecure);
    if (parameters.isCertValidate) {
      setupTLS(cnt_index);
    } else {
      static_cast<WiFiClientSecure*>(eClient.get())->setInsecure();
    }
  } else {
    eClient.reset(new WiFiClient);
  }

#  if defined(MDNS_SD)
  Logger.debug(OMG_LOGID, F("Connecting to MQTT by mDNS without MQTT hostname" CR));
  const auto discovered_broker = discoverMQTTbroker();
  const auto broker_host = discovered_broker.first.c_str();
  const auto broker_port = discovered_broker.second;
#  else
  const auto broker_host = parameters.mqtt_server;
  const auto broker_port = String(parameters.mqtt_port).toInt();
#  endif

  Logger.debug(OMG_LOGID, F("Mqtt server: %s" CR), broker_host);
  Logger.debug(OMG_LOGID, F("Port: %u" CR), broker_port);

  mqtt.reset(new PicoMQTT::Client(*eClient, broker_host, broker_port, g_gateway_name,
                                  parameters.mqtt_user, parameters.mqtt_pass,
                                  0, // minimum reconnect attempt interval [ms]
                                  60 * 1000, // keep alive interval [ms]
                                  (GeneralTimeOut - 1) * 1000 // socket timeout [ms]
                                  ));

#  if AWS_IOT
  // AWS doesn't support will topic for the moment
  mqtt->will.topic = "";
  mqtt->will.payload = "";
  mqtt->will.qos = 0;
  mqtt->will.retain = false;
#  else
  mqtt->will.topic = String(mqtt_topic) + g_gateway_name + will_Topic;
  mqtt->will.payload = will_Message;
  mqtt->will.qos = will_QoS;
  mqtt->will.retain = will_Retain;
#  endif

  mqtt->connected_callback = [] {
#  if AWS_IOT
    {
      // Define a threshold for instability detection
      constexpr unsigned int reconnection_threshold = 5;
      constexpr unsigned long reconnection_window_millis = 120000; // Consider unstable if more than 5 reconnections in 2 minutes

      static unsigned int reconnection_count = 0;
      static unsigned long start_reconnection_window_millis = 0;

      const unsigned long current_millis = millis();
      if (start_reconnection_window_millis == 0 || current_millis - start_reconnection_window_millis > reconnection_window_millis) {
        // Reset the count and timestamp at the start of a new window
        reconnection_count = 0;
        start_reconnection_window_millis = current_millis;
      }
      reconnection_count++; // Increment reconnection count
      Logger.debug(OMG_LOGID, F("MQTT connection count: %d" CR), reconnection_count);
      if (reconnection_count > reconnection_threshold && SYSConfig.mqtt) {
        // Detected instability in MQTT connection
        Logger.warning(OMG_LOGID, F("MQTT connection instability detected: %d reconnections in the last %d seconds" CR), reconnection_count, reconnection_window_millis / 1000);
        // Stop xtoMQTT to see if it helps and still enables to receive data
        SYSConfig.mqtt = false;
      }
    }
#  endif
#  ifdef OMG_GATEWAY_BT
    BTProcessLock = !BTConfig.enabled; // Release BLE processes at start if enabled
#  endif
    ProcessLock = false; // Release the loop process
    displayPrint("MQTT connected");
    Logger.notice(OMG_LOGID, F("Connected to broker" CR));
    gatewayState = GatewayState::BROKER_CONNECTED;
    failure_number_mqtt = 0;
    // Once connected, publish an announcement...
    pub(will_Topic, Gateway_AnnouncementMsg, will_Retain);

    if (cnt_parameters_backup) {
      // this was the first attempt to connect to a new server and it succeeded
      Logger.notice(OMG_LOGID, F("MQTT connection parameters %d successful" CR), cnt_index);
      cnt_parameters_array[cnt_index].validConnection = true;
      readCntParameters(cnt_index);

#  ifndef ESPWifiManualSetup
      if (cnt_parameters_backup->saveOnSuccess) // Save the new parameters to the flash
        saveConfig();
#  endif

      cnt_parameters_backup.reset();
      ESPRestart(7);
    }
    handle_autodiscovery();
  };

  mqtt->connection_failure_callback = [] {
    if ((WiFi.status() != WL_CONNECTED && ethConnected == false) || SYSConfig.offline) {
      // No network connection or offline, ignore this failure
      return;
    }
    failure_number_mqtt++; // we count the failure
    gatewayState = GatewayState::BROKER_DISCONNECTED;
    Logger.warning(OMG_LOGID, F("failure_number_mqtt: %d" CR), failure_number_mqtt);

    const auto& parameters = cnt_parameters_array[cnt_index];

    if (parameters.isConnectionSecure) {
      WiFiClientSecure* client = static_cast<WiFiClientSecure*>(eClient.get());
#  if defined(ESP32)
      Logger.warning(OMG_LOGID, F("failed, ssl error code=%d" CR), client->lastError(nullptr, 0));
#  elif defined(ESP8266)
      Logger.warning(OMG_LOGID, F("failed, ssl error code=%d" CR), client->getLastSSLError());
#  endif
    }

    if (cnt_parameters_backup) {
      // this was the first attempt to connect to a new server and it failed, revert to old settings
      Logger.error(OMG_LOGID, F("MQTT connection failed, reverting to previous settings" CR));
      gatewayState = GatewayState::ERROR;
      cnt_parameters_array[cnt_index] = cnt_parameters_backup->parameters;
      cnt_index = cnt_parameters_backup->cnt_index;
      mqttSetupPending = true;
      cnt_parameters_backup.reset();
      ESPRestart(7);
      return;
    }

    delayWithOTA(10000);

    if (failure_number_mqtt > maxRetryWatchDog) {
#  ifndef ESPWifiManualSetup
      // Look for the next valid connection
      for (int i = 0; i < cnt_parameters_array_size; i++) {
        cnt_index++;
        if (cnt_index >= cnt_parameters_array_size) {
          cnt_index = 0;
        }
        if (cnt_parameters_array[cnt_index].validConnection) {
          Logger.notice(OMG_LOGID, F("Connection %d valid, switching" CR), cnt_index);
          saveConfig();
          break;
        } else {
          Logger.notice(OMG_LOGID, F("Connection %d not valid" CR), cnt_index);
        }
      }
#  endif
      unsigned long millis_since_last_ota;
      while (
          // When
          // ...incomplete OTA in progress
          (last_ota_activity_millis != 0)
          // ...AND last OTA activity fairly recently
          && ((millis_since_last_ota = millis() - last_ota_activity_millis) < ota_timeout_millis)) {
        // ... We consider that OTA might be still active, and we sleep for a while, and giving
        // OTA chance to proceed (ArduinoOTA.handle())
        Logger.warning(OMG_LOGID, F("OTA might be still active (activity %d ms ago)" CR), millis_since_last_ota);
        ArduinoOTA.handle();
        delay(100);
      }
      ESPRestart(1);
    }
  };

  mqtt->subscribe(String(mqtt_topic) + g_gateway_name + subjectMQTTtoX, receivingDATA, mqtt_max_payload_size);

#  ifdef OMG_GATEWAY_RF
  // subject on which other OMG will publish, this OMG will store these msg and by the way don't republish them if they have been already published
  mqtt->subscribe(subjectMultiGTWRF, receivingDATA, mqtt_max_payload_size);
#  endif

#  ifdef OMG_GATEWAY_IR
  // subject on which other OMG will publish, this OMG will store these msg and by the way don't republish them if they have been already published
  mqtt->subscribe(subjectMultiGTWIR, receivingDATA, mqtt_max_payload_size);
#  endif

  mqtt->begin();
}
#endif

#if defined(ESP32) && (defined(WifiGMode) || defined(WifiPower))
void setESPWifiProtocolTxPower() {
  //Reduce WiFi interference when using ESP32 using custom WiFi mode and tx power
  //https://github.com/espressif/arduino-esp32/search?q=WIFI_PROTOCOL_11G
  //https://www.letscontrolit.com/forum/viewtopic.php?t=671&start=20
#  if WifiGMode == true
  if (esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G) != ESP_OK) {
    Logger.error(OMG_LOGID, F("Failed to change WifiMode." CR));
    gatewayState = GatewayState::ERROR;
  }
#  endif

#  if WifiGMode == false
  if (esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N) != ESP_OK) {
    Logger.error(OMG_LOGID, F("Failed to change WifiMode." CR));
    gatewayState = GatewayState::ERROR;
  }
#  endif

  uint8_t getprotocol;
  esp_err_t err;
  err = esp_wifi_get_protocol(WIFI_IF_STA, &getprotocol);

  if (err != ESP_OK) {
    Logger.notice(OMG_LOGID, F("Could not get protocol!" CR));
  }
  if (getprotocol & WIFI_PROTOCOL_11N) {
    Logger.notice(OMG_LOGID, F("WiFi_Protocol_11n" CR));
  }
  if (getprotocol & WIFI_PROTOCOL_11G) {
    Logger.notice(OMG_LOGID, F("WiFi_Protocol_11g" CR));
  }
  if (getprotocol & WIFI_PROTOCOL_11B) {
    Logger.notice(OMG_LOGID, F("WiFi_Protocol_11b" CR));
  }

#  ifdef WifiPower
  Logger.notice(OMG_LOGID, F("Requested WiFi power level: %i" CR), WifiPower);
  WiFi.setTxPower(WifiPower);
#  endif
  Logger.notice(OMG_LOGID, F("Operating WiFi power level: %i" CR), WiFi.getTxPower());
}
#endif

#if defined(ESP8266) && (defined(WifiGMode) || defined(WifiPower))
void setESPWifiProtocolTxPower() {
#  if WifiGMode == true
  if (!wifi_set_phy_mode(PHY_MODE_11G)) {
    Logger.error(OMG_LOGID, F("Failed to change WifiMode." CR));
    gatewayState = GatewayState::ERROR;
  }
#  endif

#  if WifiGMode == false
  if (!wifi_set_phy_mode(PHY_MODE_11N)) {
    Logger.error(OMG_LOGID, F("Failed to change WifiMode." CR));
    gatewayState = GatewayState::ERROR;
  }
#  endif

  phy_mode_t getprotocol = wifi_get_phy_mode();
  if (getprotocol == PHY_MODE_11N) {
    Logger.notice(OMG_LOGID, F("WiFi_Protocol_11n" CR));
  }
  if (getprotocol == PHY_MODE_11G) {
    Logger.notice(OMG_LOGID, F("WiFi_Protocol_11g" CR));
  }
  if (getprotocol == PHY_MODE_11B) {
    Logger.notice(OMG_LOGID, F("WiFi_Protocol_11b" CR));
  }

#  ifdef WifiPower
  Logger.notice(OMG_LOGID, F("Requested WiFi power level: %i dBm" CR), WifiPower);

  int i_dBm = int(WifiPower * 4.0f);

  // i_dBm 82 == 20.5 dBm
  if (i_dBm > 82) {
    i_dBm = 82;
  } else if (i_dBm < 0) {
    i_dBm = 0;
  }

  system_phy_set_max_tpw((uint8_t)i_dBm);
#  endif
}
#endif

#ifdef ESP32
void updateAndHandleLEDsTask(void* pvParameters) {
  for (;;) {
    updateAndHandleLEDsTask();
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
#endif

void updateAndHandleLEDsTask() {
  static GatewayState previousGatewayState;
  if (previousGatewayState != gatewayState) {
#ifdef LED_POWER
    ledManager.setMode(STRIP_POWER, LED_POWER, LEDManager::STATIC, LED_POWER_COLOR, -1);
#endif
    switch (gatewayState) {
      case PROCESSING:
#ifdef LED_PROCESSING
        ledManager.setMode(STRIP_PROCESSING, LED_PROCESSING, LEDManager::BLINK, LED_PROCESSING_COLOR, 3);
#endif
        break;
      case WAITING_ONBOARDING:
#ifdef LED_BROKER
        ledManager.setMode(STRIP_BROKER, LED_BROKER, LEDManager::STATIC, LED_WAITING_ONBOARD_COLOR, -1);
#endif
        break;
      case ONBOARDING:
#ifdef LED_BROKER
        ledManager.setMode(STRIP_BROKER, LED_BROKER, LEDManager::STATIC, LED_ONBOARD_COLOR, -1);
#endif
        break;
      case NTWK_CONNECTED:
#ifdef LED_NETWORK
        ledManager.setMode(STRIP_NETWORK, LED_NETWORK, LEDManager::STATIC, LED_NETWORK_OK_COLOR, -1);
#endif
        break;
      case BROKER_CONNECTED:
#ifdef LED_BROKER
        ledManager.setMode(STRIP_BROKER, LED_BROKER, LEDManager::STATIC, LED_BROKER_OK_COLOR, -1);
#endif
        break;
      case NTWK_DISCONNECTED:
#ifdef LED_NETWORK
        ledManager.setMode(STRIP_NETWORK, LED_NETWORK, LEDManager::BLINK, LED_NETWORK_ERROR_COLOR, -1);
#endif
        break;
      case BROKER_DISCONNECTED:
#ifdef LED_BROKER
        ledManager.setMode(STRIP_BROKER, LED_BROKER, LEDManager::BLINK, LED_BROKER_ERROR_COLOR, -1);
#endif
        break;
      case SLEEPING:
        ledManager.setMode(-1, -1, LEDManager::OFF, 0, -1);
        break;
      case OFFLINE:
#ifdef LED_NETWORK
        ledManager.setMode(STRIP_NETWORK, LED_NETWORK, LEDManager::BLINK, LED_OFFLINE_COLOR, -1);
#endif
        break;
      case LOCAL_OTA_IN_PROGRESS:
#ifdef LED_PROCESSING
        ledManager.setMode(STRIP_PROCESSING, LED_PROCESSING, LEDManager::BLINK, LED_OTA_LOCAL_COLOR, -1);
#endif
        break;
      case REMOTE_OTA_IN_PROGRESS:
#ifdef LED_PROCESSING
        ledManager.setMode(STRIP_PROCESSING, LED_PROCESSING, LEDManager::BLINK, LED_OTA_REMOTE_COLOR, -1);
#endif
        break;
      case ERROR:
#ifdef LED_ERROR
        ledManager.setMode(STRIP_ERROR, LED_ERROR, LEDManager::BLINK, LED_ERROR_COLOR, 3);
#endif
        break;
      default:
        break;
    }
    previousGatewayState = gatewayState;
  }
  ledManager.update();
}

void setup() {
  //Launch serial for debugging purposes
  Serial.begin(SERIAL_BAUD);
  Logger.registerSerial(OMG_LOGID, OMG_LOG_LEVEL, "OMG");
#if OMG_LOG_TO_SYSLOG
  if (strcmp(g_syslog_server, "") != 0 && strcmp(g_syslog_port, "") != 0) {
    Logger.configureSyslog(g_syslog_server, String(g_syslog_port).toInt(), g_gateway_name);
    Logger.registerSyslog(OMG_LOGID, OMG_LOG_LEVEL_SYSLOG, OMG_SYSLOG_FACILITY, "OMG");
  } else {
    Logger.error(OMG_LOGID, F("Invalid syslog configuration, skipping registration" CR));
  }
#endif
  Logger.notice(OMG_LOGID, F(CR "************* WELCOME TO OpenMQTTGateway **************" CR));
#if defined(TRIGGER_GPIO) && !defined(ESPWifiManualSetup)
  pinMode(TRIGGER_GPIO, INPUT_PULLUP);
  checkButton();
#endif

  delay(100); //give time to start the flash and avoid issue when reading the preferences

  SYSConfig_init();
  SYSConfig_load();

  if (SYSConfig.offline) {
    gatewayState = GatewayState::OFFLINE;
  }

#ifdef LED_ADDRESSABLE
#  ifdef LED_ADDRESSABLE_PIN1
  ledManager.addLEDStrip(LED_ADDRESSABLE_PIN1, LED_ADDRESSABLE_NUM);
#  endif
#  ifdef LED_ADDRESSABLE_PIN2
  ledManager.addLEDStrip(LED_ADDRESSABLE_PIN2, LED_ADDRESSABLE_NUM);
#  endif
#  ifdef LED_ADDRESSABLE_PIN3
  ledManager.addLEDStrip(LED_ADDRESSABLE_PIN3, LED_ADDRESSABLE_NUM);
#  endif
  ledManager.setBrightness(SYSConfig.rgbbrightness);
#elif LED_PIN
  ledManager.addLEDStrip(LED_PIN, 1);
#endif

#ifdef ESP8266
#  ifndef OMG_GATEWAY_SRFB // if we are not in sonoff rf bridge case we apply the ESP8266 GPIO optimization
  Serial.end();
  Serial.begin(SERIAL_BAUD, SERIAL_8N1, SERIAL_TX_ONLY); // enable on ESP8266 to free some pin
#  endif
#elif ESP32
  xTaskCreate(updateAndHandleLEDsTask, "updateAndHandleLEDsTask", 2500, NULL, 1, NULL);
  xQueueMutex = xSemaphoreCreateMutex();
  xMqttMutex = xSemaphoreCreateMutex();
#  if defined(OMG_BOARD_M5STICKC) || defined(OMG_BOARD_M5STICKCP) || defined(OMG_BOARD_M5STACK) || defined(OMG_BOARD_M5TOUGH)
  setupM5();
#  endif
#  if defined(OMG_DISPLAY_SSD1306)
  setupSSD1306();
  modules.add(OMG_DISPLAY_SSD1306);
#  endif
#endif

  Logger.notice(OMG_LOGID, F("OpenMQTTGateway Version: " OMG_VERSION CR));

#ifdef ESP32_EXT0_WAKE_PIN
  Logger.notice(OMG_LOGID, F("Setting EXT0 Wakeup for deep sleep." CR));
  gpio_num_t wake_pin0 = static_cast<gpio_num_t>(ESP32_EXT0_WAKE_PIN);
  if (esp_sleep_enable_ext0_wakeup(wake_pin0, ESP32_EXT0_WAKE_PIN_STATE) != ESP_OK) {
    Logger.error(OMG_LOGID, F("Failed to set deep sleep EXT0 Wakeup." CR));
    gatewayState = GatewayState::ERROR;
  }
#endif
#ifdef ESP32_EXT1_WAKE_PIN
  Logger.notice(OMG_LOGID, F("Setting EXT1 Wakeup for deep sleep." CR));
  uint64_t wake_pin_bitmask = 1ULL << ESP32_EXT1_WAKE_PIN; // Adjust this line if multiple pins are used.
  esp_sleep_ext1_wakeup_mode_t wake_state1 = static_cast<esp_sleep_ext1_wakeup_mode_t>(ESP32_EXT1_WAKE_PIN_STATE);
  if (esp_sleep_enable_ext1_wakeup(wake_pin_bitmask, wake_state1) != ESP_OK) {
    Logger.error(OMG_LOGID, F("Failed to set deep sleep EXT1 Wakeup." CR));
    gatewayState = GatewayState::ERROR;
  }
#endif
/*
 Note that the ONOFF module need to start after the RN8209 so that the overCurrent function is launched after the setup of the sensor
*/
#ifdef OMG_SENSOR_RN8209
  setupRN8209();
  modules.add(OMG_SENSOR_RN8209);
#endif
#ifdef OMG_ACTUATOR_ONOFF
  setupONOFF();
  modules.add(OMG_ACTUATOR_ONOFF);
#endif
#ifdef OMG_GATEWAY_SERIAL
  setupSERIAL();
  modules.add(OMG_GATEWAY_SERIAL);
#endif

#if defined(ESP32) && defined(USE_BLUFI)
  if (SYSConfig.blufi)
    startBlufi();
#endif

#if defined(ESPWifiManualSetup)
  setupWiFiFromBuild();
#else
  if (loadConfigFromFlash()) { // Config present
    Logger.notice(OMG_LOGID, F("Config loaded from flash" CR));
#  ifdef ESP32_ETHERNET
    setup_ethernet_esp32();
#  endif
    // If not in failSafeMode and no connection to the network with Ethernet, launch the wifi manager
    if (!failSafeMode && !ethConnected) setupWiFiManager();
  } else { // No config in flash
#  ifdef ESP32_ETHERNET
    setup_ethernet_esp32();
#  endif

#  if SELF_TEST
    // Check serial input to trigger a Self Test sequence if required
    checkSerial();
#  endif

    Logger.notice(OMG_LOGID, F("No config in flash, launching wifi manager" CR));
    // In failSafeMode we don't want to setup wifi manager as it has already been done before
    if (!failSafeMode) setupWiFiManager();
  }

#endif
  Logger.debug(OMG_LOGID, F("OpenMQTTGateway mac: %s" CR), WiFi.macAddress().c_str());
  Logger.debug(OMG_LOGID, F("OpenMQTTGateway ip: %s" CR), WiFi.localIP().toString().c_str());
  Logger.debug(OMG_LOGID, F("OpenMQTTGateway index %d" CR), cnt_index);
  Logger.debug(OMG_LOGID, F("OpenMQTTGateway mqtt topic: %s" CR), mqtt_topic);
#ifdef OMG_MQTT_DISCOVERY
  Logger.debug(OMG_LOGID, F("OpenMQTTGateway mqtt discovery prefix: %s" CR), discovery_prefix);
#endif
  Logger.debug(OMG_LOGID, F("OpenMQTTGateway gateway name: %s" CR), g_gateway_name);
#if !OMG_MQTT_BROKER_MODE
  Logger.debug(OMG_LOGID, F("OpenMQTTGateway mqtt server: %s" CR), cnt_parameters_array[cnt_index].mqtt_server);
  Logger.debug(OMG_LOGID, F("OpenMQTTGateway mqtt port: %s" CR), cnt_parameters_array[cnt_index].mqtt_port);
  Logger.debug(OMG_LOGID, F("OpenMQTTGateway mqtt user: %s" CR), cnt_parameters_array[cnt_index].mqtt_user);
  Logger.debug(OMG_LOGID, F("OpenMQTTGateway secure connection: %s" CR), cnt_parameters_array[cnt_index].isConnectionSecure ? "true" : "false");
  Logger.debug(OMG_LOGID, F("OpenMQTTGateway validate cert: %s" CR), cnt_parameters_array[cnt_index].isCertValidate ? "true" : "false");
#endif

  setOTA();

#if defined(ZwebUI) && defined(ESP32)
  WebUISetup();
  modules.add(ZwebUI);
#endif

  delay(1500);
#if defined(OMG_GATEWAY_RF) || defined(OMG_GATEWAY_PILIGHT) || defined(OMG_GATEWAY_RTL_433) || defined(OMG_GATEWAY_RF2) || defined(OMG_ACTUATOR_SOMFY)
  setupCommonRF();
#endif
#ifdef OMG_SENSOR_BME280
  setupZsensorBME280();
  modules.add(OMG_SENSOR_BME280);
#endif
#ifdef OMG_SENSOR_HTU21
  setupZsensorHTU21();
  modules.add(OMG_SENSOR_HTU21);
#endif
#ifdef OMG_SENSOR_LM75
  setupZsensorLM75();
  modules.add(OMG_SENSOR_LM75);
#endif
#ifdef OMG_SENSOR_AHTX0
  setupZsensorAHTx0();
  modules.add(OMG_SENSOR_AHTX0);
#endif
#ifdef OMG_SENSOR_BH1750
  setupZsensorBH1750();
  modules.add(OMG_SENSOR_BH1750);
#endif
#ifdef OMG_SENSOR_MQ2
  setupZsensorMQ2();
  modules.add(OMG_SENSOR_MQ2);
#endif
#ifdef OMG_SENSOR_TEMT6000
  setupZsensorTEMT6000();
  modules.add(OMG_SENSOR_TEMT6000);
#endif
#ifdef OMG_SENSOR_TSL2561
  setupZsensorTSL2561();
  modules.add(OMG_SENSOR_TSL2561);
#endif
#ifdef OMG_GATEWAY_2G
  setup2G();
  modules.add(OMG_GATEWAY_2G);
#endif
#ifdef OMG_GATEWAY_IR
  setupIR();
  modules.add(OMG_GATEWAY_IR);
#endif
#ifdef OMG_GATEWAY_LORA
  setupLORA();
  modules.add(OMG_GATEWAY_LORA);
#endif
#ifdef OMG_GATEWAY_RF
  modules.add(OMG_GATEWAY_RF);
#  define ACTIVE_RECEIVER ACTIVE_RF
#endif
#ifdef OMG_GATEWAY_RF2
  modules.add(OMG_GATEWAY_RF2);
#  ifdef ACTIVE_RECEIVER
#    undef ACTIVE_RECEIVER
#  endif
#  define ACTIVE_RECEIVER ACTIVE_RF2
#endif
#ifdef OMG_GATEWAY_PILIGHT
  modules.add(OMG_GATEWAY_PILIGHT);
#  ifdef ACTIVE_RECEIVER
#    undef ACTIVE_RECEIVER
#  endif
#  define ACTIVE_RECEIVER ACTIVE_PILIGHT
#endif
#ifdef OMG_GATEWAY_WEATHERSTATION
  setupWeatherStation();
  modules.add(OMG_GATEWAY_WEATHERSTATION);
#endif
#ifdef OMG_GATEWAY_GFSUNINVERTER
  setupGFSunInverter();
  modules.add(OMG_GATEWAY_GFSUNINVERTER);
#endif
#ifdef OMG_GATEWAY_SRFB
  setupSRFB();
  modules.add(OMG_GATEWAY_SRFB);
#endif
#ifdef OMG_GATEWAY_BT
  setupBT();
  modules.add(OMG_GATEWAY_BT);
#endif
#ifdef OMG_GATEWAY_RFM69
  setupRFM69();
  modules.add(OMG_GATEWAY_RFM69);
#endif
#ifdef OMG_SENSOR_INA226
  setupINA226();
  modules.add(OMG_SENSOR_INA226);
#endif
#ifdef OMG_SENSOR_HCSR501
  setupHCSR501();
  modules.add(OMG_SENSOR_HCSR501);
#endif
#ifdef OMG_SENSOR_HCSR04
  setupHCSR04();
  modules.add(OMG_SENSOR_HCSR04);
#endif
#ifdef OMG_SENSOR_GPIOINPUT
  setupGPIOInput();
  modules.add(OMG_SENSOR_GPIOINPUT);
#endif
#ifdef OMG_SENSOR_GPIOKEYCODE
  setupGPIOKeyCode();
  modules.add(OMG_SENSOR_GPIOKEYCODE);
#endif
#ifdef OMG_ACTUATOR_FASTLED
  setupFASTLED();
  modules.add(OMG_ACTUATOR_FASTLED);
#endif
#ifdef OMG_ACTUATOR_PWM
  setupPWM();
  modules.add(OMG_ACTUATOR_PWM);
#endif
#ifdef OMG_ACTUATOR_SOMFY
#  ifdef ACTIVE_RECEIVER
#    undef ACTIVE_RECEIVER
#  endif
#  define ACTIVE_RECEIVER ACTIVE_NONE
  setupSomfy();
  modules.add(OMG_ACTUATOR_SOMFY);
#endif
#ifdef OMG_SENSOR_DS1820
  setupZsensorDS1820();
  modules.add(OMG_SENSOR_DS1820);
#endif
#ifdef OMG_SENSOR_ADC
  setupADC();
  modules.add(OMG_SENSOR_ADC);
#endif
#ifdef OMG_SENSOR_TOUCH
  setupTouch();
  modules.add(OMG_SENSOR_TOUCH);
#endif
#ifdef OMG_SENSOR_C37_YL83_HMRD
  setupZsensorC37_YL83_HMRD();
  modules.add(OMG_SENSOR_C37_YL83_HMRD);
#endif
#ifdef OMG_SENSOR_DHT
  setupDHT();
  modules.add(OMG_SENSOR_DHT);
#endif
#ifdef OMG_SENSOR_SHTC3
  setupSHTC3();
#endif
#ifdef OMG_GATEWAY_RTL_433
#  ifdef ACTIVE_RECEIVER
#    undef ACTIVE_RECEIVER
#  endif
#  define ACTIVE_RECEIVER ACTIVE_RTL
  setupRTL_433();
  modules.add(OMG_GATEWAY_RTL_433);
#endif
  Logger.debug(OMG_LOGID, F("mqtt_max_payload_size: %d" CR), mqtt_max_payload_size);
  SYSConfig.offline ? Logger.notice(OMG_LOGID, F("Offline enabled" CR)) : Logger.notice(0, F("Offline disabled" CR));
  char jsonChar[100];
  serializeJson(modules, jsonChar, measureJson(modules) + 1);
  Logger.notice(OMG_LOGID, F("OpenMQTTGateway modules: %s" CR), jsonChar);
  Logger.notice(OMG_LOGID, F("************** Setup OpenMQTTGateway end **************" CR));
}

// Bypass for ESP not reconnecting automaticaly the second time https://github.com/espressif/arduino-esp32/issues/2501
bool wifi_reconnect_bypass() {
#if defined(ESP32) && defined(USE_BLUFI)
  extern bool omg_blufi_ble_connected;
  if (omg_blufi_ble_connected) {
    Logger.notice(OMG_LOGID, F("BLUFI is connected, bypassing wifi reconnect" CR));
    gatewayState = GatewayState::ONBOARDING;
    return true;
  }
#endif
  uint8_t wifi_autoreconnect_cnt = 0;
#ifdef ESP32
  while (WiFi.status() != WL_CONNECTED && wifi_autoreconnect_cnt < maxConnectionRetryNetwork) {
#else
  while (WiFi.waitForConnectResult() != WL_CONNECTED && wifi_autoreconnect_cnt < maxConnectionRetryNetwork) {
#endif
    Logger.notice(OMG_LOGID, F("Attempting Wifi connection with saved AP: %d" CR), wifi_autoreconnect_cnt);

    WiFi.begin();
#if defined(WifiGMode) || defined(WifiPower)
    setESPWifiProtocolTxPower();
#endif
    delay(1000);
    wifi_autoreconnect_cnt++;
  }
  if (wifi_autoreconnect_cnt < maxConnectionRetryNetwork) {
    return true;
  } else {
    return false;
  }
}

void setOTA() {
  // Port defaults to 8266
  ArduinoOTA.setPort(ota_port);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(g_gateway_name);

  // No authentication by default
  ArduinoOTA.setPassword(g_ota_pass);

  ArduinoOTA.onStart([]() {
    Logger.debug(OMG_LOGID, F("Start OTA, lock other functions" CR));
    last_ota_activity_millis = millis();
#ifdef ESP32
    ProcessLock = true;
#  ifdef OMG_GATEWAY_BT
    stopProcessing();
#  endif
#endif
    lpDisplayPrint("OTA in progress");
  });
  ArduinoOTA.onEnd([]() {
    Logger.debug(OMG_LOGID, F("\nOTA done" CR));
    last_ota_activity_millis = 0;
    lpDisplayPrint("OTA done");
    ESPRestart(6);
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Logger.debug(OMG_LOGID, F("Progress: %u%%\r" CR), (progress / (total / 100)));
    gatewayState = GatewayState::LOCAL_OTA_IN_PROGRESS;
    last_ota_activity_millis = millis();
  });
  ArduinoOTA.onError([](ota_error_t error) {
    last_ota_activity_millis = millis();
    Serial.printf("Error[%u]: ", error);
    gatewayState = GatewayState::ERROR;
    if (error == OTA_AUTH_ERROR)
      Logger.error(OMG_LOGID, F("Auth Failed" CR));
    else if (error == OTA_BEGIN_ERROR)
      Logger.error(OMG_LOGID, F("Begin Failed" CR));
    else if (error == OTA_CONNECT_ERROR)
      Logger.error(OMG_LOGID, F("Connect Failed" CR));
    else if (error == OTA_RECEIVE_ERROR)
      Logger.error(OMG_LOGID, F("Receive Failed" CR));
    else if (error == OTA_END_ERROR)
      Logger.error(OMG_LOGID, F("End Failed" CR));
    ESPRestart(6);
  });
  ArduinoOTA.begin();
}

#if !OMG_MQTT_BROKER_MODE
void setupTLS(int index) {
  configTime(0, 0, NTP_SERVER);
  WiFiClientSecure* sClient = static_cast<WiFiClientSecure*>(eClient.get());
  Logger.notice(OMG_LOGID, F("cnt index used: %d" CR), index);
  if (!cnt_parameters_array[index].isCertValidate) {
    Logger.notice(OMG_LOGID, F("Disabling cert validation" CR));
    sClient->setInsecure();
  } else {
    Logger.notice(OMG_LOGID, F("Enabling cert validation" CR));
#  if defined(ESP32)
    if (cnt_parameters_array[index].server_cert.length() > MIN_CERT_LENGTH) {
      sClient->setCACert(cnt_parameters_array[index].server_cert.c_str());
      Logger.notice(OMG_LOGID, F("Server cert found from cert array" CR));
    } else if (strlen(ss_server_cert) > MIN_CERT_LENGTH) {
      sClient->setCACert(ss_server_cert);
      Logger.notice(OMG_LOGID, F("Server cert found from ss_server_cert" CR));
    } else {
      Logger.error(OMG_LOGID, F("No server cert found" CR));
      gatewayState = GatewayState::ERROR;
    }

#    if AWS_IOT
    if (strcmp(cnt_parameters_array[index].mqtt_port, "443") == 0) {
      Logger.notice(OMG_LOGID, F("Using ALPN" CR));
      sClient->setAlpnProtocols(alpnProtocols);
    }
#    endif
#    if OMG_MQTT_SECURE_SIGNED_CLIENT
    if (cnt_parameters_array[index].client_cert.length() > MIN_CERT_LENGTH) {
      sClient->setCertificate(cnt_parameters_array[index].client_cert.c_str());
      Logger.notice(OMG_LOGID, F("Client cert found from cert array" CR));
    } else if (strlen(ss_client_cert) > MIN_CERT_LENGTH) {
      sClient->setCertificate(ss_client_cert);
      Logger.notice(OMG_LOGID, F("Client cert found from ss_client_cert" CR));
    } else {
      Logger.error(OMG_LOGID, F("No client cert found" CR));
      gatewayState = GatewayState::ERROR;
    }
    if (cnt_parameters_array[index].client_key.length() > MIN_CERT_LENGTH) {
      sClient->setPrivateKey(cnt_parameters_array[index].client_key.c_str());
      Logger.notice(OMG_LOGID, F("Client key found from cert array" CR));
    } else if (strlen(ss_client_key) > MIN_CERT_LENGTH) {
      sClient->setPrivateKey(ss_client_key);
      Logger.notice(OMG_LOGID, F("Client key found from ss_client_key" CR));
    } else {
      Logger.error(OMG_LOGID, F("No client key found" CR));
      gatewayState = GatewayState::ERROR;
    }
#    endif
#  elif defined(ESP8266)
    caCert.append(cnt_parameters_array[index].server_cert.c_str());
    sClient->setTrustAnchors(&caCert);
    sClient->setBufferSizes(512, 512);
#    if OMG_MQTT_SECURE_SIGNED_CLIENT
    if (pClCert != nullptr) {
      delete pClCert;
    }
    if (pClKey != nullptr) {
      delete pClKey;
    }
    pClCert = new X509List(cnt_parameters_array[index].client_cert.c_str());
    pClKey = new PrivateKey(cnt_parameters_array[index].client_key.c_str());
    sClient->setClientRSACert(pClCert, pClKey);
#    endif
#  endif
  }
}
#endif

/*
  Reboot for Reason Codes
  0 - Erase and Restart
  1 - Repeated MQTT Connection Failure
  2 - Repeated WiFi Connection Failure
  3 - Failed WiFiManager configuration portal
  4 - BLE Scan watchdog
  5 - User requested reboot
  6 - OTA Update
  7 - Parameters changed
  8 - not enough memory to pursue
  9 - SELFTEST end
*/
void ESPRestart(byte reason) {
#ifdef SecondaryModule
  // Erase the secondary module config
  String restartCmdStr = "{\"cmd\":\"" + String(restartCmd) + "\"}";
  Logger.notice(OMG_LOGID, F("Restarting secondary module : %s" CR), restartCmdStr.c_str());
  receivingDATA(subjectMQTTtoSYSsetSecondaryModule, restartCmdStr.c_str());
  delay(2000);
#endif
  StaticJsonDocument<128> jsonBuffer;
  JsonObject jsondata = jsonBuffer.to<JsonObject>();
  jsondata["reason"] = reason;
  jsondata["retain"] = true;
  jsondata["uptime"] = uptime();
  jsondata["origin"] = subjectLOGtoMQTT;
  pub(jsondata); // We go to MQTT bypassing the queue to ensure the message is sent
  // Clean queue
  while (!jsonQueue.empty()) {
    jsonQueue.pop();
  }
  Logger.warning(OMG_LOGID, F("Rebooting for reason code %d" CR), reason);
#if defined(ESP32)
  ESP.restart();
#elif defined(ESP8266)
  ESP.reset();
#endif
}

#if defined(ESPWifiManualSetup)
void setupWiFiFromBuild() {
  // Must set hostname before mode. See https://github.com/espressif/arduino-esp32/issues/6700
  WiFi.setHostname(g_gateway_name);
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(OMG_WIFI_SSID, OMG_WIFI_PASSWORD);
  Logger.debug(OMG_LOGID, F("Connecting to %s" CR), OMG_WIFI_SSID);
#  ifdef OMG_WIFI_SSID1
  wifiMulti.addAP(OMG_WIFI_SSID1, OMG_WIFI_PASSWORD1);
  Logger.debug(OMG_LOGID, F("Connecting to %s" CR), OMG_WIFI_SSID1);
#  endif
  delay(10);

  // We start by connecting to a WiFi network

#  ifdef NetworkAdvancedSetup
  IPAddress ip_adress;
  IPAddress gateway_adress;
  IPAddress subnet_adress;
  IPAddress dns_adress;
  ip_adress.fromString(NET_IP);
  gateway_adress.fromString(NET_GW);
  subnet_adress.fromString(NET_MASK);
  dns_adress.fromString(NET_DNS);

  if (!WiFi.config(ip_adress, gateway_adress, subnet_adress, dns_adress)) {
    Logger.error(OMG_LOGID, F("Wifi STA Failed to configure" CR));
    gatewayState = GatewayState::ERROR;
  }

#  endif

  while (wifiMulti.run() != WL_CONNECTED) {
    delay(500);
    Logger.debug(OMG_LOGID, F("." CR));
    failure_number_ntwk++;
#  if defined(ESP32) && defined(OMG_GATEWAY_BT)
    if (SYSConfig.powerMode) {
      if (failure_number_ntwk > maxConnectionRetryNetwork) {
        sleep();
      }
    } else {
      if (failure_number_ntwk > maxRetryWatchDog) {
        ESPRestart(2);
      }
    }
#  else
    if (failure_number_ntwk > maxRetryWatchDog) {
      ESPRestart(2);
    }
#  endif
  }
  Logger.notice(OMG_LOGID, F("WiFi ok with manual config credentials" CR));
  displayPrint("Wifi connected");
}

#else

WiFiManager wifiManager;

//flag for saving data
bool shouldSaveConfig = false;
//do we have been connected once to MQTT

//callback notifying us of the need to save config
void saveConfigCallback() {
  Logger.debug(OMG_LOGID, F("Should save config" CR));
  shouldSaveConfig = true;
}

#  ifdef TRIGGER_GPIO
/**
 * Identify a long button press to trigger a reset or a failsafe mode
* */
void blockingWaitForReset() {
  if (digitalRead(TRIGGER_GPIO) == LOW) {
    delay(50);
    if (digitalRead(TRIGGER_GPIO) == LOW) {
      Logger.debug(OMG_LOGID, F("Trigger button Pressed" CR));
      delay(3000); // reset delay hold
      if (digitalRead(TRIGGER_GPIO) == LOW) {
        Logger.notice(OMG_LOGID, F("Button Held" CR));
// Switching off the relay during reset or failsafe operations
#    ifdef OMG_ACTUATOR_ONOFF
        uint8_t level = digitalRead(ACTUATOR_ONOFF_GPIO);
        if (level == ACTUATOR_ON) {
          ActuatorTrigger();
        }
#    endif
        gatewayState = GatewayState::WAITING_ONBOARDING;
        // Checking if the flash has already been erased to identify if we erase it or go into failsafe mode
        // going to failsafe mode is done by doing a long button press from a state where the flash has already been erased
        if (SPIFFS.begin()) {
          Logger.debug(OMG_LOGID, F("mounted file system" CR));
          if (SPIFFS.exists("/config.json")) {
            Logger.notice(OMG_LOGID, F("Erasing ESP Config, restarting" CR));
            erase(true);
          }
        }
        delay(30000);
        if (digitalRead(TRIGGER_GPIO) == LOW) {
          Logger.notice(OMG_LOGID, F("Going into failsafe mode without peripherals" CR));
          // Failsafe mode enable to connect to Wifi or change the firmware without the peripherals setup
          failSafeMode = true;
          setupWiFiManager();
        }
      }
    }
  }
}

/**
 * Check if button is pressed so as to reset the credentials and parameters stored into the flash
* */
void checkButton() {
  unsigned long timeFromStart = millis();
  // Identify if the reset button is pushed at start
  if (timeFromStart < TimeToResetAtStart) {
    blockingWaitForReset();
  } else { // When we are not at start we either check the button as a regular input (ZsensorGPIOInput used) or for a reset
#    if defined(INPUT_GPIO) && defined(OMG_SENSOR_GPIOINPUT) && INPUT_GPIO == TRIGGER_GPIO
    MeasureGPIOInput();
#    else
    blockingWaitForReset();
#    endif
  }
}
#  else
void checkButton() {}
#  endif

void saveConfig() {
  Logger.debug(OMG_LOGID, F("saving configs" CR));

  int totalSize = 512;
#  if !OMG_MQTT_BROKER_MODE
  for (int i = 0; i < 3; ++i) { // index 0 contains the default values from the build, these values can't be changed at runtime
    if (cnt_parameters_array[i].validConnection) {
      totalSize += cnt_parameters_array[i].server_cert.length();
      totalSize += cnt_parameters_array[i].client_cert.length();
      totalSize += cnt_parameters_array[i].client_key.length();
      totalSize += cnt_parameters_array[i].ota_server_cert.length();
    }
  }
#  endif

  Logger.notice(OMG_LOGID, F("Total size: %d" CR), totalSize);

  DynamicJsonDocument json(512 + totalSize);

#  if !OMG_MQTT_BROKER_MODE
  for (int i = 0; i < 3; ++i) {
    if (cnt_parameters_array[i].validConnection) {
      char index_suffix[2];
      if (i == 0) {
        index_suffix[0] = '\0';
      } else {
        index_suffix[0] = '0' + i;
        index_suffix[1] = '\0';
      }
      char key[mqtt_key_max_size];
      if (cnt_parameters_array[i].server_cert.length() > MIN_CERT_LENGTH) {
        strcpy(key, "mqtt_broker_cert");
        strcat(key, index_suffix);
        json[key] = cnt_parameters_array[i].server_cert;
      }
      if (cnt_parameters_array[i].client_cert.length() > MIN_CERT_LENGTH) {
        strcpy(key, "mqtt_client_cert");
        strcat(key, index_suffix);
        json[key] = cnt_parameters_array[i].client_cert;
      }
      if (cnt_parameters_array[i].client_key.length() > MIN_CERT_LENGTH) {
        strcpy(key, "mqtt_client_key");
        strcat(key, index_suffix);
        json[key] = cnt_parameters_array[i].client_key;
      }
      if (cnt_parameters_array[i].ota_server_cert.length() > MIN_CERT_LENGTH) {
        strcpy(key, "ota_server_cert");
        strcat(key, index_suffix);
        json[key] = cnt_parameters_array[i].ota_server_cert;
      }
      strcpy(key, "mqtt_server");
      strcat(key, index_suffix);
      json[key] = cnt_parameters_array[i].mqtt_server;
      strcpy(key, "mqtt_port");
      strcat(key, index_suffix);
      json[key] = cnt_parameters_array[i].mqtt_port;
      strcpy(key, "mqtt_user");
      strcat(key, index_suffix);
      json[key] = cnt_parameters_array[i].mqtt_user;
      strcpy(key, "mqtt_pass");
      strcat(key, index_suffix);
      json[key] = cnt_parameters_array[i].mqtt_pass;
      strcpy(key, "mqtt_broker_secure");
      strcat(key, index_suffix);
      json[key] = cnt_parameters_array[i].isConnectionSecure;
      strcpy(key, "mqtt_iscertvalid");
      strcat(key, index_suffix);
      json[key] = cnt_parameters_array[i].isCertValidate;
      strcpy(key, "valid_cnt");
      strcat(key, index_suffix);
      json[key] = cnt_parameters_array[i].validConnection;
    }
  }

  if (cnt_parameters_array[cnt_index].validConnection) {
    json["cnt_index"] = cnt_index;
  }
#  endif

  json["mqtt_topic"] = mqtt_topic;
#  ifdef OMG_MQTT_DISCOVERY
  json["discovery_prefix"] = discovery_prefix;
#  endif
  json["gateway_name"] = g_gateway_name;
  json["ota_pass"] = g_ota_pass;
#  if OMG_LOG_TO_SYSLOG
  json["syslog_server"] = g_syslog_server;
  json["syslog_port"] = g_syslog_port;
#  endif

  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    Logger.error(OMG_LOGID, F("failed to open config file for writing" CR));
    gatewayState = GatewayState::ERROR;
  }

  serializeJson(json, configFile);
  configFile.close();
}

bool loadConfigFromFlash() {
  Logger.debug(OMG_LOGID, F("mounting FS..." CR));
  bool result = false;

  if (SPIFFS.begin()) {
    Logger.debug(OMG_LOGID, F("mounted file system" CR));
  } else {
    Logger.warning(OMG_LOGID, F("failed to mount FS -> formating" CR));
    SPIFFS.format();
    if (SPIFFS.begin())
      Logger.debug(OMG_LOGID, F("mounted file system after formating" CR));
  }
  if (SPIFFS.exists("/config.json")) {
    //file exists, reading and loading
    Logger.debug(OMG_LOGID, F("reading config file" CR));
    File configFile = SPIFFS.open("/config.json", "r");
    if (configFile) {
      Logger.debug(OMG_LOGID, F("opened config file" CR));
      DynamicJsonDocument json(configFile.size() * 2);
      auto error = deserializeJson(json, configFile);
      if (error) {
        Logger.error(OMG_LOGID, F("deserialize config failed: %s, buffer capacity: %u" CR), error.c_str(), json.capacity());
        gatewayState = GatewayState::ERROR;
      }
      if (!json.isNull()) {
        Logger.debug(OMG_LOGID, F("\nparsed json, size: %u" CR), json.memoryUsage());
        // Print json to serial port
        //serializeJsonPretty(json, Serial);

#  if !OMG_MQTT_BROKER_MODE
        for (int i = 0; i < 3; ++i) {
          char index_suffix[2]; // Large enough for 0, 1, or 2 and the null terminator
          if (i == 0) {
            index_suffix[0] = '\0'; // Empty string
          } else {
            index_suffix[0] = '0' + i; // Convert int to char
            index_suffix[1] = '\0'; // Null terminator
          }
          char key[mqtt_key_max_size];
          strcpy(key, "mqtt_broker_cert");
          strcat(key, index_suffix);
          if (json.containsKey(key)) {
            cnt_parameters_array[i].server_cert = json[key].as<const char*>();
          }
          strcpy(key, "mqtt_client_cert");
          strcat(key, index_suffix);
          if (json.containsKey(key)) {
            cnt_parameters_array[i].client_cert = json[key].as<const char*>();
          }
          strcpy(key, "mqtt_client_key");
          strcat(key, index_suffix);
          if (json.containsKey(key)) {
            cnt_parameters_array[i].client_key = json[key].as<const char*>();
          }
          strcpy(key, "ota_server_cert");
          strcat(key, index_suffix);
          if (json.containsKey(key)) {
#    ifdef ESP32
            // Read hash from the file
            std::string hash = generateHash(json["ota_server_cert"]);
            // Compare the hash with the expected hash
            if (hash == GITHUB_OTA_SERVER_CERT_HASH) {
              // Do nothing
              Logger.warning(OMG_LOGID, F("Old Github OTA server detected, skipping" CR));
            } else {
              Logger.notice(OMG_LOGID, F("OTA server cert hash: %s" CR), hash.c_str());
              cnt_parameters_array[i].ota_server_cert = json["ota_server_cert"].as<const char*>();
            }
#    else
            cnt_parameters_array[i].ota_server_cert = json["ota_server_cert"].as<const char*>();
#    endif
          }
          strcpy(key, "mqtt_server");
          strcat(key, index_suffix);
          if (json.containsKey(key)) {
            strcpy(cnt_parameters_array[i].mqtt_server, json[key].as<const char*>());
          }
          strcpy(key, "mqtt_port");
          strcat(key, index_suffix);
          if (json.containsKey(key)) {
            strcpy(cnt_parameters_array[i].mqtt_port, json[key].as<const char*>());
          }
          strcpy(key, "mqtt_user");
          strcat(key, index_suffix);
          if (json.containsKey(key)) {
            strcpy(cnt_parameters_array[i].mqtt_user, json[key].as<const char*>());
          }
          strcpy(key, "mqtt_pass");
          strcat(key, index_suffix);
          if (json.containsKey(key)) {
            strcpy(cnt_parameters_array[i].mqtt_pass, json[key].as<const char*>());
          }
          strcpy(key, "mqtt_broker_secure");
          strcat(key, index_suffix);
          if (json.containsKey(key)) {
            cnt_parameters_array[i].isConnectionSecure = json[key].as<bool>();
          }
          strcpy(key, "mqtt_iscertvalid");
          strcat(key, index_suffix);
          if (json.containsKey(key)) {
            cnt_parameters_array[i].isCertValidate = json[key].as<bool>();
          }
          strcpy(key, "valid_cnt");
          strcat(key, index_suffix);
          if (json.containsKey(key)) {
            cnt_parameters_array[i].validConnection = json[key].as<bool>();
          } else if (i == CNT_DEFAULT_INDEX) {
            // For backward compatibility, if valid_cnt is not found, we assume the connection is valid for CNT_DEFAULT_INDEX
            Logger.warning(OMG_LOGID, F("valid_cnt not found, assuming connection is valid" CR));
            cnt_parameters_array[i].validConnection = true;
          }
        }
        if (json.containsKey("cnt_index")) {
          cnt_index = json["cnt_index"].as<int>();
        }
#  endif
        if (json.containsKey("mqtt_topic"))
          strcpy(mqtt_topic, json["mqtt_topic"]);
#  ifdef OMG_MQTT_DISCOVERY
        if (json.containsKey("discovery_prefix"))
          strcpy(discovery_prefix, json["discovery_prefix"]);
#  endif
        if (json.containsKey("gateway_name"))
          strcpy(g_gateway_name, json["gateway_name"]);
        if (json.containsKey("ota_pass")) {
          strcpy(g_ota_pass, json["ota_pass"]);
#  ifdef WM_PWD_FROM_MAC // From ESP Mac Address, last 8 digits as the password
          // Compare the existing g_ota_pass if g_ota_pass = OTAPASSWORD then replace with the last 8 digits of the mac address
          // This enable user migrating from previous version to have the same WiFi portal password as previously unless they changed it
          if (strcmp(g_ota_pass, "OTAPASSWORD") == 0) {
            String s = WiFi.macAddress();
            sprintf(g_ota_pass, "%.2s%.2s%.2s%.2s",
                    s.c_str() + 6, s.c_str() + 9, s.c_str() + 12, s.c_str() + 15);
          }
#  endif
        }
#  if OMG_LOG_TO_SYSLOG
        if (json.containsKey("syslog_server"))
          strcpy(g_syslog_server, json["syslog_server"]);
        if (json.containsKey("syslog_port"))
          strcpy(g_syslog_port, json["syslog_port"]);
#  endif
        result = true;
      } else {
        Logger.warning(OMG_LOGID, F("failed to load json config" CR));
      }
      configFile.close();
    }
  } else {
    Logger.notice(OMG_LOGID, F("No config file found defining default values" CR));
#  ifdef OMG_USE_MAC_AS_GATEWAY_NAME
    String s = WiFi.macAddress();
    sprintf(g_gateway_name, "%.2s%.2s%.2s%.2s%.2s%.2s",
            s.c_str(), s.c_str() + 3, s.c_str() + 6, s.c_str() + 9, s.c_str() + 12, s.c_str() + 15);
    Log.notice(F("Gateway Name: %s.local" CR), g_gateway_name);
#  endif
#  ifdef WM_PWD_FROM_MAC // From ESP Mac Address, last 8 digits as the password
    sprintf(g_ota_pass, "%.2s%.2s%.2s%.2s",
            s.c_str() + 6, s.c_str() + 9, s.c_str() + 12, s.c_str() + 15);
#  endif
  }

  return result;
}

void setupWiFiManager() {
  delay(10);
  // Must set hostname before mode. See https://github.com/espressif/arduino-esp32/issues/6700
  wifiManager.setHostname(g_gateway_name);
  WiFi.mode(WIFI_STA);

#  ifdef OMG_USE_MAC_AS_GATEWAY_NAME
  String s = WiFi.macAddress();
  snprintf(g_WifiManager_ssid, MAC_NAME_MAX_LEN, "%s_%.2s%.2s", Gateway_Short_Name, s.c_str(), s.c_str() + 3);
#  endif

  wifiManager.setDebugOutput(OMG_WM_DEBUG);

  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default
#  ifndef WIFIMNG_HIDE_MQTT_CONFIG
#    if !OMG_MQTT_BROKER_MODE
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", cnt_parameters_array[CNT_DEFAULT_INDEX].mqtt_server, parameters_size, " minlength='1' maxlength='64' required");
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", cnt_parameters_array[CNT_DEFAULT_INDEX].mqtt_port, 6, " minlength='1' maxlength='5' required");
  WiFiManagerParameter custom_mqtt_user("user", "mqtt user", cnt_parameters_array[CNT_DEFAULT_INDEX].mqtt_user, parameters_size, " maxlength='64'");
  WiFiManagerParameter custom_mqtt_pass("pass", "mqtt pass", OMG_MQTT_PASS, parameters_size, " input type='password' maxlength='64'");
  WiFiManagerParameter custom_mqtt_secure("secure", "<br/>mqtt secure", "1", 2, cnt_parameters_array[CNT_DEFAULT_INDEX].isConnectionSecure ? "type=\"checkbox\" checked" : "type=\"checkbox\"");
  WiFiManagerParameter custom_validate_cert("validate", "<br/>validate cert", "1", 2, cnt_parameters_array[CNT_DEFAULT_INDEX].isCertValidate ? "type=\"checkbox\" checked" : "type=\"checkbox\"");
  WiFiManagerParameter custom_mqtt_cert("cert", "<br/>mqtt server cert", "", 4096);
  WiFiManagerParameter custom_ota_server_cert("ota_cert", "<br/>ota server cert", "", 4096);
#      if OMG_MQTT_SECURE_SIGNED_CLIENT
  WiFiManagerParameter custom_client_cert("client_cert", "<br/>mqtt client cert", "", 4096);
  WiFiManagerParameter custom_client_key("client_key", "<br/>mqtt client key", "", 4096);
#      endif
#    endif
  WiFiManagerParameter custom_mqtt_topic("topic", "mqtt base topic", mqtt_topic, mqtt_topic_max_size, " minlength='1' maxlength='64' required");
  WiFiManagerParameter custom_g_gateway_name("name", "gateway name", g_gateway_name, parameters_size, " minlength='1' maxlength='64' required");
  WiFiManagerParameter custom_g_ota_pass("ota", "gateway password", g_ota_pass, parameters_size, " input type='password' minlength='8' maxlength='64' required");
#  endif
  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around

  wifiManager.setConnectTimeout(WiFi_TimeOut);
  //Set timeout before going to portal
  wifiManager.setConfigPortalTimeout(WifiManager_ConfigPortalTimeOut);

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

//set static IP
#  ifdef NetworkAdvancedSetup
  Logger.debug(OMG_LOGID, F("Adv wifi cfg" CR));
  IPAddress ip_adress;
  IPAddress gateway_adress;
  IPAddress subnet_adress;
  IPAddress dns_adress;
  ip_adress.fromString(NET_IP);
  gateway_adress.fromString(NET_GW);
  subnet_adress.fromString(NET_MASK);
  dns_adress.fromString(NET_DNS);
  wifiManager.setSTAStaticIPConfig(ip_adress, gateway_adress, subnet_adress, dns_adress);
#  endif

#  ifndef WIFIMNG_HIDE_MQTT_CONFIG
  //add all your parameters here
#    if !OMG_MQTT_BROKER_MODE
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_user);
  wifiManager.addParameter(&custom_mqtt_pass);
  wifiManager.addParameter(&custom_mqtt_secure);
  wifiManager.addParameter(&custom_mqtt_cert);
  wifiManager.addParameter(&custom_validate_cert);
  wifiManager.addParameter(&custom_ota_server_cert);
#      if OMG_MQTT_SECURE_SIGNED_CLIENT
  wifiManager.addParameter(&custom_client_cert);
  wifiManager.addParameter(&custom_client_key);
#      endif
#    endif
  wifiManager.addParameter(&custom_gateway_name);
  wifiManager.addParameter(&custom_mqtt_topic);
  wifiManager.addParameter(&custom_ota_pass);
#  endif
  //set minimum quality of signal so it ignores AP's under that quality
  wifiManager.setMinimumSignalQuality(MinimumWifiSignalQuality);

  if (SPIFFS.begin()) {
    Logger.debug(OMG_LOGID, F("mounted file system" CR));
    // Check if the config file exists and prevent the portal from showing if yes
    // Showing the portal if the config file exist would enable access to the configuration data and to the ESP update page
    // This is a security risk if an attacker has access to the gateway password
    if (SPIFFS.exists("/config.json")) {
      wifiManager.setEnableConfigPortal(false);
    }
  }

#  ifdef ESP32_ETHERNET
  wifiManager.setBreakAfterConfig(true); // If ethernet is used, we don't want to block the connection by keeping the portal up
#  endif

  if (!SYSConfig.offline && !wifi_reconnect_bypass()) // if we didn't connect with saved credential we start Wifimanager web portal
  {
    Logger.notice(OMG_LOGID, F("Connect your phone to WIFI AP: %s with PWD: %s" CR), g_WifiManager_ssid, g_ota_pass);
    gatewayState = GatewayState::ONBOARDING;
    //fetches ssid and pass and tries to connect
    //if it does not connect it starts an access point with the specified name
    //and goes into a blocking loop awaiting configuration
    if (!wifiManager.autoConnect(g_WifiManager_ssid, g_ota_pass)) {
      Logger.warning(OMG_LOGID, F("failed to connect and hit timeout" CR));
      delay(3000);

#  ifdef ESP32
      /* Workaround for bug in arduino core that causes the AP to become unsecure on reboot */
      esp_wifi_set_mode(WIFI_MODE_AP);
      esp_wifi_start();
      wifi_config_t conf;
      esp_wifi_get_config(WIFI_IF_AP, &conf);
      conf.ap.ssid_hidden = 1;
      esp_wifi_set_config(WIFI_IF_AP, &conf);
#  endif

      bool shouldRestart = (gatewayState != GatewayState::BROKER_CONNECTED && gatewayState != GatewayState::NTWK_CONNECTED);

#  ifdef USE_BLUFI
      shouldRestart = shouldRestart && !isStaConnecting();
#  endif
      // Restart/sleep only if not connected and not serial communication mode
      if (shouldRestart && !SYSConfig.serial) {
#  ifdef DEEP_SLEEP_IN_US
        sleep();
#  endif
        ESPRestart(3);
      }
    }
  }

  displayPrint("Network connected");
  gatewayState = GatewayState::NTWK_CONNECTED;

  if (shouldSaveConfig) {
    //read updated parameters
    cnt_index = CNT_DEFAULT_INDEX;
#  ifndef WIFIMNG_HIDE_MQTT_CONFIG
#    if !OMG_MQTT_BROKER_MODE
    strcpy(cnt_parameters_array[cnt_index].mqtt_server, custom_mqtt_server.getValue());
    strcpy(cnt_parameters_array[cnt_index].mqtt_port, custom_mqtt_port.getValue());
    strcpy(cnt_parameters_array[cnt_index].mqtt_user, custom_mqtt_user.getValue());
    // Check if the MQTT password field contains the default value
    if (strcmp(custom_mqtt_pass.getValue(), OMG_MQTT_PASS) != 0) {
      // If it's not the default password, update the MQTT password
      strcpy(cnt_parameters_array[cnt_index].mqtt_pass, custom_mqtt_pass.getValue());
    }
    strcpy(mqtt_topic, custom_mqtt_topic.getValue());
    if (mqtt_topic[strlen(mqtt_topic) - 1] != '/' && strlen(mqtt_topic) < parameters_size) {
      strcat(mqtt_topic, "/");
    }

    cnt_parameters_array[cnt_index].isConnectionSecure = *custom_mqtt_secure.getValue();
    cnt_parameters_array[cnt_index].isCertValidate = *custom_validate_cert.getValue();
    if (strlen(custom_mqtt_cert.getValue()) > MIN_CERT_LENGTH) {
      cnt_parameters_array[cnt_index].server_cert = TheengsUtils::processCert(custom_mqtt_cert.getValue());
    }
    if (strlen(custom_ota_server_cert.getValue()) > MIN_CERT_LENGTH) {
      cnt_parameters_array[cnt_index].ota_server_cert = TheengsUtils::processCert(custom_ota_server_cert.getValue());
    }
#      if OMG_MQTT_SECURE_SIGNED_CLIENT
    if (strlen(custom_client_cert.getValue()) > MIN_CERT_LENGTH) {
      cnt_parameters_array[cnt_index].client_cert = TheengsUtils::processCert(custom_client_cert.getValue());
    }
    if (strlen(custom_client_key.getValue()) > MIN_CERT_LENGTH) {
      cnt_parameters_array[cnt_index].client_key = TheengsUtils::processCert(custom_client_key.getValue());
    }
#      endif
#    endif
    strcpy(g_gateway_name, custom_gateway_name.getValue());
    strcpy(g_ota_pass, custom_ota_pass.getValue());
#  endif

#  if !OMG_MQTT_BROKER_MODE
    // We suppose the connection is valid (could be tested before)
    cnt_parameters_array[cnt_index].validConnection = true;
#  endif

    //save the custom parameters to FS
    saveConfig();
  }
}
#  ifdef ESP32_ETHERNET
void setup_ethernet_esp32() {
  bool ethBeginSuccess = false;
  WiFi.onEvent(WiFiEvent);
#    ifdef NetworkAdvancedSetup
  IPAddress ip_adress;
  IPAddress gateway_adress;
  IPAddress subnet_adress;
  IPAddress dns_adress;
  ip.fromString(NET_IP);
  gateway.fromString(NET_GW);
  subnet.fromString(NET_MASK);
  Dns.fromString(NET_DNS);

  Logger.debug(OMG_LOGID, F("Adv eth cfg" CR));
  ETH.config(ip, gateway, subnet, Dns);
  ethBeginSuccess = ETH.begin();
#    else
  Logger.notice(OMG_LOGID, F("Spl eth cfg" CR));
  ethBeginSuccess = ETH.begin();
#    endif
  if (ethBeginSuccess) {
    Logger.notice(OMG_LOGID, F("Ethernet started" CR));
    while (!ethConnected && failure_number_ntwk <= maxConnectionRetryNetwork) {
      delay(500);
      Logger.notice(OMG_LOGID, F("." CR));
      failure_number_ntwk++;
    }
  } else {
    Logger.error(OMG_LOGID, F("Ethernet not started" CR));
    gatewayState = GatewayState::ERROR;
  }
}

void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      Logger.debug(OMG_LOGID, F("Ethernet Started" CR));
      ETH.setHostname(g_gateway_name);
      break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      Logger.notice(OMG_LOGID, F("Ethernet Connected" CR));
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      Logger.notice(OMG_LOGID, F("OpenMQTTGateway Ethernet MAC: %s" CR), ETH.macAddress().c_str());
      Logger.notice(OMG_LOGID, F("OpenMQTTGateway Ethernet IP: %s" CR), ETH.localIP().toString().c_str());
      Logger.notice(OMG_LOGID, F("OpenMQTTGateway Ethernet link speed: %d Mbps" CR), ETH.linkSpeed());
      gatewayState = GatewayState::NTWK_CONNECTED;
      ethConnected = true;
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      Logger.warning(OMG_LOGID, F("Ethernet Disconnected" CR));
      ethConnected = false;
      break;
    case ARDUINO_EVENT_ETH_STOP:
      Logger.warning(OMG_LOGID, F("Ethernet Stopped" CR));
      ethConnected = false;
      break;
    default:
      break;
  }
}
#  endif
#endif

#if DEFAULT_LOW_POWER_MODE != DEACTIVATED && defined(ESP32)
/**
 * Deep-sleep for the ESP32 - e.g. DEEP_SLEEP_IN_US 30000000 for 30 seconds / wake by ESP32_EXT0_WAKE_PIN/ESP32_EXT1_WAKE_PIN.
 * Everything is off and (almost) all execution state is lost.
 */
void sleep() {
  if (SYSConfig.powerMode < PowerMode::INTERVAL)
    return;
  Logger.notice(OMG_LOGID, F("Entering deep sleep" CR));
  gatewayState = GatewayState::SLEEPING;
  delay(250); // To allow the LEDs to switch off and MQTT message to be sent
#  if defined(OMG_BOARD_M5STACK) || defined(OMG_BOARD_M5STICKC) || defined(OMG_BOARD_M5STICKCP) || defined(OMG_BOARD_M5TOUGH)
  sleepScreen();
  esp_sleep_enable_ext0_wakeup((gpio_num_t)SLEEP_BUTTON, LOW);
#  endif
  Logger.debug(OMG_LOGID, F("Deactivating ESP32 components" CR));
#  ifdef OMG_GATEWAY_BT
  stopProcessing();
  ProcessLock = true;
#  endif
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  adc_power_off();
#  pragma GCC diagnostic pop
  esp_wifi_stop();
#  ifdef ESP32_EXT0_WAKE_PIN
  Logger.notice(OMG_LOGID, F("Entering deep sleep, EXT0 Wakeup by pin : %l." CR), ESP32_EXT0_WAKE_PIN);
#  endif
#  ifdef ESP32_EXT1_WAKE_PIN
  Logger.notice(OMG_LOGID, F("Entering deep sleep, EXT1 Wakeup by pin : %l." CR), ESP32_EXT1_WAKE_PIN);
#  endif
  if (SYSConfig.powerMode == PowerMode::ACTION) {
    esp_deep_sleep_start();
  } else if (SYSConfig.powerMode == PowerMode::INTERVAL) {
    Logger.notice(OMG_LOGID, F("Entering deep sleep for %l us." CR), DEEP_SLEEP_IN_US);
    esp_deep_sleep(DEEP_SLEEP_IN_US);
  }
}
#else
void sleep() {}
#endif

void loop() {
#ifndef ESPWifiManualSetup
  checkButton(); // check if a reset of wifi/mqtt settings is asked
#endif

#ifdef ESP8266
  updateAndHandleLEDsTask(); // With ESP8266 we need to update the LEDs in the loop
#endif
  if (!SYSConfig.offline) { // Online mode
    if (mqttSetupPending) {
      setupMQTT();
      mqttSetupPending = false;
    }
    // When online the MQTT connection callback release the processes
  }
  if (firstStart) {
#ifdef OMG_GATEWAY_SERIAL
    if (SYSConfig.serial && isSerialReady()) {
#  ifdef OMG_GATEWAY_BT
      BTProcessLock = !BTConfig.enabled;
#  endif
      ProcessLock = false;
      firstStart = false;
    }
#else
    ProcessLock = false;
    firstStart = false;
#endif
  }
  unsigned long now = millis();

#ifdef OMG_GATEWAY_SERIAL // Serial is a module and a communication layer so it's always processed
  SERIALtoX();
#endif

  if (ethConnected || WiFi.status() == WL_CONNECTED) {
    if (ethConnected && WiFi.status() == WL_CONNECTED) {
      WiFi.disconnect(); // we disconnect the wifi as we are connected to ethernet
    }
    ArduinoOTA.handle();
    failure_number_ntwk = 0;
    if (now > (timer_sys_checks + (TimeBetweenCheckingSYS * 1000)) || !timer_sys_checks) {
#if OMG_MQTT_MESSAGE_UTC_TIMESTAMP || OMG_MQTT_MESSAGE_UNIX_TIMESTAMP || OMG_MQTT_MESSAGE_LOCAL_TIMESTAMP
      TheengsUtils::syncNTP();
#if OMG_MQTT_MESSAGE_LOCAL_TIMESTAMP
      TheengsUtils::setTimezone(TIMEZONE);
#endif
#endif
      if (!timer_sys_checks) { // Update check at start up only
#if (defined(ESP32) && defined(OMG_MQTT_HTTPS_FW_UPDATE)) || defined(OMG_LOCAL_OTA_FW_UPDATE)
        checkForUpdates();
#endif
      }
      timer_sys_checks = millis();
    }
#if defined(ZwebUI) && defined(ESP32)
    WebUILoop();
#endif
    mqtt->loop();
    if (mqtt->connected()) { // MQTT client is still connected
      failure_number_ntwk = 0;

#ifdef OMG_MQTT_DISCOVERY
      // Deactivate autodiscovery after DiscoveryAutoOffTimer.
      // Exception: when discovery_republish_on_reconnect is enabled, we never never automatically disable discovery
      if (!discovery_republish_on_reconnect && SYSConfig.discovery && (now > lastDiscovery + DiscoveryAutoOffTimer))
        SYSConfig.discovery = false;
#endif
    }
  } else if (!SYSConfig.offline && !SYSConfig.serial) { // disconnected from network
    Logger.warning(OMG_LOGID, F("Network disconnected" CR));
    gatewayState = GatewayState::NTWK_DISCONNECTED;
    if (!wifi_reconnect_bypass()) {
      sleep();
    } else {
      gatewayState = GatewayState::NTWK_CONNECTED;
    }
  }
  if (!ProcessLock) {
    if (now > (timer_sys_measures + (TimeBetweenReadingSYS * 1000)) || !timer_sys_measures) {
      timer_sys_measures = millis();
      stateMeasures();
#ifdef OMG_GATEWAY_BT
      stateBTMeasures(false);
#endif
#ifdef OMG_ACTUATOR_ONOFF
      stateONOFFMeasures();
#endif
#ifdef OMG_DISPLAY_SSD1306
      stateSSD1306Display();
#endif
#ifdef OMG_GATEWAY_LORA
      stateLORAMeasures();
#endif
#if defined(OMG_GATEWAY_RTL_433) || defined(OMG_GATEWAY_PILIGHT) || defined(OMG_GATEWAY_RF) || defined(OMG_GATEWAY_RF2) || defined(OMG_ACTUATOR_SOMFY)
      stateRFMeasures();
#endif
#if defined(ZwebUI) && defined(ESP32)
      stateWebUIStatus();
#endif
    }
// Function that doesn't need an active connection
#if defined(OMG_BOARD_M5STICKC) || defined(OMG_BOARD_M5STICKCP) || defined(OMG_BOARD_M5STACK) || defined(OMG_BOARD_M5TOUGH)
    loopM5();
#endif
#if defined(OMG_DISPLAY_SSD1306)
    loopSSD1306();
#endif
#ifdef OMG_SENSOR_BME280
    MeasureTempHumAndPressure(); //Addon to measure Temperature, Humidity, Pressure and Altitude with a Bosch BME280/BMP280
#endif
#ifdef OMG_SENSOR_HTU21
    MeasureTempHum(); //Addon to measure Temperature, Humidity, of a HTU21 sensor
#endif
#ifdef OMG_SENSOR_LM75
    MeasureTemp(); //Addon to measure Temperature of an LM75 sensor
#endif
#ifdef OMG_SENSOR_AHTX0
    MeasureAHTTempHum(); //Addon to measure Temperature, Humidity, of an 'AHTx0' sensor
#endif
#ifdef OMG_SENSOR_HCSR04
    MeasureDistance(); //Addon to measure distance with a HC-SR04
#endif
#ifdef OMG_SENSOR_BH1750
    MeasureLightIntensity(); //Addon to measure Light Intensity with a BH1750
#endif
#ifdef OMG_SENSOR_MQ2
    MeasureGasMQ2();
#endif
#ifdef OMG_SENSOR_TEMT6000
    MeasureLightIntensityTEMT6000();
#endif
#ifdef OMG_SENSOR_TSL2561
    MeasureLightIntensityTSL2561();
#endif
#ifdef OMG_SENSOR_C37_YL83_HMRD
    MeasureC37_YL83_HMRDWater(); //Addon for leak detection with a C-37 YL-83 H-MRD
#endif
#ifdef OMG_SENSOR_DHT
    MeasureTempAndHum(); //Addon to measure the temperature with a DHT
#endif
#ifdef OMG_SENSOR_SHTC3
    MeasureTempAndHum(); //Addon to measure the temperature with a DHT
#endif
#ifdef OMG_SENSOR_DS1820
    MeasureDS1820Temp(); //Addon to measure the temperature with DS1820 sensor(s)
#endif
#ifdef OMG_SENSOR_INA226
    MeasureINA226();
#endif
#ifdef OMG_SENSOR_HCSR501
    MeasureHCSR501();
#endif
#ifdef OMG_SENSOR_GPIOINPUT
    MeasureGPIOInput();
#endif
#ifdef OMG_SENSOR_GPIOKEYCODE
    MeasureGPIOKeyCode();
#endif
#ifdef OMG_SENSOR_ADC
    MeasureADC(); //Addon to measure the analog value of analog pin
#endif
#ifdef OMG_SENSOR_TOUCH
    MeasureTouch();
#endif
#ifdef OMG_GATEWAY_LORA
    LORAtoX();
#  ifdef OMG_MQTT_DISCOVERY
    if (SYSConfig.discovery)
      launchLORADiscovery(false);
#  endif
#endif
#ifdef OMG_GATEWAY_RF
    RFtoX();
#endif
#ifdef OMG_GATEWAY_RF2
    RF2toX();
#endif
#ifdef OMG_GATEWAY_WEATHERSTATION
    ZgatewayWeatherStationtoX();
#endif
#ifdef OMG_GATEWAY_GFSUNINVERTER
    ZgatewayGFSunInverterMQTT();
#endif
#ifdef OMG_GATEWAY_PILIGHT
    PilighttoX();
#endif
#ifdef OMG_GATEWAY_BT
#  ifdef OMG_MQTT_DISCOVERY
    if (SYSConfig.discovery)
      launchBTDiscovery(false);
#  endif
#endif
#ifdef OMG_GATEWAY_SRFB
    SRFBtoX();
#endif
#ifdef OMG_GATEWAY_IR
    IRtoX();
#endif
#ifdef OMG_GATEWAY_2G
    if (_2GtoX())
      Logger.debug(OMG_LOGID, F("2GtoMQTT OK" CR));
#endif
#ifdef OMG_GATEWAY_RFM69
    if (RFM69toX())
      Logger.debug(OMG_LOGID, F("RFM69toMQTT OK" CR));
#endif
#ifdef OMG_ACTUATOR_FASTLED
    FASTLEDLoop();
#endif
#ifdef OMG_ACTUATOR_PWM
    PWMLoop();
#endif
#ifdef OMG_GATEWAY_RTL_433
    RTL_433Loop();
#  ifdef OMG_MQTT_DISCOVERY
    if (SYSConfig.discovery)
      launchRTL_433Discovery(false);
#  endif
#endif
  }
  // Empty the queue
  emptyQueue();
  // Sleep if ready
  if (ready_to_sleep) {
    sleep();
  }
}

/**
 * Calculate uptime and take into account the millis() rollover
 */
unsigned long uptime() {
  static unsigned long lastUptime = 0;
  static unsigned long uptimeAdd = 0;
  unsigned long uptime = millis() / 1000 + uptimeAdd;
  if (uptime < lastUptime) {
    uptime += 4294967;
    uptimeAdd += 4294967;
  }
  lastUptime = uptime;
  return uptime;
}

/**
 * Calculate internal ESP32 temperature
 */
#if defined(ESP32) && !defined(NO_INT_TEMP_READING)
float intTemperatureRead() {
  SET_PERI_REG_BITS(SENS_SAR_MEAS_WAIT2_REG, SENS_FORCE_XPD_SAR, 3, SENS_FORCE_XPD_SAR_S);
  SET_PERI_REG_BITS(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_CLK_DIV, 10, SENS_TSENS_CLK_DIV_S);
  CLEAR_PERI_REG_MASK(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_POWER_UP);
  CLEAR_PERI_REG_MASK(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_DUMP_OUT);
  SET_PERI_REG_MASK(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_POWER_UP_FORCE);
  SET_PERI_REG_MASK(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_POWER_UP);
  ets_delay_us(100);
  SET_PERI_REG_MASK(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_DUMP_OUT);
  ets_delay_us(5);
  float temp_f = (float)GET_PERI_REG_BITS2(SENS_SAR_SLAVE_ADDR3_REG, SENS_TSENS_OUT, SENS_TSENS_OUT_S);
  float temp_c = (temp_f - 32) / 1.8;
  return temp_c;
}
#endif

/*
 Erase flash and restart the ESP
*/
void erase(bool restart) {
#ifdef SecondaryModule
  // Erase the secondary module config
  String eraseCmdStr = "{\"cmd\":\"" + String(eraseCmd) + "\"}";
  Logger.notice(OMG_LOGID, F("Erasing secondary module config: %s" CR), eraseCmdStr.c_str());
  receivingDATA(subjectMQTTtoSYSsetSecondaryModule, eraseCmdStr.c_str());
  delay(2000);
#endif
  Logger.debug(OMG_LOGID, F("Formatting requested, result: %d" CR), SPIFFS.format());

#if defined(ESP8266)
  WiFi.disconnect(true);
#  ifndef ESPWifiManualSetup
  wifiManager.resetSettings();
#  endif
  delay(5000);
#else
  nvs_flash_erase();
#endif
  if (restart)
    ESPRestart(0);
}

String stateMeasures() {
  StaticJsonDocument<JSON_MSG_BUFFER> SYSdata;

  SYSdata["uptime"] = uptime();

  SYSdata["version"] = OMG_VERSION;
#ifdef LED_ADDRESSABLE
  SYSdata["rgbb"] = SYSConfig.rgbbrightness;
#endif
#if USE_BLUFI
  SYSdata["blufi"] = SYSConfig.blufi;
#endif
  SYSdata["mqtt"] = SYSConfig.mqtt;
  SYSdata["serial"] = SYSConfig.serial;
#ifdef OMG_MQTT_DISCOVERY
  SYSdata["disc"] = SYSConfig.discovery;
  SYSdata["ohdisc"] = SYSConfig.ohdiscovery;
#endif
  SYSdata["env"] = ENV_NAME;
  uint32_t freeMem;
  uint32_t minFreeMem;
  freeMem = ESP.getFreeHeap();
#ifdef OMG_GATEWAY_RTL_433
  // Some RTL_433 decoders have memory leak, this is a temporary workaround
  if (freeMem < MinimumMemory) {
    Logger.error(OMG_LOGID, F("Not enough memory %d, restarting" CR), freeMem);
    gatewayState = GatewayState::ERROR;
    ESPRestart(8);
  }
#endif
  SYSdata["freemem"] = freeMem;
#if !OMG_MQTT_BROKER_MODE
  SYSdata["mqttp"] = cnt_parameters_array[cnt_index].mqtt_port;
  SYSdata["mqtts"] = cnt_parameters_array[cnt_index].isConnectionSecure;
  SYSdata["mqttv"] = cnt_parameters_array[cnt_index].isCertValidate;
#endif
  SYSdata["msgprc"] = queueLengthSum;
  SYSdata["msgblck"] = blockedMessages;
  SYSdata["msgrcv"] = receivedMessages;
  SYSdata["maxq"] = maxQueueLength;
  SYSdata["cnt_index"] = cnt_index;
#ifdef ESP32
  minFreeMem = ESP.getMinFreeHeap();
  SYSdata["minmem"] = minFreeMem;
#  ifndef NO_INT_TEMP_READING
  SYSdata["tempc"] = TheengsUtils::round2(intTemperatureRead());
#  endif
  SYSdata["freestck"] = uxTaskGetStackHighWaterMark(NULL);
  SYSdata["powermode"] = SYSConfig.powerMode;
#endif

  SYSdata["eth"] = ethConnected;
  if (ethConnected) {
#ifdef ESP32_ETHERNET
    SYSdata["mac"] = (char*)ETH.macAddress().c_str();
    SYSdata["ip"] = TheengsUtils::ip2CharArray(ETH.localIP());
    ETH.fullDuplex() ? SYSdata["fd"] = (bool)"true" : SYSdata["fd"] = (bool)"false";
    SYSdata["linkspeed"] = (int)ETH.linkSpeed();
#endif
  } else {
    SYSdata["rssi"] = (long)WiFi.RSSI();
    SYSdata["SSID"] = (char*)WiFi.SSID().c_str();
    SYSdata["BSSID"] = (char*)WiFi.BSSIDstr().c_str();
    SYSdata["ip"] = TheengsUtils::ip2CharArray(WiFi.localIP());
    SYSdata["mac"] = (char*)WiFi.macAddress().c_str();
  }
#ifdef OMG_BOARD_M5STACK
  M5.Power.begin();
  SYSdata["m5battlevel"] = (int8_t)M5.Power.getBatteryLevel();
  SYSdata["m5ischarging"] = (bool)M5.Power.isCharging();
  SYSdata["m5ischargefull"] = (bool)M5.Power.isChargeFull();
#endif
#if defined(OMG_BOARD_M5STICKC) || defined(OMG_BOARD_M5STICKCP) || defined(OMG_BOARD_M5TOUGH)
  M5.Axp.EnableCoulombcounter();
  SYSdata["m5batvoltage"] = (float)M5.Axp.GetBatVoltage();
  SYSdata["m5batcurrent"] = (float)M5.Axp.GetBatCurrent();
  SYSdata["m5vinvoltage"] = (float)M5.Axp.GetVinVoltage();
  SYSdata["m5vincurrent"] = (float)M5.Axp.GetVinCurrent();
  SYSdata["m5vbusvoltage"] = (float)M5.Axp.GetVBusVoltage();
  SYSdata["m5vbuscurrent"] = (float)M5.Axp.GetVBusCurrent();
  SYSdata["m5tempaxp"] = (float)M5.Axp.GetTempInAXP192();
  SYSdata["m5batpower"] = (float)M5.Axp.GetBatPower();
  SYSdata["m5batchargecurrent"] = (float)M5.Axp.GetBatChargeCurrent();
  SYSdata["m5apsvoltage"] = (float)M5.Axp.GetAPSVoltage();
#endif
  SYSdata["modules"] = modules;

  SYSdata["origin"] = subjectSYStoMQTT;
  enqueueJsonObject(SYSdata);

  char jsonChar[100];
  serializeJson(modules, jsonChar, 99);

  String _modules = jsonChar;

  _modules.replace(",", ", ");
  _modules.replace("[", "");
  _modules.replace("]", "");
  _modules.replace("\"", "'");

  SYSdata["modules"] = _modules.c_str();

  String output;
  serializeJson(SYSdata, output);
  Logger.notice(OMG_LOGID, F("SYS json: %s" CR), output.c_str());
  return output;
}

#if defined(OMG_GATEWAY_RF) || defined(OMG_GATEWAY_IR) || defined(OMG_GATEWAY_SRFB) || defined(OMG_GATEWAY_WEATHERSTATION) || defined(OMG_GATEWAY_RTL_433)
/**
 * Store signal values from RF, IR, SRFB or Weather stations so as to avoid duplicates
 */
void storeSignalValue(uint64_t MQTTvalue) {
  unsigned long now = millis();
  // find oldest value of the buffer
  int o = getMin();
  Logger.debug(OMG_LOGID, F("Min ind: %d" CR), o);
  // replace it by the new one
  receivedSignal[o].value = MQTTvalue;
  receivedSignal[o].time = now;

  // Casting "receivedSignal[o].value" to (unsigned long) because ArduinoLog doesn't support uint64_t for ESP's
  Logger.debug(OMG_LOGID, F("store code : %u / %u" CR), (unsigned long)receivedSignal[o].value, receivedSignal[o].time);
  Logger.debug(OMG_LOGID, F("Col: val/timestamp" CR));
  for (int i = 0; i < struct_size; i++) {
    Logger.debug(OMG_LOGID, F("mem code : %u / %u" CR), (unsigned long)receivedSignal[i].value, receivedSignal[i].time);
  }
}

/**
 * get oldest time index from the values array from RF, IR, SRFB or Weather stations so as to avoid duplicates
 */
int getMin() {
  unsigned int minimum = receivedSignal[0].time;
  int minindex = 0;
  for (int i = 1; i < struct_size; i++) {
    if (receivedSignal[i].time < minimum) {
      minimum = receivedSignal[i].time;
      minindex = i;
    }
  }
  return minindex;
}

/**
 * Check if signal values from RF, IR, SRFB or Weather stations are duplicates
 */
bool isAduplicateSignal(uint64_t value) {
  Logger.debug(OMG_LOGID, F("isAdupl?" CR));
  for (int i = 0; i < struct_size; i++) {
    if (receivedSignal[i].value == value) {
      unsigned long now = millis();
      if (now - receivedSignal[i].time < time_avoid_duplicate) { // change
        Logger.debug(OMG_LOGID, F("no pub. dupl" CR));
        return true;
      }
    }
  }
  return false;
}
#endif

void receivingDATA(const char* topicOri, const char* datacallback) {
  std::string strTopicOri = topicOri;
  StaticJsonDocument<JSON_MSG_BUFFER_MAX> jsonBuffer;
  JsonObject jsondata = jsonBuffer.to<JsonObject>();
  DeserializationError error = deserializeJson(jsonBuffer, datacallback);
  if (error || jsondata.isNull()) {
    Logger.error(OMG_LOGID, F("deserialize MQTT data failed: %s" CR), error.c_str());
    gatewayState = GatewayState::ERROR;
    return;
  }
  if (topicOri == nullptr || strcmp(topicOri, "") == 0) {
    if (jsondata.containsKey("target") && jsondata["target"].is<const char*>()) {
      strTopicOri = jsondata["target"].as<const char*>();
      Logger.debug(OMG_LOGID, F("BUS Msg target: %s" CR), strTopicOri.c_str());
    } else if (jsondata.containsKey("origin") && jsondata["origin"].is<const char*>()) {
      strTopicOri = jsondata["origin"].as<const char*>();
      Logger.debug(OMG_LOGID, F("BUS Msg origin: %s" CR), strTopicOri.c_str());
    }
  } else {
#if defined(SecondaryModule) // Redirect certain commands to Serial
    String topicSerial = String(mqtt_topic) + String(g_gateway_name) + subjectMQTTtoSERIAL;
    const char* cSecondaryModule = SecondaryModule;
    if (strcmp(cSecondaryModule, "BT") == 0) {
      if (cmpToMainTopic(topicOri, subjectMQTTtoBTset)) {
        strTopicOri = topicSerial.c_str();
        jsondata["target"] = subjectMQTTtoBTset;
      } else if (cmpToMainTopic(topicOri, subjectMQTTtoBT)) {
        strTopicOri = topicSerial.c_str();
        jsondata["target"] = subjectMQTTtoBT;
      }
    }
    if (cmpToMainTopic(topicOri, subjectMQTTtoSYSsetSecondaryModule)) {
      strTopicOri = topicSerial.c_str();
      jsondata["target"] = subjectMQTTtoSYSsetSecondaryModule;
    }
#endif
    Logger.debug(OMG_LOGID, F("MQTT Msg topic: %s" CR), strTopicOri.c_str());
  }

#if defined(OMG_GATEWAY_RF) || defined(OMG_GATEWAY_IR) || defined(OMG_GATEWAY_SRFB) || defined(OMG_GATEWAY_WEATHERSTATION)
  if (strstr(strTopicOri.c_str(), subjectMultiGTWKey) != NULL) { // storing received value so as to avoid publishing this value if it has been already sent by this or another OpenMQTTGateway
    uint64_t data = jsondata.isNull() ? strtoull(datacallback, NULL, 10) : jsondata["value"];
    if (data != 0 && !isAduplicateSignal(data)) {
      storeSignalValue(data);
    }
  }
#endif

  if (!jsondata.isNull()) { // json object ok -> json decoding
    // log the received json
    String buffer = "";
    serializeJson(jsondata, buffer);
    //Logger.notice(OMG_LOGID, F("[ MQTT->OMG ]: %s" CR), buffer.c_str());

#ifdef OMG_GATEWAY_PILIGHT // OMG_GATEWAY_PILIGHT is only defined with json publishing due to its numerous parameters
    XtoPilight(strTopicOri.c_str(), jsondata);
#endif
#if defined(OMG_GATEWAY_RTL_433) || defined(OMG_GATEWAY_PILIGHT) || defined(OMG_GATEWAY_RF) || defined(OMG_GATEWAY_RF2) || defined(OMG_ACTUATOR_SOMFY)
    XtoRFset(strTopicOri.c_str(), jsondata);
#endif
#if jsonReceiving
#  ifdef OMG_GATEWAY_LORA
    XtoLORA(strTopicOri.c_str(), jsondata);
#  endif
#  ifdef OMG_GATEWAY_RF
    XtoRF(strTopicOri.c_str(), jsondata);
#  endif
#  ifdef OMG_GATEWAY_RF2
    XtoRF2(strTopicOri.c_str(), jsondata);
#  endif
#  ifdef OMG_GATEWAY_2G
    Xto2G(strTopicOri.c_str(), jsondata);
#  endif
#  ifdef OMG_GATEWAY_SRFB
    XtoSRFB(strTopicOri.c_str(), jsondata);
#  endif
#  ifdef OMG_GATEWAY_IR
    XtoIR(strTopicOri.c_str(), jsondata);
#  endif
#  ifdef OMG_GATEWAY_RFM69
    XtoRFM69(strTopicOri.c_str(), jsondata);
#  endif
#  ifdef OMG_GATEWAY_BT
    XtoBT(strTopicOri.c_str(), jsondata);
#  endif
#  ifdef OMG_ACTUATOR_FASTLED
    XtoFASTLED(strTopicOri.c_str(), jsondata);
#  endif
#  ifdef OMG_ACTUATOR_PWM
    XtoPWM(strTopicOri.c_str(), jsondata);
#  endif
#  if defined(OMG_BOARD_M5STICKC) || defined(OMG_BOARD_M5STICKCP) || defined(OMG_BOARD_M5STACK) || defined(OMG_BOARD_M5TOUGH)
    XtoM5(strTopicOri.c_str(), jsondata);
#  endif
#  if defined(OMG_DISPLAY_SSD1306)
    XtoSSD1306(strTopicOri.c_str(), jsondata);
#  endif
#  ifdef OMG_ACTUATOR_ONOFF
    XtoONOFF(strTopicOri.c_str(), jsondata);
#  endif
#  ifdef OMG_ACTUATOR_SOMFY
    XtoSomfy(strTopicOri.c_str(), jsondata);
#  endif
#  ifdef OMG_GATEWAY_SERIAL
    XtoSERIAL(strTopicOri.c_str(), jsondata);
#  endif
#  ifdef OMG_MQTT_HTTPS_FW_UPDATE
    MQTTHttpsFWUpdate(strTopicOri.c_str(), jsondata);
#  endif
#  if defined(ZwebUI) && defined(ESP32)
    XtoWebUI(strTopicOri.c_str(), jsondata);
#  endif
#endif

    XtoSYS(strTopicOri.c_str(), jsondata);
  } else { // not a json object --> simple decoding
#if simpleReceiving
#  ifdef OMG_GATEWAY_LORA
    XtoLORA(strTopicOri.c_str(), datacallback);
#  endif
#  ifdef OMG_GATEWAY_RF
    XtoRF(strTopicOri.c_str(), datacallback);
#  endif
#  ifdef OMG_GATEWAY_RF315
    XtoRF315(strTopicOri.c_str(), datacallback);
#  endif
#  ifdef OMG_GATEWAY_RF2
    XtoRF2(strTopicOri.c_str(), datacallback);
#  endif
#  ifdef OMG_GATEWAY_2G
    Xto2G(strTopicOri.c_str(), datacallback);
#  endif
#  ifdef OMG_GATEWAY_SRFB
    XtoSRFB(strTopicOri.c_str(), datacallback);
#  endif
#  ifdef OMG_GATEWAY_RFM69
    XtoRFM69(strTopicOri.c_str(), datacallback);
#  endif
#  ifdef OMG_ACTUATOR_FASTLED
    XtoFASTLED(strTopicOri.c_str(), datacallback);
#  endif
#endif
#ifdef OMG_ACTUATOR_ONOFF
    XtoONOFF(strTopicOri.c_str(), datacallback);
#endif
  }
}

#if OMG_MQTT_HTTPS_FW_UPDATE
String latestVersion;
#  ifdef ESP32
#    include <HTTPClient.h>

#    include "zzHTTPUpdate.h"

#    if CHECK_OTA_UPDATE
/**
 * Check on a server the latest version information to build a releaseLink
 * The release link will be used when the user trigger an OTA update command
 * Only available for ESP32
 */
bool checkForUpdates() {
  Logger.notice(OMG_LOGID, F("Update check, free heap: %d"), ESP.getFreeHeap());
  HTTPClient http;
  http.setTimeout((GeneralTimeOut - 1) * 1000); // -1 to avoid WDT
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  std::string ota_cert;
#      if !OMG_MQTT_BROKER_MODE
  if (cnt_parameters_array[cnt_index].ota_server_cert.length() > MIN_CERT_LENGTH) {
    Logger.notice(OMG_LOGID, F("Using memory cert" CR));
    ota_cert = cnt_parameters_array[cnt_index].ota_server_cert;
  } else
#      endif
  {
    Logger.notice(OMG_LOGID, F("Using config cert" CR));
    ota_cert = OTAserver_cert;
  }

  http.begin(OTA_JSON_URL, ota_cert.c_str());
  int httpCode = http.GET();
  StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject jsondata = jsonBuffer.to<JsonObject>();

  if (httpCode > 0) { //Check for the returning code
    String payload = http.getString();
    auto error = deserializeJson(jsonBuffer, payload);
    if (error) {
      Logger.error(OMG_LOGID, F("Deserialize MQTT data failed: %s" CR), error.c_str());
      gatewayState = GatewayState::ERROR;
    }
    Logger.debug(OMG_LOGID, F("HttpCode %d" CR), httpCode);
    Logger.debug(OMG_LOGID, F("Payload %s" CR), payload.c_str());
  } else {
    Logger.error(OMG_LOGID, F("Error on HTTP request"));
    gatewayState = GatewayState::ERROR;
  }
  http.end(); //Free the resources
  Logger.notice(OMG_LOGID, F("Update check done, free heap: %d"), ESP.getFreeHeap());
  if (jsondata.containsKey("latest_version")) {
    jsondata["installed_version"] = OMG_VERSION;
    jsondata["entity_picture"] = ENTITY_PICTURE;
    if (!jsondata.containsKey("release_summary"))
      jsondata["release_summary"] = "";
    latestVersion = jsondata["latest_version"].as<String>();
    jsondata["origin"] = subjectRLStoMQTT;
    jsondata["retain"] = true;
    enqueueJsonObject(jsondata);

    Logger.debug(OMG_LOGID, F("Update file found on server" CR));
    return true;
  } else {
    Logger.debug(OMG_LOGID, F("No update file found on server" CR));
    return false;
  }
}

#    else
bool checkForUpdates() {
  return false;
}
#    endif
#  elif ESP8266
#    include <ESP8266httpUpdate.h>
#  endif

void MQTTHttpsFWUpdate(const char* topicOri, JsonObject& HttpsFwUpdateData) {
  if (strstr(topicOri, subjectMQTTtoSYSupdate) != NULL) {
    const char* version = HttpsFwUpdateData["version"] | "latest";
    if (version && ((strlen(version) != strlen(OMG_VERSION)) || strcmp(version, OMG_VERSION) != 0)) {
      const char* url = HttpsFwUpdateData["url"];
      String systemUrl;
      if (url) {
        if (!strstr((url + (strlen(url) - 5)), ".bin")) {
          Logger.error(OMG_LOGID, F("Invalid firmware extension" CR));
          gatewayState = GatewayState::ERROR;
          return;
        }
#  if OMG_MQTT_HTTPS_FW_UPDATE_USE_PASSWORD > 0
        const char* pwd = HttpsFwUpdateData["password"];
        if (pwd) {
          if (strcmp(pwd, g_ota_pass) != 0) {
            Logger.error(OMG_LOGID, F("Invalid OTA password" CR));
            gatewayState = GatewayState::ERROR;
            return;
          }
        } else {
          Logger.error(OMG_LOGID, F("No password sent" CR));
          gatewayState = GatewayState::ERROR;
          return;
        }
#  endif
#  ifdef ESP32
      } else if (strcmp(version, "latest") == 0) {
        systemUrl = RELEASE_LINK + latestVersion + "/" + ENV_NAME + "-firmware.bin";
        url = systemUrl.c_str();
        Logger.notice(OMG_LOGID, F("Using system OTA url with latest version %s" CR), url);
      } else if (strcmp(version, "dev") == 0) {
        systemUrl = String(RELEASE_LINK_DEV) + ENV_NAME + "-firmware.bin";
        url = systemUrl.c_str();
        Logger.notice(OMG_LOGID, F("Using system OTA url with dev version %s" CR), url);
      } else if (version[0] == 'v') {
        systemUrl = String(RELEASE_LINK) + version + "/" + ENV_NAME + "-firmware.bin";
        url = systemUrl.c_str();
        Logger.notice(OMG_LOGID, F("Using system OTA url with defined version %s" CR), url);
#  endif
      } else {
        Logger.error(OMG_LOGID, F("Invalid URL" CR));
        gatewayState = GatewayState::ERROR;
        return;
      }
#  ifdef ESP32
      ProcessLock = true;
#    ifdef OMG_GATEWAY_BT
      stopProcessing();
#    endif
#  endif
      Logger.warning(OMG_LOGID, F("Starting firmware update" CR));
      gatewayState = GatewayState::REMOTE_OTA_IN_PROGRESS;

      StaticJsonDocument<JSON_MSG_BUFFER> jsondata;
      jsondata["release_summary"] = "Update in progress ...";
      jsondata["origin"] = subjectRLStoMQTT;
      enqueueJsonObject(jsondata);

      std::string ota_cert = TheengsUtils::processCert(HttpsFwUpdateData["ota_server_cert"] | "");
      Logger.notice(OMG_LOGID, F("OTA cert: %s" CR), ota_cert.c_str());
      if (ota_cert.length() < MIN_CERT_LENGTH && !strstr(url, "http:")) {
#  if !OMG_MQTT_BROKER_MODE
        if (cnt_parameters_array[cnt_index].ota_server_cert.length() > MIN_CERT_LENGTH) {
          Logger.notice(OMG_LOGID, F("Using memory cert" CR));
          ota_cert = cnt_parameters_array[cnt_index].ota_server_cert.c_str();
        } else
#  endif
        {
          Logger.notice(OMG_LOGID, F("Using config cert" CR));
          ota_cert = OTAserver_cert;
        }
      }

      t_httpUpdate_return result = HTTP_UPDATE_FAILED;
      if (strstr(url, "http:")) {
        Logger.notice(OMG_LOGID, F("Http update" CR));
        WiFiClient update_client;
#  ifdef ESP32
        httpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        result = httpUpdate.update(update_client, url);
#  elif ESP8266
        ESPhttpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        result = ESPhttpUpdate.update(update_client, url);
#  endif

      } else {
        WiFiClientSecure update_client;
#  if !OMG_MQTT_BROKER_MODE
        if (cnt_parameters_array[cnt_index].isConnectionSecure) {
          mqtt.reset();
          mqttSetupPending = true;
          update_client = *static_cast<WiFiClientSecure*>(eClient.get());
        } else
#  endif
        {
          TheengsUtils::syncNTP();
        }

#  ifdef ESP32
        update_client.setCACert(ota_cert.c_str());
        update_client.setTimeout(12);
        httpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        httpUpdate.rebootOnUpdate(false);
        result = httpUpdate.update(update_client, url);
#  elif ESP8266
        caCert.append(ota_cert.c_str());
        update_client.setTrustAnchors(&caCert);
        update_client.setTimeout(12000);
        ESPhttpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        ESPhttpUpdate.rebootOnUpdate(false);
        result = ESPhttpUpdate.update(update_client, url);
#  endif
      }

      switch (result) {
        case HTTP_UPDATE_FAILED:
#  ifdef ESP32
          Logger.error(OMG_LOGID, F("HTTP_UPDATE_FAILED Error (%d): %s\n" CR), httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
#  elif ESP8266
          Logger.error(OMG_LOGID, F("HTTP_UPDATE_FAILED Error (%d): %s\n" CR), ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
#  endif
          gatewayState = GatewayState::ERROR;
          break;

        case HTTP_UPDATE_NO_UPDATES:
          Logger.notice(OMG_LOGID, F("HTTP_UPDATE_NO_UPDATES" CR));
          break;

        case HTTP_UPDATE_OK:
          Logger.notice(OMG_LOGID, F("HTTP_UPDATE_OK" CR));
          jsondata["release_summary"] = "Update success !";
          jsondata["installed_version"] = latestVersion;
          jsondata["origin"] = subjectRLStoMQTT;
          enqueueJsonObject(jsondata);
#  if !OMG_MQTT_BROKER_MODE
          if (cnt_index != 0) // We don't enable the change of cert provided at build time
            cnt_parameters_array[cnt_index].ota_server_cert = ota_cert;
#  endif
#  ifndef ESPWifiManualSetup
          saveConfig();
#  endif
          ESPRestart(6);
          break;
      }

      ESPRestart(6);
    }
  }
}
#endif // OMG_MQTT_HTTPS_FW_UPDATE

#ifdef OMG_LOCAL_OTA_FW_UPDATE
#ifdef ESP32
#  include <HTTPClient.h>
#  include "zzHTTPUpdate.h"
#else
#error Platform not supported yet
#endif

void MQTTHttpsFWUpdate(const char* topicOri, JsonObject& HttpsFwUpdateData) {
  if (strstr(topicOri, subjectMQTTtoSYSupdate) == NULL) {
    return;
  }

  const char* version = HttpsFwUpdateData["version"];
  if (!version || (strcmp(version, OMG_VERSION) == 0)) {
    return;
  }

  const char* url = HttpsFwUpdateData["url"];
  String systemUrl;
  if (!url) {
    Logger.error(OMG_LOGID, F("No firmware URL specified" CR));
    gatewayState = GatewayState::ERROR;
    return;
  }

  if (!strstr((url + (strlen(url) - 5)), ".bin")) {
    Logger.error(OMG_LOGID, F("Invalid firmware extension" CR));
    gatewayState = GatewayState::ERROR;
    return;
  }

  ProcessLock = true;

#    ifdef OMG_GATEWAY_BT
  stopProcessing();
#    endif

  Logger.warning(OMG_LOGID, F("Starting firmware update" CR));
  gatewayState = GatewayState::REMOTE_OTA_IN_PROGRESS;

  StaticJsonDocument<JSON_MSG_BUFFER> jsondata;
  jsondata["release_summary"] = "Update in progress ...";
  jsondata["origin"] = subjectRLStoMQTT;
  enqueueJsonObject(jsondata);

  t_httpUpdate_return result = HTTP_UPDATE_FAILED;
  WiFiClient update_client;
  httpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  result = httpUpdate.update(update_client, url);

  switch (result) {
    case HTTP_UPDATE_FAILED:
      Logger.error(OMG_LOGID, F("HTTP_UPDATE_FAILED Error (%d): %s\n" CR), httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
      gatewayState = GatewayState::ERROR;
      break;

    case HTTP_UPDATE_NO_UPDATES:
      Logger.notice(OMG_LOGID, F("HTTP_UPDATE_NO_UPDATES" CR));
      break;

    case HTTP_UPDATE_OK:
      Logger.notice(OMG_LOGID, F("HTTP_UPDATE_OK" CR));
      jsondata["release_summary"] = "Update success !";
      jsondata["installed_version"] = version;
      jsondata["origin"] = subjectRLStoMQTT;
      enqueueJsonObject(jsondata);

#  ifndef ESPWifiManualSetup
      saveConfig();
#  endif

      ESPRestart(6);
      break;
  }

  ESPRestart(6);
}

/**
 * Check on a server the latest available version of firmware. If the version
 * is different from our version, an upgrade is triggered.
 */
bool checkForUpdates() {
  HTTPClient http;
  int httpResponseCode = -1;
  const String uri_latest = OMG_LOCAL_OTA_BASE_URI "latest";
  String payload = "";

  Logger.notice(OMG_LOGID, F("Update check, free heap: %d"), ESP.getFreeHeap());
  http.setTimeout((GeneralTimeOut - 1) * 1000); // -1 to avoid WDT
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.addHeader("Accept", "text/plain");

  http.begin(uri_latest);
  httpResponseCode = http.GET();
  Logger.debug(OMG_LOGID, F("HTTP Response Code: %d" CR), httpResponseCode);

  if (httpResponseCode > 0) {
    payload = http.getString();
    Logger.debug(OMG_LOGID, F("Payload: %s" CR), payload.c_str());
  } else {
    Logger.error(OMG_LOGID, F("Error %d on GET request for \"%s\""), httpResponseCode, uri_latest.c_str());
    gatewayState = GatewayState::ERROR;
  }
  http.end(); //Free the resources
  Logger.notice(OMG_LOGID, F("Update check done, free heap: %d"), ESP.getFreeHeap());

  // Firmware filename format: "${PIOENV}-${UNIX_TIME}-firmware.bin"

  int first_dash_ix = -1;
  int second_dash_ix = -1;

  first_dash_ix = payload.indexOf('-');
  if (first_dash_ix >= 0) {
    second_dash_ix = payload.indexOf('-', first_dash_ix + 1);
  }

  String pioenv = "";
  String version = "";
  String suffix = "";

  if ((first_dash_ix >= 0) && (second_dash_ix >= 0)) {
    pioenv = payload.substring(0, first_dash_ix);
    version = payload.substring(first_dash_ix + 1, second_dash_ix);
    suffix = payload.substring(second_dash_ix + 1);
  }
  Logger.debug(OMG_LOGID, F("Pioenv: %s" CR), pioenv.c_str());
  Logger.debug(OMG_LOGID, F("Version: %s" CR), version.c_str());
  Logger.debug(OMG_LOGID, F("Suffix: %s" CR), suffix.c_str());

  // Check that the filename is valid
  if ((pioenv != ENV_NAME || suffix != "firmware.bin")) {
    Logger.warning(OMG_LOGID, F("Invalid update file \"%s\" found on server" CR), payload.c_str());
    return false;
  }

  const String url_firmware = OMG_LOCAL_OTA_BASE_URI + payload;

  // Advertise the update we found, if the version is different
  // from the current version
  if (version != OMG_VERSION) {
    StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
    JsonObject jsondata = jsonBuffer.to<JsonObject>();

    jsondata["origin"] = subjectRLStoMQTT;
    jsondata["retain"] = true;
    jsondata["installed_version"] = OMG_VERSION;
    jsondata["version"] = version;
    jsondata["url"] = url_firmware;
    jsondata["release_summary"] = "Update file found on server";
    enqueueJsonObject(jsondata);

#ifdef OMG_LOCAL_OTA_AUTOMATIC

    Logger.debug(OMG_LOGID, F("Update file found on server, upgrading" CR));

    MQTTHttpsFWUpdate(subjectMQTTtoSYSupdate, jsondata);

#else // OMG_LOCAL_OTA_AUTOMATIC

    Logger.debug(OMG_LOGID, F("Update file found on server" CR));

#endif // OMG_LOCAL_OTA_AUTOMATIC

    return true;
  } else {
    Logger.debug(OMG_LOGID, F("No update file found on server" CR));
    return false;
  }
}

#endif // OMG_LOCAL_OTA_FW_UPDATE

#if !OMG_MQTT_BROKER_MODE
/**
 * Read the certificates from the memory and publish a hash of the cert to the broker for identification purposes
*/
void readCntParameters(int index) {
  if (index < 0 || index > 2) {
    Logger.warning(OMG_LOGID, F("Invalid cnt index" CR));
    return;
  }
  StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject jsondata = jsonBuffer.to<JsonObject>();
  jsondata["cnt_index"] = index;
  jsondata["valid_cnt"] = cnt_parameters_array[index].validConnection;
  if (cnt_parameters_array[index].server_cert.length() > MIN_CERT_LENGTH) {
    jsondata["mqtt_server_cert_hash"] = generateHash(cnt_parameters_array[index].server_cert);
  }
  if (cnt_parameters_array[index].client_cert.length() > MIN_CERT_LENGTH) {
    jsondata["mqtt_client_cert_hash"] = generateHash(cnt_parameters_array[index].client_cert);
  }
  if (cnt_parameters_array[index].client_key.length() > MIN_CERT_LENGTH) {
    jsondata["mqtt_client_key_hash"] = generateHash(cnt_parameters_array[index].client_key);
  }
  if (cnt_parameters_array[index].ota_server_cert.length() > MIN_CERT_LENGTH) {
    jsondata["ota_server_cert_hash"] = generateHash(cnt_parameters_array[index].ota_server_cert);
  }
  jsondata["mqtt_server"] = cnt_parameters_array[index].mqtt_server;
  jsondata["mqtt_port"] = cnt_parameters_array[index].mqtt_port;
  jsondata["mqtt_user"] = cnt_parameters_array[index].mqtt_user;
  jsondata["mqtt_pass"] = generateHash(cnt_parameters_array[index].mqtt_pass);
  jsondata["mqtt_secure"] = cnt_parameters_array[index].isConnectionSecure;
  jsondata["mqtt_validate"] = cnt_parameters_array[index].isCertValidate;
  jsondata["origin"] = subjectSYStoMQTT;
  enqueueJsonObject(jsondata);
}
#endif

void XtoSYS(const char* topicOri, JsonObject& SYSdata) { // json object decoding
  if (cmpToMainTopic(topicOri, subjectMQTTtoSYSset)) {
    bool restartESP = false;
    bool publishState = false;
    Logger.debug(OMG_LOGID, F("MQTTtoSYS json" CR));
    if (SYSdata.containsKey("cmd")) {
      const char* cmd = SYSdata["cmd"];
      Logger.notice(OMG_LOGID, F("Command: %s" CR), cmd);
      if (strstr(cmd, restartCmd) != NULL) { //restart
        ESPRestart(5);
      } else if (strstr(cmd, eraseCmd) != NULL) { //erase and restart
        erase(true);
      } else if (strstr(cmd, statusCmd) != NULL) {
        publishState = true;
      }
    }
#ifdef LED_ADDRESSABLE
    if (SYSdata.containsKey("rgbb") && SYSdata["rgbb"].is<float>()) {
      if (SYSdata["rgbb"] >= 0 && SYSdata["rgbb"] <= 255) {
        SYSConfig.rgbbrightness = TheengsUtils::round2(SYSdata["rgbb"]);
        ledManager.setBrightness(SYSConfig.rgbbrightness);
#  ifdef OMG_ACTUATOR_ONOFF
        updatePowerIndicator();
#  endif
        Logger.notice(OMG_LOGID, F("RGB brightness: %d" CR), SYSConfig.rgbbrightness);
        publishState = true;
      } else {
        Logger.warning(OMG_LOGID, F("RGB brightness value invalid - ignoring command" CR));
      }
    }
#endif
#ifdef OMG_MQTT_DISCOVERY
    if (SYSdata.containsKey("ohdisc") && SYSdata["ohdisc"].is<bool>()) {
      SYSConfig.ohdiscovery = SYSdata["ohdisc"];
      Logger.notice(OMG_LOGID, F("OpenHAB discovery: %T" CR), SYSConfig.ohdiscovery);
      publishState = true;
    }
#endif
    if (SYSdata.containsKey("wifi_ssid") && SYSdata["wifi_ssid"].is<const char*>() && SYSdata.containsKey("wifi_pass") && SYSdata["wifi_pass"].is<const char*>()) {
#ifdef ESP32
      ProcessLock = true;
#  ifdef OMG_GATEWAY_BT
      stopProcessing();
#  endif
#endif
      String prev_ssid = WiFi.SSID();
      String prev_pass = WiFi.psk();
      // NOTE: There's no need to disconnect MQTT manually
      WiFi.disconnect(true);

      Logger.warning(OMG_LOGID, F("Attempting connection to new AP %s" CR), (const char*)SYSdata["wifi_ssid"]);
      WiFi.begin((const char*)SYSdata["wifi_ssid"], (const char*)SYSdata["wifi_pass"]);
#if defined(WifiGMode) || defined(WifiPower)
      setESPWifiProtocolTxPower();
#endif
      WiFi.waitForConnectResult(WiFi_TimeOut * 1000);

      if (WiFi.status() != WL_CONNECTED) {
        Logger.warning(OMG_LOGID, F("Failed to connect to new AP; falling back" CR));
        WiFi.disconnect(true);
        WiFi.begin(prev_ssid.c_str(), prev_pass.c_str());
#if defined(WifiGMode) || defined(WifiPower)
        setESPWifiProtocolTxPower();
#endif
      }
      restartESP = true;
    }

    if ((SYSdata.containsKey("mqtt_topic") && SYSdata["mqtt_topic"].is<const char*>()) ||
#ifdef OMG_MQTT_DISCOVERY
        (SYSdata.containsKey("discovery_prefix") && SYSdata["discovery_prefix"].is<const char*>()) ||
#endif
#if OMG_LOG_TO_SYSLOG
        (SYSdata.containsKey("syslog_server") && SYSdata["syslog_server"].is<const char*>()) ||
        (SYSdata.containsKey("syslog_port") && SYSdata["syslog_port"].is<const char*>()) ||
#endif
        (SYSdata.containsKey("gateway_name") && SYSdata["gateway_name"].is<const char*>()) ||
        (SYSdata.containsKey("gw_pass") && SYSdata["gw_pass"].is<const char*>())) {
      if (SYSdata.containsKey("mqtt_topic")) {
        strncpy(mqtt_topic, SYSdata["mqtt_topic"], parameters_size);
      }
#ifdef OMG_MQTT_DISCOVERY
      if (SYSdata.containsKey("discovery_prefix")) {
        strncpy(discovery_prefix, SYSdata["discovery_prefix"], parameters_size);
      }
#endif
      if (SYSdata.containsKey("gateway_name")) {
        strncpy(g_gateway_name, SYSdata["gateway_name"], parameters_size);
      }
      if (SYSdata.containsKey("gw_pass")) {
        strncpy(g_ota_pass, SYSdata["gw_pass"], parameters_size);
        restartESP = true;
      }
#if OMG_LOG_TO_SYSLOG
      if (SYSdata.containsKey("syslog_server")) {
        strncpy(g_syslog_server, SYSdata["syslog_server"], parameters_size);
        restartESP = true;
      }
      if (SYSdata.containsKey("syslog_port")) {
        strncpy(g_syslog_port, SYSdata["syslog_port"], parameters_size);
        restartESP = true;
      }
#endif
#ifndef ESPWifiManualSetup
      saveConfig();
#endif
      mqttSetupPending = true; // trigger reconnect in loop using the new topic/name
    }

#if !OMG_MQTT_BROKER_MODE
#  ifdef MQTTsetMQTT

    bool save_cnt = false;
    bool read_cnt = false;
    bool test_cnt = false;

    if (SYSdata.containsKey("save_cnt") && SYSdata["save_cnt"].is<bool>()) {
      save_cnt = SYSdata["save_cnt"].as<bool>();
    }
    if (SYSdata.containsKey("read_cnt") && SYSdata["read_cnt"].is<bool>()) {
      read_cnt = SYSdata["read_cnt"].as<bool>();
    }
    if (SYSdata.containsKey("test_cnt") && SYSdata["test_cnt"].is<bool>()) {
      test_cnt = SYSdata["test_cnt"].as<bool>();
    }

    if (SYSdata.containsKey("cnt_index") && SYSdata["cnt_index"].is<int>()) {
      if (SYSdata["cnt_index"].as<int>() < 0 || SYSdata["cnt_index"].as<int>() > 2) {
        Logger.warning(OMG_LOGID, F("Invalid cnt index provided - ignoring command" CR));
        return;
      }

      // we're overwrittng parameters, create a backup to be able to revert
      cnt_parameters_backup.reset(new ss_cnt_parameters_backup());
      cnt_parameters_backup->cnt_index = cnt_index;
      cnt_parameters_backup->saveOnSuccess = save_cnt;

      cnt_index = SYSdata["cnt_index"].as<int>();
      cnt_parameters_backup->parameters = cnt_parameters_array[cnt_index];

      Logger.notice(OMG_LOGID, F("MQTT cnt index %d" CR), cnt_index);

      if (SYSdata.containsKey("mqtt_user") && SYSdata["mqtt_user"].is<const char*>() && SYSdata.containsKey("mqtt_pass") && SYSdata["mqtt_pass"].is<const char*>()) {
        strcpy(cnt_parameters_array[cnt_index].mqtt_user, SYSdata["mqtt_user"]);
        strcpy(cnt_parameters_array[cnt_index].mqtt_pass, SYSdata["mqtt_pass"]);
        cnt_parameters_array[cnt_index].validConnection = false;
      }

      if (SYSdata.containsKey("mqtt_server") && SYSdata["mqtt_server"].is<const char*>()) {
        strcpy(cnt_parameters_array[cnt_index].mqtt_server, SYSdata["mqtt_server"]);
        cnt_parameters_array[cnt_index].validConnection = false;
      }

      if (SYSdata.containsKey("mqtt_port") && SYSdata["mqtt_port"].is<const char*>()) {
        strcpy(cnt_parameters_array[cnt_index].mqtt_port, SYSdata["mqtt_port"]);
        cnt_parameters_array[cnt_index].validConnection = false;
      }

      if (SYSdata.containsKey("mqtt_secure") && SYSdata["mqtt_secure"].is<bool>()) {
        cnt_parameters_array[cnt_index].isConnectionSecure = SYSdata["mqtt_secure"].as<bool>();
        cnt_parameters_array[cnt_index].validConnection = false;
      }

      if (SYSdata.containsKey("mqtt_validate") && SYSdata["mqtt_validate"].is<bool>()) {
        cnt_parameters_array[cnt_index].isCertValidate = SYSdata["mqtt_validate"].as<bool>();
        cnt_parameters_array[cnt_index].validConnection = false;
      }

      // Copy the certs to the memory
      if (SYSdata.containsKey("mqtt_server_cert") && SYSdata["mqtt_server_cert"].is<const char*>()) {
        cnt_parameters_array[cnt_index].server_cert = TheengsUtils::processCert(SYSdata["mqtt_server_cert"].as<const char*>());
        Logger.debug(OMG_LOGID, F("Assigning server cert %s" CR), generateHash(cnt_parameters_array[cnt_index].server_cert).c_str());
        cnt_parameters_array[cnt_index].validConnection = false;
      }
      if (SYSdata.containsKey("mqtt_client_cert") && SYSdata["mqtt_client_cert"].is<const char*>()) {
        cnt_parameters_array[cnt_index].client_cert = TheengsUtils::processCert(SYSdata["mqtt_client_cert"].as<const char*>());
        Logger.debug(OMG_LOGID, F("Assigning client cert %s" CR), generateHash(cnt_parameters_array[cnt_index].client_cert).c_str());
        cnt_parameters_array[cnt_index].validConnection = false;
      }
      if (SYSdata.containsKey("mqtt_client_key") && SYSdata["mqtt_client_key"].is<const char*>()) {
        cnt_parameters_array[cnt_index].client_key = TheengsUtils::processCert(SYSdata["mqtt_client_key"].as<const char*>());
        Logger.debug(OMG_LOGID, F("Assigning client key %s" CR), generateHash(cnt_parameters_array[cnt_index].client_key).c_str());
        cnt_parameters_array[cnt_index].validConnection = false;
      }
      if (SYSdata.containsKey("ota_server_cert") && SYSdata["ota_server_cert"].is<const char*>()) {
        cnt_parameters_array[cnt_index].ota_server_cert = TheengsUtils::processCert(SYSdata["ota_server_cert"].as<const char*>());
        Logger.debug(OMG_LOGID, F("Assigning OTA server cert %s" CR), generateHash(cnt_parameters_array[cnt_index].ota_server_cert).c_str());
      }

      // Read the memory certs hash to MQTT
      if (read_cnt)
        readCntParameters(cnt_index);
    }

    // Change of mqtt secure connection or certs
    void* prev_client = nullptr;

    if (save_cnt || test_cnt) {
      // Stop the processing/disconnect
#    ifdef ESP32
      ProcessLock = true;
#      ifdef OMG_GATEWAY_BT
      stopProcessing();
#      endif
#    endif

      mqttSetupPending = true;
    } else {
      // no request to test or save the parameters, forget the old settings
      cnt_parameters_backup.reset();
    }
#  endif
#endif

    if (SYSdata.containsKey("mqtt") && SYSdata["mqtt"].is<bool>()) {
      SYSConfig.mqtt = SYSdata["mqtt"];
      Logger.notice(OMG_LOGID, F("xtomqtt: %T" CR), SYSConfig.mqtt);
    }
    if (SYSdata.containsKey("serial") && SYSdata["serial"].is<bool>()) {
      SYSConfig.serial = SYSdata["serial"];
      Logger.notice(OMG_LOGID, F("SERIAL: %T" CR), SYSConfig.serial);
    }
    if (SYSdata.containsKey("offline") && SYSdata["offline"].is<bool>()) {
      SYSConfig.offline = SYSdata["offline"];
      Logger.notice(OMG_LOGID, F("offline: %T" CR), SYSConfig.offline);
      if (SYSConfig.offline) {
        gatewayState = GatewayState::OFFLINE;
// Disconnect MQTT
#if !OMG_MQTT_BROKER_MODE
        mqtt->disconnect();
#else
        mqtt->stop();
#endif
        // Disconnect network
        if (WiFi.status() == WL_CONNECTED)
          WiFi.disconnect(true);
      }
    }
    if (SYSdata.containsKey("powermode") && SYSdata["powermode"].is<int>()) {
      SYSConfig.powerMode = SYSdata["powermode"];
      Logger.notice(OMG_LOGID, F("Power mode: %d" CR), SYSConfig.powerMode);
    }
#if USE_BLUFI
    if (SYSdata.containsKey("blufi") && SYSdata["blufi"].is<bool>()) {
      bool res = false;
      if (SYSdata["blufi"] && !SYSConfig.blufi) { // Start Blufi
        res = startBlufi();
      } else if (!SYSdata["blufi"] && SYSConfig.blufi) { // Stop Blufi
        res = stopBlufi();
      }
      if (res)
        SYSConfig.blufi = SYSdata["blufi"];
      publishState = true;
      Logger.notice(OMG_LOGID, F("Blufi: %T" CR), SYSConfig.blufi);
    }
#endif
    if (SYSdata.containsKey("disc")) {
      if (SYSdata["disc"].is<bool>()) {
        if (SYSdata["disc"] == true && SYSConfig.discovery == false)
          lastDiscovery = millis();
        SYSConfig.discovery = SYSdata["disc"];
        publishState = true;
        if (SYSConfig.discovery)
          pubMqttDiscovery();
      } else {
        Logger.warning(OMG_LOGID, F("Discovery command not a boolean" CR));
      }
      Logger.notice(OMG_LOGID, F("Discovery state: %T" CR), SYSConfig.discovery);
    }
    if (SYSdata.containsKey("save") && SYSdata["save"].as<bool>()) {
      SYSConfig_save();
    }
    if (publishState) {
      stateMeasures();
    }
    if (restartESP) {
      ESPRestart(7);
    }
  }
}
