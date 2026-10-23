#ifndef OTA_H
#define OTA_H

#include <Arduino.h>

extern void OTA_init(void);
extern void OTA_process(void);
extern bool OTA_requestCheck(void);
extern String OTA_getStatus(void);

#endif
