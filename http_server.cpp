/*
 *  Author: Rada Berar
 *  email: ujagaga@gmail.com
 *
 *  HTTP server: home page, settings page, IR trigger API and WiFi configuration.
 *
 *  GET /tv?cmd=power                                   - TV button: power (stored code), volume_up, volume_down, mute
 *  GET /ac?power=1&mode=cool&temp=24&fan=auto[&protocol=COOLIX] - AC (mode cool,heat,auto,dry,fan; fan auto,low,medium,high)
 *                                                        Without protocol the stored one is used.
 *  GET /temperature                                    - room temperature in C ("" while unread), 404 without DS18B20
 *  GET /update                                         - check GitHub for newer firmware now
 *  GET /tvcodes                                        - TV power code list, one description per line
 *  GET /scan?i=N                                       - send TV power code N
 *  GET /save?tv=N&ac=COOLIX                            - store TV code index and AC protocol to EEPROM
 *  GET /acprotocols                                    - supported AC protocols, '|' separated
 *  GET /send?protocol=NEC&code=0x20DF10EF[&bits=32][&repeat=0] - send any code (for finding the right one)
 */

#include <ESP8266WebServer.h>
#include <pgmspace.h>
#include "wifi_connection.h"
#include "ir_control.h"
#include "ota.h"
#include "temp_sensor.h"
#include "config.h"
#include "ui.h"

static ESP8266WebServer webServer(80);

/* Keep page assets in flash: concatenating the entire page can exhaust heap. */
static void sendFlash(PGM_P content){
  char chunk[512];
  size_t remaining = strlen_P(content);
  while(remaining){
    size_t count = remaining < sizeof(chunk) ? remaining : sizeof(chunk);
    memcpy_P(chunk, content, count);
    webServer.sendContent(chunk, count);
    content += count;
    remaining -= count;
  }
}

static void beginPage(void){
  webServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
  webServer.sendHeader("Cache-Control", "no-store");
  webServer.send(200, "text/html; charset=utf-8", "");
  sendFlash(HTML_BEGIN);
  webServer.sendContent(String(FPSTR(UI_TITLE_OPEN)) + WIFIC_getHostname() + String(FPSTR(UI_TITLE_CLOSE)));
  sendFlash(HTML_LAYOUT);
}

static void endPage(void){
  sendFlash(HTML_END);
  webServer.sendContent("");
}

static void sendPage(const String& body){
  beginPage();
  webServer.sendContent(body);
  endPage();
}

// Last AC settings sent from the home page, shown again on reload. Lost on reboot.
static String acMode = "cool";
static String acFan = "auto";
static int acTemp = 24;
static bool acOn = false;

static void showStartPage(void){
  uint8_t controls = WIFIC_getVisibleControls();
  String body = String(FPSTR(UI_STYLE_OPEN));
  if(!(controls & 1)) body += String(FPSTR(UI_HIDE_TV));
  if(!(controls & 2)) body += String(FPSTR(UI_HIDE_AC));
  if(controls != 3) body += String(FPSTR(UI_SINGLE_COLUMN));
  if(controls == 0) body += String(FPSTR(UI_HIDE_FEEDBACK));
  body += String(FPSTR(UI_STYLE_CLOSE));
  if(controls == 0) body += String(FPSTR(UI_EMPTY_HOME));
  beginPage();
  webServer.sendContent(body);
  sendFlash(INDEX_HTML);
  webServer.sendContent(String(FPSTR(UI_AC_SET)) + acMode + String(FPSTR(UI_QUOTE_COMMA)) + String(acTemp) + String(FPSTR(UI_COMMA_QUOTE)) + acFan + String(FPSTR(UI_CALL_END)) + String(acOn) + String(FPSTR(UI_SCRIPT_CALL_END)));
  if(TEMP_present()) webServer.sendContent(String(FPSTR(UI_ROOM_START)) + TEMP_get() + String(FPSTR(UI_CALL_END_QUOTE)));
  endPage();
}

static void showSettings(void){
  String body = String(FPSTR(UI_TV_STATE)) + String(IR_getTvCode()) + String(FPSTR(UI_AC_STATE)) + IR_getAcProtocol() + String(FPSTR(UI_STRING_SCRIPT_END));
  body += String(FPSTR(UI_HOST_STATE)) + WIFIC_getHostname() + String(FPSTR(UI_STRING_SCRIPT_END));
  body += String(FPSTR(UI_VISIBILITY_STATE)) + String(WIFIC_getVisibleControls()) + String(FPSTR(UI_SCRIPT_END));
  beginPage();
  webServer.sendContent(body);
  sendFlash(SETTINGS_HTML);
  webServer.sendContent(String(FPSTR(UI_PARAGRAPH_OPEN)) + WIFIC_getStatus() + String(FPSTR(UI_BREAK)) + OTA_getStatus() + String(FPSTR(UI_SYSTEM_END)));
  endPage();
}

static void tvCommand(void){
  if(IR_sendTv(webServer.arg("cmd"))){
    webServer.send(200, "text/plain", "OK");
  }else{
    webServer.send(400, "text/plain", "Unknown command. Use power, volume_up, volume_down or mute.");
  }
}

static void sendCode(void){
  String protocol = webServer.arg("protocol");
  uint64_t code = strtoull(webServer.arg("code").c_str(), nullptr, 0);
  uint16_t bits = webServer.arg("bits").toInt();
  uint16_t repeat = webServer.arg("repeat").toInt();

  if(IR_send(protocol, code, bits, repeat)){
    webServer.send(200, "text/plain", "OK");
  }else{
    webServer.send(400, "text/plain", "Unknown protocol or bad parameters");
  }
}

static void checkUpdate(void){
  if(OTA_requestCheck()){
    webServer.send(200, "text/plain", "Update check started. Reload the page later for status.");
  }else{
    webServer.send(409, "text/plain", "Not connected to WiFi or check already in progress.");
  }
}

static void scanCode(void){
  uint16_t i = webServer.arg("i").toInt();
  if(i >= IR_tvCodeCount()){
    webServer.send(400, "text/plain", "End of list");
    return;
  }
  webServer.send(200, "text/plain", String(i) + ": " + IR_tvCodeSend(i));
}

/* List is too long for one String, so it is sent in chunks. */
static void tvCodes(void){
  webServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
  webServer.send(200, "text/plain", "");
  String chunk = "";
  for(uint16_t i = 0; i < IR_tvCodeCount(); i++){
    chunk += IR_tvCodeDesc(i) + "\n";
    if(chunk.length() > 1000){
      webServer.sendContent(chunk);
      chunk = "";
    }
  }
  if(chunk.length() > 0) webServer.sendContent(chunk);
  webServer.sendContent("");  // Empty chunk ends the response.
}

static void saveSettings(void){
  if(IR_saveSettings(webServer.arg("tv").toInt(), webServer.arg("ac"))){
    webServer.send(200, "text/plain", "Saved.");
  }else{
    webServer.send(400, "text/plain", "Invalid TV code or AC protocol.");
  }
}

static void airCon(void){
  String protocol = webServer.hasArg("protocol") ? webServer.arg("protocol") : IR_getAcProtocol();
  if(IR_sendAc(protocol, webServer.arg("power").toInt(), webServer.arg("mode"),
               webServer.arg("temp").toInt(), webServer.arg("fan"))){
    // Protocol scan from Settings passes protocol and fixed values; don't remember those.
    String mode = webServer.arg("mode"), fan = webServer.arg("fan");
    int temp = webServer.arg("temp").toInt();
    if(!webServer.hasArg("protocol") && strstr("|cool|heat|auto|dry|fan|", ("|" + mode + "|").c_str())
       && strstr("|auto|low|medium|high|", ("|" + fan + "|").c_str()) && temp >= 17 && temp <= 30){
      acMode = mode;
      acFan = fan;
      acTemp = temp;
      acOn = webServer.arg("power").toInt();
    }
    webServer.send(200, "text/plain", "OK");
  }else{
    webServer.send(400, "text/plain", "Unsupported AC protocol");
  }
}

static void roomTemperature(void){
  if(TEMP_present()){
    webServer.send(200, "text/plain", TEMP_get());
  }else{
    webServer.send(404, "text/plain", "No temperature sensor");
  }
}

static void acProtocols(void){
  webServer.send(200, "text/plain", IR_acProtocols());
}

static void showNotFound(void){
  webServer.send(404, "text/plain", "Not Found");
}

static void selectAP(void){
  beginPage();
  sendFlash(APLIST_HTML);
  endPage();
}

static void apList(void){
  webServer.send(200, "text/plain", WIFIC_getApList());
}

static void saveWiFi(void){
  String ssid = webServer.arg("s");
  String pass = webServer.arg("p");

  if((ssid.length() >= SSID_SIZE) || (pass.length() >= WIFI_PASS_SIZE)){
    sendPage(String(FPSTR(UI_WIFI_INVALID)));
    return;
  }

  WIFIC_setStSSID(ssid);
  WIFIC_setStPass(pass);

  String msg;
  if(ssid.length() > 0){
    msg = String(FPSTR(UI_WIFI_CONNECTING)) + ssid;
    msg += String(FPSTR(UI_WIFI_LINK)) + WIFIC_getHostname() + String(FPSTR(UI_LOCAL_LINK_TEXT)) + WIFIC_getHostname() + String(FPSTR(UI_WIFI_LINK_END));
  }else{
    msg = String(FPSTR(UI_WIFI_AP));
  }
  sendPage(String(FPSTR(UI_CONNECTION_TITLE)) + msg + String(FPSTR(UI_PARAGRAPH_CLOSE)));

  /* Let the response go out, then restart to apply.
   Note: ESP.restart may hang the first time after serial programming. Reset once manually after programming. */
  delay(1000);
  ESP.restart();
}

static void saveVisibility(void){
  String tv = webServer.arg("tv"), ac = webServer.arg("ac");
  if((tv != "0" && tv != "1") || (ac != "0" && ac != "1")){
    webServer.send(400, "text/plain", "TV and AC visibility must each be 0 or 1.");
    return;
  }
  uint8_t controls = (tv == "1" ? 1 : 0) | (ac == "1" ? 2 : 0);
  if(!WIFIC_saveVisibleControls(controls)){
    webServer.send(500, "text/plain", "Could not save visibility. Please try again.");
    return;
  }
  webServer.send(200, "text/plain", "Saved.");
}

static void saveHostname(void){
  if(!webServer.hasArg("hostname")){
    webServer.send(400, "text/plain", "Missing hostname.");
    return;
  }
  String name = webServer.arg("hostname");
  name.trim();
  name.toLowerCase();
  if(!name.isEmpty() && !WIFIC_validHostname(name)){
    sendPage(String(FPSTR(UI_HOST_INVALID)));
    return;
  }
  if(!WIFIC_saveHostname(name)){
    webServer.send(500, "text/plain", "Could not save hostname. Please try again.");
    return;
  }
  if(name.isEmpty()) name = HOSTNAME;
  sendPage(String(FPSTR(UI_HOST_SAVED)) + name + String(FPSTR(UI_LOCAL_LINK_TEXT)) + name + String(FPSTR(UI_HOST_LINK_END)));
  delay(1000);
  ESP.restart();
}

void HTTP_SERVER_process(void){
  webServer.handleClient();
}

static void sendIcon(PGM_P content){
  webServer.sendHeader("Cache-Control", "public, max-age=31536000, immutable");
  webServer.sendHeader("X-Content-Type-Options", "nosniff");
  webServer.setContentLength(strlen_P(content));
  webServer.send(200, "image/svg+xml", "");
  sendFlash(content);
}

static void handleNotFound(void){
  if(webServer.uri().startsWith("/icons/")){
    webServer.sendHeader("Cache-Control", "no-store");
    showNotFound();
    return;
  }
  showStartPage();
}

void HTTP_SERVER_init(void){
  for(const auto& icon : ICON_ASSETS){
    PGM_P content = icon.content;
    webServer.on(icon.url, HTTP_GET, [content](){ sendIcon(content); });
  }
  webServer.on("/", showStartPage);
  webServer.on("/favicon.ico", showNotFound);
  webServer.on("/settings", showSettings);
  webServer.on("/hostname", HTTP_POST, saveHostname);
  webServer.on("/visibility", HTTP_POST, saveVisibility);
  webServer.on("/tv", tvCommand);
  webServer.on("/send", sendCode);
  webServer.on("/scan", scanCode);
  webServer.on("/tvcodes", tvCodes);
  webServer.on("/save", saveSettings);
  webServer.on("/ac", airCon);
  webServer.on("/acprotocols", acProtocols);
  webServer.on("/temperature", roomTemperature);
  webServer.on("/update", checkUpdate);
  webServer.on("/selectap", selectAP);
  webServer.on("/aplist", apList);
  webServer.on("/wifisave", saveWiFi);
  webServer.onNotFound(handleNotFound);

  webServer.begin();
}
