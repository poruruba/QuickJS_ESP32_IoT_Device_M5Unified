import * as sonicio from "UnitSonicIo";
import * as ledc from "Ledc";

const PIN_TRIG = 26;
const PIN_ECHO = 36;

const channel = 0;

function map(x, in_min, in_max, out_min, out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

function setup() {
    ledc.setup(channel, 2000, 8);
    ledc.attachPin(33, channel)
    sonicio.begin(PIN_TRIG, PIN_ECHO);
}

var touched = false;

function loop() {
    var distance = sonicio.getDistance();
//    console.log(distance);

    if (distance > 0 && distance < 500) {
        if (distance < 30) {
            if (!touched) {
                ledc.writeTone(channel, 2000);
                touched = true;
            }
        } else {
            touched = false;
            ledc.writeTone(channel, 2000);
            delay(100);
            ledc.writeTone(channel, 0);
            var waitTime = map(distance, 30, 500, 20, 1000);
            delay(waitTime);
        }
    } else {
        touched = false;
        ledc.writeTone(channel, 0);
        delay(100);
    }
}