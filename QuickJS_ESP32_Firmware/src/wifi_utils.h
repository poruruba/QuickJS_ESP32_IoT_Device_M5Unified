#ifndef _WIFI_UTILS_H_
#define _WIFI_UTILS_H_

//#define WIFI_SSID "【固定のWiFiアクセスポイントのSSID】" // WiFiアクセスポイントのSSID
//#define WIFI_PASSWORD "【固定のWiFIアクセスポイントのパスワード】" // WiFIアクセスポイントのパスワード
#define WIFI_SSID NULL // WiFiアクセスポイントのSSID
#define WIFI_PASSWORD NULL // WiFIアクセスポイントのパスワード
#define WIFI_TIMEOUT  10000
#define SERIAL_TIMEOUT1  10000
#define SERIAL_TIMEOUT2  20000

#include <Arduino.h>

long wifi_try_connect(bool infinit_loop);
long wifi_connect(const char *ssid, const char *password, unsigned long timeout);
long wifi_disconnect(void);
bool wifi_is_connected(void);
uint32_t get_ip_address(void);
uint8_t *get_mac_address(void);

#endif
