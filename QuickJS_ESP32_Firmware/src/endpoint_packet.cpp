#include <Arduino.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include <unordered_map> 
#include "module_utils.h"

#include "main_config.h"
#include "endpoint_types.h"
#include "endpoint_packet.h"
#include "wifi_utils.h"

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>

#include "endpoint_esp32.h"
#include "endpoint_gpio.h"
#include "endpoint_wire.h"
#include "endpoint_prefs.h"
#ifdef _LEDC_ENABLE_
#include "endpoint_ledc.h"
#endif
#ifdef _RTC_ENABLE_
#include "endpoint_rtc.h"
#endif
#ifdef _IMU_ENABLE_
#include "endpoint_imu.h"
#endif
#ifdef _SD_ENABLE_
#include "endpoint_sd.h"
#endif
#ifdef _LCD_ENABLE_
#include "endpoint_lcd.h"
#endif

static AsyncWebServer server(HTTP_PORT);

static std::unordered_map<std::string, EndpointEntry*> endpoint_list;

void packet_appendEntry(EndpointEntry *tables, int num_of_entry)
{
  for(int i = 0 ; i < num_of_entry ; i++ )
    endpoint_list[tables[i].name] = &tables[i];
}

long packet_execute(const char *endpoint, const JsonObject& params, const JsonObject& responseResult)
{
  std::unordered_map<std::string, EndpointEntry*>::iterator itr = endpoint_list.find(endpoint);
  if( itr != endpoint_list.end() ){
    EndpointEntry *entry = itr->second;
    long ret = entry->impl((JsonObject&)params, (JsonObject&)responseResult, entry->magic);
    return ret;
  }

  Serial.println("endpoint not found");
  return -1;
}

static void notFound(AsyncWebServerRequest *request)
{
  if (request->method() == HTTP_OPTIONS){
    request->send(200);
  }else{
#ifdef STATIC_REDIRECT_PAGE
    String url(STATIC_REDIRECT_PAGE);
    IPAddress address = WiFi.localIP();
    url += "?base_url=http%3A%2F%2F" + String(address[0]) + "." + String(address[1]) + "." + String(address[2]) + "." + String(address[3]);
    request->redirect(url);
#else
    request->send(404);
#endif
  }
}

long packet_initialize(void)
{
  packet_appendEntry(esp32_table, num_of_esp32_entry);
  packet_appendEntry(gpio_table, num_of_gpio_entry);
  packet_appendEntry(wire_table, num_of_wire_entry);
  packet_appendEntry(prefs_table, num_of_prefs_entry);
#ifdef _LEDC_ENABLE_
  packet_appendEntry(ledc_table, num_of_ledc_entry);
#endif
#ifdef _RTC_ENABLE_
  packet_appendEntry(rtc_table, num_of_rtc_entry);
#endif
#ifdef _IMU_ENABLE_
  packet_appendEntry(imu_table, num_of_imu_entry);
#endif
#ifdef _SD_ENABLE_
  packet_appendEntry(sd_table, num_of_sd_entry);
#endif
#ifdef _LCD_ENABLE_
  packet_appendEntry(lcd_table, num_of_lcd_entry);
#endif

  AsyncCallbackJsonWebHandler *handler = new AsyncCallbackJsonWebHandler("/endpoint", [](AsyncWebServerRequest *request, JsonVariant &json) {
    bool sem = xSemaphoreTake(binSem, portMAX_DELAY);
    const JsonObject& jsonObj = json.as<JsonObject>();
    const char *endpoint = jsonObj["endpoint"];
    AsyncJsonResponse *response = new AsyncJsonResponse(false, PACKET_JSON_DOCUMENT_SIZE);
    const JsonObject& responseResult = response->getRoot();
    responseResult["status"] = "OK";
    responseResult["endpoint"] = (char*)endpoint;
    const JsonObject& params = jsonObj["params"];
    long ret = packet_execute(endpoint, params, responseResult);
    if( ret != 0 ){
      responseResult.clear();
      responseResult["status"] = "NG";
      responseResult["endpoint"] = (char*)endpoint;
      responseResult["message"] = "unknown";
    }
    response->setLength();
    request->send(response);
    if( sem )
      xSemaphoreGive(binSem);
  });
  server.addHandler(handler);

  AsyncCallbackJsonWebHandler *handler_putText = new AsyncCallbackJsonWebHandler("/webcall_putText", [](AsyncWebServerRequest *request, JsonVariant &json) {
    bool sem = xSemaphoreTake(binSem, portMAX_DELAY);
    const JsonObject& jsonObj = json.as<JsonObject>();
    AsyncJsonResponse *response = new AsyncJsonResponse(false, PACKET_JSON_DOCUMENT_SIZE);
    const JsonObject& responseResult = response->getRoot();
    responseResult["status"] = "OK";
    long ret = webcall_putText(jsonObj);
    if( ret != 0 ){
      responseResult.clear();
      responseResult["status"] = "NG";
      responseResult["message"] = "unknown";
    }
    response->setLength();
    request->send(response);
    if( sem )
      xSemaphoreGive(binSem);
  });
  server.addHandler(handler_putText);

  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "*");
#ifdef ENABLE_STATIC_WEB_PAGE
  server.serveStatic("/", SPIFFS, "/html/").setDefaultFile("index.html");
#endif
  server.onNotFound(notFound);
//  server.begin();

  return 0;
}

long packet_open(void)
{
  if( !wifi_is_connected() )
    return -1;

  if (!MDNS.begin(MDNS_NAME)){
    Serial.println("MDNS.begin error");
  }else{
    Serial.printf("MDNS_NAME: %s\n", MDNS_NAME);
    MDNS.addService("http", "tcp", HTTP_PORT);
    Serial.printf("serivce_name: %s, TCP_PORT: %d\n", "http", HTTP_PORT);
  }

  server.begin();
  
  return 0;
}

long packet_close(void){
  server.end();
  MDNS.end();

  return 0;
}

