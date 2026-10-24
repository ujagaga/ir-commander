#ifndef TEMP_SENSOR_H
#define TEMP_SENSOR_H

#include <Arduino.h>

extern void TEMP_init(void);
extern void TEMP_process(void);
extern bool TEMP_present(void);
extern String TEMP_get(void);

#endif
