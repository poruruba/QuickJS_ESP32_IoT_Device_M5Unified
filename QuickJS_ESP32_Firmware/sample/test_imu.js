import * as lcd from "Lcd";
import * as imu from "Imu";

function setup(){
	lcd.setFont(16);
	lcd.fillScreen(0x000000);
}

function loop(){
	var accl = imu.getAccel();
	var gyro = imu.getGyro();

	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.println("accl: x=" + accl.x.toFixed(2) + " y=" + accl.y.toFixed(2) + " z=" + accl.z.toFixed(2));
	lcd.println("gyro: x=" + gyro.x.toFixed(2) + " y=" + gyro.y.toFixed(2) + " z=" + gyro.z.toFixed(2));
}
