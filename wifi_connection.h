#ifndef WIFI_CONNECTION_H
#define WIFI_CONNECTION_H

#include <Arduino.h>

extern bool WIFIC_initStorage(void);
extern String WIFIC_getHostname(void);
extern bool WIFIC_validHostname(const String& name);
extern bool WIFIC_saveHostname(String name);
extern void WIFIC_init(void);
extern void WIFIC_setStSSID(String new_ssid);
extern void WIFIC_setStPass(String new_pass);
extern String WIFIC_getApList(void);
extern String WIFIC_getStSSID(void);
extern String WIFIC_getStPass(void);
extern String WIFIC_getStatus(void);
extern void WIFIC_process(void);

#endif
