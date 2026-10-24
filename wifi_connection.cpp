/*
 *  Author: Rada Berar
 *  email: ujagaga@gmail.com
 *
 *  WiFi connection module.
 *  No saved SSID: runs config AP only.
 *  Saved SSID: connects as station (AP kept running until connected and unused).
 *  On failure falls back to AP only and retries periodically.
 */
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP_EEPROM.h>
#include "config.h"

static char myApName[32] = {0};    /* Array to form AP name based on read MAC */
static char st_ssid[SSID_SIZE] = {0};    /* SSID to connect to */
static char st_pass[WIFI_PASS_SIZE];    /* Password for the requested SSID */
static IPAddress apIP(192, 168, 4, 1);
static bool apMode = false;
static uint32_t apModeAttempTime = 0;
static bool staConnecting = false;
static uint32_t staConnectStartTime = 0;
static bool mdnsStarted = false;

static char deviceHostname[HOSTNAME_SIZE] = HOSTNAME;
static const uint32_t hostnameMarker = 0x484E3031;

/* Run before any module reads EEPROM: ESP_EEPROM does not migrate resized blocks. */
bool WIFIC_initStorage(void){
  EEPROM.begin(EEPROM_SIZE);
  uint32_t marker = 0;
  EEPROM.get(HOSTNAME_MARKER_ADDR, marker);
  if(marker == hostnameMarker) return true;

  uint8_t legacy[EEPROM_LEGACY_SIZE];
  EEPROM.begin(EEPROM_LEGACY_SIZE);
  EEPROM.get(0, legacy);
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.put(0, legacy);
  EEPROM.put(HOSTNAME_MARKER_ADDR, hostnameMarker);
  for(unsigned int i = 0; i < HOSTNAME_SIZE; ++i) EEPROM.write(HOSTNAME_EEPROM_ADDR + i, 0);
  return EEPROM.commit();
}

bool WIFIC_validHostname(const String& name){
  if(name.length() == 0 || name.length() >= HOSTNAME_SIZE) return false;
  for(unsigned int i = 0; i < name.length(); ++i){
    char c = name[i];
    bool alnum = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
    if(!alnum && (c != '-' || i == 0 || i == name.length() - 1)) return false;
  }
  return true;
}

String WIFIC_getHostname(void){ return String(deviceHostname); }

bool WIFIC_saveHostname(String name){
  if(!name.isEmpty() && !WIFIC_validHostname(name)) return false;
  char saved[HOSTNAME_SIZE] = {0};
  name.toCharArray(saved, sizeof(saved));
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.put(HOSTNAME_EEPROM_ADDR, saved);
  return EEPROM.commit();
}

/* Returns wifi scan results */
String WIFIC_getApList(void){
  String result = "";
  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();
  if (n >0){
    result = WiFi.SSID(0);
    for (int i = 1; i < n; ++i)
    {
      result += "|" + WiFi.SSID(i);
    }
  }
  return result;
}

String WIFIC_getStatus(void){
  String result = "";
  if(apMode){
    result += "AP: " + String(myApName) + " (" + apIP.toString() + ")<br>";
  }
  if(WiFi.status() == WL_CONNECTED){
    result += "Connected to " + String(st_ssid) + " as " + WiFi.localIP().toString() + " / " + String(deviceHostname) + ".local";
  }else if(strlen(st_ssid) > 0){
    result += "Not connected to " + String(st_ssid);
  }else{
    result += "WiFi not configured";
  }
  return result;
}

static void startSoftAP(void){
  String ApName = AP_NAME_PREFIX + WiFi.macAddress();
  ApName.toCharArray(myApName, ApName.length() + 1);

  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

  if(WiFi.softAP(myApName, PASSWORD)){
    Serial.println("AP running. SSID: " + String(myApName) + ", IP:" + apIP.toString());
    apMode = true;
  }else{
    Serial.println("Failed to start AP.");
  }
}

/* Initiates a local AP only */
static void APMode(void){
  Serial.println("\nStarting AP mode");

  WiFi.mode(WIFI_AP);
  startSoftAP();

  staConnecting = false;
  apModeAttempTime = millis();
}

/* Keeps the AP running and starts a non blocking STA connection attempt. */
static void stationMode(void){
  Serial.printf("\n\nTrying STA+AP mode with [%s]\r\n", st_ssid);

  WiFi.setAutoReconnect(true);
  WiFi.mode(WIFI_AP_STA);
  startSoftAP();
  WiFi.hostname(deviceHostname);
  WiFi.config(0U, 0U, 0U);  // This disables static config.
  WiFi.begin(st_ssid, st_pass);

  staConnecting = true;
  staConnectStartTime = millis();
}

static void onStationConnected(void){
  Serial.printf("Connected. IP address: %s\n", WiFi.localIP().toString().c_str());

  if(!mdnsStarted){
    mdnsStarted = MDNS.begin(deviceHostname);
    MDNS.addService("http", "tcp", 80);
  }
}

String WIFIC_getStSSID(void){
  return String(st_ssid);
}

void WIFIC_setStSSID(String new_ssid){
  EEPROM.begin(EEPROM_SIZE);

  uint16_t addr;

  for(addr = 0; addr < new_ssid.length(); addr++){
    EEPROM.put(addr + SSID_EEPROM_ADDR, new_ssid[addr]);
    st_ssid[addr] = new_ssid[addr];
  }
  EEPROM.put(addr + SSID_EEPROM_ADDR, 0);
  st_ssid[addr] = 0;

  EEPROM.commit();
}

String WIFIC_getStPass(void){
  return String(st_pass);
}

void WIFIC_setStPass(String new_pass){
  EEPROM.begin(EEPROM_SIZE);

  uint16_t addr;
  for(addr = 0; addr < new_pass.length(); addr++){
    EEPROM.put(addr + WIFI_PASS_EEPROM_ADDR, new_pass[addr]);
    st_pass[addr] = new_pass[addr];
  }
  EEPROM.put(addr + WIFI_PASS_EEPROM_ADDR, 0);
  st_pass[addr] = 0;

  EEPROM.commit();
}

void WIFIC_init(void){
  ESP.wdtFeed();
   /* Read settings from EEPROM */
  EEPROM.begin(EEPROM_SIZE);
  char savedHostname[HOSTNAME_SIZE] = {0};
  EEPROM.get(HOSTNAME_EEPROM_ADDR, savedHostname);
  if(savedHostname[HOSTNAME_SIZE - 1] == 0 && WIFIC_validHostname(String(savedHostname))){
    memcpy(deviceHostname, savedHostname, sizeof(deviceHostname));
  }
  uint16_t i = 0;

  do{
    EEPROM.get(i + WIFI_PASS_EEPROM_ADDR, st_pass[i]);
    if((st_pass[i] < 32) || (st_pass[i] > 126)){
      /* Non printable character */
      break;
    }
    i++;
  }while(i < (WIFI_PASS_SIZE - 1));
  st_pass[i] = 0;

  i = 0;
  do{
    EEPROM.get(i + SSID_EEPROM_ADDR, st_ssid[i]);
    if((st_ssid[i] < 32) || (st_ssid[i] > 126)){
      /* Non printable character */
      break;
    }
    i++;
  }while(i < (SSID_SIZE - 1));
  st_ssid[i] = 0;

  WiFi.persistent(false);   /* Do not use or overwrite credentials stored in SDK flash. */

  if(strlen(st_ssid) > 0){
    stationMode();
  }else{
    APMode();
  }
}

void WIFIC_process(void) {
  uint32_t now = millis();

  if(mdnsStarted){
    MDNS.update();
  }

  if (staConnecting) {
    if (WiFi.status() == WL_CONNECTED) {
      staConnecting = false;
      onStationConnected();
    } else if ((now - staConnectStartTime) > (STA_CONNECT_TIMEOUT_S * 1000ul)) {
      Serial.println("STA connection failed, falling back to AP only.");
      APMode();
    }
    return;
  }

  if (apMode) {
    if (WiFi.status() == WL_CONNECTED) {
      /* STA+AP connected. Drop the AP once nobody uses it. */
      if (wifi_softap_get_station_num() == 0) {
        Serial.println("No AP clients, switching to STA mode only.");
        WiFi.mode(WIFI_STA);
        apMode = false;
      }
    } else if ((strlen(st_ssid) > 0) && ((now - apModeAttempTime) > (AP_MODE_TIMEOUT_S * 1000ul))) {
      stationMode();
    }
    return;
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection lost, reconnecting in STA+AP mode...");
    stationMode();
  }
}
