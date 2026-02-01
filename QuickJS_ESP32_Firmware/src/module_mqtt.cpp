#include <Arduino.h>
#include "main_config.h"

#ifdef _MQTT_ENABLE_

#include <WiFi.h>
#include "quickjs.h"
#include "quickjs_esp32.h"
#include "config_utils.h"

#include "module_mqtt.h"
#include <PubSubClient.h>
#include <ArduinoJson.h>

#define DEFAULT_MQTT_BUFFER_SIZE 256
#define MQTT_CONNECT_TRY_COUNT 5
#define MAX_MQTT_EVENT  4

static JSContext *g_ctx = NULL;

static WiFiClient wifiClient;
static PubSubClient mqttClient(wifiClient);
static char *g_client_name = NULL;
static char *g_topic_name = NULL;
static JSValue g_callback_func = JS_UNDEFINED;
static bool isConnected = false;

typedef struct{
  char* topic_name;
  byte* payload;
} MQTT_EVENT_INFO;
static std::vector<MQTT_EVENT_INFO> g_event_list;

static void mqttCallback(char* topic, byte* payload, unsigned int length)
{
  if(g_callback_func == JS_UNDEFINED)
    return;
  if( g_event_list.size() >= MAX_MQTT_EVENT )
    return;

  MQTT_EVENT_INFO info;

  info.topic_name = (char*)strdup(topic);
  if( info.topic_name == NULL )
    return;

  info.payload = (byte*)malloc(length + 1);
  if( info.payload == NULL ){
    free(info.topic_name);
    return;
  }
  memmove(info.payload, payload, length);
  info.payload[length] = '\0';

  g_event_list.push_back(info);
}

static long mqttUnsubscribe(void)
{
  if( g_callback_func != JS_UNDEFINED ){
    mqttClient.unsubscribe(g_topic_name);
    free(g_topic_name);
    g_topic_name = NULL;

    while(g_event_list.size() > 0){
      MQTT_EVENT_INFO info = (MQTT_EVENT_INFO)g_event_list.front();
      free(info.topic_name);
      free(info.payload);
      g_event_list.erase(g_event_list.begin());
    }

    JS_FreeValue(g_ctx, g_callback_func);
    g_callback_func = JS_UNDEFINED;
  }

  return 0;
}

static void mqttDisconnect(void){
  if( isConnected ){
    mqttUnsubscribe();

    if (mqttClient.connected())
        mqttClient.disconnect();

    free(g_client_name);
    g_client_name = NULL;

    isConnected = false;
  }
}

static JSValue mqtt_connect(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  mqttDisconnect();

  String server = read_config_string(CONFIG_FNAME_MQTT);
  int delim = server.indexOf(':');
  if( delim < 0 )
    return JS_EXCEPTION;

  const char *client_name = JS_ToCString(ctx, argv[0]);
  if( client_name == NULL )
    return JS_EXCEPTION;
  g_client_name = (char*)strdup(client_name);
  JS_FreeCString(ctx, client_name);
  if( g_client_name == NULL )
    return JS_EXCEPTION;

  uint32_t buffer_size = DEFAULT_MQTT_BUFFER_SIZE;
  if( argc >= 2 )
      JS_ToUint32(ctx, &buffer_size, argv[1]);

  const char *username = NULL;
  if( argc >= 3 )
    username = JS_ToCString(ctx, argv[2]);
  const char *password = NULL;
  if( argc >= 4 )
    password = JS_ToCString(ctx, argv[3]);

  String host = server.substring(0, delim);
  String port = server.substring(delim + 1);

  mqttClient.setBufferSize(buffer_size);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setServer(host.c_str(), port.toInt());

  boolean ret = mqttClient.connect(g_client_name, username, password);
  if( username != NULL )
    JS_FreeCString(ctx, username);
  if( password != NULL )
    JS_FreeCString(ctx, password);
  if( !ret ){
    free(g_client_name);
    g_client_name = NULL;
    return JS_EXCEPTION;
  }
  isConnected = true;

  return JS_UNDEFINED;
}

static JSValue mqtt_disconnect(JSContext * ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  mqttDisconnect();
  return JS_UNDEFINED;
}

static JSValue mqtt_subscribe(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  if( !isConnected )
    return JS_EXCEPTION;

  mqttUnsubscribe();
  g_ctx = NULL;

  const char *topic = JS_ToCString(ctx, argv[0]);
  if( topic == NULL )
    return JS_EXCEPTION;

  g_topic_name = (char*)strdup(topic);
  JS_FreeCString(ctx, topic);
  if( g_topic_name == NULL )
    return JS_EXCEPTION;

  boolean result = mqttClient.subscribe(g_topic_name);
  if( !result ){
    free(g_topic_name);
    g_topic_name = NULL;
    return JS_EXCEPTION;
  }

  JSValue func = JS_DupValue(g_ctx, argv[1]);
  g_callback_func = func;

  g_ctx = ctx;

  return JS_UNDEFINED;
}

static JSValue mqtt_unsubscribe(JSContext * ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  if( !isConnected )
    return JS_EXCEPTION;

  mqttUnsubscribe();
  g_ctx = NULL;

  return JS_UNDEFINED;
}

static JSValue mqtt_publish(JSContext * ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  if( !isConnected )
    return JS_EXCEPTION;

  const char *topic = JS_ToCString(ctx, argv[0]);
  if( topic == NULL )
    return JS_EXCEPTION;

  const char *payload = JS_ToCString(ctx, argv[1]);
  if( payload == NULL ){
    JS_FreeCString(ctx, topic);
    return JS_EXCEPTION;
  }

  bool ret = mqttClient.publish(topic, payload);
  JS_FreeCString(ctx, payload);
  JS_FreeCString(ctx, topic);
  if( !ret )
    return JS_EXCEPTION;

  return JS_UNDEFINED;
}


static JSValue mqtt_setServer(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  const char *host = JS_ToCString(ctx, argv[0]);
  if( host == NULL )
    return JS_EXCEPTION;
  int32_t port;
  JS_ToInt32(ctx, &port, argv[1]);

  String server(host);
  server += ":";
  server += String(port);

  long ret;  
  ret = write_config_string(CONFIG_FNAME_MQTT, server.c_str());
  if( ret != 0 ){
    JS_FreeCString(ctx, host);
    return JS_EXCEPTION;
  }

  return JS_UNDEFINED;
}

static JSValue mqtt_getServer(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  String server = read_config_string(CONFIG_FNAME_MQTT);
  int delim = server.indexOf(':');
  if( delim < 0 )
    return JS_EXCEPTION;
  String host = server.substring(0, delim);
  String port = server.substring(delim + 1);

  JSValue obj = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, obj, "host", JS_NewString(ctx, host.c_str()));
  JS_SetPropertyStr(ctx, obj, "port", JS_NewUint32(ctx, port.toInt()));
  
  return obj;
}

static const JSCFunctionListEntry mqtt_funcs[] = {
    JSCFunctionListEntry{
        "connect", 0, JS_DEF_CFUNC, 0, {
          func : {4, JS_CFUNC_generic, mqtt_connect}
        }},
    JSCFunctionListEntry{
        "disconnect", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, mqtt_disconnect}
        }},
    JSCFunctionListEntry{
        "subscribe", 0, JS_DEF_CFUNC, 0, {
          func : {2, JS_CFUNC_generic, mqtt_subscribe}
        }},
    JSCFunctionListEntry{
        "unsubscribe", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, mqtt_unsubscribe}
        }},
    JSCFunctionListEntry{
        "publish", 0, JS_DEF_CFUNC, 0, {
          func : {2, JS_CFUNC_generic, mqtt_publish}
        }},
    JSCFunctionListEntry{
        "setServer", 0, JS_DEF_CFUNC, 0, {
          func : {2, JS_CFUNC_generic, mqtt_setServer}
        }},
    JSCFunctionListEntry{
        "getServer", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, mqtt_getServer}
        }},
};

JSModuleDef *addModule_mqtt(JSContext *ctx, JSValue global)
{
  JSModuleDef *mod;

  mod = JS_NewCModule(ctx, "Mqtt", [](JSContext *ctx, JSModuleDef *m) {
          return JS_SetModuleExportList(
              ctx, m, mqtt_funcs,
              sizeof(mqtt_funcs) / sizeof(JSCFunctionListEntry));
        });
  if (mod) {
    JS_AddModuleExportList(
        ctx, mod, mqtt_funcs,
        sizeof(mqtt_funcs) / sizeof(JSCFunctionListEntry));
  }

  return mod;
}

void loopModule_mqtt(void)
{
  if( isConnected ){
    mqttClient.loop();

    for( int i = 0 ; !mqttClient.connected() && i < MQTT_CONNECT_TRY_COUNT ; i++ ){
      Serial.println("Mqtt Reconnecting");
      if (mqttClient.connect(g_client_name)){
        if( g_callback_func != JS_UNDEFINED )
          mqttClient.subscribe(g_topic_name);
        Serial.println("Mqtt Reconnected");

        break;
      }

      delay(500);
    }

    if( g_callback_func != JS_UNDEFINED ){
      while(g_event_list.size() > 0){
        MQTT_EVENT_INFO info = g_event_list.front();
        JSValue obj = JS_NewObject(g_ctx);
        JS_SetPropertyStr(g_ctx, obj, "topic", JS_NewString(g_ctx, (const char *)info.topic_name));
        JS_SetPropertyStr(g_ctx, obj, "payload", JS_NewString(g_ctx, (const char *)info.payload));
        free(info.topic_name);
        free(info.payload);

        ESP32QuickJS *qjs = (ESP32QuickJS *)JS_GetContextOpaque(g_ctx);
        JSValue ret = qjs->callJsFunc_with_arg(g_ctx, g_callback_func, g_callback_func, 1, &obj);
        JS_FreeValue(g_ctx, obj);
        JS_FreeValue(g_ctx, ret);

        g_event_list.erase(g_event_list.begin());
      }
    }
  }
}

void endModule_mqtt(void)
{
  mqttDisconnect();
}

JsModuleEntry mqtt_module = {
  NULL,
  addModule_mqtt,
  loopModule_mqtt,
  endModule_mqtt
};

#endif
