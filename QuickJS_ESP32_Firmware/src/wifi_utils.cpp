#include <Arduino.h>
#include <WiFi.h>

#include "wifi_utils.h"

static uint8_t wifi_mac_address[6] = { 0 };

long wifi_connect(const char *ssid, const char *password, unsigned long timeout)
{
  Serial.println("");
  Serial.print("WiFi Connenting");

  if( ssid == NULL && password == NULL ){
    if( WiFi.begin() == WL_CONNECT_FAILED )
      return -1;
  }else{
    WiFi.begin(ssid, password);
  }
  unsigned long past = 0;
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(500);
    past += 500;
    if( past > timeout ){
      Serial.println("\nCan't Connect");
      return -1;
    }
  }
  Serial.print("\nConnected : IP=");
  Serial.print(WiFi.localIP());
  Serial.print(" Mac=");
  Serial.println(WiFi.macAddress());

  return 0;
}

long wifi_try_connect(bool infinit_loop)
{
  long ret = -1;
  do{
    ret = wifi_connect(WIFI_SSID, WIFI_PASSWORD, WIFI_TIMEOUT);
    if( ret == 0 )
      return ret;

    Serial.print("\ninput SSID:");
    Serial.setTimeout(SERIAL_TIMEOUT1);
    char ssid[32 + 1] = {'\0'};
    ret = Serial.readBytesUntil('\r', ssid, sizeof(ssid) - 1);
    if( ret <= 0 )
      continue;

    delay(10);
    Serial.read();
    Serial.print("\ninput PASSWORD:");
    Serial.setTimeout(SERIAL_TIMEOUT2);
    char password[32 + 1] = {'\0'};
    ret = Serial.readBytesUntil('\r', password, sizeof(password) - 1);
    if( ret <= 0 )
      continue;

    delay(10);
    Serial.read();
    Serial.printf("\nSSID=%s PASSWORD=", ssid);
    for( int i = 0 ; i < strlen(password); i++ )
      Serial.print("*");
    Serial.println("");

    ret = wifi_connect(ssid, password, WIFI_TIMEOUT);
    if( ret == 0 )
      return ret;
  }while(infinit_loop);

  return ret;
}

bool wifi_is_connected(void)
{
  return (WiFi.status() == WL_CONNECTED);
}

long wifi_disconnect(void)
{
  if( !WiFi.disconnect(true) )
    return -1;
  return 0;
}

uint32_t get_ip_address(void)
{
  uint32_t ipaddress = WiFi.localIP();
  return (uint32_t)((((ipaddress >> 24) & 0xff) << 0) | (((ipaddress >> 16) & 0xff) << 8) | (((ipaddress >> 8) & 0xff) << 16) | (((ipaddress >> 0) & 0xff) << 24));
}

uint8_t *get_mac_address(void)
{
  WiFi.macAddress(wifi_mac_address);
  return wifi_mac_address;
}