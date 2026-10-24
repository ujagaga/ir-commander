/*
 *  Author: Rada Berar
 *  email: ujagaga@gmail.com
 *
 *  Optional DS18B20 room temperature sensor. Detected once at boot; without it the reading stays hidden.
 */

#include <OneWire.h>
#include <DallasTemperature.h>
#include "temp_sensor.h"
#include "config.h"

static OneWire oneWire(TEMP_PIN);
static DallasTemperature sensors(&oneWire);
static bool present = false;
static String lastReading = "";
static uint32_t requestTime = 0;

void TEMP_init(void){
  sensors.begin();
  present = sensors.getDeviceCount() > 0;
  Serial.printf("DS18B20 %s\n", present ? "found" : "not found");
  if(!present) return;
  sensors.setWaitForConversion(false);  // Conversion takes 750 ms; read it on a later pass instead of blocking.
  sensors.requestTemperatures();
  requestTime = millis();
}

void TEMP_process(void){
  if(!present || (millis() - requestTime) < TEMP_READ_INTERVAL_MS) return;
  float t = sensors.getTempCByIndex(0);
  lastReading = (t == DEVICE_DISCONNECTED_C) ? "" : String(t, 1);
  sensors.requestTemperatures();
  requestTime = millis();
}

bool TEMP_present(void){ return present; }

/* Last reading in °C with one decimal, or empty if it failed or is not ready yet. */
String TEMP_get(void){ return lastReading; }
