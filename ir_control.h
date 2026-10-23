#ifndef IR_CONTROL_H
#define IR_CONTROL_H

#include <Arduino.h>

extern void IR_init(void);
extern void IR_sendPower(void);
extern bool IR_send(String protocol, uint64_t code, uint16_t bits, uint16_t repeat);
extern String IR_scanLists(void);
extern uint16_t IR_scanCount(uint8_t list);
extern String IR_scanSend(uint8_t list, uint16_t index);
extern String IR_acProtocols(void);
extern bool IR_sendAc(String protocol, bool power, String mode, uint8_t temp, String fan);

#endif
