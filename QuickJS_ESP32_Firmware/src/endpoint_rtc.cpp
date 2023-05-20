#include <Arduino.h>
#include "main_config.h"

#ifdef _RTC_ENABLE_

#include "endpoint_types.h"
#include "endpoint_rtc.h"

long endp_rtc_setTime(JsonObject request, JsonObject response, int magic)
{
  uint32_t hours = request["Hours"];
  uint32_t minutes = request["Minutes"];
  uint32_t seconds = request["Seconds"];

  m5::rtc_time_t def;
  def.hours = hours;
  def.minutes = minutes;
  def.seconds = seconds;
  M5.Rtc.setTime(&def);

  return 0;
}

long endp_rtc_setDate(JsonObject request, JsonObject response, int magic)
{
  uint32_t year = request["Year"];
  uint32_t month = request["Month"];
  uint32_t date = request["Date"];
  uint32_t weekday = request["WeekDay"];

  m5::rtc_date_t def;
  def.year = year;
  def.month = month;
  def.date = date;
  def.weekDay = weekday;
  M5.Rtc.setDate(&def);

  return 0;
}

long endp_rtc_getTime(JsonObject request, JsonObject response, int magic)
{
  m5::rtc_time_t def;
  M5.Rtc.getTime(&def);

  response["result"]["Hours"] = def.hours;
  response["result"]["Minutes"] = def.minutes;
  response["result"]["Seconds"] = def.seconds;

  return 0;
}

long endp_rtc_getDate(JsonObject request, JsonObject response, int magic)
{
  m5::rtc_date_t def;
  M5.Rtc.getDate(&def);

  response["result"]["Year"] = def.year;
  response["result"]["Month"] = def.month;
  response["result"]["Date"] = def.date;
  response["result"]["WeekDay"] = def.weekDay;

  return 0;
}

EndpointEntry rtc_table[] = {
  EndpointEntry{ endp_rtc_setTime, "/rtc-setTime", -1 },
  EndpointEntry{ endp_rtc_setDate, "/rtc-setDate", -1 },
  EndpointEntry{ endp_rtc_getTime, "/rtc-getTime", -1 },
  EndpointEntry{ endp_rtc_getDate, "/rtc-getDate", -1 },
};

const int num_of_rtc_entry = sizeof(rtc_table) / sizeof(EndpointEntry);

#endif
