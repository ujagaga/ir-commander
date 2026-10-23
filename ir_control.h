#ifndef IR_CONTROL_H
#define IR_CONTROL_H

#include <Arduino.h>

extern void IR_init(void);
extern bool IR_sendTv(String cmd);
extern bool IR_send(String protocol, uint64_t code, uint16_t bits, uint16_t repeat);
extern uint16_t IR_tvCodeCount(void);
extern String IR_tvCodeDesc(uint16_t index);
extern String IR_tvCodeSend(uint16_t index);
extern uint16_t IR_getTvCode(void);
extern String IR_acProtocols(void);
extern String IR_getAcProtocol(void);
extern bool IR_saveSettings(uint16_t tvCode, String acProtocol);
extern bool IR_sendAc(String protocol, bool power, String mode, uint8_t temp, String fan);

#endif
