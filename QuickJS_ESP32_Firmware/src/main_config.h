#ifndef _MAIN_CONFIG_H_
#define _MAIN_CONFIG_H_

#include <Arduino.h>

//#define _WEBSERV_DISABLE_
#define _HTTP_CUSTOMCALL_

#if 0
#define _UNIT_ANGLE8_ENABLE_
#define _UNIT_ASR_ENABLE_
#define _UNIT_AUDIOPLAYER_ENABLE_
#define _UNIT_BYTEBUTTON_ENABLE_
#define _UNIT_COLOR_ENABLE_
#define _UNIT_ENVPRO_ENABLE_
#define _UNIT_GAS_ENABLE_
#define _UNIT_GESTURE_ENABLE_
#define _UNIT_IMUPRO_ENABLE_
#define _UNIT_PBHUB_ENABLE_
#define _UNIT_SONICIO_ENABLE_
#define _UNIT_STEP16_ENABLE_
#define _AUDIO_ENABLE_
#define _BLEPERIPHERAL_ENABLE_
#define _CAMERA_ENABLE_
#define _COAP_ENABLE_
#define _CRYPTO_ENABLE_
#define _ENV_ENABLE_
#define _ESPNOW_ENABLE_
#define _GRAPHQL_ENABLE_
#define _HTTP_ENABLE_
#define _IMU_ENABLE_
#define _IR_ENABLE_
#define _LCD_ENABLE_
#define _LEDC_ENABLE_
#define _MQTT_ENABLE_
#define _RTC_ENABLE_
#define _SD_ENABLE_
#define _SNMP_AGENT_ENABLE_
#define _WEBSOCKET_ENABLE_
#define _WEBSOCKET_CLIENT_ENABLE_
#endif

#define _HTTP_ENABLE_

#if defined(ARDUINO_M5Stack_ATOM)
#include <M5Unified.h>
#define _IMU_ENABLE_
#define _AUDIO_ENABLE_
#define MDNS_NAME "QuickJS_ESP32_M5Atom" // mDNSサービスホスト名
#elif defined(ARDUINO_M5Stick_C)
#include <M5Unified.h>
#define _LCD_ENABLE_
#define _RTC_ENABLE_
#define _IMU_ENABLE_
#define MDNS_NAME "QuickJS_ESP32_M5StickC" // mDNSサービスホスト名
#elif defined(ARDUINO_M5STACK_FIRE)
#define M5STACK_MPU6886
#include <SD.h>
#include <M5Unified.h>
#define _LCD_ENABLE_
#define _SD_ENABLE_
#define _IMU_ENABLE_
#define _AUDIO_ENABLE_
#define MDNS_NAME "QuickJS_ESP32_M5Stack" // mDNSサービスホスト名
#elif defined(ARDUINO_M5STACK_Core2)
#include <SD.h>
#include <M5Unified.h>
#define _LCD_ENABLE_
#define _RTC_ENABLE_
#define _IMU_ENABLE_
#define _SD_ENABLE_
#define _AUDIO_ENABLE_
#define MDNS_NAME "QuickJS_ESP32_M5Core2" // mDNSサービスホスト名
#elif defined(ARDUINO_ESP32C3_DEV)
#include <M5Unified.h>
#define MDNS_NAME "QuickJS_ESP32_M5StampC3" // mDNSサービスホスト名
#elif defined(ARDUINO_ESP32C3U_DEV)
#include <M5Unified.h>
#define MDNS_NAME "QuickJS_ESP32_M5StampC3U" // mDNSサービスホスト名
#elif defined(ARDUINO_M5Stack_ATOMS3)
#include <M5Unified.h>
#define _LCD_ENABLE_
#define _IMU_ENABLE_
#define MDNS_NAME "QuickJS_ESP32_M5AtomS3" // mDNSサービスホスト名
#elif defined(ARDUINO_M5Stack_StampS3)
#include <M5Unified.h>
#define MDNS_NAME "QuickJS_ESP32_M5StampS3" // mDNSサービスホスト名
#elif defined(ARDUINO_ESP32S3_DEV)
#include <M5Unified.h>
#define _LCD_ENABLE_
#define _RTC_ENABLE_
#define _IMU_ENABLE_
#define _IR_ENABLE_
#define _AUDIO_ENABLE_
#define MDNS_NAME "QuickJS_ESP32_M5StickS3" // mDNSサービスホスト名
#elif defined(ARDUINO_ESP32_S3_BOX)
#include <SD.h>
#include <M5Unified.h>
#define _SD_ENABLE_
#define _CAMERA_ENABLE_
#define MDNS_NAME "QuickJS_ESP32_UnitCamS3" // mDNSサービスホスト名
#elif defined(ARDUINO_ESP32C6_DEV)
#include <M5Unified.h>
#define MDNS_NAME "QuickJS_ESP32_C6" // mDNSサービスホスト名
#elif defined(ARDUINO_ESP32_M5CAMERA)
#include <M5Unified.h>
#define _CAMERA_ENABLE_
#define MDNS_NAME "QuickJS_ESP32_M5CAMERA" // mDNSサービスホスト名
#elif defined(ARDUINO_ESP32_TEMP)
#include <M5Unified.h>
#define _LCD_ENABLE_
#define MDNS_NAME "QuickJS_ESP32_TEMP" // mDNSサービスホスト名
#endif

#define DUMMY_FNAME  "/dummy"
#define MAIN_FNAME  "/main.js"
#define MODULE_DIR  "/modules/"
#define CONFIG_FNAME  "/config.ini"
#define CONFIG_FNAME_SYSLOG  "/syslog.ini"
#define CONFIG_FNAME_MQTT  "/mqtt.ini"
#define CONFIG_FNAME_BRIDGE  "/bridge.ini"
#define CONFIG_AWS_CREDENTIAL1  "/awscred1.ini"
#define CONFIG_AWS_CREDENTIAL2  "/awscred2.ini"
#define CONFIG_AWS_CREDENTIAL3  "/awscred3.ini"

#define CONFIG_INDEX_AUTOUPDATE 0
#define CONFIG_INDEX_AUTOSYSLOG 1

//#define WIFI_SSID "【固定のWiFiアクセスポイントのSSID】" // WiFiアクセスポイントのSSID
//#define WIFI_PASSWORD "【固定のWiFIアクセスポイントのパスワード】" // WiFIアクセスポイントのパスワード
#define WIFI_SSID NULL // WiFiアクセスポイントのSSID
#define WIFI_PASSWORD NULL // WiFIアクセスポイントのパスワード
#define MDNS_SERVICE "quickjs" // mDNSサービスタイプ
#define HTTP_PORT 80

#define WIFI_TIMEOUT  10000
#define SERIAL_TIMEOUT1  10000
#define SERIAL_TIMEOUT2  20000
#define SEMAPHORE_TIMEOUT   2000

#define DEFAULT_BUFFER_SIZE 4000
#define PACKET_JSON_DOCUMENT_SIZE  DEFAULT_BUFFER_SIZE
#define FILE_BUFFER_SIZE DEFAULT_BUFFER_SIZE

#define NUM_BTN_FUNC 8

//#define ENABLE_STATIC_WEB_PAGE
#define STATIC_REDIRECT_PAGE  "https://poruruba.github.io/QuickJS_ESP32_IoT_Device_M5Unified/QuickJS_ESP32_Firmware/data/html/"

#define FILE_LOADING_NONE     0
#define FILE_LOADING_RESTART  1
#define FILE_LOADING_REBOOT   2
#define FILE_LOADING_EXEC     3
#define FILE_LOADING_TEXT     4
#define FILE_LOADING_PAUSE    5
#define FILE_LOADING_START    6
#define FILE_LOADING_STOPPING 7
#define FILE_LOADING_STOP     8
extern unsigned char g_fileloading;
extern char g_download_buffer[FILE_BUFFER_SIZE];
extern esp_sleep_wakeup_cause_t g_sleepReason;
extern bool g_autoupdate;

extern SemaphoreHandle_t binSem;

long save_jscode(const char *p_code);
long save_module(const char* p_fname, const char *p_code);
long delete_module(const char *p_fname);
long read_module(const char* p_fname, char *p_buffer, uint32_t maxlen);
long read_jscode(char *p_buffer, uint32_t maxlen);

#endif
