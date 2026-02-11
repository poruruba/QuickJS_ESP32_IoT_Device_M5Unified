#include <Arduino.h>
#include "main_config.h"

#include "quickjs.h"
#include "quickjs_esp32.h"
#include "module_type.h"

#ifdef _BLEPERIPHERAL_ENABLE_

#include <NimBLEDevice.h>
#include <NimBLEBeacon.h>
#include "module_utils.h"
#include "module_bleperipheral.h"

static NimBLEAdvertising *g_pAdvertising = NULL;

#define DEFAULT_ADVERTISE_INTERVAL 0x0080
static uint16_t g_advertise_interval = DEFAULT_ADVERTISE_INTERVAL;

static JSValue bleperipheral_startAdvertise(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  g_pAdvertising->stop();
  g_pAdvertising->reset();
  g_pAdvertising->setConnectableMode(BLE_GAP_CONN_MODE_NON);
  g_pAdvertising->enableScanResponse(false);

  uint32_t duration = 0;
  JS_ToUint32(ctx, &duration, argv[0]);

  if( argc >= 2 ){
    JSValue value;

    value = JS_GetPropertyStr(ctx, argv[1], "serviceUuid");
    if( value != JS_UNDEFINED ){
      const char *serviceUuid = JS_ToCString(ctx, value);
      BLEUUID uuid = BLEUUID(serviceUuid);
      JS_FreeCString(ctx, serviceUuid);
      JS_FreeValue(ctx, value);
      if( uuid.bitSize() == 0 )
        return JS_EXCEPTION;

      g_pAdvertising->addServiceUUID(uuid);
    }

    value = JS_GetPropertyStr(ctx, argv[1], "name");
    if( value != JS_UNDEFINED ){
      const char *name = JS_ToCString(ctx, value);
      g_pAdvertising->setName(name);
      JS_FreeCString(ctx, name);
      JS_FreeValue(ctx, value);
    }

    value = JS_GetPropertyStr(ctx, argv[1], "manufacturerData");
    if( value != JS_UNDEFINED ){
      uint32_t unit_num;
      uint8_t *p_buffer;
      JSValue vbuffer = from_Uint8Array(ctx, value, &p_buffer, &unit_num);
      if( JS_IsNull(vbuffer) ){
        JS_FreeValue(ctx, value);
        return JS_EXCEPTION;
      }
      JS_FreeValue(ctx, value);

      g_pAdvertising->setManufacturerData(p_buffer, unit_num);
      JS_FreeValue(ctx, vbuffer);
    }

    value = JS_GetPropertyStr(ctx, argv[1], "addTxPower");
    if( value != JS_UNDEFINED ){
      int addTxPower = JS_ToBool(ctx, value);
      JS_FreeValue(ctx, value);
      if( addTxPower )
        g_pAdvertising->addTxPower();
    }

    value = JS_GetPropertyStr(ctx, argv[1], "uri");
    if( value != JS_UNDEFINED ){
      const char *uri = JS_ToCString(ctx, value);
      g_pAdvertising->setURI(uri);
      JS_FreeCString(ctx, uri);
      JS_FreeValue(ctx, value);
    }
  }

  bool ret = g_pAdvertising->start(duration);

  return JS_NewBool(ctx, ret);
}

static JSValue bleperipheral_startAdvertise2(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  g_pAdvertising->stop();
  g_pAdvertising->reset();
  g_pAdvertising->setConnectableMode(BLE_GAP_CONN_MODE_NON);
  g_pAdvertising->enableScanResponse(false);

  uint32_t duration = 0;
  JS_ToUint32(ctx, &duration, argv[0]);

  if( argc >= 2 ){
    uint32_t unit_num;
    uint8_t *p_buffer;
    JSValue vbuffer = from_Uint8Array(ctx, argv[1], &p_buffer, &unit_num);
    if( JS_IsNull(vbuffer) )
      return JS_EXCEPTION;

    NimBLEAdvertisementData advertisementData = NimBLEAdvertisementData();
    advertisementData.addData(p_buffer, unit_num);
    JS_FreeValue(ctx, vbuffer);

    g_pAdvertising->setAdvertisementData(advertisementData);
  }

  g_pAdvertising->start(duration);

  return JS_UNDEFINED;
}

static JSValue bleperipheral_startIbeacon(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  g_pAdvertising->stop();
  g_pAdvertising->reset();
  g_pAdvertising->setConnectableMode(BLE_GAP_CONN_MODE_NON);
  g_pAdvertising->enableScanResponse(false);

  uint32_t duration = 0;
  JS_ToUint32(ctx, &duration, argv[0]);

  const char *uuid = JS_ToCString(ctx, argv[1]);
  if( uuid == NULL )
    return JS_EXCEPTION;
  BLEUUID p_uuid = BLEUUID(uuid);

  int32_t major, minor, power;
  JS_ToInt32(ctx, &major, argv[2]);
  JS_ToInt32(ctx, &minor, argv[3]);
  JS_ToInt32(ctx, &power, argv[4]);

  NimBLEBeacon ibeacon = NimBLEBeacon();
  ibeacon.setProximityUUID(p_uuid);
  ibeacon.setMajor(major);
  ibeacon.setMinor(minor);
  ibeacon.setSignalPower(power);
  ibeacon.setManufacturerId(0x4c00);

  JS_FreeCString(ctx, uuid);

  NimBLEAdvertisementData advertisementData = NimBLEAdvertisementData();
  advertisementData.setFlags(0x06);
  advertisementData.setManufacturerData(ibeacon.getData());
  g_pAdvertising->setAdvertisementData(advertisementData);

  g_pAdvertising->start(duration);

  return JS_UNDEFINED;
}

static JSValue bleperipheral_stopAdvertise(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  g_pAdvertising->stop();

  return JS_UNDEFINED;
}

static JSValue bleperipheral_getMacAddress(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  return JS_NewString(ctx, BLEDevice::getAddress().toString().c_str());
}

static JSValue bleperipheral_setPower(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  int32_t power;
  JS_ToInt32(ctx, &power, argv[0]);

  esp_power_level_t level;

  switch(power){
    case -12: level = ESP_PWR_LVL_N12; break;
    case -9: level = ESP_PWR_LVL_N9; break;
    case -6: level = ESP_PWR_LVL_N6; break;
    case -3: level = ESP_PWR_LVL_N3; break;
    case 0: level = ESP_PWR_LVL_N0; break;
    case 3: level = ESP_PWR_LVL_P3; break;
    case 6: level = ESP_PWR_LVL_P6; break;
    case 9: level = ESP_PWR_LVL_P9; break; // default
    default: return JS_EXCEPTION;
  }

  BLEDevice::setPower(level);

  return JS_UNDEFINED;
}

static JSValue bleperipheral_getPower(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  int power = BLEDevice::getPower();

  return JS_NewInt32(ctx, power);
}

static JSValue bleperipheral_setAdvertiseInterval(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  int32_t interval;
  JS_ToInt32(ctx, &interval, argv[0]);

  g_advertise_interval = interval;
  g_pAdvertising->setMinInterval(g_advertise_interval);
  g_pAdvertising->setMaxInterval(g_advertise_interval);

  return JS_UNDEFINED;
}

static JSValue bleperipheral_getAdvertiseInterval(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  int power = BLEDevice::getPower();

  return JS_NewInt32(ctx, g_advertise_interval);
}

static const JSCFunctionListEntry bleperipheral_funcs[] = {
    JSCFunctionListEntry{
        "startAdvertise", 0, JS_DEF_CFUNC, 0, {
          func : {2, JS_CFUNC_generic, bleperipheral_startAdvertise}
        }},
    JSCFunctionListEntry{
        "startAdvertise2", 0, JS_DEF_CFUNC, 0, {
          func : {2, JS_CFUNC_generic, bleperipheral_startAdvertise2}
        }},
    JSCFunctionListEntry{
        "startIbeacon", 0, JS_DEF_CFUNC, 0, {
          func : {5, JS_CFUNC_generic, bleperipheral_startIbeacon}
        }},
    JSCFunctionListEntry{
        "stopAdvertise", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, bleperipheral_stopAdvertise}
        }},
    JSCFunctionListEntry{
        "getMacAddress", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, bleperipheral_getMacAddress}
        }},        
    JSCFunctionListEntry{
        "setPower", 0, JS_DEF_CFUNC, 0, {
          func : {1, JS_CFUNC_generic, bleperipheral_setPower}
        }},        
    JSCFunctionListEntry{
        "getPower", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, bleperipheral_getPower}
        }},
    JSCFunctionListEntry{
        "setAdvertiseInterval", 0, JS_DEF_CFUNC, 0, {
          func : {1, JS_CFUNC_generic, bleperipheral_setAdvertiseInterval}
        }},        
    JSCFunctionListEntry{
        "getAdvertiseInterval", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, bleperipheral_getAdvertiseInterval}
        }},
};

JSModuleDef *addModule_bleperipheral(JSContext *ctx, JSValue global)
{
  JSModuleDef *mod;
  mod = JS_NewCModule(ctx, "BlePeripheral", [](JSContext *ctx, JSModuleDef *m){
          return JS_SetModuleExportList(
                      ctx, m, bleperipheral_funcs,
                      sizeof(bleperipheral_funcs) / sizeof(JSCFunctionListEntry));
          });
  if (mod){
    JS_AddModuleExportList(
        ctx, mod, bleperipheral_funcs,
        sizeof(bleperipheral_funcs) / sizeof(JSCFunctionListEntry));
  }

  return mod;
}

void endModule_bleperipheral(void){
  g_pAdvertising->stop();

  // default
  g_pAdvertising->reset();
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);
  g_pAdvertising->setConnectableMode(BLE_GAP_CONN_MODE_NON);
  g_pAdvertising->enableScanResponse(false);
  g_advertise_interval = DEFAULT_ADVERTISE_INTERVAL;
  g_pAdvertising->setMinInterval(g_advertise_interval);
  g_pAdvertising->setMaxInterval(g_advertise_interval);
}

long initializeModule_bleperipheral(void)
{
  NimBLEDevice::init("");
  g_pAdvertising = NimBLEDevice::getAdvertising();

  endModule_bleperipheral();

  Serial.printf("[bleperipheral ready] MAC=%s\n", BLEDevice::getAddress().toString().c_str());

  return 0;
}

JsModuleEntry bleperipheral_module = {
  initializeModule_bleperipheral,
  addModule_bleperipheral,
  NULL,
  endModule_bleperipheral
};

#endif