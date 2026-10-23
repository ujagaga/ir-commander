/*
 *  Author: Rada Berar
 *  email: ujagaga@gmail.com
 *
 *  HTTP server: control page, IR trigger API and WiFi configuration.
 *
 *  GET /power                                          - send TV power code
 *  GET /update                                         - check GitHub for newer firmware now
 *  GET /send?protocol=NEC&code=0x20DF10EF[&bits=32][&repeat=0] - send any code (for finding the right one)
 */

#include <ESP8266WebServer.h>
#include <pgmspace.h>
#include "wifi_connection.h"
#include "ir_control.h"
#include "ota.h"
#include "config.h"

static const char HTML_BEGIN[] PROGMEM = R"html(
<!DOCTYPE HTML>
<html>
  <head>
    <meta name = "viewport" content = "width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0">
    <title>IR Commander</title>
    <style>
      body { background-color: white; font-family: Arial, Helvetica, Sans-Serif; Color: #000000; }
      .contain{width: 100%;}
      .center_div{margin:0 auto; max-width: 400px;position:relative;}
      button{width:100%;border:0;border-radius:0.3rem;color:#fff;line-height:2.4rem;font-size:1.2rem;background-color:#1fa3ec;}
      .btn_pwr{line-height:6rem;font-size:3rem;background-color:#e53935;}
      .btn_cfg{width:10rem;font-size:0.8rem;line-height:2rem;background-color:#ff3300;}
      input{width:95%;padding:5px;font-size:1em;}
    </style>
  </head>
  <body><div class="contain"><div class="center_div">
)html";

static const char HTML_END[] PROGMEM = "</div></div></body></html>";

static const char INDEX_HTML[] PROGMEM = R"html(
  <h1>IR Commander</h1>
  <button class="btn_pwr" type="button" onclick="pwr()">&#x23FB;</button>
  <p id='res'></p>
  <hr>
  <h3>Test code</h3>
  <input id='pr' placeholder='protocol (NEC, RC5, RC6, SAMSUNG...)'><br>
  <input id='cd' placeholder='code (e.g. 0x20DF10EF)'><br>
  <input id='bt' placeholder='bits (empty = protocol default)'><br><br>
  <button type="button" onclick="tst()">Send</button>
  <hr>
  <button class="btn_cfg" type="button" onclick="location.href='/selectap';">Configure wifi</button>
  <button class="btn_cfg" type="button" onclick="get('/update')">Check for update</button>
<script>
  function get(url){
    fetch(url).then(r=>r.text()).then(t=>{document.getElementById('res').innerText=t;});
  }
  function pwr(){ get('/power'); }
  function tst(){
    var v=function(id){return encodeURIComponent(document.getElementById(id).value);};
    get('/send?protocol='+v('pr')+'&code='+v('cd')+'&bits='+v('bt'));
  }
</script>
)html";

static const char APLIST_HTML[] PROGMEM = R"html(
  <style>#vm{height:50vh;overflow-y:auto;margin-bottom:1em;}</style>
  <h1 id='ttl'>Networks found:</h1>
  <div id='vm'>Please wait...</div>
  <form method='get' action='wifisave'>
    <button type='button' onclick='refresh();'>Rescan</button><br/><br/>
    <input id='s' name='s' length=32 placeholder='SSID (Leave blank for AP mode)'><br>
    <input id='p' name='p' length=63 placeholder='password'><br>
    <br><button type='submit'>save</button>
  </form>
<script>
  function c(l){
    document.getElementById('s').value=l.innerText||l.textContent;
    document.getElementById('p').focus();
  }
  function refresh(){
    var vm=document.getElementById('vm');
    vm.innerHTML='Please wait...';
    fetch('/aplist').then(r=>r.text()).then(t=>{
      vm.innerHTML='';
      if(!t.length){ document.getElementById('ttl').innerHTML='No networks found.'; return; }
      t.split('|').forEach(function(s,i){
        vm.innerHTML+='<span>'+(i+1)+": </span><a href='#p' onclick='c(this)'>"+s+'</a><br>';
      });
    });
  }
  refresh();
</script>
)html";

static ESP8266WebServer webServer(80);

static void sendPage(String body){
  String response = FPSTR(HTML_BEGIN);
  response += body;
  response += FPSTR(HTML_END);
  webServer.send(200, "text/html", response);
}

static void showStartPage(void){
  String body = FPSTR(INDEX_HTML);
  body += "<p>" + WIFIC_getStatus() + "<br>" + OTA_getStatus() + "</p>";
  sendPage(body);
}

static void power(void){
  IR_sendPower();
  webServer.send(200, "text/plain", "OK");
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

static void showNotFound(void){
  webServer.send(404, "text/plain", "Not Found");
}

static void selectAP(void){
  sendPage(FPSTR(APLIST_HTML));
}

static void apList(void){
  webServer.send(200, "text/plain", WIFIC_getApList());
}

static void saveWiFi(void){
  String ssid = webServer.arg("s");
  String pass = webServer.arg("p");

  if((ssid.length() >= SSID_SIZE) || (pass.length() >= WIFI_PASS_SIZE)){
    sendPage("<p>Sorry, this module can only remember SSID up to 32 and a PASSWORD up to 63 bytes long.</p>");
    return;
  }

  WIFIC_setStSSID(ssid);
  WIFIC_setStPass(pass);

  String msg;
  if(ssid.length() > 0){
    msg = "Saving settings and connecting to SSID: " + ssid;
    msg += "<br>Then open <a href='http://" HOSTNAME ".local'>http://" HOSTNAME ".local</a> or the IP your router assigned.";
  }else{
    msg = "Saving settings and switching to AP mode only.";
  }
  sendPage("<h1>Connection Status</h1><p>" + msg + "</p>");

  /* Let the response go out, then restart to apply.
   Note: ESP.restart may hang the first time after serial programming. Reset once manually after programming. */
  delay(1000);
  ESP.restart();
}

void HTTP_SERVER_process(void){
  webServer.handleClient();
}

void HTTP_SERVER_init(void){
  webServer.on("/", showStartPage);
  webServer.on("/favicon.ico", showNotFound);
  webServer.on("/power", power);
  webServer.on("/send", sendCode);
  webServer.on("/update", checkUpdate);
  webServer.on("/selectap", selectAP);
  webServer.on("/aplist", apList);
  webServer.on("/wifisave", saveWiFi);
  webServer.onNotFound(showStartPage);

  webServer.begin();
}
