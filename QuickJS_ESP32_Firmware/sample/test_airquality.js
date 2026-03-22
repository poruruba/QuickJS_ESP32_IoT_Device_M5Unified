import * as airquality from "UnitAirquality";

function setup(){
	airquality.begin(33);
	console.log("now warming up");
}

function loop(){
	airquality.update();
}

setInterval(() =>{
	if( !airquality.isReady() )
		return;

	switch (airquality.slope()){
        case 0: console.log("-> HIGH POLLUTION (forced)"); break;
        case 1: console.log("-> HIGH POLLUTION");          break;
        case 2: console.log("-> LOW POLLUTION");           break;
        case 3: console.log("-> FRESH AIR");               break;
        default: break; 
    }
}, 1000);