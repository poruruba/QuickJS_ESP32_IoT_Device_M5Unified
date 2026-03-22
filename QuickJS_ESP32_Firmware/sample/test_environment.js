import * as wire from "Wire";
import * as env from "Env";
import * as pbhub from "UnitPbhub";
import * as gas from "UnitGas";
import * as utils from "Utils";

const PIN_SDA = 32;
const PIN_SCL = 33;

const beebotte_channel = "【ChannelName】";
const beebotte_token = "【BeebotteChannelToken】";

function setup(){
  wire.begin(PIN_SDA, PIN_SCL);
  pbhub.begin();
  gas.begin();
  env.sht40_begin();
  env.bmp280_begin();

  update_value();
}

setInterval(() =>{
  update_value();
}, 60000);

function update_value(){
  var gas_value = gas.IAQmeasure();
//  console.log(JSON.stringify(gas_value));
  console.log("TVOC=" + gas_value.TVOC);
  console.log("eCO2=" + gas_value.eCO2);

  var env_value = env.sht40_get();
//  console.log(JSON.stringify(env_value));
  console.log("temperature=" + env_value.temperature);
  console.log("humidity=" + env_value.humidity);
  var pressure = env.bmp280_readPressure();
  console.log("pressure=" + pressure / 100);

  var light = pbhub.analogRead(0);
  console.log("light=" + light);
  var pir = pbhub.digitalRead(1, 0);
  console.log("pir=" + pir);

  console.log("");
  
  var headers = {
    "X-Auth-Token": beebotte_token
  };
  var values = {
    records: [
      {
        resource: "TVOC",
        data: gas_value.TVOC
      },
      {
        resource: "eCO2",
        data: gas_value.eCO2
      },
      {
        resource: "temperature",
        data: env_value.temperature,
      },
      {
        resource: "humidity",
        data: env_value.humidity,
      },
      {
        resource: "pressure",
        data: pressure / 100,
      },
      {
        resource: "light",
        data: light,
      },
      {
        resource: "pir",
        data: pir != 0
      }
    ]
  };
  var url = "http://api.beebotte.com/v1/data/write/" + beebotte_channel;
  utils.httpPostJson(url, values, headers);
  console.log("beebotte uploaded");
};
