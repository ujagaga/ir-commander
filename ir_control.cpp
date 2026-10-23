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
#include <ESP_EEPROM.h>
#include "config.h"
#include "ir_codes.h"

static const uint8_t irPins[] = IR_PINS;
static uint32_t irPinMask = 0;
static IRsend irsend(IR_PIN_MAIN);
static IRac ac(IR_PIN_MAIN);
static const ir_code_list_t& tvCodes = irCodeLists[0];
static uint16_t tvCode = TV_CODE_DEFAULT;
static decode_type_t acProtocol;

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

  /* Load settings, keep defaults if never saved or invalid. */
  acProtocol = strToDecodeType(AC_PROTOCOL_DEFAULT);
  uint16_t storedTv;
  uint8_t storedAc;
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(TV_CODE_EEPROM_ADDR, storedTv);
  EEPROM.get(AC_PROTOCOL_EEPROM_ADDR, storedAc);
  if(storedTv < tvCodes.count){
    tvCode = storedTv;
  }
  if(IRac::isProtocolSupported((decode_type_t)storedAc)){
    acProtocol = (decode_type_t)storedAc;
  }
}

uint16_t IR_getTvCode(void){
  return tvCode;
}

String IR_getAcProtocol(void){
  return typeToString(acProtocol);
}

bool IR_saveSettings(uint16_t newTvCode, String newAcProtocol){
  decode_type_t newAc = strToDecodeType(newAcProtocol.c_str());
  if((newTvCode >= tvCodes.count) || !IRac::isProtocolSupported(newAc)){
    return false;
  }
  tvCode = newTvCode;
  acProtocol = newAc;
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.put(TV_CODE_EEPROM_ADDR, tvCode);
  EEPROM.put(AC_PROTOCOL_EEPROM_ADDR, (uint8_t)acProtocol);
  return EEPROM.commit();
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

uint16_t IR_tvCodeCount(void){
  return tvCodes.count;
}

static String codeDesc(const ir_code_t& entry){
  String desc;
  if(entry.type == decode_type_t::RAW){
    desc = "RAW " + String(entry.bits) + "kHz";
  }else{
    desc = typeToString(entry.type) + " 0x" + String(entry.code, HEX) + " " + String(entry.bits) + " bits";
  }
  if(entry.model[0]) desc += " (" + String(entry.model) + ")";
  return desc;
}

String IR_tvCodeDesc(uint16_t index){
  ir_code_t entry;
  memcpy_P(&entry, &tvCodes.codes[index], sizeof(entry));
  return codeDesc(entry);
}

/* Sends one code from the TV power list and returns its description. */
String IR_tvCodeSend(uint16_t index){
  static uint16_t rawBuf[IR_CODES_RAW_MAX];
  ir_code_t entry;
  memcpy_P(&entry, &tvCodes.codes[index], sizeof(entry));

  if(entry.type == decode_type_t::RAW){
    memcpy_P(rawBuf, entry.raw, entry.rawLen * sizeof(uint16_t));
    irsend.sendRaw(rawBuf, entry.rawLen, entry.bits);
  }else{
    irsend.send(entry.type, entry.code, entry.bits);
  }
  String desc = codeDesc(entry);
  Serial.println("IR TV code " + String(index) + ": " + desc);
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

/* cmd: power, volume_up, volume_down, mute. Returns false for an unknown command. */
bool IR_sendTv(String cmd){
  if(cmd == "power"){
    IR_tvCodeSend(tvCode);
    return true;
  }
  if(cmd == "volume_up") return IR_send(TV_BUTTONS_PROTOCOL, TV_VOLUME_UP_CODE, TV_BUTTONS_BITS, 0);
  if(cmd == "volume_down") return IR_send(TV_BUTTONS_PROTOCOL, TV_VOLUME_DOWN_CODE, TV_BUTTONS_BITS, 0);
  if(cmd == "mute") return IR_send(TV_BUTTONS_PROTOCOL, TV_MUTE_CODE, TV_BUTTONS_BITS, 0);
  return false;
}
