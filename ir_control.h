#ifndef IR_CONTROL_H
#define IR_CONTROL_H

#include <Arduino.h>

extern void IR_init(void);
extern void IR_sendPower(void);
extern bool IR_send(String protocol, uint64_t code, uint16_t bits, uint16_t repeat);
extern uint16_t IR_scanCount(void);
extern String IR_scanSend(uint16_t index);
extern void IR_sendMideaAc(bool power, uint8_t mode, uint8_t temp, uint8_t fan);

#endif
