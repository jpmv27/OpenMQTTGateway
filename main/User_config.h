/*
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation

   Act as a gateway between your 433mhz, infrared IR, BLE, LoRa signal and one interface like an MQTT broker
   Send and receiving command by MQTT

  This program enables to:
 - receive MQTT data from a topic and send signals corresponding to the received MQTT data
 - publish MQTT data to a different topic related to received signals

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
#ifndef OMG_USER_CONFIG_H
#define OMG_USER_CONFIG_H
/*-------------------VERSION----------------------*/
#ifndef OMG_VERSION
#  define OMG_VERSION "version_tag"
#endif

/*-------------CONFIGURE WIFIMANAGER-------------(only ESP8266 & SONOFF RFBridge)*/
/*
 * The following parameters are set during the WifiManager setup process:
 * - wifi_ssid
 * - wifi_password
 * - mqtt_user
 * - mqtt_pass
 * - mqtt_server
 * - mqtt_port
 *
 * To completely disable WifiManager, define OMG_ESP_WIFI_MANUAL_SETUP.
 * If you do so, please don't forget to set these variables before compiling
 *
 * Otherwise you can provide these credentials on the web interface after connecting
 * to the access point with your password (SSID: WifiManager_ssid, password: WifiManager_password)
 */
/*-------------DEFINE GATEWAY NAME BELOW IT CAN ALSO BE DEFINED IN platformio.ini----------------*/

// Uncomment to use the MAC address first 4 digits in the format of 5566 as the suffix of the short gateway name.
// Any definition of OMG_GATEWAY_NAME will be ignored. The Gateway_Short_name _ MAC will be used as the access point name.
//#define OMG_USE_MAC_AS_GATEWAY_NAME
#ifndef OMG_GATEWAY_NAME
#  define OMG_GATEWAY_NAME "OpenMQTTGateway"
#endif
#ifndef OMG_GATEWAY_SHORT_NAME
#  define OMG_GATEWAY_SHORT_NAME "OMG" // 3 characters maximum
#endif

#ifndef OMG_MQTT_BASE_TOPIC
#  define OMG_MQTT_BASE_TOPIC "home/"
#endif

/*-------------DEFINE YOUR NETWORK PARAMETERS BELOW----------------*/

//#define OMG_NETWORK_ADVANCED_SETUP true //uncomment if you want to set advanced network parameters, not uncommented you can set the IP and MAC only
#ifdef OMG_NETWORK_ADVANCED_SETUP
#  ifndef OMG_NET_IP
#    define OMG_NET_IP "192.168.1.99"
#  endif
#  ifndef OMG_NET_MASK
#    define OMG_NET_MASK "255.255.255.0"
#  endif
#  ifndef OMG_NET_GW
#    define OMG_NET_GW "192.168.1.1"
#  endif
#  ifndef OMG_NET_DNS
#    define OMG_NET_DNS "192.168.1.1"
#  endif
#endif

//#  define OMG_ESP_WIFI_MANUAL_SETUP true //uncomment you don't want to use wifimanager for your credential settings on ESP

//#define OMG_ESP32_ETHERNET=true // Uncomment to use Ethernet module on ESP32 Ethernet gateway and adapt the settings to your board below, the default parameter are for OLIMEX ESP32 gateway
#ifdef OMG_ESP32_ETHERNET
#  ifndef OMG_ETH_PHY_ADDR
#    define OMG_ETH_PHY_ADDR 0
#  endif
#  ifndef OMG_ETH_PHY_TYPE
#    define OMG_ETH_PHY_TYPE ETH_PHY_LAN8720
#  endif
#  ifndef OMG_ETH_PHY_POWER
#    define OMG_ETH_PHY_POWER 12
#  endif
#  ifndef OMG_ETH_PHY_MDC
#    define OMG_ETH_PHY_MDC 23
#  endif
#  ifndef OMG_ETH_PHY_MDIO
#    define OMG_ETH_PHY_MDIO 18
#  endif
#  ifndef OMG_ETH_CLK_MODE
#    define OMG_ETH_CLK_MODE ETH_CLOCK_GPIO17_OUT
#  endif
#endif

#if defined(OMG_ESP_WIFI_MANUAL_SETUP) // for nodemcu, weemos and esp8266
#  ifndef OMG_WIFI_SSID
#    define OMG_WIFI_SSID "wifi ssid"
#  endif
#  ifndef OMG_WIFI_PASSWORD
#    define OMG_WIFI_PASSWORD "wifi password"
#  endif
#endif

//#define OMG_GW_PWD_FROM_MAC true // enable to set the password from the last 8 digits of the ESP MAC address for enhanced security, enabling this option requires to have access to the MAC address, either through a sticker or with serial monitoring
#ifndef OMG_WIFIMANAGER_SSID
#  define OMG_WIFIMANAGER_SSID OMG_GATEWAY_NAME //this is the network name of the initial setup access point
#endif
#ifndef OMG_WIFIMANAGER_CONFIG_PORTAL_TIMEOUT
#  define OMG_WIFIMANAGER_CONFIG_PORTAL_TIMEOUT 240 //time in seconds for the setup portal to stay open, default 240s
#endif
#ifndef OMG_WIFI_TIMEOUT
#  define OMG_WIFI_TIMEOUT 30
#endif
#ifndef OMG_WM_DEBUG // WiFi Manager debug
#  define OMG_WM_DEBUG 1
#endif
//#define OMG_WIFIMNG_HIDE_MQTT_CONFIG //Uncomment so as to hide MQTT setting from Wifi manager page

/*-------------DEFINE YOUR ADVANCED NETWORK PARAMETERS BELOW----------------*/
//#define OMG_MDNS_SD //uncomment if you  want to use mDNS for discovering automatically your IP server, please note that mDNS with ESP32 can cause the BLE to not work
#define maxConnectionRetryNetwork 5 //maximum Wifi connection attempts with existing credential at start (used to bypass ESP32 issue on wifi connect)
#define maxRetryWatchDog          11 //maximum Wifi or MQTT re-connection attempts before restarting

//set minimum quality of signal so it ignores AP's under that quality
#define MinimumWifiSignalQuality 8

/*-------------DEFINE YOUR OTA PARAMETERS BELOW----------------*/

// Possible values for OMG_OTA_CHECK_OTA_UPDATE
#define OMG_OTA_NONE    0
#define OMG_OTA_RELEASE 1
#define OMG_OTA_DEV     2

#ifndef OMG_OTA_WEBUI_FW_UPDATE
#  define OMG_OTA_WEBUI_FW_UPDATE true
#endif

#ifndef OMG_OTA_PASSIVE_FW_UPDATE
#  define OMG_OTA_PASSIVE_FW_UPDATE true
#endif
#if OMG_OTA_PASSIVE_FW_UPDATE
#  ifndef OMG_OTA_PASSIVE_FW_UPDATE_MDNS_ENABLED
#    define OMG_OTA_PASSIVE_FW_UPDATE_MDNS_ENABLED true
#  endif
#  ifndef OMG_OTA_PASSIVE_FW_UPDATE_PORT
#    define OMG_OTA_PASSIVE_FW_UPDATE_PORT 8266
#  endif
#  ifndef OMG_OTA_PASSIVE_FW_UPDATE_TIMEOUT_MILLIS
//   timeout for OTA activities
//   OTA upload with no activity in this period is considered inactive
//   As long as OTA upload is considered "active", we avoid rebooting e.g.
//   in case of failures connecting to MQTT
#    define OMG_OTA_PASSIVE_FW_UPDATE_TIMEOUT_MILLIS 30000
#  endif
#endif

#ifndef OMG_OTA_SELF_FW_UPDATE
#  define OMG_OTA_SELF_FW_UPDATE false
#endif

#ifndef OMG_OTA_MQTT_HTTPS_FW_UPDATE
#  define OMG_OTA_MQTT_HTTPS_FW_UPDATE false
#endif

#ifndef OMG_OTA_FW_UPDATE_USE_PASSWORD
#  define OMG_OTA_FW_UPDATE_USE_PASSWORD 1 
#endif

#ifndef OMG_OTA_FW_UPDATE_URL_STYLE
#  define OMG_OTA_FW_UPDATE_URL_STYLE openmqttgateway
#endif

#ifndef OMG_OTA_FW_UPDATE_LATEST_STYLE
#  define OMG_OTA_FW_UPDATE_LATEST_STYLE json
#endif

#if OMG_OTA_SELF_FW_UPDATE
#  if !defined(OMG_OTA_CHECK_OTA_UPDATE) || OMG_OTA_CHECK_OTA_UPDATE == OMG_OTA_NONE
#    undef OMG_OTA_CHECK_OTA_UPDATE
#    define OMG_OTA_CHECK_OTA_UPDATE OMG_OTA_RELEASE
#  endif
#elif OMG_OTA_WEBUI_FW_UPDATE || OMG_OTA_MQTT_HTTPS_FW_UPDATE
#  ifndef OMG_OTA_CHECK_OTA_UPDATE
#    define OMG_OTA_CHECK_OTA_UPDATE OMG_OTA_RELEASE // enable to check for the presence of a new version for your environment on Github
#  endif
#else
#  undef OMG_OTA_CHECK_OTA_UPDATE
#  define OMG_OTA_CHECK_OTA_UPDATE OMG_OTA_NONE
#endif

#ifndef OMG_GW_PASSWORD
#  define OMG_GW_PASSWORD ""
#endif

/*-------------DEFINE YOUR MQTT PARAMETERS BELOW----------------*/
//MQTT Parameters definition
#define parameters_size     65
#define mqtt_topic_max_size 150
#define mqtt_key_max_size   20

#ifndef JSON_MSG_BUFFER
#  if defined(ESP32)
#    define JSON_MSG_BUFFER 1024 // adjusted to minimum size covering largest home assistant discovery messages
#  elif defined(ESP8266)
#    define JSON_MSG_BUFFER 512 // Json message max buffer size, don't put 768 or higher it is causing unexpected behaviour on ESP8266, certificates handling with ESP8266 is not tested
#  endif
#  if OMG_MQTT_SECURE_DEFAULT
#    define JSON_MSG_BUFFER_MAX 2048 // Json message buffer size increased to handle certificate changes through MQTT, used for the queue and the coming MQTT messages
#  else
#    define JSON_MSG_BUFFER_MAX 1024 // Minimum size for the cover MQTT discovery message
#  endif
#endif

#ifndef mqtt_max_payload_size
#  define mqtt_max_payload_size JSON_MSG_BUFFER_MAX + mqtt_topic_max_size + 10 // maximum size of the MQTT payload
#endif

#ifndef OMG_MQTT_USER
#  define OMG_MQTT_USER "your_username"
#endif
#ifndef OMG_MQTT_PASS
#  define OMG_MQTT_PASS "your_password"
#endif
#ifndef OMG_MQTT_SERVER
#  define OMG_MQTT_SERVER "192.168.1.17"
#endif
#ifndef OMG_MQTT_PORT
#  define OMG_MQTT_PORT "1883"
#endif

#ifndef OMG_GENERAL_TIMEOUT
#  define OMG_GENERAL_TIMEOUT 20 // time out if a task is stuck in seconds (should be more than TimeBetweenReadingRN8209/1000) and more than 3 seconds, the WDT will reset the ESP, used also for MQTT connection
#endif
#ifndef QueueSemaphoreTimeOutTask
#  define QueueSemaphoreTimeOutTask 3000 // time out for semaphore retrieval from a task
#endif
#ifndef QueueSemaphoreTimeOutLoop
#  define QueueSemaphoreTimeOutLoop 100 // time out for semaphore retrieval from the loop
#endif

// Uncomment to use a device running TheengsGateway to decode BLE data. (https://github.com/theengs/gateway)
// Set the topic to the subscribe topic configured in the TheengGateway
// #define OMG_BT_MQTT_DECODE_TOPIC "MQTTDecode"

#define ATTEMPTS_BEFORE_BG 10 // Number of wifi connection attempts before going to BG protocol
#define ATTEMPTS_BEFORE_B  20 // Number of wifi connection attempts before going to B protocol

#ifndef OMG_NTP_SERVER
#  define OMG_NTP_SERVER "pool.ntp.org"
#endif

#ifndef OMG_LOG_TO_SYSLOG
#  define OMG_LOG_TO_SYSLOG false
#endif

#if OMG_LOG_TO_SYSLOG
#  ifndef ELOG_SYSLOG_ENABLE
#    error ELOG_SYSLOG_ENABLE must be defined
#  endif
#  ifndef OMG_SYSLOG_SERVER
#    error OMG_SYSLOG_SERVER must be defined
#  endif
#  ifndef OMG_SYSLOG_PORT
#    define OMG_SYSLOG_PORT "514"
#  endif
#  ifndef OMG_SYSLOG_FACILITY
#    define OMG_SYSLOG_FACILITY ELOG_FAC_USER
#  endif
#  ifndef OMG_SYSLOG_WAIT_IF_NOT_READY
#    define OMG_SYSLOG_WAIT_IF_NOT_READY false
#  endif
#  ifndef OMG_SYSLOG_MAX_WAIT_MILLISECONDS
#    define OMG_SYSLOG_MAX_WAIT_MILLISECONDS 0
#  endif
#endif

#ifndef OMG_MQTT_SECURE_DEFAULT
#  define OMG_MQTT_SECURE_DEFAULT false
#endif

#ifndef OMG_MQTT_CERT_VALIDATE_DEFAULT
#  define OMG_MQTT_CERT_VALIDATE_DEFAULT false
#endif

#ifndef AWS_IOT
#  define AWS_IOT false
#endif

#ifndef OMG_MQTT_BROKER_MODE
#  define OMG_MQTT_BROKER_MODE false
#endif

#if OMG_MQTT_BROKER_MODE
// In MQTT broker mode the MQTT web config is not needed
#  define OMG_WIFIMNG_HIDE_MQTT_CONFIG true
#endif

#define GITHUB_OTA_SERVER_CERT_HASH "d4d211b4553af9fac371f24c2268d59d2b0fec6b9aa0fdbbde068f078d7daf86" // SHA256 fingerprint of the certificate used by the OTA server

#if AWS_IOT
// Enable the use of ALPN for AWS IoT Core with the port 443
const char* alpnProtocols[] = {"x-amzn-mqtt-ca", NULL};
#endif

#ifdef OMG_OTA_MQTT_HTTPS_FW_UPDATE || OMG_OTA_WEBUI_FW_UPDATE || OMG_OTA_SELF_FW_UPDATE
// If used, this should be set to the root CA certificate of the server hosting the firmware.
#  ifdef OMG_MQTT_USE_PRIVATE_CERTS
#    include "certs/private_ota_cert.h"
#  else
#    include "certs/default_ota_cert.h"
#  endif

#  define ENTITY_PICTURE   "https://github.com/1technophile/OpenMQTTGateway/raw/development/docs/img/Openmqttgateway_logo_mini_margins.png"

#  ifndef OMG_OTA_RELEASE_BASE_URL
#    define OMG_OTA_RELEASE_BASE_URL     "https://ota.openmqttgateway.com/binaries/"
#  endif

#  ifndef OMG_OTA_DEV_BASE_URL
#    define OMG_OTA_DEV_BASE_URL OMG_OTA_RELEASE_BASE_URL "dev/"
#  endif

#  if OMG_OTA_FW_UPDATE_LATEST_STYLE == json
#    if OMG_OTA_CHECK_OTA_UPDATE == OMG_OTA_DEV
#      define OMG_OTA_FW_UPDATE_LATEST_URL OMG_OTA_DEV_BASE_URL "latest_version_dev.json" //OTA url used to discover new versions of the firmware from development nightly builds
#    elif OMG_OTA_CHECK_OTA_UPDATE == OMG_OTA_RELEASE
#      define OMG_OTA_FW_UPDATE_LATEST_URL OMG_OTA_RELEASE_BASE_URL "latest_version.json" //OTA url used to discover new versions of the firmware
#    endif
#  elif OMG_OTA_FW_UPDATE_LATEST_STYLE == filename
#    if OMG_OTA_CHECK_OTA_UPDATE == OMG_OTA_DEV
#      define OMG_OTA_FW_UPDATE_LATEST_URL OMG_OTA_DEV_BASE_URL "latest"
#    elif OMG_OTA_CHECK_OTA_UPDATE == OMG_OTA_RELEASE
#      define OMG_OTA_FW_UPDATE_LATEST_URL OMG_OTA_RELEASE_BASE_URL "latest"
#    endif
#  endif

#else
const char* OTAserver_cert = "";
#endif

#ifndef OMG_MQTT_SECURE_SIGNED_CLIENT
#  define OMG_MQTT_SECURE_SIGNED_CLIENT 0 // If using a signed certificate for the broker and using client certificate/key set this to true or 1
#endif

#ifdef OMG_MQTT_USE_PRIVATE_CERTS
#  include "certs/private_client_cert.h"
#  include "certs/private_client_key.h"
#  include "certs/private_server_cert.h"
#else
#  include "certs/default_client_cert.h"
#  include "certs/default_client_key.h"
#  include "certs/default_server_cert.h"
#endif

#include <string>

#ifndef OMG_MQTT_CNT_DEFAULT_INDEX
#  define OMG_MQTT_CNT_DEFAULT_INDEX 0 // Default set of connection parameters
#endif

#if !OMG_MQTT_BROKER_MODE
struct ss_cnt_parameters {
  std::string server_cert;
  std::string client_cert;
  std::string client_key;
  std::string ota_server_cert;
  char mqtt_server[parameters_size];
  char mqtt_port[6];
  char mqtt_user[parameters_size];
  char mqtt_pass[parameters_size];
  bool isConnectionSecure;
  bool isCertValidate;
  bool validConnection;
};

#  define cnt_parameters_array_size 3

ss_cnt_parameters cnt_parameters_array[cnt_parameters_array_size] = {
    {ss_server_cert, ss_client_cert, ss_client_key, OTAserver_cert, OMG_MQTT_SERVER, OMG_MQTT_PORT, OMG_MQTT_USER, OMG_MQTT_PASS, OMG_MQTT_SECURE_DEFAULT, OMG_MQTT_CERT_VALIDATE_DEFAULT, false},
    {"", "", "", "", OMG_MQTT_SERVER, OMG_MQTT_PORT, OMG_MQTT_USER, OMG_MQTT_PASS, OMG_MQTT_SECURE_DEFAULT, OMG_MQTT_CERT_VALIDATE_DEFAULT, false},
    {"", "", "", "", OMG_MQTT_SERVER, OMG_MQTT_PORT, OMG_MQTT_USER, OMG_MQTT_PASS, OMG_MQTT_SECURE_DEFAULT, OMG_MQTT_CERT_VALIDATE_DEFAULT, false}};
#endif

#define MIN_CERT_LENGTH 200 // Minimum length of a certificate to be considered valid

/**
 * Ext wake for Deep-sleep for the ESP32.
 * Set the wake pin state.
 */
#ifdef ESP32_EXT0_WAKE_PIN
#  ifndef ESP32_EXT0_WAKE_PIN_STATE
#    define ESP32_EXT0_WAKE_PIN_STATE 1
#  endif
#endif

#ifdef ESP32_EXT1_WAKE_PIN
#  ifndef ESP32_EXT1_WAKE_PIN_STATE
#    define ESP32_EXT1_WAKE_PIN_STATE 1
#  endif
#endif

#ifndef DEEP_SLEEP_IN_US
#  define DEEP_SLEEP_IN_US 60000000 // 1 minute
#endif

/*------------------DEEP SLEEP parameters ------------------*/
//DEFAULT_LOW_POWER_MODE DEACTIVATED low power mode can't be used on this build to prevent bricking devices that does not support low power mode
//DEFAULT_LOW_POWER_MODE ALWAYS_ON normal mode (no power consumption optimisations)
//DEFAULT_LOW_POWER_MODE INTERVAL to activate deep sleep with intervals and action wake up
//DEFAULT_LOW_POWER_MODE ACTION to activate deep sleep with action wake up
#ifndef DEFAULT_LOW_POWER_MODE
#  define DEFAULT_LOW_POWER_MODE DEACTIVATED
#endif

/*-------------DEFINE THE MODULES YOU WANT BELOW----------------*/
//Addons and module management, uncomment the Z line corresponding to the module you want to use

//#define OMG_GATEWAY_RF     "RF"       //ESP8266, Arduino, ESP32
//#define OMG_GATEWAY_IR     "IR"       //ESP8266, Arduino,         Sonoff RF Bridge
//#define OMG_GATEWAY_LORA   "LORA"       //ESP8266, Arduino, ESP32
//#define OMG_GATEWAY_PILIGHT "Pilight" //ESP8266, Arduino, ESP32
//#define OMG_GATEWAY_WEATHERSTATION "WeatherStation" //ESP8266, Arduino, ESP32
//#define OMG_GATEWAY_GFSUNINVERTER "GFSunInverter"   //ESP32
//#define OMG_GATEWAY_BT     "BT"       //ESP8266, ESP32
//#define OMG_GATEWAY_RF2    "RF2"      //ESP8266, Arduino, ESP32
//#define OMG_GATEWAY_SRFB   "SRFB"     //                          Sonoff RF Bridge
//#define OMG_GATEWAY_2G     "2G"       //ESP8266, Arduino, ESP32
//#define OMG_GATEWAY_RFM69  "RFM69"    //ESP8266, Arduino, ESP32
//#define OMG_ACTUATOR_ONOFF "ONOFF"    //ESP8266, Arduino, ESP32,  Sonoff RF Bridge
//#define OMG_SENSOR_INA226  "INA226"   //ESP8266, Arduino, ESP32
//#define OMG_SENSOR_HCSR04  "HCSR04"   //ESP8266, Arduino, ESP32
//#define OMG_SENSOR_HCSR501 "HCSR501"  //ESP8266, Arduino, ESP32,  Sonoff RF Bridge
//#define OMG_SENSOR_ADC     "ADC"      //ESP8266, Arduino, ESP32
//#define OMG_SENSOR_BH1750  "BH1750"   //ESP8266, Arduino, ESP32
//#define OMG_SENSOR_MQ2 "MQ2"  //ESP8266, Arduino, ESP32
//#define OMG_SENSOR_TEMT6000 "TEMT6000"  //ESP8266
//#define OMG_SENSOR_TSL2561 "TSL2561"  //ESP8266, Arduino, ESP32
//#define OMG_SENSOR_BME280  "BME280"   //ESP8266, Arduino, ESP32
//#define OMG_SENSOR_HTU21   "HTU21"    //ESP8266, Arduino, ESP32
//#define OMG_SENSOR_LM75   "LM75"    //ESP8266, Arduino, ESP32
//#define OMG_SENSOR_DHT     "DHT"      //ESP8266, Arduino, ESP32,  Sonoff RF Bridge
//#define OMG_SENSOR_DS1820  "DS1820"   //ESP8266, Arduino, ESP32
//#define OMG_SENSOR_GPIOKEYCODE "GPIOKeyCode" //ESP8266, Arduino, ESP32
//#define OMG_SENSOR_GPIOINPUT "GPIOInput" //ESP8266, Arduino, ESP32
//#define OMG_MQTT_DISCOVERY "HADiscovery"//ESP8266, Arduino, ESP32, Sonoff RF Bridge
//#define OMG_ACTUATOR_FASTLED "FASTLED" //ESP8266, Arduino, ESP32, Sonoff RF Bridge
//#define OMG_BOARD_M5STICKC "M5StickC"
//#define OMG_BOARD_M5STICKCP "M5StickCP"
//#define OMG_BOARD_M5STACK  "M5STACK"
//#define OMG_BOARD_M5TOUGH  "M5TOUGH"
//#define OMG_RADIO_CC1101   "CC1101"   //ESP8266, ESP32
//#define OMG_ACTUATOR_PWM   "PWM"      //ESP8266, ESP32
//#define OMG_SENSOR_SHTC3 "SHTC3" //ESP8266, Arduino, ESP32,  Sonoff RF Bridge
//#define OMG_ACTUATOR_SOMFY "Somfy"    //ESP8266, Arduino, ESP32
//#define OMG_GATEWAY_SERIAL   "SERIAL"  //ESP8266, Arduino, ESP32

/*-------------DEFINE YOUR MQTT ADVANCED PARAMETERS BELOW----------------*/
#ifndef will_Topic
#  define will_Topic "/LWT"
#endif
#ifndef will_QoS
#  define will_QoS 0
#endif
#ifndef will_Retain
#  define will_Retain true
#endif
#ifndef sensor_Retain
#  define sensor_Retain false
#endif
#ifndef will_Message
#  define will_Message "offline"
#endif
#ifndef Gateway_AnnouncementMsg
#  define Gateway_AnnouncementMsg "online"
#endif

#ifndef OMG_MQTT_JSON_PUBLISHING
#  define OMG_MQTT_JSON_PUBLISHING true //define false if you don't want to use Json publishing (one topic for all the parameters)
#endif
//example home/OpenMQTTGateway_ESP32_DEVKIT/BTtoMQTT/4XXXXXXXXXX4 {"rssi":-63,"servicedata":"fe0000000000000000000000000000000000000000"}
#ifndef jsonReceiving
#  define jsonReceiving true //define false if you don't want to use Json  reception analysis
#endif

#ifndef OMG_MQTT_SIMPLE_PUBLISHING
#  define OMG_MQTT_SIMPLE_PUBLISHING false //define true if you want to use simple publishing (one topic for one parameter)
#endif
#ifndef OMG_MQTT_SIMPLE_PUBLISHING_OVERRIDE_RETAIN
#  define OMG_MQTT_SIMPLE_PUBLISHING_OVERRIDE_RETAIN false
#endif
//example
// home/OpenMQTTGateway_ESP32_DEVKIT/BTtoMQTT/4XXXXXXXXXX4/rssi -63.0
// home/OpenMQTTGateway_ESP32_DEVKIT/BTtoMQTT/4XXXXXXXXXX4/servicedata fe0000000000000000000000000000000000000000
#ifndef OMG_MQTT_SIMPLE_RECEIVING
#  define OMG_MQTT_SIMPLE_RECEIVING true //define false if you don't want to use old way reception analysis
#endif
#ifndef OMG_MQTT_MESSAGE_UTC_TIMESTAMP
#  define OMG_MQTT_MESSAGE_UTC_TIMESTAMP false //define true if you want messages to be timestamped in ISO8601 UTC format (e.g.: "UTCtime"="2023-12-26T19:10:20Z")
#endif
#ifndef OMG_MQTT_MESSAGE_LOCAL_TIMESTAMP
#  define OMG_MQTT_MESSAGE_LOCAL_TIMESTAMP false //define true if you want messages to be timestamped in ISO8601 UTC format with time zone (e.g.: "UTCtime"="2023-12-26T19:10:20-05:00")
#endif
#ifndef OMG_MQTT_MESSAGE_UNIX_TIMESTAMP
#  define OMG_MQTT_MESSAGE_UNIX_TIMESTAMP false //define true if you want messages to have an unix timestamp (e.g.: "unixtime"=1679015107)
#endif

// LED index depending on state, each state can have a different LED index or be grouped if there is a limited number of LEDs
#ifndef LED_ERROR
#  define LED_ERROR 0
#endif
#ifndef LED_PROCESSING
#  define LED_PROCESSING 0
#endif
#ifndef LED_BROKER
#  define LED_BROKER 0
#endif
#ifndef LED_NETWORK
#  define LED_NETWORK 0
#endif

// LED Strip index
#ifndef STRIP_ERROR
#  define STRIP_ERROR 0
#endif
#ifndef STRIP_PROCESSING
#  define STRIP_PROCESSING 0
#endif
#ifndef STRIP_BROKER
#  define STRIP_BROKER 0
#endif
#ifndef STRIP_NETWORK
#  define STRIP_NETWORK 0
#endif
#ifndef STRIP_POWER
#  define STRIP_POWER 0
#endif

// Single standard LED pin
#ifndef LED_PIN
#  ifdef LED_BUILTIN
#    define LED_PIN LED_BUILTIN
#  endif
#endif
#ifndef LED_PIN_ON
#  define LED_PIN_ON HIGH
#endif
#ifndef LED_ACTUATOR_ONOFF
#  ifdef LED_BUILTIN
#    define LED_ACTUATOR_ONOFF LED_BUILTIN
#  endif
#endif

#ifndef DEFAULT_ADJ_BRIGHTNESS
#  define DEFAULT_ADJ_BRIGHTNESS 255 // Set Default RGB adjustable brightness
#endif

#ifndef LED_POWER_COLOR
#  define LED_POWER_COLOR 0x00FF00 // Green
#endif
#ifndef LED_PROCESSING_COLOR
#  define LED_PROCESSING_COLOR 0x0000FF // Blue
#endif
#ifndef LED_WAITING_ONBOARD_COLOR
#  define LED_WAITING_ONBOARD_COLOR 0xFFA500 // Orange
#endif
#ifndef LED_ONBOARD_COLOR
#  define LED_ONBOARD_COLOR 0xFFFF00 // Yellow
#endif
#ifndef LED_NETWORK_OK_COLOR
#  define LED_NETWORK_OK_COLOR 0x00FF00 // Green
#endif
#ifndef LED_NETWORK_ERROR_COLOR
#  define LED_NETWORK_ERROR_COLOR 0xFFA500 // Orange
#endif
#ifndef LED_BROKER_OK_COLOR
#  define LED_BROKER_OK_COLOR 0x00FF00 // Green
#endif
#ifndef LED_BROKER_ERROR_COLOR
#  define LED_BROKER_ERROR_COLOR 0xFFA500 // Orange
#endif
#ifndef LED_OFFLINE_COLOR
#  define LED_OFFLINE_COLOR 0x0000FF // Blue
#endif
#ifndef LED_OTA_LOCAL_COLOR
#  define LED_OTA_LOCAL_COLOR 0xFF00FF // Magenta
#endif
#ifndef LED_OTA_REMOTE_COLOR
#  define LED_OTA_REMOTE_COLOR 0x8000FF // Purple
#endif
#ifndef LED_ERROR_COLOR
#  define LED_ERROR_COLOR 0xFF0000 // Red
#endif
#ifndef LED_ACTUATOR_ONOFF_COLOR
#  define LED_ACTUATOR_ONOFF_COLOR 0x00FF00 // Green
#endif
#ifndef LED_COLOR_BLACK
#  define LED_COLOR_BLACK 0x000000
#endif

#ifdef ESP8266
//#  define TRIGGER_GPIO 14 // pin D5 as full reset button (long press >10s)
#elif ESP32
//#  define TRIGGER_GPIO 0 // boot button as full reset button (long press >10s)
//#  define NO_INT_TEMP_READING true //Define if we don't want internal temperature reading for the ESP32
#endif

//      VCC   ------------D|-----------/\/\/\/\ -----------------  Arduino PIN
//                        LED       Resistor 270-510R

/*----------------------------OTHER PARAMETERS-----------------------------*/
/*-------------------CHANGING THEM IS NOT COMPULSORY-----------------------*/
/*----------------------------USER PARAMETERS-----------------------------*/
#ifdef OMG_GATEWAY_SRFB
#  define SERIAL_BAUD 19200
#else
#  ifndef SERIAL_BAUD
#    define SERIAL_BAUD 115200
#  endif
#endif
/*--------------MQTT general topics-----------------*/
// global MQTT subject listened by the gateway to execute commands (send RF, IR or others)
#ifndef subjectMQTTtoX
#  define subjectMQTTtoX "/commands/#"
#endif
#ifndef subjectMultiGTWKey
#  define subjectMultiGTWKey "toMQTT"
#endif
#ifndef subjectGTWSendKey
#  define subjectGTWSendKey "MQTTto"
#endif

// key used for launching commands to the gateway
#define restartCmd "restart"
#define eraseCmd   "erase"
#define statusCmd  "status"

#ifndef OMG_MQTT_VALUE_AS_A_TOPIC
#  define OMG_MQTT_VALUE_AS_A_TOPIC false // define true to integrate msg value into the subject when receiving
#endif

#if defined(OMG_GATEWAY_RF) || defined(OMG_GATEWAY_IR) || defined(OMG_GATEWAY_SRFB) || defined(OMG_GATEWAY_WEATHERSTATION) || defined(OMG_GATEWAY_RTL_433)
// variable to avoid duplicates
#  ifndef time_avoid_duplicate
#    define time_avoid_duplicate 3000 // if you want to avoid duplicate MQTT message received set this to > 0, the value is the time in milliseconds during which we don't publish duplicates
#  endif
#endif

#ifndef TimeBetweenReadingSYS
#  define TimeBetweenReadingSYS 120 // time between (s) system readings (like memory)
#endif
#define TimeBetweenCheckingSYS       3600 // time between (s) system checkings (like updates)
#define TimeLedON                    1 // time LED are ON
#define InitialMQTTConnectionTimeout 10 // time estimated (s) before the board is connected to MQTT
#ifndef subjectSYStoMQTT
#  define subjectSYStoMQTT "/SYStoMQTT" // system parameters
#endif
#ifndef subjectLOGtoMQTT
#  define subjectLOGtoMQTT "/LOGtoMQTT" // log informations
#endif
#ifndef subjectRLStoMQTT
#  define subjectRLStoMQTT "/RLStoMQTT" // latest release information
#endif
#ifndef subjectMQTTtoSYSset
#  define subjectMQTTtoSYSset "/commands/MQTTtoSYS/config"
#endif
#ifndef subjectMQTTtoSYSupdate
#  define subjectMQTTtoSYSupdate "/commands/MQTTtoSYS/firmware_update"
#endif
#define TimeToResetAtStart 5000 // Time we allow the user at start for the reset command by button press
/*-------------------DEFINE LOG LEVEL----------------------*/
#ifndef OMG_LOGID
#  define OMG_LOGID 0
#endif
#ifndef OMG_LOG_LEVEL
#  define OMG_LOG_LEVEL ELOG_LEVEL_NOTICE
#endif
#if OMG_LOG_TO_SYSLOG
#  ifndef OMG_LOG_LEVEL_SYSLOG
#    define OMG_LOG_LEVEL_SYSLOG OMG_LOG_LEVEL
#  endif
#endif

/*-------------------ESP Wifi band and tx power ---------------------*/
//Certain sensors are sensitive to Wifi which can cause interference with their normal operation
//For example it can cause false triggers on a PIR HC-SR501
//It is reccomended to change Wifi BAND to G and reduce tx power level to 11dBm
//Since the WiFi protocol is persisted in the flash of the ESP you have to run at least once with `WiFiGMode` defined false to get Band N back.
#ifndef WifiGMode
//#    define WifiGMode                 true
#endif
#ifndef WifiPower
//#    define WifiPower                 WIFI_POWER_11dBm //When using an ESP32
//#    define WifiPower                 11 //When using an ESP8266
#endif

/*-----------PLACEHOLDERS FOR WebUI DISPLAY--------------*/
#define pubWebUI(...) // display the published message onto the WebUI display

/*-----------PLACEHOLDERS FOR OLED/LCD DISPLAY--------------*/
// The real definitions are in config_M5.h / config_SSD1306.h
#define displayPrint(...)   // only print if not in low power mode
#define lpDisplayPrint(...) // print in low power mode

/*----------- SHARED WITH OMG MODULES --------------*/

char mqtt_topic[parameters_size + 1] = OMG_MQTT_BASE_TOPIC;
char g_gateway_name[parameters_size + 1] = OMG_GATEWAY_NAME;

#if OMG_LOG_TO_SYSLOG
char g_syslog_server[parameters_size + 1] = OMG_SYSLOG_SERVER;
char g_syslog_port[parameters_size + 1] = OMG_SYSLOG_PORT;
#endif

void connectMQTT();

unsigned long uptime();
bool cmpToMainTopic(const char*, const char*);
bool pub(const char*, const char*, bool);
bool pub(const char*, const char*);

#if defined(ESP32)
#  include <Preferences.h>
Preferences preferences;
#endif

unsigned long lastDiscovery = 0; // Time of the last discovery to trigger automaticaly to off after OMG_MQTT_DISCOVERY_AUTO_OFF_TIMER
#ifndef DEFAULT_DISCOVERY
#  define DEFAULT_DISCOVERY true
#endif

#include <vector>
// Flags definition for white list, black list, discovery management
#define device_flags_init     0 << 0
#define device_flags_isDisc   1 << 0
#define device_flags_isWhiteL 1 << 1
#define device_flags_isBlackL 1 << 2
#define device_flags_connect  1 << 3
#define isWhite(device)       device->isWhtL
#define isBlack(device)       device->isBlkL
#define isDiscovered(device)  device->isDisc

enum PowerMode { DEACTIVATED = -1,
                 ALWAYS_ON,
                 INTERVAL,
                 ACTION };

/*--------------------Check for time hogs--------------------*/
#ifndef OMG_LOOP_TIME_LIMIT
#  define OMG_LOOP_TIME_LIMIT 100
#endif

#define CheckElapsedTime(proc) {                                                                        \
                                 unsigned long const start = millis();                                  \
                                 proc;                                                                  \
                                 unsigned long const diff = millis() - start;                           \
                                 if (diff > OMG_LOOP_TIME_LIMIT) {                                      \
                                   Logger.warning(OMG_LOGID, F("Call to " #proc " took %lu ms"), diff); \
                                 }                                                                      \
                               }

/*--------------------Minimum freeHeap--------------------*/
// Below this parameter we trigger a restart, this avoid stuck boards like seen in https://github.com/1technophile/OpenMQTTGateway/issues/1693
#define MinimumMemory 40000

/*----------------CONFIGURABLE PARAMETERS-----------------*/
struct SYSConfig_s {
  bool mqtt; // if true the gateway will publish the received data on the MQTT broker
  bool serial; // if true the gateway will publish the received data on the SERIAL
  bool blufi; // if true the gateway will be accesible with blufi
  bool offline;
  bool discovery; // HA discovery convention
  bool ohdiscovery; // OH discovery specificities
#ifdef LED_ADDRESSABLE
  int rgbbrightness; // brightness of the RGB LED
#endif
  enum PowerMode powerMode;
};

#ifndef DEFAULT_MQTT
#  define DEFAULT_MQTT true
#endif
#ifndef DEFAULT_SERIAL
#  define DEFAULT_SERIAL false
#endif
#ifndef DEFAULT_BLUFI
#  define DEFAULT_BLUFI true
#endif
#ifndef DEFAULT_OFFLINE
#  define DEFAULT_OFFLINE false
#endif

#if defined(OMG_GATEWAY_RF) || defined(OMG_GATEWAY_IR) || defined(OMG_GATEWAY_SRFB) || defined(OMG_GATEWAY_WEATHERSTATION) || defined(OMG_GATEWAY_RTL_433)
bool isAduplicateSignal(uint64_t);
void storeSignalValue(uint64_t);
#endif

#define convertTemp_CtoF(c) ((c * 1.8) + 32)
#define convertTemp_FtoC(f) ((f - 32) * 5 / 9)

#endif // OMG_USER_CONFIG
