/*
 *  Author: Rada Berar
 *  email: ujagaga@gmail.com
 *
 *  ESP8266 (D1 mini) IR remote: sends TV power code on HTTP GET /power.
 */

#include "wifi_connection.h"
#include "http_server.h"
#include "ir_control.h"
#include "ota.h"

void setup(void) {
  /* Need to wait for background processes to complete. Otherwise trouble with gpio.*/
  delay(100);
  Serial.begin(115200);

  IR_init();
  OTA_init();
  WIFIC_init();
  HTTP_SERVER_init();
}

void loop(void) {
  HTTP_SERVER_process();
  WIFIC_process();
  OTA_process();
}
