#include <Arduino.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include <time.h>

#include "main_config.h"
#include "quickjs_esp32.h"
#include "config_utils.h"
#include "wifi_utils.h"
#include "lib_snmp.h"

#include "endpoint_types.h"
#include "endpoint_packet.h"

extern const char jscode_default[] asm("_binary_rom_default_js_start");
extern const char jscode_epilogue[] asm("_binary_rom_epilogue_js_start");

bool g_autoupdate = false;

char g_download_buffer[FILE_BUFFER_SIZE];
unsigned char g_fileloading = FILE_LOADING_NONE;
esp_sleep_wakeup_cause_t g_sleepReason = ESP_SLEEP_WAKEUP_UNDEFINED;

ESP32QuickJS qjs;
SemaphoreHandle_t binSem;

static long m5_connect(void);
static long start_qjs(void);
static char* load_jscode(void);
static long load_all_modules(void);

void setup()
{
  long ret;
  ret = esp32_initialize();
  if( ret != 0 )
    Serial.println("esp32_initialize failed");

  if( !SPIFFS.begin() )
    Serial.println("SPIFFS begin failed");
  delay(100);

  if( !SPIFFS.exists(DUMMY_FNAME) ){
    File fp = SPIFFS.open(DUMMY_FNAME, FILE_WRITE);
    if( !fp ){
      Serial.println("SPIFFS FORMAT START");
      if( !SPIFFS.format() )
        Serial.println("SPIFFS format failed");
      Serial.println("SPIFFS FORMAT END");

      if( !SPIFFS.begin() )
        Serial.println("SPIFFS begin failed");
      delay(100);

      fp = SPIFFS.open(DUMMY_FNAME, FILE_WRITE);
      if( fp )
        fp.close();
    }else{
      fp.close();
    }
  }
  if( !SPIFFS.exists(MODULE_DIR) )
    SPIFFS.mkdir(MODULE_DIR);

  g_sleepReason = esp_sleep_get_wakeup_cause();

  ret = m5_connect();
  if( ret != 0 )
    Serial.println("m5_connect error");

  ret = packet_initialize();
  if( ret != 0 )
    Serial.println("packet_initialize error");

  binSem = xSemaphoreCreateBinary();
  xSemaphoreGive(binSem);

  ret = packet_open();
  if( ret != 0 )
    Serial.println("packet_open error");

#ifdef _SNMP_AGENT_ENABLE_    
  ret = snmp_initialize();
  if( ret != 0 )
    Serial.println("snmp_initialize error");
#endif

  long conf = read_config_long(CONFIG_INDEX_AUTOUPDATE, 0);
  g_autoupdate = (conf != 0) ? true : false;
  if( g_autoupdate )
    Serial.println("autoupdate: on");

  qjs.initialize_modules();

  start_qjs();
}

void loop()
{
  if( g_fileloading == FILE_LOADING_PAUSE || g_fileloading == FILE_LOADING_STOP ){
    delay(100);
    return;
  }else{
// FILE_LOADING_NONE
// FILE_LOADING_RESTART
// FILE_LOADING_REBOOT
// FILE_LOADING_EXEC
// FILE_LOADING_TEXT
// FILE_LOADING_START
// FILE_LOADING_STOPPING
  }

#ifdef _SNMP_AGENT_ENABLE_    
  snmp_loop();
#endif

  if( g_autoupdate )
    qjs.update_modules();

  // For timer, async, etc.
  if( !qjs.loop() ){
    qjs.end();

    if( g_fileloading != FILE_LOADING_NONE ){
      if( g_fileloading == FILE_LOADING_REBOOT ){
        Serial.println("[now rebooting]");
        delay(2000);
        ESP.restart();
        return;
      }else if( g_fileloading == FILE_LOADING_RESTART ){
        Serial.println("[now restarting]");
        delay(2000);
      }else if( g_fileloading == FILE_LOADING_STOPPING ){
        Serial.println("[now stopping]");
        delay(2000);
      }else if( g_fileloading == FILE_LOADING_START ){
        Serial.println("[now starting]");
      }
    }

    if( g_fileloading == FILE_LOADING_STOPPING ){
      g_fileloading = FILE_LOADING_STOP;
    }else{
      start_qjs();
    }
  }else{
    if( g_fileloading == FILE_LOADING_EXEC ){
      qjs.exec(g_download_buffer);
    }
  }

  delay(1);
}

static long start_qjs(void)
{
  qjs.begin();

  long ret = load_all_modules();
  if( ret != 0 ){
    Serial.println("[can't load module]");
  }

  char *js_code = load_jscode();
  if( js_code != NULL ){
    Serial.println("[executing]");
    qjs.exec(js_code);
  }else{
    Serial.println("[can't load main]");
    qjs.exec(jscode_default);
  }

  return (js_code != NULL) ? ret : -1;
}

long save_jscode(const char *p_code)
{
  File fp = SPIFFS.open(MAIN_FNAME, FILE_WRITE);
  if( !fp )
    return -1;
  fp.write((uint8_t*)p_code, strlen(p_code));
  fp.close();

  return 0;
}

long read_jscode(char *p_buffer, uint32_t maxlen)
{
  if( !SPIFFS.exists(MAIN_FNAME) ){
    p_buffer[0] = '\0';
    return 0;
  }
  File fp = SPIFFS.open(MAIN_FNAME, FILE_READ);
  if( !fp )
    return -1;
  size_t size = fp.size();
  if( size + 1 > maxlen ){
    fp.close();
    return -1;
  }
  fp.readBytes(p_buffer, size);
  fp.close();
  p_buffer[size] = '\0';

  return 0;
  }
    
static char* load_jscode(void)
{
  if( !SPIFFS.exists(MAIN_FNAME) )
    return NULL;
  File fp = SPIFFS.open(MAIN_FNAME, FILE_READ);
  if( !fp )
    return NULL;
  size_t size = fp.size();
  if( (size + strlen(jscode_epilogue) + 1) > sizeof(g_download_buffer) ){
    fp.close();
    return NULL;
  }
  char* js_code = g_download_buffer;
  fp.readBytes(js_code, size);
  fp.close();
  js_code[size] = '\0';

  strcat(js_code, jscode_epilogue);

  return js_code;
}

long save_module(const char* p_fname, const char *p_code)
{
  char filename[64];
  if( strlen(p_fname) > sizeof(filename) - strlen(MODULE_DIR) - 1)
    return -1;
  strcpy(filename, MODULE_DIR);
  strcat(filename, p_fname);

  File fp = SPIFFS.open(filename, FILE_WRITE);
  if( !fp )
    return -1;
  fp.write((uint8_t*)p_code, strlen(p_code));
  fp.close();

  return 0;
}

long read_module(const char* p_fname, char *p_buffer, uint32_t maxlen)
{
  char filename[64];
  if( strlen(p_fname) > sizeof(filename) - strlen(MODULE_DIR) - 1)
    return -1;
  strcpy(filename, MODULE_DIR);
  strcat(filename, p_fname);

  File fp = SPIFFS.open(filename, FILE_READ);
  if( !fp )
    return -1;
  uint32_t size = fp.size();
  if( maxlen < size + 1 ){
    fp.close();
    return -1;
  }
  fp.read((uint8_t*)p_buffer, size);
  p_buffer[size] = '\0';
  fp.close();

  return 0;
}

long delete_module(const char *p_fname)
{
  char filename[64];
  if( strlen(p_fname) > sizeof(filename) - strlen(MODULE_DIR) - 1)
    return -1;
  strcpy(filename, MODULE_DIR);
  strcat(filename, p_fname);

  bool ret = SPIFFS.remove(filename);
  return ret ? 0 : -1;
}

static long load_all_modules(void)
{
  File dir = SPIFFS.open("/");
  if( !dir )
    return -1;

  File file = dir.openNextFile();
  while(file){
    const char *fname = file.path();
    if( strncmp(fname, MODULE_DIR, strlen(MODULE_DIR)) == 0 ){
      const char *module_name = file.name();
      size_t size = file.size();

      char *js_modules_code = (char*)malloc(size + 1);
      if( js_modules_code == NULL ){
        Serial.println("malloc failed");
        return -1;
      }
      js_modules_code[0] = '\0';

      file.readBytes(&js_modules_code[0], size);
      js_modules_code[size] = '\0';

      long ret = qjs.load_module(&js_modules_code[0], strlen(js_modules_code), module_name);
      if( ret != 0 ){
        Serial.printf("load module(%s) failed\n", module_name);
        free(js_modules_code);
        file.close();
        dir.close();
        return -1;
      }
      free(js_modules_code);
      js_modules_code = NULL;
      Serial.printf("load module(%s) loaded\n", module_name);
    }
    file.close();
    file = dir.openNextFile();
  }
  dir.close();

  return 0;
}

static long m5_connect(void)
{
  long ret = wifi_try_connect(false);
  if( ret != 0 )
    return 0;

  configTzTime("JST-9", "ntp.nict.jp", "ntp.jst.mfeed.ad.jp");
  time_t t = time(nullptr) + 1; // Advance one second.
  while (t > time(nullptr));  /// Synchronization in seconds

  return 0;
}
