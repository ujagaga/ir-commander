/*
 *  Author: Rada Berar
 *  email: ujagaga@gmail.com
 *
 *  HTTP server: control page, IR trigger API and WiFi configuration.
 *
 *  GET /power                                          - send TV power code
 *  GET /update                                         - check GitHub for newer firmware now
 *  GET /scan?l=L&i=N                                  - send code N from scan list L (see /scanlists)
 *  GET /scanlists                                      - scan list titles, '|' separated
 *  GET /ac?power=1&mode=cool&temp=24&fan=auto[&protocol=MIDEA] - AC (mode cool,heat,auto,dry,fan; fan auto,low,medium,high)
 *  GET /acprotocols                                    - supported AC protocols, '|' separated
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
      .btn_pwr{line-height:6rem;font-size:3rem;background-color:#e53935;user-select:none;-webkit-user-select:none;touch-action:none;}
      .btn_cfg{width:10rem;font-size:0.8rem;line-height:2rem;background-color:#ff3300;}
      input{width:95%;padding:5px;font-size:1em;}
    </style>
  </head>
  <body><div class="contain"><div class="center_div">
)html";

static const char HTML_END[] PROGMEM = "</div></div></body></html>";

static const char INDEX_HTML[] PROGMEM = R"html(
  <h1>IR Commander</h1>
  <button class="btn_pwr" type="button" onpointerdown="pwr(true)" onpointerup="pwr(false)" onpointerleave="pwr(false)" onpointercancel="pwr(false)" oncontextmenu="return false;">&#x23FB;</button>
  <p id='res'></p>
  <hr>
  <h3>Air conditioner</h3>
  <select id='ap'></select>
  <select id='am'><option value='cool'>Cool</option><option value='heat'>Heat</option><option value='auto'>Auto</option><option value='dry'>Dry</option><option value='fan'>Fan</option></select>
  <select id='at'></select>
  <select id='af'><option value='auto'>Fan auto</option><option value='low'>Low</option><option value='medium'>Medium</option><option value='high'>High</option></select><br><br>
  <div style="display:flex;gap:4px">
    <button type="button" onclick="ac(1)">AC on</button>
    <button type="button" onclick="ac(0)">AC off</button>
    <button type="button" id="aa" onclick="acAuto()">Scan</button>
  </div>
  <p>Scan: with the AC running, sends "off" in the next protocol every 3s. Stop when the AC turns off.</p>
  <p id='ares'></p>
  <hr>
  <h3>Find code</h3>
  <p>Auto sends the next code every 2s. Stop when the device reacts, then use Prev/Again to confirm.</p>
  <select id='sl' onchange="document.getElementById('si').value=0"></select>
  <input id='si' value='0' placeholder='code index'><br><br>
  <div style="display:flex;gap:4px">
    <button type="button" id="sa" onclick="auto()">Auto</button>
    <button type="button" onclick="step(-1)">Prev</button>
    <button type="button" onclick="step(0)">Again</button>
    <button type="button" onclick="step(1)">Next</button>
  </div>
  <p id='sres'></p>
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
  /* Keeps sending the power code while the button is held. */
  var held=false, n=0;
  function pwr(down){
    if(down && !held){ held=true; n=0; rep(); }
    if(!down) held=false;
  }
  function rep(){
    if(!held) return;
    fetch('/power').then(r=>r.text()).then(t=>{document.getElementById('res').innerText=t+' x'+(++n);}).finally(rep);
  }
  for(var t=17;t<=30;t++){ document.getElementById('at').innerHTML+='<option'+(t==24?' selected':'')+'>'+t+'</option>'; }
  fetch('/acprotocols').then(r=>r.text()).then(t=>{
    t.split('|').forEach(function(p){ document.getElementById('ap').innerHTML+='<option'+(p==')html" AC_PROTOCOL R"html('?' selected':'')+'>'+p+'</option>'; });
  });
  function ac(p){
    var v=function(id){return document.getElementById(id).value;};
    return fetch('/ac?protocol='+v('ap')+'&power='+p+'&mode='+v('am')+'&temp='+v('at')+'&fan='+v('af')).then(r=>r.text()).then(t=>{
      var s=document.getElementById('ap'); document.getElementById('ares').innerText=s.selectedIndex+'/'+(s.length-1)+': '+s.value+' '+t;
    });
  }
  var acScanning=false;
  function acAuto(){
    acScanning=!acScanning;
    document.getElementById('aa').innerText=acScanning?'Stop':'Scan';
    if(acScanning) ac(0).then(acNext);
  }
  function acNext(){
    setTimeout(function(){
      var s=document.getElementById('ap');
      if(!acScanning) return;
      if(s.selectedIndex>=s.length-1){ acAuto(); return; }
      s.selectedIndex++;
      ac(0).then(acNext);
    },3000);
  }
  fetch('/scanlists').then(r=>r.text()).then(t=>{
    t.split('|').forEach(function(n,i){ document.getElementById('sl').innerHTML+='<option value='+i+'>'+n+'</option>'; });
  });
  var scanning=false;
  function scan(i){
    if(i<0) i=0;
    document.getElementById('si').value=i;
    return fetch('/scan?l='+document.getElementById('sl').value+'&i='+i).then(r=>{
      return r.text().then(t=>{document.getElementById('sres').innerText=t; return r.ok;});
    });
  }
  function step(d){
    scanning=false; document.getElementById('sa').innerText='Auto';
    scan(parseInt(document.getElementById('si').value||0)+d);
  }
  function auto(){
    scanning=!scanning;
    document.getElementById('sa').innerText=scanning?'Stop':'Auto';
    if(scanning) scan(parseInt(document.getElementById('si').value||0)).then(next);
  }
  function next(ok){
    if(!ok){ scanning=false; document.getElementById('sa').innerText='Auto'; }
    if(!scanning) return;
    setTimeout(function(){
      if(scanning) scan(parseInt(document.getElementById('si').value)+1).then(next);
    },2000);
  }
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

static void scanCode(void){
  uint8_t l = webServer.arg("l").toInt();
  uint16_t i = webServer.arg("i").toInt();
  if(i >= IR_scanCount(l)){
    webServer.send(400, "text/plain", "End of list");
    return;
  }
  webServer.send(200, "text/plain", IR_scanSend(l, i));
}

static void scanLists(void){
  webServer.send(200, "text/plain", IR_scanLists());
}

static void airCon(void){
  String protocol = webServer.hasArg("protocol") ? webServer.arg("protocol") : AC_PROTOCOL;
  if(IR_sendAc(protocol, webServer.arg("power").toInt(), webServer.arg("mode"),
               webServer.arg("temp").toInt(), webServer.arg("fan"))){
    webServer.send(200, "text/plain", "OK");
  }else{
    webServer.send(400, "text/plain", "Unsupported AC protocol");
  }
}

static void acProtocols(void){
  webServer.send(200, "text/plain", IR_acProtocols());
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
  webServer.on("/scan", scanCode);
  webServer.on("/scanlists", scanLists);
  webServer.on("/ac", airCon);
  webServer.on("/acprotocols", acProtocols);
  webServer.on("/update", checkUpdate);
  webServer.on("/selectap", selectAP);
  webServer.on("/aplist", apList);
  webServer.on("/wifisave", saveWiFi);
  webServer.onNotFound(showStartPage);

  webServer.begin();
}
