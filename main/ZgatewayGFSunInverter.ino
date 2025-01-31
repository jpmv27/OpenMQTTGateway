/*
  Theengs OpenMQTTGateway - We Unite Sensors in One Open-Source Interface

   Act as a wifi or ethernet gateway between your GridFree SUN-2000G inverter and a MQTT broker.
   Send inverter metrics by MQTT.

  This gateway enables to:
  - publish MQTT data, which are received by RS232 ModBus.

    Library for read metrics from GWL Power, GridFree SUN-2000G inverter <https://shop.gwl.eu/GridFree-Inverters/GridFree-AC-Inverter-with-limiter-2kW-SUN-2000G-45-90V.html>.

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

#ifdef OMG_GATEWAY_GFSUNINVERTER

GfSun2000 GF = GfSun2000();

void GFSunInverterDataHandler(GfSun2000Data data) {
  StaticJsonDocument<JSON_MSG_BUFFER> jdataBuffer;
  JsonObject jdata = jdataBuffer.to<JsonObject>();

  jdata["device_id"] = (char*)data.deviceID;
  Logger.debug(OMG_LOGID, F("Device ID     : %s\n"), data.deviceID);
  jdata["ac_voltage"] = data.ACVoltage;
  Logger.debug(OMG_LOGID, F("AC Voltage    : %.1f\tV\n"), data.ACVoltage);
  jdata["dc_voltage"] = data.DCVoltage;
  Logger.debug(OMG_LOGID, F("DC Voltage    : %.1f\tV\n"), data.DCVoltage);
  jdata["power"] = data.averagePower;
  Logger.debug(OMG_LOGID, F("Output Power  : %.1f\tW (5min avg)\n"), data.averagePower);
  jdata["c_energy"] = data.customEnergyCounter;
  Logger.debug(OMG_LOGID, F("Custom Energy : %.1f\tkW/h (can be reseted)\n"), data.customEnergyCounter);
  jdata["t_energy"] = data.totalEnergyCounter;
  Logger.debug(OMG_LOGID, F("Total Energy  : %.1f\tkW/h\n"), data.totalEnergyCounter);

#  ifdef GFSUNINVERTER_DEVEL
  StaticJsonDocument<JSON_MSG_BUFFER> jregisterBuffer;
  JsonObject jregister = jregisterBuffer.to<JsonObject>();
  char buffer[4];
  std::map<int16_t, int16_t>::iterator itr;
  for (itr = data.modbusRegistry.begin(); itr != data.modbusRegistry.end(); ++itr) {
    Logger.notice(OMG_LOGID, "%d: %d\n", itr->first, itr->second);
    sprintf(buffer, "%d", itr->first);
    jregister[buffer] = itr->second;
  }
  jdata["register"] = jregister;
#  endif
  jdata["origin"] = subjectRFtoMQTT;
  enqueueJsonObject(jdata);
}

void GFSunInverterErrorHandler(int errorId, char* errorMessage) {
  char buffer[50];
  sprintf(buffer, "Error response: %02X - %s\n", errorId, errorMessage);
  Logger.error(OMG_LOGID, buffer);
  StaticJsonDocument<JSON_MSG_BUFFER> jdataBuffer;
  JsonObject jdata = jdataBuffer.to<JsonObject>();
  jdata["status"] = "error";
  jdata["msg"] = errorMessage;
  jdata["id"] = errorId;
  jdata["origin"] = subjectRFtoMQTT;
  enqueueJsonObject(jdata);
}

void setupGFSunInverter() {
  GF.setup(Serial2);
  GF.setDataHandler(GFSunInverterDataHandler);
  GF.setErrorHandler(GFSunInverterErrorHandler);
  Logger.debug(OMG_LOGID, F("ZgatewayGFSunInverter setup done "));
}

void ZgatewayGFSunInverterMQTT() {
  GF.readData();
  delay(GFSUNINVERTER_DELAY);
}

#endif
