#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <Syslog.h>
#include "main_config.h"
#include <ESP32Ping.h>
#include <M5Unified.h>

#include "main_config.h"
#include "quickjs.h"
#include "quickjs_esp32.h"
#include "module_type.h"
#include "module_esp32.h"
#include "config_utils.h"
#include "wifi_utils.h"

#include "endpoint_packet.h"

#define GLOBAL_ESP32
#define GLOBAL_CONSOLE

#define MODEL_OTHER         0
#define MODEL_M5Core2       1
#define MODEL_M5Core        2 /* no use */
#define MODEL_M5Fire        3 /* no use */
#define MODEL_M5StickCPlus  4
#define MODEL_M5CoreInk     5
#define MODEL_M5Paper       6
#define MODEL_M5Tough       7
#define MODEL_M5StickC      8
#define MODEL_M5Atom        9
#define MODEL_M5StampC3     10
#define MODEL_M5StampC3U    11
#define MODEL_M5StampS3     12
#define MODEL_M5Stack       13
#define MODEL_M5StickCPlus2 14
#define MODEL_M5Station     15
#define MODEL_M5CoreS3      16
#define MODEL_M5AtomS3      17
#define MODEL_M5Dial        18
#define MODEL_M5DinMeter    19
#define MODEL_M5Cardputer   20
#define MODEL_M5AirQ        21
#define MODEL_M5VAMeter     22
#define MODEL_M5CoreS3SE    23
#define MODEL_M5AtomS3R     24
#define MODEL_M5AtomPsram   25
#define MODEL_M5AtomU       26
#define MODEL_M5Camera      27
#define MODEL_M5TimerCam    28
#define MODEL_M5StampPico   29
#define MODEL_M5AtomS3Lite  30
#define MODEL_M5AtomS3U     31
#define MODEL_M5Capsule     32
#define MODEL_M5NanoC6      33

static WiFiUDP syslog_udp;
static Syslog g_syslog(syslog_udp);
static char *p_syslog_host = NULL;

long syslog_send(uint16_t pri, const char *p_message)
{
  bool ret = g_syslog.log(pri, p_message);
  return ret ? 0 : -1;
}

long syslog_changeServer(const char *host, uint16_t port)
{
  g_syslog.appName(MDNS_SERVICE);
  g_syslog.deviceHostname(MDNS_NAME);
  g_syslog.defaultPriority(LOG_INFO | LOG_USER);

  if( p_syslog_host != NULL )
    free(p_syslog_host);
  p_syslog_host = (char*)malloc(strlen(host) + 1);
  if( p_syslog_host == NULL )
    return -1;
  strcpy(p_syslog_host, host);
  g_syslog.server(p_syslog_host, port);

  Serial.printf("syslog: host=%s, port=%d\n", p_syslog_host, port);

  return 0;
}

static JSValue esp32_reboot(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  g_download_buffer[0] = '\0';
  g_fileloading = FILE_LOADING_REBOOT;
  
  return JS_UNDEFINED;
}

static JSValue esp32_restart(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  g_download_buffer[0] = '\0';
  g_fileloading = FILE_LOADING_RESTART;

  return JS_UNDEFINED;
}

static JSValue esp32_set_loop(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  ESP32QuickJS *qjs = (ESP32QuickJS *)JS_GetContextOpaque(ctx);
  qjs->setLoopFunc(JS_DupValue(ctx, argv[0]));
  return JS_UNDEFINED;
}

static JSValue esp32_update(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  ESP32QuickJS *qjs = (ESP32QuickJS *)JS_GetContextOpaque(ctx);
  qjs->update_modules();
  return JS_UNDEFINED;
}

static JSValue esp32_get_ipaddress(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  return JS_NewUint32(ctx, get_ip_address());
}

static JSValue esp32_get_macaddress(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint8_t *address = get_mac_address();

  JSValue jsArray = JS_NewArray(ctx);
  for (int i = 0; i < 6; i++)
    JS_SetPropertyUint32(ctx, jsArray, i, JS_NewInt32(ctx, address[i]));

  return jsArray;
}

static JSValue esp32_get_deviceModel(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint8_t model;
  m5::board_t boad = M5.getBoard();
  switch(boad){
    case lgfx::board_t::board_M5Stack: model = MODEL_M5Stack; break;
    case lgfx::board_t::board_M5StackCore2: model = MODEL_M5Core2; break;
    case lgfx::board_t::board_M5StickC: model = MODEL_M5StickC; break;
    case lgfx::board_t::board_M5StickCPlus: model = MODEL_M5StickCPlus; break;
    case lgfx::board_t::board_M5StickCPlus2: model = MODEL_M5StickCPlus2; break;
    case lgfx::board_t::board_M5StackCoreInk: model = MODEL_M5CoreInk; break;
    case lgfx::board_t::board_M5Paper: model = MODEL_M5Paper; break;
    case lgfx::board_t::board_M5Tough: model = MODEL_M5Tough; break;
    case lgfx::board_t::board_M5Station: model = MODEL_M5Station; break;
    case lgfx::board_t::board_M5StackCoreS3: model = MODEL_M5CoreS3; break;
    case lgfx::board_t::board_M5AtomS3: model = MODEL_M5AtomS3; break;
    case lgfx::board_t::board_M5Dial: model = MODEL_M5Dial; break;
    case lgfx::board_t::board_M5DinMeter: model = MODEL_M5DinMeter; break;
    case lgfx::board_t::board_M5Cardputer: model = MODEL_M5Cardputer; break;
    case lgfx::board_t::board_M5AirQ: model = MODEL_M5AirQ; break;
    case lgfx::board_t::board_M5VAMeter: model = MODEL_M5VAMeter; break;
    case lgfx::board_t::board_M5StackCoreS3SE: model = MODEL_M5CoreS3SE; break;
    case lgfx::board_t::board_M5AtomS3R: model = MODEL_M5AtomS3R; break;
    case lgfx::board_t::board_M5Atom: model = MODEL_M5Atom; break;
    case lgfx::board_t::board_M5AtomPsram: model = MODEL_M5AtomPsram; break;
    case lgfx::board_t::board_M5AtomU: model = MODEL_M5AtomU; break;
    case lgfx::board_t::board_M5Camera: model = MODEL_M5Camera; break;
    case lgfx::board_t::board_M5TimerCam: model = MODEL_M5TimerCam; break;
    case lgfx::board_t::board_M5StampPico: model = MODEL_M5StampPico; break;
    case lgfx::board_t::board_M5StampC3: model = MODEL_M5StampC3; break;
    case lgfx::board_t::board_M5StampC3U: model = MODEL_M5StampC3U; break;
    case lgfx::board_t::board_M5StampS3: model = MODEL_M5StampS3; break;
    case lgfx::board_t::board_M5AtomS3Lite: model = MODEL_M5AtomS3Lite; break;
    case lgfx::board_t::board_M5AtomS3U: model = MODEL_M5AtomS3U; break;
    case lgfx::board_t::board_M5Capsule: model = MODEL_M5Capsule; break;
    case lgfx::board_t::board_M5NanoC6: model = MODEL_M5NanoC6; break;
    default: model = MODEL_OTHER; break;
  }

  return JS_NewUint32(ctx, model);
}

static JSValue esp32_check_putText(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  if( g_fileloading != FILE_LOADING_TEXT ){
    return JS_NULL;
  }

  JSValue value = JS_NewString(ctx, g_download_buffer);
  g_fileloading = FILE_LOADING_NONE;

  return value;
}

static JSValue esp32_syslog(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  const char *message = JS_ToCString(ctx, argv[0]);
  if( message == NULL )
    return JS_EXCEPTION;

  syslog_send(LOG_INFO, message);

  JS_FreeCString(ctx, message);

  return JS_UNDEFINED;
}

static JSValue esp32_syslog2(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint32_t pri;
  JS_ToUint32(ctx, &pri, argv[0]);

  const char *message = JS_ToCString(ctx, argv[1]);
  if( message == NULL )
    return JS_EXCEPTION;

  syslog_send((uint16_t)pri, message);

  JS_FreeCString(ctx, message);

  return JS_UNDEFINED;
}

static JSValue esp32_setSyslogServer(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  const char *host = JS_ToCString(ctx, argv[0]);
  if( host == NULL )
    return JS_EXCEPTION;
  uint32_t port;
  JS_ToUint32(ctx, &port, argv[1]);

  String server(host);
  server += ":";
  server += String(port);

  long ret;  
  ret = write_config_string(CONFIG_FNAME_SYSLOG, server.c_str());
  if( ret != 0 ){
    JS_FreeCString(ctx, host);
    return JS_EXCEPTION;
  }

  ret = syslog_changeServer(host, port);
  JS_FreeCString(ctx, host);
  if( ret != 0 ){
    return JS_EXCEPTION;
  }

  return JS_UNDEFINED;
}

static JSValue esp32_getSyslogServer(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  String server = read_config_string(CONFIG_FNAME_SYSLOG);
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

static JSValue esp32_getMemoryUsage(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  JSMemoryUsage usage;
  ESP32QuickJS *qjs = (ESP32QuickJS *)JS_GetContextOpaque(ctx);
  qjs->getMemoryUsage(&usage);

  JSValue obj = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, obj, "malloc_limit", JS_NewUint32(ctx, usage.malloc_limit));
  JS_SetPropertyStr(ctx, obj, "memory_usage_size", JS_NewUint32(ctx, usage.memory_used_size));
  JS_SetPropertyStr(ctx, obj, "malloc_size", JS_NewUint32(ctx, usage.malloc_size));
  JS_SetPropertyStr(ctx, obj, "total_heap", JS_NewUint32(ctx, ESP.getHeapSize()));
  JS_SetPropertyStr(ctx, obj, "free_heap", JS_NewUint32(ctx, ESP.getFreeHeap()));
  JS_SetPropertyStr(ctx, obj, "total_psram", JS_NewUint32(ctx, ESP.getPsramSize()));
  JS_SetPropertyStr(ctx, obj, "free_psram", JS_NewUint32(ctx, ESP.getFreePsram()));

  return obj;
}

static JSValue esp32_ping(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  const char *host = JS_ToCString(ctx, argv[0]);
  if( host == NULL )
    return JS_EXCEPTION;

  bool result = Ping.ping(host);

  JS_FreeCString(ctx, host);

  return JS_NewBool(ctx, result);
}

static JSValue esp32_wifi_connect(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint32_t timeout;
  JS_ToUint32(ctx, &timeout, argv[0]);

  if( wifi_is_connected() )
    return JS_EXCEPTION;

  long ret = wifi_connect(NULL, NULL, timeout);
  if( ret != 0 )
    return JS_EXCEPTION;

  ret = packet_open();
  if( ret != 0 )
    return JS_EXCEPTION;

  return JS_NewInt32(ctx, ret);
}

static JSValue esp32_wifi_disconnect(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  if( !wifi_is_connected() )
    return JS_EXCEPTION;

  packet_close();

  long ret = wifi_disconnect();

  return JS_NewInt32(ctx, ret);
}

static JSValue esp32_wifi_is_connected(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  bool ret = wifi_is_connected();

  return JS_NewBool(ctx, ret);
}

static JSValue esp32_console_log(JSContext *ctx, JSValueConst jsThis, int argc,
                            JSValueConst *argv, int magic) {
  int i = 0;
  if( magic == 0 ){
    bool enable = JS_ToBool(ctx, argv[0]);
    if( enable )
      return JS_UNDEFINED;
    i = 1;
  }
  for (; i < argc; i++) {
    const char *str = JS_ToCString(ctx, argv[i]);
    if (str) {
      if( magic == 2 ) Serial.print("[info]");
      else if( magic == 3 ) Serial.print("[debug] ");
      else if( magic == 4 ) Serial.print("[warn] ");
      else if( magic == 5 ) Serial.print("[error] ");
      Serial.println(str);
      JS_FreeCString(ctx, str);
    }
  }

  return JS_UNDEFINED;
}

static const JSCFunctionListEntry console_funcs[] = {
    JSCFunctionListEntry{"assert", 0, JS_DEF_CFUNC, 0, {
                           func : {2, JS_CFUNC_generic_magic, {generic_magic : esp32_console_log }}
                         }},
    JSCFunctionListEntry{"log", 0, JS_DEF_CFUNC, 1, {
                           func : {1, JS_CFUNC_generic_magic, {generic_magic : esp32_console_log }}
                         }},
    JSCFunctionListEntry{"info", 0, JS_DEF_CFUNC, 2, {
                           func : {1, JS_CFUNC_generic_magic, {generic_magic : esp32_console_log }}
                         }},
    JSCFunctionListEntry{"debug", 0, JS_DEF_CFUNC, 3, {
                           func : {1, JS_CFUNC_generic_magic, {generic_magic : esp32_console_log }}
                         }},
    JSCFunctionListEntry{"warn", 0, JS_DEF_CFUNC, 4, {
                           func : {1, JS_CFUNC_generic_magic, {generic_magic : esp32_console_log }}
                         }},
    JSCFunctionListEntry{"error", 0, JS_DEF_CFUNC, 5, {
                           func : {1, JS_CFUNC_generic_magic, {generic_magic : esp32_console_log }}
                         }},
};

JSModuleDef *addModule_console(JSContext *ctx, JSValue global)
{
  JSModuleDef *mod;
  mod = JS_NewCModule(ctx, "Console", [](JSContext *ctx, JSModuleDef *m)
                      { return JS_SetModuleExportList(
                            ctx, m, console_funcs,
                            sizeof(console_funcs) / sizeof(JSCFunctionListEntry)); });
  if (mod){
    JS_AddModuleExportList(
        ctx, mod, console_funcs,
        sizeof(console_funcs) / sizeof(JSCFunctionListEntry));
  }

#ifdef GLOBAL_CONSOLE
  // import * as console from "Console";
  JSValue console = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, global, "console", console);
  JS_SetPropertyFunctionList(
      ctx, console, console_funcs,
      sizeof(console_funcs) / sizeof(JSCFunctionListEntry));
#endif

  return mod;
}

static const JSCFunctionListEntry esp32_funcs[] = {
    JSCFunctionListEntry{"reboot", 0, JS_DEF_CFUNC, 0, {
                           func : {0, JS_CFUNC_generic, esp32_reboot}
                         }},
    JSCFunctionListEntry{"restart", 0, JS_DEF_CFUNC, 0, {
                           func : {0, JS_CFUNC_generic, esp32_restart}
                         }},
    JSCFunctionListEntry{"setLoop", 0, JS_DEF_CFUNC, 0, {
                           func : {1, JS_CFUNC_generic, esp32_set_loop}
                         }},
    JSCFunctionListEntry{"update", 0, JS_DEF_CFUNC, 0, {
                           func : {0, JS_CFUNC_generic, esp32_update}
                         }},
    JSCFunctionListEntry{"getIpAddress", 0, JS_DEF_CFUNC, 0, {
                           func : {0, JS_CFUNC_generic, esp32_get_ipaddress}
                         }},
    JSCFunctionListEntry{"getMacAddress", 0, JS_DEF_CFUNC, 0, {
                           func : {0, JS_CFUNC_generic, esp32_get_macaddress}
                         }},
    JSCFunctionListEntry{"getDeviceModel", 0, JS_DEF_CFUNC, 0, {
                           func : {0, JS_CFUNC_generic, esp32_get_deviceModel}
                         }},
    JSCFunctionListEntry{"checkPutText", 0, JS_DEF_CFUNC, 0, {
                           func : {0, JS_CFUNC_generic, esp32_check_putText}
                         }},
    JSCFunctionListEntry{"syslog", 0, JS_DEF_CFUNC, 0, {
                           func : {1, JS_CFUNC_generic, esp32_syslog}
                         }},
    JSCFunctionListEntry{"syslog2", 0, JS_DEF_CFUNC, 0, {
                           func : {2, JS_CFUNC_generic, esp32_syslog2}
                         }},
    JSCFunctionListEntry{"setSyslogServer", 0, JS_DEF_CFUNC, 0, {
                           func : {2, JS_CFUNC_generic, esp32_setSyslogServer}
                         }},
    JSCFunctionListEntry{"getSyslogServer", 0, JS_DEF_CFUNC, 0, {
                           func : {0, JS_CFUNC_generic, esp32_getSyslogServer}
                         }},
    JSCFunctionListEntry{"getMemoryUsage", 0, JS_DEF_CFUNC, 0, {
                           func : {0, JS_CFUNC_generic, esp32_getMemoryUsage}
                         }},
    JSCFunctionListEntry{"ping", 0, JS_DEF_CFUNC, 0, {
                           func : {1, JS_CFUNC_generic, esp32_ping}
                         }},
    JSCFunctionListEntry{"wifiConnect", 0, JS_DEF_CFUNC, 0, {
                           func : {1, JS_CFUNC_generic, esp32_wifi_connect}
                         }},
    JSCFunctionListEntry{"wifiDisconnect", 0, JS_DEF_CFUNC, 0, {
                           func : {0, JS_CFUNC_generic, esp32_wifi_disconnect}
                         }},
    JSCFunctionListEntry{"wifiIsConnected", 0, JS_DEF_CFUNC, 0, {
                           func : {0, JS_CFUNC_generic, esp32_wifi_is_connected}
                         }},
    JSCFunctionListEntry{
        "MODEL_OTHER", 0, JS_DEF_PROP_INT32, 0, {
          i32 : MODEL_OTHER
        }},
    JSCFunctionListEntry{
        "MODEL_M5Core2", 0, JS_DEF_PROP_INT32, 0, {
          i32 : MODEL_M5Core2
        }},
    JSCFunctionListEntry{
        "MODEL_M5Core", 0, JS_DEF_PROP_INT32, 0, {
          i32 : MODEL_M5Core
        }},
    JSCFunctionListEntry{
        "MODEL_M5Fire", 0, JS_DEF_PROP_INT32, 0, {
          i32 : MODEL_M5Fire
        }},
    JSCFunctionListEntry{
        "MODEL_M5StickCPlus", 0, JS_DEF_PROP_INT32, 0, {
          i32 : MODEL_M5StickCPlus
        }},
    JSCFunctionListEntry{
        "MODEL_M5CoreInk", 0, JS_DEF_PROP_INT32, 0, {
          i32 : MODEL_M5CoreInk
        }},
    JSCFunctionListEntry{
        "MODEL_M5Paper", 0, JS_DEF_PROP_INT32, 0, {
          i32 : MODEL_M5Paper
        }},
    JSCFunctionListEntry{
        "MODEL_M5Tough", 0, JS_DEF_PROP_INT32, 0, {
          i32 : MODEL_M5Tough
        }},
    JSCFunctionListEntry{
        "MODEL_M5StickC", 0, JS_DEF_PROP_INT32, 0, {
          i32 : MODEL_M5StickC
        }},
    JSCFunctionListEntry{
        "MODEL_M5Atom", 0, JS_DEF_PROP_INT32, 0, {
          i32 : MODEL_M5Atom
        }},
    JSCFunctionListEntry{
        "MODEL_M5StampC3", 0, JS_DEF_PROP_INT32, 0, {
          i32 : MODEL_M5StampC3
        }},
    JSCFunctionListEntry{
        "MODEL_M5StampC3U", 0, JS_DEF_PROP_INT32, 0, {
          i32 : MODEL_M5StampC3U
        }},
    JSCFunctionListEntry{
        "MODEL_M5StampS3", 0, JS_DEF_PROP_INT32, 0, {
          i32 : MODEL_M5StampS3
        }},
    JSCFunctionListEntry{
        "MODEL_M5Stack", 0, JS_DEF_PROP_INT32, 0, {
          i32 : MODEL_M5Stack
        }},
    JSCFunctionListEntry{
        "MODEL_M5StickCPlus2", 0, JS_DEF_PROP_INT32, 0, {
          i32 : MODEL_M5StickCPlus2
        }},
    JSCFunctionListEntry{
        "MODEL_M5Station", 0, JS_DEF_PROP_INT32, 0, {
          i32 : MODEL_M5Station
        }},
    JSCFunctionListEntry{
        "MODEL_M5CoreS3", 0, JS_DEF_PROP_INT32, 0, {
          i32 : MODEL_M5CoreS3
        }},
    JSCFunctionListEntry{
        "MODEL_M5AtomS3", 0, JS_DEF_PROP_INT32, 0, {
          i32 : MODEL_M5AtomS3
        }},
    JSCFunctionListEntry{
        "MODEL_M5Dial", 0, JS_DEF_PROP_INT32, 0, {
          i32 : MODEL_M5Dial
        }},
    JSCFunctionListEntry{
        "MODEL_M5DinMeter", 0, JS_DEF_PROP_INT32, 0, {
          i32 : MODEL_M5DinMeter
        }},
    JSCFunctionListEntry{
        "MODEL_M5Cardputer", 0, JS_DEF_PROP_INT32, 0, {
          i32 : MODEL_M5Cardputer
        }},
    JSCFunctionListEntry{
        "MODEL_M5AirQ", 0, JS_DEF_PROP_INT32, 0, {
          i32 : MODEL_M5AirQ
        }},
    JSCFunctionListEntry{
        "MODEL_M5VAMeter", 0, JS_DEF_PROP_INT32, 0, {
          i32 : MODEL_M5VAMeter
        }},
    JSCFunctionListEntry{
        "MODEL_M5CoreS3SE", 0, JS_DEF_PROP_INT32, 0, {
          i32 : MODEL_M5CoreS3SE
        }},
    JSCFunctionListEntry{
        "MODEL_M5AtomS3R", 0, JS_DEF_PROP_INT32, 0, {
          i32 : MODEL_M5AtomS3R
        }},
    JSCFunctionListEntry{
        "MODEL_M5AtomPsram", 0, JS_DEF_PROP_INT32, 0, {
          i32 : MODEL_M5AtomPsram
        }},
    JSCFunctionListEntry{
        "MODEL_M5AtomU", 0, JS_DEF_PROP_INT32, 0, {
          i32 : MODEL_M5AtomU
        }},
    JSCFunctionListEntry{
        "MODEL_M5Camera", 0, JS_DEF_PROP_INT32, 0, {
          i32 : MODEL_M5Camera
        }},
    JSCFunctionListEntry{
        "MODEL_M5TimerCam", 0, JS_DEF_PROP_INT32, 0, {
          i32 : MODEL_M5TimerCam
        }},
    JSCFunctionListEntry{
        "MODEL_M5StampPico", 0, JS_DEF_PROP_INT32, 0, {
          i32 : MODEL_M5StampPico
        }},
    JSCFunctionListEntry{
        "MODEL_M5AtomS3Lite", 0, JS_DEF_PROP_INT32, 0, {
          i32 : MODEL_M5AtomS3Lite
        }},
    JSCFunctionListEntry{
        "MODEL_M5AtomS3U", 0, JS_DEF_PROP_INT32, 0, {
          i32 : MODEL_M5AtomS3U
        }},
    JSCFunctionListEntry{
        "MODEL_M5Capsule", 0, JS_DEF_PROP_INT32, 0, {
          i32 : MODEL_M5Capsule
        }},
    JSCFunctionListEntry{
        "MODEL_M5NanoC6", 0, JS_DEF_PROP_INT32, 0, {
          i32 : MODEL_M5NanoC6
        }},
    JSCFunctionListEntry{
        "SYSLOG_PRIORITY_EMERG", 0, JS_DEF_PROP_INT32, 0, {
          i32 : LOG_EMERG
        }},
    JSCFunctionListEntry{
        "SYSLOG_PRIORITY_ALERT", 0, JS_DEF_PROP_INT32, 0, {
          i32 : LOG_ALERT
        }},
    JSCFunctionListEntry{
        "SYSLOG_PRIORITY_CRIT", 0, JS_DEF_PROP_INT32, 0, {
          i32 : LOG_CRIT
        }},
    JSCFunctionListEntry{
        "SYSLOG_PRIORITY_ERR", 0, JS_DEF_PROP_INT32, 0, {
          i32 : LOG_ERR
        }},
    JSCFunctionListEntry{
        "SYSLOG_PRIORITY_WARNING", 0, JS_DEF_PROP_INT32, 0, {
          i32 : LOG_WARNING
        }},
    JSCFunctionListEntry{
        "SYSLOG_PRIORITY_NOTICE", 0, JS_DEF_PROP_INT32, 0, {
          i32 : LOG_NOTICE
        }},
    JSCFunctionListEntry{
        "SYSLOG_PRIORITY_INFO", 0, JS_DEF_PROP_INT32, 0, {
          i32 : LOG_INFO
        }},
    JSCFunctionListEntry{
        "SYSLOG_PRIORITY_DEBUG", 0, JS_DEF_PROP_INT32, 0, {
          i32 : LOG_DEBUG
        }},
};

JSModuleDef *addModule_esp32(JSContext *ctx, JSValue global)
{
  JSModuleDef *mod;
  mod = JS_NewCModule(ctx, "Esp32", [](JSContext *ctx, JSModuleDef *m)
                      { return JS_SetModuleExportList(
                            ctx, m, esp32_funcs,
                            sizeof(esp32_funcs) / sizeof(JSCFunctionListEntry)); });
  if (mod){
    JS_AddModuleExportList(
        ctx, mod, esp32_funcs,
        sizeof(esp32_funcs) / sizeof(JSCFunctionListEntry));
  }

#ifdef GLOBAL_ESP32
  // import * as esp32 from "Esp32";
  JSValue esp32 = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, global, "esp32", esp32);
  JS_SetPropertyFunctionList(
      ctx, esp32, esp32_funcs,
      sizeof(esp32_funcs) / sizeof(JSCFunctionListEntry));
#endif

  return mod;
}

long initialize_esp32(void)
{
  String server = read_config_string(CONFIG_FNAME_SYSLOG);
  int delim = server.indexOf(':');
  if( delim >= 0 ){
    String host = server.substring(0, delim);
    String port = server.substring(delim + 1);
    syslog_changeServer(host.c_str(), port.toInt());
  }

  return 0;
}

JsModuleEntry esp32_module = {
  initialize_esp32,
  addModule_esp32,
  NULL,
  NULL
};

JsModuleEntry console_module = {
  NULL,
  addModule_console,
  NULL,
  NULL
};

long esp32_initialize(void)
{
  Serial.begin(115200);

  auto cfg = M5.config();
  M5.begin(cfg);

  delay(500);

  Serial.println("[initializing]");

  return 0;
}

void esp32_update(void)
{
  M5.update();
}