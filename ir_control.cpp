/*
 *  Author: Rada Berar
 *  email: ujagaga@gmail.com
 *
 *  IR transmitter module. Drives one IR LED from several pins at once.
 */
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRutils.h>
#include <IRac.h>
#include "config.h"
#include "ir_codes.h"

static const uint8_t irPins[] = IR_PINS;
static uint32_t irPinMask = 0;
static IRsend irsend(IR_PIN_MAIN);
static IRac ac(IR_PIN_MAIN);

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

static const uint8_t scanListCount = sizeof(irCodeLists) / sizeof(irCodeLists[0]);

/* Returns titles of the scan lists, separated by '|'. */
String IR_scanLists(void){
  String result = "";
  for(uint8_t i = 0; i < scanListCount; i++){
    if(i > 0) result += "|";
    result += irCodeLists[i].title;
  }
  return result;
}

uint16_t IR_scanCount(uint8_t list){
  return (list < scanListCount) ? irCodeLists[list].count : 0;
}

/* Sends one code from a scan list and returns its description. */
String IR_scanSend(uint8_t list, uint16_t index){
  static uint16_t rawBuf[IR_CODES_RAW_MAX];
  ir_code_t entry;
  memcpy_P(&entry, &irCodeLists[list].codes[index], sizeof(entry));

  String desc = String(index) + "/" + String(IR_scanCount(list) - 1) + ": ";
  if(entry.type == decode_type_t::RAW){
    memcpy_P(rawBuf, entry.raw, entry.rawLen * sizeof(uint16_t));
    irsend.sendRaw(rawBuf, entry.rawLen, entry.bits);
    desc += "RAW " + String(entry.bits) + "kHz";
  }else{
    irsend.send(entry.type, entry.code, entry.bits);
    desc += typeToString(entry.type) + " 0x" + String(entry.code, HEX) + " " + String(entry.bits) + " bits";
  }
  if(entry.model[0]) desc += " (" + String(entry.model) + ")";
  Serial.println("IR scan " + desc);
  return desc;
}

/* Returns names of all AC protocols the library can generate, separated by '|'. */
String IR_acProtocols(void){
  String result = "";
  for(int i = 0; i <= kLastDecodeType; i++){
    if(IRac::isProtocolSupported((decode_type_t)i)){
      if(result.length() > 0) result += "|";
      result += typeToString((decode_type_t)i);
    }
  }
  return result;
}

/* AC remotes send the complete state with every command.
 * mode: cool, heat, auto, dry, fan. fan: auto, low, medium, high. temp in C. */
bool IR_sendAc(String protocol, bool power, String mode, uint8_t temp, String fan){
  decode_type_t type = strToDecodeType(protocol.c_str());
  if(!IRac::isProtocolSupported(type)){
    return false;
  }
  ac.next.protocol = type;
  ac.next.power = power;
  ac.next.mode = IRac::strToOpmode(mode.c_str());
  ac.next.celsius = true;
  ac.next.degrees = temp;
  ac.next.fanspeed = IRac::strToFanspeed(fan.c_str());
  Serial.printf("IR AC %s power %d %s %uC fan %s\n", protocol.c_str(), power, mode.c_str(), temp, fan.c_str());
  return ac.sendAc();
}

void IR_sendPower(void){
  IR_send(TV_POWER_PROTOCOL, TV_POWER_CODE, TV_POWER_BITS, 0);
}
