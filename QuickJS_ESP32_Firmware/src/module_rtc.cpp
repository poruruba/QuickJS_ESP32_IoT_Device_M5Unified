#include <Arduino.h>
#include "main_config.h"

#ifdef _RTC_ENABLE_

#include "module_type.h"
#include "quickjs.h"

static JSValue esp32_rtc_SetTime(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint32_t hours, minutes, seconds;
  JS_ToUint32(ctx, &hours, argv[0]);
  JS_ToUint32(ctx, &minutes, argv[1]);
  JS_ToUint32(ctx, &seconds, argv[2]);
  m5::rtc_time_t def;
  def.hours = hours;
  def.minutes = minutes;
  def.seconds = seconds;
  M5.Rtc.setTime(&def);
  return JS_UNDEFINED;
}

static JSValue esp32_rtc_SetDate(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint32_t year, month, date, weekday;
  JS_ToUint32(ctx, &year, argv[0]);
  JS_ToUint32(ctx, &month, argv[1]);
  JS_ToUint32(ctx, &date, argv[2]);
  JS_ToUint32(ctx, &weekday, argv[3]);
  m5::rtc_date_t def;
  def.year = year;
  def.month = month;
  def.date = date;
  def.weekDay = weekday;
  M5.Rtc.setDate(&def);
  return JS_UNDEFINED;
}

static JSValue esp32_rtc_GetTime(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  m5::rtc_time_t def;
  M5.Rtc.getTime(&def);
  JSValue obj = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, obj, "Hours", JS_NewUint32(ctx, def.hours));
  JS_SetPropertyStr(ctx, obj, "Minutes", JS_NewUint32(ctx, def.minutes));
  JS_SetPropertyStr(ctx, obj, "Seconds", JS_NewUint32(ctx, def.seconds));
  return obj;
}

static JSValue esp32_rtc_GetDate(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  m5::rtc_date_t def;
  M5.Rtc.getDate(&def);
  JSValue obj = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, obj, "Year", JS_NewUint32(ctx, def.year));
  JS_SetPropertyStr(ctx, obj, "Month", JS_NewUint32(ctx, def.month));
  JS_SetPropertyStr(ctx, obj, "Date", JS_NewUint32(ctx, def.date));
  JS_SetPropertyStr(ctx, obj, "WeekDay", JS_NewUint32(ctx, def.weekDay));
  return obj;
}

static const JSCFunctionListEntry rtc_funcs[] = {
    JSCFunctionListEntry{
        "setTime", 0, JS_DEF_CFUNC, 0, {
          func : {3, JS_CFUNC_generic, esp32_rtc_SetTime}
        }},
    JSCFunctionListEntry{
        "setDate", 0, JS_DEF_CFUNC, 0, {
          func : {4, JS_CFUNC_generic, esp32_rtc_SetDate}
        }},
    JSCFunctionListEntry{
        "getTime", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, esp32_rtc_GetTime}
        }},
    JSCFunctionListEntry{
        "getDate", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, esp32_rtc_GetDate}
        }},
};

JSModuleDef *addModule_rtc(JSContext *ctx, JSValue global)
{
  JSModuleDef *mod;

  mod = JS_NewCModule(ctx, "Rtc", [](JSContext *ctx, JSModuleDef *m) {
          return JS_SetModuleExportList(
              ctx, m, rtc_funcs,
              sizeof(rtc_funcs) / sizeof(JSCFunctionListEntry));
        });
  if (mod) {
    JS_AddModuleExportList(
        ctx, mod, rtc_funcs,
        sizeof(rtc_funcs) / sizeof(JSCFunctionListEntry));
  }

  return mod;
}

long initialize_rtc(void){
  if (M5.Rtc.isEnabled())
    M5.Rtc.setDateTime( gmtime( &t ) );

/*      
  struct tm timeInfo;
  getLocalTime(&timeInfo);
//  Serial.printf("%d/%d/%d %d:%d:%d\n", timeInfo.tm_year + 1900, timeInfo.tm_mon + 1, timeInfo.tm_mday, timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec);
  m5::rtc_date_t def;
  def.year = timeInfo.tm_year + 1900;
  def.month = timeInfo.tm_mon + 1;
  def.date = timeInfo.tm_mday;
  def.weekDay = timeInfo.tm_wday;
  M5.Rtc.setDate(&def);

  m5::rtc_time_t def2;
  def2.hours = timeInfo.tm_hour;
  def2.minutes = timeInfo.tm_min;
  def2.seconds = timeInfo.tm_sec;
  M5.Rtc.setTime(&def2);
*/
  
  return 0;
}

JsModuleEntry rtc_module = {
  initialize_rtc,
  addModule_rtc,
  NULL,
  NULL
};
#endif
