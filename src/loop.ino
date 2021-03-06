
void loop() {

   switch (mode) {
      case BEGIN:

      buttonStatus = gpio.readModeButton();
      //Serial.println(buttonStatus);

      if (buttonStatus == 1) {
         Serial.println("Mode1");
         mode = MODE1;

      } else if (buttonStatus == 2) {
         Serial.println("Mode2");
         mode = MODE2;

      } else if (buttonStatus == 3) {
         Serial.println("Mode3");
         mode = MODE3;

      } else {
         delay(100);
      }

      // Call the functions which should execute on every loop cycle,  this is available only on MODE_0
      loopEvents();

      break;

      // ------------------------------------------------------------------------------------------ Mode 1 ----------
      case MODE1:
      // What should happen on mode 1
      motors.write(200, 200);
      delay(4000);
      motors.stop();
      mode = BEGIN;

      break;

      // ------------------------------------------------------------------------------------------ Mode 2 ----------
      case MODE2:
      // What should happen on mode 2
      mode_sensorTest();
      delay(1000);
      mode = BEGIN;

      break;

      // ------------------------------------------------------------------------------------------ Mode 3 ----------
      case MODE3:
      // What should happen on mode 3
      mode_ramdomMotion();

      break;
   }
}

void loopEvents() {

   // Handle OTA requests, if it is enabled
   #ifdef ENABLE_OTA_UPLOAD
   //ArduinoOTA.handle();
   #endif

   // WiFi Monitor : Only support on MODE_0
   #ifdef ENABLE_WIFI_MONITOR
   wifiMonitor.handleClient();
   #endif

}

void mode_sensorTest(){
   distance.test();
   compass.test();
   colorSensor.test();
   //ir.sendTestSignal();
}

void mode_ramdomMotion(){
   // Random move mode

   motors.write(200, 200);
   while (1) {
      int d = distance.getDistanceInt();
      Serial.println(d);

      if (d < 12) {
         int timeout = random(-1500, 1500);

         if (timeout < 0) {
            Serial.println("CCW");
            motors.write(-80, 80);
            delay((-1 * timeout) + 500);
         } else {
            Serial.println("CW");
            motors.write(80, -80);
            delay(timeout + 500);
         }
         motors.write(200, 200);
         delay(10);
      }
   }
}
