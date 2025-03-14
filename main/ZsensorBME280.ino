/*
  OpenMQTTGateway Addon  - ESP8266 or Arduino program for home automation

   Act as a gateway between your 433mhz, infrared IR, BLE, LoRa signal and one interface like an MQTT broker
   Send and receiving command by MQTT

   This is the Climate Addon:
   - Measures Temperature, Humidity and Pressure
   - Generates Values for: Temperature in degrees C and F, Humidity in %, Pressure in Pa, Altitude in Meter and Feet
   - Required Hardware Module: Bosch BME280/BMP280
   - Required Library: SparkFun BME280 Library v1.1.0 by Marshall Taylor

   Connection Schemata:
   --------------------

   BME280/BMP280 ------> ESP8266
   =====================================================
   Vcc ----------------> 5V/3.3V    (5V or 3.3V depends on the BME280/BMP280 board variant)
   GND ----------------> GND
   SCL ----------------> D1
   SDA ----------------> D2

    Copyright: (c) Hans-Juergen Dinges

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

#ifdef OMG_SENSOR_BME280
#  include <stdint.h>

#  include "SparkFunBME280.h"
#  include "Wire.h" // Library for communication with I2C / TWI devices

//Global sensor object
BME280 mySensor;
bool initialized = false; // Sensor successfully initialized?

void setupZsensorBME280() {
  // Allow custom pins on ESP Platforms
  Wire.begin(OMG_BME280_PIN_SDA, OMG_BME280_PIN_SCL);

  mySensor.settings.commInterface = I2C_MODE;
  mySensor.settings.I2CAddress = OMG_BME280_I2C_ADDRESS;
  Logger.notice(OMG_LOGID, F("Setup BME280/BMP280 on address: 0x%02X"), OMG_BME280_I2C_ADDRESS);
  //***Operation settings*****************************//

  // runMode Setting - Values:
  // -------------------------
  //  0, Sleep mode
  //  1 or 2, Forced mode
  //  3, Normal mode
  mySensor.settings.runMode = 3; //Normal mode

  // tStandby Setting - Values:
  // --------------------------
  //  0, 0.5ms
  //  1, 62.5ms
  //  2, 125ms
  //  3, 250ms
  //  4, 500ms
  //  5, 1000ms
  //  6, 10ms
  //  7, 20ms
  mySensor.settings.tStandby = 1;

  // Filter can be off or number of FIR coefficients - Values:
  // ---------------------------------------------------------
  //  0, filter off
  //  1, coefficients = 2
  //  2, coefficients = 4
  //  3, coefficients = 8
  //  4, coefficients = 16
  mySensor.settings.filter = OMG_BME280_FIR_FILTER_COEFFS;

  // tempOverSample - Values:
  // ------------------------
  //  0, skipped
  //  1 through 5, oversampling *1, *2, *4, *8, *16 respectively
  mySensor.settings.tempOverSample = OMG_BME280_TEMPERATURE_OVERSAMPLING;

  // pressOverSample - Values:
  // -------------------------
  //  0, skipped
  //  1 through 5, oversampling *1, *2, *4, *8, *16 respectively
  mySensor.settings.pressOverSample = OMG_BME280_PRESSURE_OVERSAMPLING;

  // humidOverSample - Values:
  // -------------------------
  //  0, skipped
  //  1 through 5, oversampling *1, *2, *4, *8, *16 respectively
  mySensor.settings.humidOverSample = OMG_BME280_HUMIDITY_OVERSAMPLING;

  // tempCorrection - Correction in celcius of temperature reported by BME280/BMP280 sensor. Both Celcius and Farenheit temperatures are adjusted.
  // -------------------------
  // Value is a float
  // ie Compiler Directive '-DOMG_BME280_TEMPERATURE_CORRECTION=-3.4'
  mySensor.settings.tempCorrection = OMG_BME280_TEMPERATURE_CORRECTION;

  delay(10); // Gives the Sensor enough time to turn on (The BME280/BMP280 requires 2ms to start up)

  int ret = mySensor.begin();
  if (ret == 0x60) {
    Logger.notice(OMG_LOGID, F("Bosch BME280 successfully initialized: 0x%02X"), ret);
    initialized = true;
  } else if (ret == 0x58) {
    Logger.notice(OMG_LOGID, F("Bosch BMP280 successfully initialized: 0x%02X"), ret);
    initialized = true;
  } else {
    Logger.error(OMG_LOGID, F("Bosch BME280/BMP280 (address 0x%02X) initialization failed: 0x%02X"), OMG_BME280_I2C_ADDRESS, ret);
  }
}

void MeasureTempHumAndPressure() {
  if (millis() > (timebme280 + OMG_BME280_TIME_BTW_READINGS)) {
    timebme280 = millis();
    static float persisted_bme_tempc;
    static float persisted_bme_hum;
    static float persisted_bme_pa;
    static float persisted_bme_altim;
#if !OMG_BME280_METRIC_UNITS_ONLY
    static float persisted_bme_tempf;
    static float persisted_bme_altift;
#endif

    if (!initialized) {
      Logger.notice(OMG_LOGID, F("BME280/BMP280 not initialized, skip reading"));
      return;
    }

    float BmeTempC = mySensor.readTempC();
    float BmeHum = mySensor.readFloatHumidity();
    float BmePa = mySensor.readFloatPressure();
    float BmeAltiM = mySensor.readFloatAltitudeMeters();
#if !OMG_BME280_METRIC_UNITS_ONLY
    float BmeAltiFt = mySensor.readFloatAltitudeFeet();
    float BmeTempF = mySensor.readTempF();
#endif

    Logger.debug(OMG_LOGID, F("Creating BME280/BMP280 buffer"));
    StaticJsonDocument<JSON_MSG_BUFFER> BME280dataBuffer;
    JsonObject BME280data = BME280dataBuffer.to<JsonObject>();
    // Generate Temperature in degrees C
    if (BmeTempC != persisted_bme_tempc || OMG_BME280_ALWAYS_SEND) {
      BME280data["temperature_c"] = (float)BmeTempC;
    } else {
      Logger.debug(OMG_LOGID, F("Same Degrees C don't send it"));
    }

    // Generate Humidity in percent
    if (BmeHum != persisted_bme_hum || OMG_BME280_ALWAYS_SEND) {
      BME280data["humidity_pct"] = (float)BmeHum;
    } else {
      Logger.debug(OMG_LOGID, F("Same Humidity don't send it"));
    }

    // Generate Pressure in Pa
    if (BmePa != persisted_bme_pa || OMG_BME280_ALWAYS_SEND) {
      BME280data["pressure_pa"] = (float)BmePa;
    } else {
      Logger.debug(OMG_LOGID, F("Same Pressure don't send it"));
    }

    // Generate Altitude in Meter
    if (BmeAltiM != persisted_bme_altim || OMG_BME280_ALWAYS_SEND) {
      Logger.debug(OMG_LOGID, F("Sending Altitude Meter to MQTT"));
      BME280data["altitude_m"] = (float)BmeAltiM;
    } else {
      Logger.debug(OMG_LOGID, F("Same Altitude Meter don't send it"));
    }

#if !OMG_BME280_METRIC_UNITS_ONLY
    // Generate Temperature in degrees F
    if (BmeTempF != persisted_bme_tempf || OMG_BME280_ALWAYS_SEND) {
      BME280data["temperature_f"] = (float)BmeTempF;
    } else {
      Logger.debug(OMG_LOGID, F("Same Degrees F don't send it"));
    }

    // Generate Altitude in Feet
    if (BmeAltiFt != persisted_bme_altift || OMG_BME280_ALWAYS_SEND) {
      BME280data["altitude_ft"] = (float)BmeAltiFt;
    } else {
      Logger.debug(OMG_LOGID, F("Same Altitude Feet don't send it"));
    }
#endif

    BME280data["origin"] = OMG_MQTT_BME_TOPIC;
    enqueueJsonObject(BME280data);

    persisted_bme_tempc = BmeTempC;
    persisted_bme_hum = BmeHum;
    persisted_bme_pa = BmePa;
    persisted_bme_altim = BmeAltiM;
#if !OMG_BME280_METRIC_UNITS_ONLY
    persisted_bme_tempf = BmeTempF;
    persisted_bme_altift = BmeAltiFt;
#endif
  }
}

#endif
