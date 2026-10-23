/*
 *  Author: Rada Berar
 *  email: ujagaga@gmail.com
 *
 *  IR transmitter module. Drives one IR LED from several pins at once.
 */
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRutils.h>
#include "config.h"

static const uint8_t irPins[] = IR_PINS;
static uint32_t irPinMask = 0;
static IRsend irsend(IR_PIN_MAIN);

/* The IR library toggles only IR_PIN_MAIN using digitalWrite. The core declares digitalWrite
 * as a weak symbol, so we override it here and set/clear all IR pins with a single register write
 * for that pin. All other pins behave as usual. */
extern "C" void __digitalWrite(uint8_t pin, uint8_t val);

extern "C" void IRAM_ATTR digitalWrite(uint8_t pin, uint8_t val){
  if((pin == IR_PIN_MAIN) && (irPinMask != 0)){
    if(val) GPOS = irPinMask;
    else GPOC = irPinMask;
  }else{
    __digitalWrite(pin, val);
  }
}

void IR_init(void){
  for(uint8_t i = 0; i < sizeof(irPins); i++){
    pinMode(irPins[i], OUTPUT);
    irPinMask |= (1ul << irPins[i]);
  }
  irsend.begin();
}

bool IR_send(String protocol, uint64_t code, uint16_t bits, uint16_t repeat){
  decode_type_t type = strToDecodeType(protocol.c_str());
  if(type == decode_type_t::UNKNOWN){
    return false;
  }
  if(bits == 0){
    bits = IRsend::defaultBits(type);
  }
  Serial.printf("IR send %s 0x%llX (%u bits, %u repeats)\n", protocol.c_str(), code, bits, repeat);
  return irsend.send(type, code, bits, repeat);
}

void IR_sendPower(void){
  IR_send(TV_POWER_PROTOCOL, TV_POWER_CODE, TV_POWER_BITS, 0);
}
