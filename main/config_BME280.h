/*
  Theengs OpenMQTTGateway - We Unite Sensors in One Open-Source Interface

   Act as a gateway between your 433mhz, infrared IR, BLE, LoRa signal and one interface like an MQTT broker 
   Send and receiving command by MQTT

   This files enables you to set parameters for the BME280 or BMP280 sensors.

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

   Connection Schemata:
   --------------------

   BME280/BMP280 ------> ESP8266
   ==============================
   Vcc ----------------> 5V/3.3V    (5V or 3.3V depends on the BME280/BMP280 board variant)
   GND ----------------> GND
   SCL ----------------> D1
   SDA ----------------> D2

*/
#ifndef OMG_CONFIG_BME280_H
#define OMG_CONFIG_BME280_H

extern void setupZsensorBME280();
extern void MeasureTempHumAndPressure();

#ifndef OMG_BME280_ALWAYS_SEND
#  define OMG_BME280_ALWAYS_SEND true // if false when the current value of the parameter is the same as previous one don't send it by MQTT
#endif
#ifndef OMG_BME280_TIME_BTW_READINGS
#  define OMG_BME280_TIME_BTW_READINGS 30000
#endif

/*----------------------------USER PARAMETERS-----------------------------*/
/*-------------DEFINE YOUR MQTT PARAMETERS BELOW----------------*/
#ifndef OMG_MQTT_BME_TOPIC
#  define OMG_MQTT_BME_TOPIC "/CLIMAtoMQTT/bme"
#endif

//Time used to wait for an interval before resending measured values
unsigned long timebme280 = 0;

#ifndef OMG_BME280_I2C_ADDRESS
#  define OMG_BME280_I2C_ADDRESS 0x76
#endif

int BME280_i2c_addr = OMG_BME280_I2C_ADDRESS; // Bosch BME280 I2C Address

// Only supported for ESP
#ifndef OMG_BME280_PIN_SDA
#  define OMG_BME280_PIN_SDA SDA
#endif
#ifndef OMG_BME280_PIN_SCL
#  define OMG_BME280_PIN_SCL SCL
#endif

// Oversampling for BME280/BMP280 devices

#ifndef OMG_BME280_TEMPERATURE_OVERSAMPLING
// OMG_BME280_TEMPERATURE_OVERSAMPLING - Values:
// ------------------------
//  0, skipped
//  1 through 5, oversampling *1, *2, *4, *8, *16 respectively
#  define OMG_BME280_TEMPERATURE_OVERSAMPLING 1
#endif

#ifndef OMG_BME280_PRESSURE_OVERSAMPLING
// OMG_BME280_PRESSURE_OVERSAMPLING - Values:
// -------------------------
//  0, skipped
//  1 through 5, oversampling *1, *2, *4, *8, *16 respectively
#  define OMG_BME280_PRESSURE_OVERSAMPLING 1
#endif

#ifndef OMG_BME280_HUMIDITY_OVERSAMPLING
// OMG_BME280_HUMIDITY_OVERSAMPLING - Values:
// -------------------------
//  0, skipped
//  1 through 5, oversampling *1, *2, *4, *8, *16 respectively
#  define OMG_BME280_HUMIDITY_OVERSAMPLING 1
#endif

#ifndef OMG_BME280_FIR_FILTER_COEFFS
// Filter can be off or number of FIR coefficients - Values:
// ---------------------------------------------------------
//  0, filter off
//  1 through 4, coefficients = 2, 4, 8, 16 respectively
#  define OMG_BME280_FIR_FILTER_COEFFS 4
#endif

// Temperature correction for BME280/BMP280 devices

#ifndef OMG_BME280_TEMPERATURE_CORRECTION
// OMG_BME280_TEMPERATURE_CORRECTION - Correction in Celsius of temperature reported by BME280/BMP280 sensor. Both Celsius and Fahrenheit temperatures are adjusted.
// -------------------------
// Value is a float
// ie Compiler Directive '-DOMG_BME280_TEMPERATURE_CORRECTION=-3.4'
#  define OMG_BME280_TEMPERATURE_CORRECTION 0
#endif

#ifndef OMG_BME280_METRIC_UNITS_ONLY
#  define OMG_BME280_METRIC_UNITS_ONLY false
#endif

#endif // OMG_CONFIG_BME280_H
