import * as lcd from "Lcd";
import * as wire from "Wire";
import * as color from "UnitColor";
import * as utils from "Utils";
import * as pixels from "Pixels";
import * as input from "Input";

const PIN_SDA = 32;
const PIN_SCL = 33;
const PIN_PIXELS = 26;
const contrast = 2;
var scaned_color = 0x000000;

function toHex6(v) {
  return v.toString(16).padStart(6, "0").toUpperCase();
}

function adjustContrast(value, contrast) {
  return Math.min(255, Math.max(0,
    (value - 128) * contrast + 128
  ));
}

function setup(){
  wire.begin(PIN_SDA, PIN_SCL);
  color.begin();
  pixels.begin(PIN_PIXELS, 3);
  lcd.clear();
  lcd.setTextColor(0xffffff);
}

function loop(){
  esp32.update();
  if( input.wasPressed(input.BUTTON_A) ){
    console.log("button pressed");
    pixels.setPixelColor(1, scaned_color);
  }
}

setInterval(() =>{
  update_value();
}, 1000);

function update_value(){
  var rgb = color.getRgb();
//  console.log(JSON.stringify(rgb));
  var red = adjustContrast(rgb.red, contrast);
  var green = adjustContrast(rgb.green, contrast);
  var blue = adjustContrast(rgb.blue, contrast);
  scaned_color = utils.toRgb888(red, green, blue);
  lcd.fillScreen(scaned_color);
  lcd.drawAlignedText([{
    text: "#" + toHex6(scaned_color),
    align: lcd.middle_center,
    base_x: lcd.width() / 2,
    base_y: lcd.height() / 2,
    scale: 2
  }]);
  pixels.setPixelColor(0, scaned_color);
}