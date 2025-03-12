/*
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation

   Act as a gateway between your 433mhz, infrared IR, BLE, LoRa signal and one interface like an MQTT broker
   Send and receiving command by MQTT

  This gateway enables to:
 - receive MQTT data from a topic and send RF 433Mhz signal corresponding to the received MQTT data
 - publish MQTT data to a different topic related to received 433Mhz signal
 - leverage the rtl_433 device decoders on a ESP32 device

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
#ifdef OMG_GATEWAY_RTL_433

#  include <ArduinoJson.h>
#  include <config_RF.h>
#  include <rtl_433_ESP.h>

#  include "Elog.h"
#  include "User_config.h"
#  ifdef OMG_MQTT_DISCOVERY
#    include "config_mqttDiscovery.h"
#  endif

char messageBuffer[JSON_MSG_BUFFER];

#  ifdef OMG_MQTT_DISCOVERY
SemaphoreHandle_t semaphorecreateOrUpdateDeviceRTL_433;
std::vector<RTL_433device*> RTL_433devices;
int newRTL_433Devices = 0;

static RTL_433device NO_RTL_433_DEVICE_FOUND = {{0},
                                                0,
                                                false};

RTL_433device* getDeviceById(const char* id); // Declared here to avoid pre-compilation issue (misplaced auto declaration by pio)
RTL_433device* getDeviceById(const char* id) {
  DISCOVERY_TRACE_LOG(F("getDeviceById %s"), id);

  for (std::vector<RTL_433device*>::iterator it = RTL_433devices.begin(); it != RTL_433devices.end(); ++it) {
    if ((strcmp((*it)->uniqueId, id) == 0)) {
      return *it;
    }
  }
  return &NO_RTL_433_DEVICE_FOUND;
}

void dumpRTL_433Devices() {
  for (std::vector<RTL_433device*>::iterator it = RTL_433devices.begin(); it != RTL_433devices.end(); ++it) {
    RTL_433device* p = *it;
    DISCOVERY_TRACE_LOG(F("uniqueId %s"), p->uniqueId);
    DISCOVERY_TRACE_LOG(F("modelName %s"), p->modelName);
    DISCOVERY_TRACE_LOG(F("type %s"), p->type);
    DISCOVERY_TRACE_LOG(F("isDisc %d"), p->isDisc);
  }
}

void createOrUpdateDeviceRTL_433(const char* id, const char* model, const char* type, uint8_t flags) {
  if (xSemaphoreTake(semaphorecreateOrUpdateDeviceRTL_433, pdMS_TO_TICKS(30000)) == pdFALSE) {
    Logger.error(OMG_LOGID, F("[rtl_433] semaphorecreateOrUpdateDeviceRTL_433 Semaphore NOT taken"));
    return;
  }

  RTL_433device* device = getDeviceById(id);
  if (device == &NO_RTL_433_DEVICE_FOUND) {
    DISCOVERY_TRACE_LOG(F("add %s"), id);
    //new device
    device = new RTL_433device();
    if (strlcpy(device->uniqueId, id, uniqueIdSize) > uniqueIdSize) {
      Logger.warning(OMG_LOGID, F("[rtl_433] Device id %s exceeds available space"), id); // Remove from production release ?
    };
    if (strlcpy(device->modelName, model, modelNameSize) > modelNameSize) {
      Logger.warning(OMG_LOGID, F("[rtl_433] Device model %s exceeds available space"), model); // Remove from production release ?
    };
    if (strlcpy(device->type, type, typeSize) > typeSize) {
      Logger.warning(OMG_LOGID, F("[rtl_433] Device type %s exceeds available space"), type); // Remove from production release ?
    }
    DISCOVERY_TRACE_LOG(F("[rtl_433] Device type is %s."), device->type); // Remove from production release ?
    device->isDisc = flags & device_flags_isDisc;
    RTL_433devices.push_back(device);
    newRTL_433Devices++;
  } else {
    DISCOVERY_TRACE_LOG(F("update %s"), id);

    if (flags & device_flags_isDisc) {
      device->isDisc = true;
    }
  }

  xSemaphoreGive(semaphorecreateOrUpdateDeviceRTL_433);
}

// This function always should be called from the main core as it generates direct mqtt messages
// When overrideDiscovery=true, we publish discovery messages of known RTL_433devices (even if no new)
void launchRTL_433Discovery(bool overrideDiscovery) {
  if (!overrideDiscovery && newRTL_433Devices == 0)
    return;
  if (xSemaphoreTake(semaphorecreateOrUpdateDeviceRTL_433, pdMS_TO_TICKS(QueueSemaphoreTimeOutLoop)) == pdFALSE) {
    Logger.error(OMG_LOGID, F("[rtl_433] semaphorecreateOrUpdateDeviceRTL_433 Semaphore NOT taken"));
    return;
  }
  newRTL_433Devices = 0;
  std::vector<RTL_433device*> localDevices = RTL_433devices;
  xSemaphoreGive(semaphorecreateOrUpdateDeviceRTL_433);
  for (std::vector<RTL_433device*>::iterator it = localDevices.begin(); it != localDevices.end(); ++it) {
    RTL_433device* pdevice = *it;
    DISCOVERY_TRACE_LOG(F("Device id %s"), pdevice->uniqueId);
    // Do not launch discovery for the RTL_433devices already discovered (unless we have overrideDiscovery) or that are not unique by their MAC Address (Ibeacon, GAEN and Microsoft Cdp)
    if (overrideDiscovery || !isDiscovered(pdevice)) {
      size_t numRows = sizeof(parameters) / sizeof(parameters[0]);
      for (int i = 0; i < numRows; i++) {
        char deviceKeyParameter[25];
        memcpy(deviceKeyParameter, &pdevice->uniqueId[strlen(pdevice->uniqueId) - strlen(parameters[i][0])], strlen(parameters[i][0]));
        deviceKeyParameter[strlen(parameters[i][0])] = '\0';
        Logger.debug(OMG_LOGID, F("deviceKeyParameter: %s"), deviceKeyParameter);

        if (strcmp(deviceKeyParameter, parameters[i][0]) == 0) {
          // Remove the key from the unique id to extract the device id
          String idWoKey = pdevice->uniqueId;
          idWoKey.remove(idWoKey.length() - (strlen(parameters[i][0]) + 1));
          DISCOVERY_TRACE_LOG(F("idWoKey %s"), idWoKey.c_str());
          String value_template = "";
          if (SYSConfig.ohdiscovery) {
            value_template = "{{ value_json." + String(parameters[i][0]) + " }}";
          } else {
            value_template = "{{ value_json." + String(parameters[i][0]) + " | is_defined }}";
          }
          String topic = subjectRTL_433toMQTT;
#    if OMG_MQTT_VALUE_AS_A_TOPIC
          // Remove the key from the unique id to extract the device id
          String idWoKeyAndModel = idWoKey;
          if (strcmp(pdevice->type, "null")) {
            idWoKeyAndModel.remove(0, (strlen(pdevice->modelName) + strlen(pdevice->type) + 1)); // type is present
            topic = topic + "/" + String(pdevice->type) + "/" + String(pdevice->modelName);
          } else {
            idWoKeyAndModel.remove(0, (strlen(pdevice->modelName)));
            topic = topic + "/" + String(pdevice->modelName);
          }
          DISCOVERY_TRACE_LOG(F("idWoKeyAndModel %s"), idWoKeyAndModel.c_str());
          idWoKeyAndModel.replace("-", "/");
          topic = topic + idWoKeyAndModel;
#    endif
          if (strcmp(parameters[i][0], "battery_ok") == 0) {
            if (strcmp(pdevice->modelName, "Govee-Water") == 0 || strcmp(pdevice->modelName, "Govee-Contact") == 0 || strcmp(pdevice->modelName, "Archos-TBH") == 0 || strcmp(pdevice->modelName, "FineOffset-WH31L") == 0 || strcmp(pdevice->modelName, "Fineoffset-WH45") == 0 || strcmp(pdevice->modelName, "Fineoffset-WN34") == 0 || strcmp(pdevice->modelName, "Fineoffset-WS80") == 0 || strcmp(pdevice->modelName, "Fineoffset-WH0290") == 0 || strcmp(pdevice->modelName, "Fineoffset-WH51") == 0 || strcmp(pdevice->modelName, "Kedsum-TH") == 0 || strcmp(pdevice->modelName, "AVE") == 0 || strcmp(pdevice->modelName, "TPMS") == 0) {
              if (SYSConfig.ohdiscovery) {
                value_template = "{{ (value_json." + String(parameters[i][0]) + " * 100) | round(0)}}";
              } else {
                value_template = "{{ (float(value_json." + String(parameters[i][0]) + ") * 100) | round(0) | is_defined }}";
              }
              createDiscovery("sensor", //set Type
                              (char*)topic.c_str(), parameters[i][1], pdevice->uniqueId, //set state_topic,name,uniqueId
                              "", parameters[i][3], (char*)value_template.c_str(), //set availability_topic,device_class,value_template,
                              "", "", "%", //set,payload_on,payload_off,unit_of_meas,
                              0, //set  off_delay
                              "", "", false, "", //set,payload_available,payload_not available   ,is a gateway entity, command topic
                              (char*)idWoKey.c_str(), "", pdevice->modelName, (char*)idWoKey.c_str(), false, // device name, device manufacturer, device model, device ID, retain
                              stateClassMeasurement //State Class
              );
            } else {
              createDiscovery("binary_sensor", //set Type
                              (char*)topic.c_str(), parameters[i][1], pdevice->uniqueId, //set state_topic,name,uniqueId
                              "", parameters[i][3], (char*)value_template.c_str(), //set availability_topic,device_class,value_template,
                              "0", "1", "", //set,payload_on,payload_off,unit_of_meas,
                              0, //set  off_delay
                              "", "", false, "", //set,payload_available,payload_not available   ,is a gateway entity, command topic
                              (char*)idWoKey.c_str(), "", pdevice->modelName, (char*)idWoKey.c_str(), false, // device name, device manufacturer, device model, device ID, retain
                              "" //State Class
              );
            }
          } else if (strcmp(parameters[i][0], "tamper") == 0 || strcmp(parameters[i][0], "alarm") == 0 || strcmp(parameters[i][0], "motion") == 0) {
            createDiscovery("binary_sensor", //set Type
                            (char*)topic.c_str(), parameters[i][1], pdevice->uniqueId, //set state_topic,name,uniqueId
                            "", parameters[i][3], (char*)value_template.c_str(), //set availability_topic,device_class,value_template,
                            "1", "0", parameters[i][2], //set,payload_on,payload_off,unit_of_meas,
                            0, //set  off_delay
                            "", "", false, "", //set,payload_available,payload_not available   ,is a gateway entity, command topic
                            (char*)idWoKey.c_str(), "", pdevice->modelName, (char*)idWoKey.c_str(), false, // device name, device manufacturer, device model, device ID, retain
                            "" //State Class
            );
          } else if (strcmp(parameters[i][0], "state") == 0 && (strcmp(pdevice->modelName, "Nexa-Security") == 0 || strcmp(pdevice->modelName, "Brennenstuhl-RCS2044") == 0 || strcmp(pdevice->modelName, "Proove-Security") == 0 || strcmp(pdevice->modelName, "Waveman-Switch") == 0)) {
            createDiscovery("binary_sensor", //set Type
                            (char*)topic.c_str(), parameters[i][1], pdevice->uniqueId, //set state_topic,name,uniqueId
                            "", parameters[i][3], (char*)value_template.c_str(), //set availability_topic,device_class,value_template,
                            "ON", "OFF", parameters[i][2], //set,payload_on,payload_off,unit_of_meas,
                            0, //set  off_delay
                            "", "", false, "", //set,payload_available,payload_not available   ,is a gateway entity, command topic
                            (char*)idWoKey.c_str(), "", pdevice->modelName, (char*)idWoKey.c_str(), false, // device name, device manufacturer, device model, device ID, retain
                            "" //State Class
            );
          } else if (strcmp(parameters[i][0], "strike_count") == 0) {
            createDiscovery("sensor", //set Type
                            (char*)topic.c_str(), parameters[i][1], pdevice->uniqueId, //set state_topic,name,uniqueId
                            "", parameters[i][3], (char*)value_template.c_str(), //set availability_topic,device_class,value_template,
                            "1", "0", parameters[i][2], //set,payload_on,payload_off,unit_of_meas,
                            0, //set  off_delay
                            "", "", false, "", //set,payload_available,payload_not available   ,is a gateway entity, command topic
                            (char*)idWoKey.c_str(), "", pdevice->modelName, (char*)idWoKey.c_str(), false, // device name, device manufacturer, device model, device ID, retain
                            stateClassTotalIncreasing //State Class
            );
          } else if (strcmp(parameters[i][0], "event") == 0 && strcmp(pdevice->modelName, "Govee-Water") == 0) { //the entity will detect Water Leak Event and go back to Off state after 60seconds
            createDiscovery("binary_sensor", //set Type
                            (char*)topic.c_str(), parameters[i][1], pdevice->uniqueId, //set state_topic,name,uniqueId
                            "", parameters[i][3], (char*)value_template.c_str(), //set availability_topic,device_class,value_template,
                            "Water Leak", "", parameters[i][2], //set,payload_on,payload_off,unit_of_meas,
                            60, //set  off_delay
                            "", "", false, "", //set,payload_available,payload_not available   ,is a gateway entity, command topic
                            (char*)idWoKey.c_str(), "Govee", pdevice->modelName, (char*)idWoKey.c_str(), false, // device name, device manufacturer, device model, device ID, retain
                            stateClassMeasurement //State Class
            );
          } else if (strcmp(pdevice->modelName, "Interlogix-Security") != 0) {
            createDiscovery("sensor", //set Type
                            (char*)topic.c_str(), parameters[i][1], pdevice->uniqueId, //set state_topic,name,uniqueId
                            "", parameters[i][3], (char*)value_template.c_str(), //set availability_topic,device_class,value_template,
                            "", "", parameters[i][2], //set,payload_on,payload_off,unit_of_meas,
                            0, //set  off_delay
                            "", "", false, "", //set,payload_available,payload_not available   ,is a gateway entity, command topic
                            (char*)idWoKey.c_str(), "", pdevice->modelName, (char*)idWoKey.c_str(), false, // device name, device manufacturer, device model, device ID, retain
                            stateClassMeasurement //State Class
            );
          }
          pdevice->isDisc = true; // we don't need the semaphore and all the search magic via createOrUpdateDevice
          dumpRTL_433Devices();
          break;
        }
      }
      if (!pdevice->isDisc) {
        DISCOVERY_TRACE_LOG(F("Device id %s was not discovered"), pdevice->uniqueId); // Remove from production release ?
      }
    } else {
      DISCOVERY_TRACE_LOG(F("Device already discovered or that doesn't require discovery %s"), pdevice->uniqueId);
    }
  }
}

void storeRTL_433Discovery(JsonObject& RFrtl_433_ESPdata, const char* model, const char* type, const char* uniqueid) {
  //Sanitize model name
  String modelSanitized = model;
  modelSanitized.replace(" ", "_");
  modelSanitized.replace("/", "_");
  modelSanitized.replace(".", "_");
  modelSanitized.replace("&", "");

  //Sensors translation matrix for sensors that requires statistics by using stateClassMeasurement
  size_t numRows = sizeof(parameters) / sizeof(parameters[0]);

  for (int i = 0; i < numRows; i++) {
    if (RFrtl_433_ESPdata.containsKey(parameters[i][0])) {
      String key_id = String(uniqueid) + "-" + String(parameters[i][0]);
      createOrUpdateDeviceRTL_433((char*)key_id.c_str(), (char*)modelSanitized.c_str(), (char*)type, device_flags_init);
    }
  }
}
#  endif

void rtl_433_Callback(char* message) {
  DynamicJsonDocument jsonBuffer2(JSON_MSG_BUFFER);
  JsonObject RFrtl_433_ESPdata = jsonBuffer2.to<JsonObject>();
  auto error = deserializeJson(jsonBuffer2, message);
  if (error) {
    Logger.error(OMG_LOGID, F("[rtl_433] deserializeJson() failed: %s"), error.c_str());
    return;
  }

  unsigned long MQTTvalue = (int)RFrtl_433_ESPdata["id"] + round((float)RFrtl_433_ESPdata["temperature_C"]);
  String model = RFrtl_433_ESPdata["model"];
  String protocol = RFrtl_433_ESPdata["protocol"];
  String type = RFrtl_433_ESPdata["type"];
  String topic;
  String uniqueid;

  // Special handling for status reports
  if (model == "status" && protocol == "rtl_433_ESP status message") {
    topic = subjectcommonRFtoMQTT;
  } else {
    topic = subjectRTL_433toMQTT;
  }

  const char naming_keys[5][8] = {"type", "model", "subtype", "channel", "id"}; // from rtl_433_mqtt_hass.py
  size_t numRows = sizeof(naming_keys) / sizeof(naming_keys[0]);
  for (int i = 0; i < numRows; i++) {
    if (RFrtl_433_ESPdata.containsKey(naming_keys[i])) {
      if (uniqueid == 0) {
        uniqueid = RFrtl_433_ESPdata[naming_keys[i]].as<String>(); // Start of the unique id with the first key
      } else {
        uniqueid = uniqueid + "/" + RFrtl_433_ESPdata[naming_keys[i]].as<String>(); // Following keys
      }
    }
  }

#  if OMG_MQTT_VALUE_AS_A_TOPIC
  topic = topic + "/" + uniqueid;
#  endif

  uniqueid.replace("/", "-");

  DISCOVERY_TRACE_LOG(F("uniqueid: %s"), uniqueid.c_str());
  if (!isAduplicateSignal(MQTTvalue)) {
#  ifdef OMG_MQTT_DISCOVERY
    if (SYSConfig.discovery)
      storeRTL_433Discovery(RFrtl_433_ESPdata, (char*)model.c_str(), (char*)type.c_str(), (char*)uniqueid.c_str());
#  endif
    RFrtl_433_ESPdata["origin"] = (char*)topic.c_str();
    enqueueJsonObject(RFrtl_433_ESPdata);
    storeSignalValue(MQTTvalue);
  }
#  ifdef MEMORY_DEBUG
  Logger.debug(OMG_LOGID, F("Post rtl_433_Callback: %d"), ESP.getFreeHeap());
#  endif
}

void setupRTL_433() {
  rtl_433.setCallback(rtl_433_Callback, messageBuffer, JSON_MSG_BUFFER);
#  ifdef OMG_MQTT_DISCOVERY
  semaphorecreateOrUpdateDeviceRTL_433 = xSemaphoreCreateBinary();
  xSemaphoreGive(semaphorecreateOrUpdateDeviceRTL_433);
#  endif
  Logger.debug(OMG_LOGID, F("ZgatewayRTL_433 command topic: %s%s%s"), mqtt_topic, g_gateway_name, subjectMQTTtoRFset);
  Logger.notice(OMG_LOGID, F("ZgatewayRTL_433 setup done "));
}

void RTL_433Loop() {
  rtl_433.loop();
}

extern void enableRTLreceive() {
  Logger.notice(OMG_LOGID, F("Enable RTL_433 Receiver: %FMhz"), RFConfig.frequency);
  rtl_433.initReceiver(RTL433_RF_MODULE_RECEIVER_GPIO, RFConfig.frequency);
  rtl_433.enableReceiver();
}

extern void disableRTLreceive() {
  Logger.debug(OMG_LOGID, F("disableRTLreceive"));
  rtl_433.disableReceiver();
}

extern int getRTLrssiThreshold() {
  return rtl_433.rssiThreshold;
}

extern int getRTLrssiThresholdDelta() {
  return rtl_433.rssiThresholdDelta;
}

extern int getRTLAverageRSSI() {
  return rtl_433.averageRssi;
}

extern int getRTLCurrentRSSI() {
  return rtl_433.currentRssi;
}

extern int getRTLMessageCount() {
  return rtl_433.messageCount;
}

extern int getRTLPulseTrainsOverruns() {
  int ret = rtl_433.pulseTrainsOverruns;
  return ret;
}

extern void resetRTLPulseTrainsOverruns() {
  rtl_433.pulseTrainsOverruns = 0 ;
}

extern int getRTLDecoderQueueOverflows() {
  int ret = rtl_433.decoderQueueOverflows;
  return ret;
}

extern void resetRTLDecoderQueueOverflows() {
  rtl_433.decoderQueueOverflows = 0;
}

#  ifdef OMG_RADIO_SX127X
extern int getOOKFixedThreshold() {
  return rtl_433.ookFixedThreshold;
}
#  endif

#endif
