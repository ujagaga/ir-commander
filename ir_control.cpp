/*
 *  Author: Rada Berar
 *  email: ujagaga@gmail.com
 *
 *  IR transmitter module. Drives one IR LED from several pins at once.
 */
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRutils.h>
#include <ir_Midea.h>
#include "config.h"
#include "tv_power_codes.h"

static const uint8_t irPins[] = IR_PINS;
static uint32_t irPinMask = 0;
static IRsend irsend(IR_PIN_MAIN);
static IRMideaAC midea(IR_PIN_MAIN);

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
  midea.begin();
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

uint16_t IR_scanCount(void){
  return sizeof(tvPowerCodes) / sizeof(tvPowerCodes[0]);
}

/* Sends one code from the universal TV power list and returns its description. */
String IR_scanSend(uint16_t index){
  static uint16_t rawBuf[TV_POWER_RAW_MAX];
  tv_power_code_t entry;
  memcpy_P(&entry, &tvPowerCodes[index], sizeof(entry));

  String desc = String(index) + "/" + String(IR_scanCount() - 1) + ": ";
  if(entry.type == decode_type_t::RAW){
    memcpy_P(rawBuf, entry.raw, entry.rawLen * sizeof(uint16_t));
    irsend.sendRaw(rawBuf, entry.rawLen, entry.bits);
    desc += "RAW " + String(entry.bits) + "kHz";
  }else{
    irsend.send(entry.type, entry.code, entry.bits);
    desc += typeToString(entry.type) + " 0x" + String(entry.code, HEX) + " " + String(entry.bits) + " bits";
  }
  Serial.println("IR scan " + desc);
  return desc;
}

/* Midea AC sends the complete state with every command.
 * mode: 0 cool, 1 dry, 2 auto, 3 heat, 4 fan. fan: 0 auto, 1 low, 2 med, 3 high. temp: 17..30 C */
void IR_sendMideaAc(bool power, uint8_t mode, uint8_t temp, uint8_t fan){
  midea.stateReset();
  midea.setPower(power);
  midea.setMode(mode);
  midea.setTemp(temp, true);
  midea.setFan(fan);
  Serial.println("IR Midea AC: " + midea.toString());
  midea.send();
}

void IR_sendPower(void){
  IR_send(TV_POWER_PROTOCOL, TV_POWER_CODE, TV_POWER_BITS, 0);
}
