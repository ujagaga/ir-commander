#ifndef IR_CONTROL_H
#define IR_CONTROL_H

#include <Arduino.h>

extern void IR_init(void);
extern void IR_sendPower(void);
extern bool IR_send(String protocol, uint64_t code, uint16_t bits, uint16_t repeat);

#endif
