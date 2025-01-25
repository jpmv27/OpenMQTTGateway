/*  
  Theengs OpenMQTTGateway - We Unite Sensors in One Open-Source Interface

   Act as a gateway between your 433mhz, infrared IR, BLE, LoRa signal and one interface like an MQTT broker 
   Send and receiving command by MQTT
 
    DHT reading Addon
  
    Copyright: (c)Florian ROBERT
    
    Contributors:
    - prahjister
    - 1technophile
  
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

#ifdef OMG_SENSOR_DHT
#  include <DHT.h>
#  include <DHT_U.h>

DHT dht(OMG_DHT_DATA_GPIO, OMG_DHT_SENSOR_TYPE);

//Time used to wait for an interval before resending temp and hum
unsigned long timedht = 0;

void setupDHT() {
  Logger.notice(OMG_LOGID, F("Reading DHT on pin: %d" CR), OMG_DHT_DATA_GPIO);
}

void MeasureTempAndHum() {
  if (millis() > (timedht + OMG_DHT_TIME_BTW_READINGS)) { //retrieving value of temperature and humidity of the box from DHT every xUL
    timedht = millis();
    static float persistedh;
    static float persistedt;
    float h = dht.readHumidity();
    // Read temperature as Celsius (the default)
    float t = dht.readTemperature();
    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t)) {
      Logger.error(OMG_LOGID, F("Failed to read from DHT sensor!" CR));
    } else {
      Logger.debug(OMG_LOGID, F("Creating DHT buffer" CR));
      StaticJsonDocument<JSON_MSG_BUFFER> DHTdataBuffer;
      JsonObject DHTdata = DHTdataBuffer.to<JsonObject>();
      if (h != persistedh || OMG_DHT_ALWAYS_SEND) {
        DHTdata["humidity_pct"] = (float)h;
      } else {
        Logger.debug(OMG_LOGID, F("Same hum don't send it" CR));
      }
      if (t != persistedt || OMG_DHT_ALWAYS_SEND) {
        DHTdata["temperature_c"] = (float)t;
#if !OMG_DHT_METRIC_UNITS_ONLY
        DHTdata["temperature_f"] = dht.convertCtoF(t);
#endif
      } else {
        Logger.debug(OMG_LOGID, F("Same temp don't send it" CR));
      }
      DHTdata["origin"] = OMG_MQTT_DHT_TOPIC;
      enqueueJsonObject(DHTdata);
    }
    persistedh = h;
    persistedt = t;
  }
}
#endif
