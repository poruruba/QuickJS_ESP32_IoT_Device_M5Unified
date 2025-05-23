#include <Arduino.h>
#include "main_config.h"

#ifdef _IMU_ENABLE_

#include "endpoint_types.h"
#include "endpoint_imu.h"

long endp_imu_getAccel(JsonObject& request, JsonObject& response, int magic)
{
  float ax, ay, az;
  M5.Imu.getAccel(&ax, &ay, &az);

  response["result"]["x"] = ax;
  response["result"]["y"] = ay;
  response["result"]["z"] = az;

  return 0;
}

long endp_imu_getGyro(JsonObject& request, JsonObject& response, int magic)
{
  float gx, gy, gz;
  M5.Imu.getGyro(&gx, &gy, &gz);

  response["result"]["x"] = gx;
  response["result"]["y"] = gy;
  response["result"]["z"] = gz;

  return 0;
}

long endp_imu_getTemp(JsonObject& request, JsonObject& response, int magic)
{
  float t;
  M5.Imu.getTemp(&t);

  response["result"] = t;

  return 0;
}

EndpointEntry imu_table[] = {
  EndpointEntry{ endp_imu_getAccel, "/imu-getAccel", -1 },
  EndpointEntry{ endp_imu_getGyro, "/imu-getGyro", -1 },
  EndpointEntry{ endp_imu_getTemp, "/imu-getTemp", -1 },
};

const int num_of_imu_entry = sizeof(imu_table) / sizeof(EndpointEntry);

#endif
