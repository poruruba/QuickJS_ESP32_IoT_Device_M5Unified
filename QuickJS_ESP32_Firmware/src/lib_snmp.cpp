#include <Arduino.h>
#include "main_config.h"

#ifdef _SNMP_AGENT_ENABLE_

#include <WiFi.h>
#include <WiFiUdp.h>
#include <SPIFFS.h>
#include <SNMP_Agent.h>
#include "wifi_utils.h"
#include "storage_info.h"
#include "lib_snmp.h"

#include "quickjs_esp32.h"

#define IDEAL_LOOP_PERIOD 8000

static WiFiUDP udp;
static SNMPAgent snmp("public");

extern ESP32QuickJS qjs;

#define hrStorageOther        ".1.3.6.1.2.1.25.2.1.1"
#define hrStorageRam          ".1.3.6.1.2.1.25.2.1.2"
#define hrStorageVirtualMemory ".1.3.6.1.2.1.25.2.1.3"
#define hrStorageFixedDisk    ".1.3.6.1.2.1.25.2.1.4"
#define hrStorageFlashMemory  ".1.3.6.1.2.1.25.2.1.9"

static uint32_t lastLoopMicros = 0;
static int cpuLoadRaw = 0;

// 多重平均
static float avgFast = 0;
static float avgMid  = 0;
static float avgSlow = 0;

static int private_number[NUM_OF_PRIV_NUMBER] = { NUMBER_DEFAULT, NUMBER_DEFAULT, NUMBER_DEFAULT };
static std::string private_string[NUM_OF_PRIV_STRING] = { "", "", "" };
static uint32_t private_timestamp[NUM_OF_PRIV_TIMESTAMP] = { 0, 0, 0 };

long snmp_set_number(unsigned char index, int value)
{
  if( index >= NUM_OF_PRIV_NUMBER )
    return -1;

  private_number[index] = value;

  return 0;
}

long snmp_set_string(unsigned char index, const char *p_str)
{
  if( index >= NUM_OF_PRIV_STRING )
    return -1;

  if( p_str != NULL )
    private_string[index] = std::string(p_str);
  else
    private_string[index] = "";

  return 0;
}

long snmp_set_timestamp(unsigned char index, unsigned long value)
{
  if( index >= NUM_OF_PRIV_TIMESTAMP )
    return -1;

  private_timestamp[index] = value;

  return 0;
}

long snmp_clear_value(void)
{
  for( int i = 0 ; i < NUM_OF_PRIV_NUMBER ; i++ )
    private_number[i] = NUMBER_DEFAULT;
  for( int i = 0 ; i < NUM_OF_PRIV_STRING ; i++ )
    private_string[i] = "";
  for( int i = 0 ; i < NUM_OF_PRIV_TIMESTAMP ; i++ )
    private_timestamp[i] = 0;
  
  return 0;
}

long snmp_initialize(void)
{
  if( !wifi_is_connected() )
    return -1;

  snmp.setUDP(&udp);
  snmp.begin();

  lastLoopMicros = micros();

  // system.sysDescr
  snmp.addReadOnlyStaticStringHandler(".1.3.6.1.2.1.1.1.0",
    std::string("ESP32 SNMP Agent")
  );
  // system.sysObjectID
  snmp.addOIDHandler(".1.3.6.1.2.1.1.2.0",
    ".1.3.6.1.4.1.8072.3.2.255"
  );
  // system.sysUpTime
  snmp.addDynamicReadOnlyTimestampHandler(".1.3.6.1.2.1.1.3.0",
    []() -> uint32_t {
        return millis() / 10;
    }
  );
  // system.sysName
  snmp.addReadOnlyStaticStringHandler(".1.3.6.1.2.1.1.5.0",
    std::string(MDNS_NAME)
  );
  // system.sysServices
  snmp.addReadOnlyIntegerHandler(".1.3.6.1.2.1.1.7.0",
    72
  );
  // interface.ifNumber
  snmp.addReadOnlyIntegerHandler(".1.3.6.1.2.1.2.1.0",
    1
  );
  // interface.ifIndex
  snmp.addReadOnlyIntegerHandler(".1.3.6.1.2.1.2.2.1.1.1",
    1
  );
  // interface.ifDescr
  snmp.addReadOnlyStaticStringHandler(".1.3.6.1.2.1.2.2.1.2.1",
    std::string("wifi")
  );
  // interface.ifType
  snmp.addReadOnlyIntegerHandler(".1.3.6.1.2.1.2.2.1.3.1",
    71
  );
  // interface.ifMtu
  snmp.addReadOnlyIntegerHandler(".1.3.6.1.2.1.2.2.1.4.1",
    1500
  );
  uint8_t mac[6];
  WiFi.macAddress(mac);
  // interface.ifPhysAddress
  snmp.addReadOnlyStaticStringHandler(".1.3.6.1.2.1.2.2.1.6.1",
    std::string((char*)mac, sizeof(mac))
  );
  // interface.ifAdminStatus
  snmp.addReadOnlyIntegerHandler(".1.3.6.1.2.1.2.2.1.7.1",
    1
  );
  // interface.ifOperStatus
  snmp.addDynamicIntegerHandler(".1.3.6.1.2.1.2.2.1.8.1",
    []() -> int {
        return (WiFi.status() == WL_CONNECTED) ? 1 : 2; 
    }
  );

  // 1:flash, 2:spiffs, 3:ram, 4:heap, 5:psram, 6:stack
  // hrStorageType
  snmp.addOIDHandler(".1.3.6.1.2.1.25.2.3.1.2.1", hrStorageFlashMemory);
  snmp.addOIDHandler(".1.3.6.1.2.1.25.2.3.1.2.2", hrStorageFixedDisk);
  snmp.addOIDHandler(".1.3.6.1.2.1.25.2.3.1.2.3", hrStorageRam);
  snmp.addOIDHandler(".1.3.6.1.2.1.25.2.3.1.2.4", hrStorageRam);
  snmp.addOIDHandler(".1.3.6.1.2.1.25.2.3.1.2.5", hrStorageRam);
  snmp.addOIDHandler(".1.3.6.1.2.1.25.2.3.1.2.6", hrStorageRam);

  // hrStorageDescr
  snmp.addReadOnlyStaticStringHandler (".1.3.6.1.2.1.25.2.3.1.3.1", std::string("flash"));
  snmp.addReadOnlyStaticStringHandler (".1.3.6.1.2.1.25.2.3.1.3.2", std::string("spiffs"));
  snmp.addReadOnlyStaticStringHandler (".1.3.6.1.2.1.25.2.3.1.3.3", std::string("ram"));
  snmp.addReadOnlyStaticStringHandler (".1.3.6.1.2.1.25.2.3.1.3.4", std::string("heap"));
  snmp.addReadOnlyStaticStringHandler (".1.3.6.1.2.1.25.2.3.1.3.5", std::string("psram"));
  snmp.addReadOnlyStaticStringHandler (".1.3.6.1.2.1.25.2.3.1.3.6", std::string("stack"));

  // hrStorageSize
  snmp.addReadOnlyIntegerHandler(".1.3.6.1.2.1.25.2.3.1.5.1", getFlashSize());
  snmp.addReadOnlyIntegerHandler(".1.3.6.1.2.1.25.2.3.1.5.2", SPIFFS.totalBytes());
  snmp.addDynamicIntegerHandler(".1.3.6.1.2.1.25.2.3.1.5.3",
    []() -> int {
      return getRamTotal();
    }
  );
  snmp.addDynamicIntegerHandler(".1.3.6.1.2.1.25.2.3.1.5.4",
    []() -> int {
      return ESP.getHeapSize();
    }
  );
  snmp.addReadOnlyIntegerHandler(".1.3.6.1.2.1.25.2.3.1.5.5", ESP.getPsramSize());
  snmp.addDynamicIntegerHandler(".1.3.6.1.2.1.25.2.3.1.5.6", 
    []() -> int {
      JSMemoryUsage usage;
      qjs.getMemoryUsage(&usage);
      return usage.memory_used_size;
    }
  );

  // hrStorageUsed
  snmp.addReadOnlyIntegerHandler(".1.3.6.1.2.1.25.2.3.1.6.1", getPartitionApplication());
  snmp.addDynamicIntegerHandler(".1.3.6.1.2.1.25.2.3.1.6.2", 
    []() -> int {
      return SPIFFS.usedBytes();
    }
  );
  snmp.addDynamicIntegerHandler(".1.3.6.1.2.1.25.2.3.1.6.3",
    []() -> int {
      return getRamUsed();
    }
  );
  snmp.addDynamicIntegerHandler(".1.3.6.1.2.1.25.2.3.1.6.4",
    []() -> int {
      return ESP.getFreeHeap();
    }
  );
  snmp.addDynamicIntegerHandler(".1.3.6.1.2.1.25.2.3.1.6.5",
    []() -> int {
      return ESP.getFreePsram();
    }
  );
  snmp.addDynamicIntegerHandler(".1.3.6.1.2.1.25.2.3.1.6.6",
    []() -> int {
      JSMemoryUsage usage;
      qjs.getMemoryUsage(&usage);
      return usage.malloc_size;
    }
  );

  // hrProcessorLoad
  snmp.addDynamicIntegerHandler(".1.3.6.1.2.1.25.3.3.1.2.1",
    []() -> int {
      return (int)(avgSlow + 0.5);
    }
  );

  snmp.addDynamicIntegerHandler(PRIVETE_OID_BASE ".1.0",
    []() -> int {
      return private_number[0];
    }
  );
  snmp.addDynamicIntegerHandler(PRIVETE_OID_BASE ".1.1",
    []() -> int {
      return private_number[1];
    }
  );
  snmp.addDynamicIntegerHandler(PRIVETE_OID_BASE ".1.2",
    []() -> int {
      return private_number[2];
    }
  );

  snmp.addDynamicReadOnlyStringHandler(PRIVETE_OID_BASE ".2.0",
    []() -> const std::string {
      return private_string[0];
    }
  );
  snmp.addDynamicReadOnlyStringHandler(PRIVETE_OID_BASE ".2.1",
    []() -> const std::string {
      return private_string[1];
    }
  );
  snmp.addDynamicReadOnlyStringHandler(PRIVETE_OID_BASE ".2.2",
    []() -> const std::string {
      return private_string[2];
    }
  );

  snmp.addDynamicReadOnlyTimestampHandler(PRIVETE_OID_BASE ".3.0",
    []() -> uint32_t {
        return private_timestamp[0] / 10;
    }
  );
  snmp.addDynamicReadOnlyTimestampHandler(PRIVETE_OID_BASE ".3.1",
    []() -> uint32_t {
        return private_timestamp[1] / 10;
    }
  );
  snmp.addDynamicReadOnlyTimestampHandler(PRIVETE_OID_BASE ".3.2",
    []() -> uint32_t {
        return private_timestamp[2] / 10;
    }
  );      

  return 0;
}

static void snmp_perform(void) {
  uint32_t now = micros();
  uint32_t diff = now - lastLoopMicros;
  lastLoopMicros = now;

  const uint32_t ideal = IDEAL_LOOP_PERIOD;
  if (diff <= ideal) {
    cpuLoadRaw = 0;
  } else {
    uint32_t over = diff - ideal;
    cpuLoadRaw = over * 100 / ideal;
    if (cpuLoadRaw > 100) cpuLoadRaw = 100;
  }

  avgFast = avgFast * 0.7 + cpuLoadRaw * 0.3;
  avgMid  = avgMid  * 0.85 + avgFast * 0.15;
  avgSlow = avgSlow * 0.95 + avgMid  * 0.05;
}

long snmp_loop(void)
{
  snmp_perform();
  snmp.loop();

  return 0;
}

#endif
