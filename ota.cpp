/*
 *  Author: Rada Berar
 *  email: ujagaga@gmail.com
 *
 *  Firmware update from GitHub. Ported from MiniBot ESP32_CAM.
 *  After each WiFi connection (or on request) reads FIRMWARE_VERSION from config.h
 *  of the latest commit and, only when it is higher than the running one,
 *  installs OTA_FIRMWARE_PATH from the same commit.
 *  TLS certificates are NOT verified (setInsecure).
 */
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <WiFiClientSecureBearSSL.h>
#include <ESP_EEPROM.h>
#include <eboot_command.h>
#include "config.h"
#include "firmware_version.h"

static WiFiEventHandler gotIpHandler;
static bool connectedEvent = false;
static bool pending = false;
static uint32_t nextAttempt = 0;
static uint8_t attempts = 0;
static uint32_t foundVersion = 0;
static const char* updateStatus = "idle";

/* A target newer than the running firmware means the last installed image did
 * not deliver the advertised version. Keep it across boots to avoid an update loop. */
static uint32_t invalidVersion = 0;

static bool saveTargetVersion(uint32_t version){
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.put(OTA_TARGET_EEPROM_ADDR, version);
  return EEPROM.commit();
}

static void loadTargetVersion(void){
  uint32_t target = 0;
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(OTA_TARGET_EEPROM_ADDR, target);
  if(target == 0xFFFFFFFF){
    target = 0;   /* Never written */
  }
  if(target > FIRMWARE_VERSION){
    invalidVersion = target;
    Serial.printf("OTA: target %lu was not installed; marking invalid\n", (unsigned long)target);
  }else if(target){
    /* The target was reached (or superseded by a USB upload). */
    saveTargetVersion(0);
  }
}

static bool getText(const String& url, const char* accept, size_t limit, String& body){
  BearSSL::WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setTimeout(15000);
  if(!http.begin(client, url)) return false;
  http.setUserAgent("ir-commander-OTA");
  http.addHeader("Accept", accept);
  int status = http.GET();
  int size = http.getSize();
  bool ok = (status == HTTP_CODE_OK) && (size > 0) && ((size_t)size <= limit);
  if(ok){
    body = http.getString();
    ok = (body.length() == (size_t)size);
  }
  if(!ok) Serial.printf("OTA: HTTP request failed (status %d, size %d)\n", status, size);
  http.end();
  return ok;
}

static bool checkRepository(void){
  updateStatus = "checking";
  Serial.println("OTA: checking GitHub firmware version");
  String commit;
  String api = String("https://api.github.com/repos/") + OTA_GITHUB_REPOSITORY +
               "/commits/" + OTA_GITHUB_BRANCH;
  if(!getText(api, "application/vnd.github.sha", 128, commit)) return false;
  commit.trim();
  if(commit.length() != 40) return false;
  for(unsigned int i = 0; i < commit.length(); ++i){
    if(!isxdigit((unsigned char)commit[i])) return false;
  }

  String base = String("https://raw.githubusercontent.com/") + OTA_GITHUB_REPOSITORY + "/" + commit;
  String config;
  uint32_t remoteVersion = 0;
  if(!getText(base + "/config.h", "text/plain", 8192, config)) return false;
  if(!parseFirmwareVersion(config.c_str(), remoteVersion)){
    Serial.println("OTA: invalid remote FIRMWARE_VERSION; skipping");
    return false;
  }
  Serial.printf("OTA: device %lu, GitHub %lu\n", (unsigned long)FIRMWARE_VERSION, (unsigned long)remoteVersion);
  foundVersion = remoteVersion;
  if(remoteVersion <= FIRMWARE_VERSION){
    updateStatus = "up_to_date";
    return true;
  }
  if(remoteVersion == invalidVersion){
    updateStatus = "invalid";
    Serial.printf("OTA: version %lu is invalid; skipping repeated update\n", (unsigned long)remoteVersion);
    return true;
  }

  Serial.println("OTA: downloading newer firmware...");
  updateStatus = "installing";
  /* Onboard LED (GPIO2, active LOW) stays on while downloading. */
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  BearSSL::WiFiClientSecure client;
  client.setInsecure();
  ESPhttpUpdate.rebootOnUpdate(false);
  ESPhttpUpdate.onProgress([](int current, int total){
    static int lastStep = -1;
    if(current == 0) lastStep = -1;
    int step = total > 0 ? (int)((uint64_t)current * 10 / total) : 0;
    if(step != lastStep){ lastStep = step; Serial.printf("OTA: %d%%\n", step * 10); }
  });
  /* Checks image header and free space, writes to the free flash area and
   * schedules the copy for next boot only after a complete download. */
  t_httpUpdate_return result = ESPhttpUpdate.update(client, base + "/" + OTA_FIRMWARE_PATH);
  digitalWrite(LED_BUILTIN, HIGH);
  if(result != HTTP_UPDATE_OK){
    Serial.printf("OTA: update failed: %s\n", ESPhttpUpdate.getLastErrorString().c_str());
    return false;
  }
  if(!saveTargetVersion(remoteVersion)){
    /* Cancel the scheduled copy if we cannot persist the guard,
     * so a later reset cannot start an update loop. */
    eboot_command_clear();
    Serial.println("OTA: target save failed; update cancelled");
    return false;
  }
  Serial.println("OTA: installed; restarting");

  Serial.flush();
  delay(1000);
  ESP.restart();
  return true;
}

void OTA_init(void){
  loadTargetVersion();
  gotIpHandler = WiFi.onStationModeGotIP([](const WiFiEventStationModeGotIP&){
    connectedEvent = true;
  });
}

void OTA_process(void){
  if(connectedEvent){
    connectedEvent = false;
    pending = true;
    foundVersion = 0;
    updateStatus = "waiting";
    attempts = 0;
    nextAttempt = millis() + 1000;
  }
  if(!pending || (WiFi.status() != WL_CONNECTED) || ((int32_t)(millis() - nextAttempt) < 0)) return;

  bool done = checkRepository();
  if(done){
    pending = false;
  }else if(++attempts >= 3){
    pending = false;
    updateStatus = "failed";
  }else{
    nextAttempt = millis() + 60000;
    updateStatus = "retry";
  }
}

bool OTA_requestCheck(void){
  if(pending || (WiFi.status() != WL_CONNECTED)) return false;
  connectedEvent = true;
  return true;
}

String OTA_getStatus(void){
  String result = "Firmware V" + String(FIRMWARE_VERSION) + ", update: " + updateStatus;
  if(foundVersion) result += " (GitHub V" + String(foundVersion) + ")";
  if(invalidVersion) result += ", invalid V" + String(invalidVersion);
  return result;
}
