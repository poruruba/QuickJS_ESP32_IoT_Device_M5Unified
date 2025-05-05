'use strict';

class Imu{
  constructor(arduino){
   this.arduino = arduino;
   this.module_type = "/imu-";
  }

  async getAccel(){
    return this.arduino.webapi_request(this.module_type + "getAccel", {});
  }

  async getGyro(){
    return this.arduino.webapi_request(this.module_type + "getGyro", {});
  }

  async getTemp(){
    return this.arduino.webapi_request(this.module_type + "getTemp", {});
  }
}

//module.exports = Imu;
