import * as gpio from "Gpio";
import * as input from "Input";

const PIN_LED = 10;
var led = false;

function setup(){
	gpio.pinMode(PIN_LED, gpio.OUTPUT);
	gpio.digitalWrite(PIN_LED, led ? gpio.LOW : gpio.HIGH);
	console.log('setup finished');
}

function loop(){
	esp32.update();

	if( input.wasPressed(input.BUTTON_A) ){
		console.log('Pressed');
		led = !led;
		gpio.digitalWrite(PIN_LED, led ? gpio.LOW : gpio.HIGH);
	}
}
