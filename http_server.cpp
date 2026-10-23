/*
 *  Author: Rada Berar
 *  email: ujagaga@gmail.com
 *
 *  HTTP server: home page, settings page, IR trigger API and WiFi configuration.
 *
 *  GET /tv?cmd=power                                   - TV button: power (stored code), volume_up, volume_down, mute
 *  GET /ac?power=1&mode=cool&temp=24&fan=auto[&protocol=COOLIX] - AC (mode cool,heat,auto,dry,fan; fan auto,low,medium,high)
 *                                                        Without protocol the stored one is used.
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
      .hold{user-select:none;-webkit-user-select:none;touch-action:none;}
      input{width:95%;padding:5px;font-size:1em;}
      select{padding:5px;font-size:1em;max-width:100%;}
      .row{display:flex;gap:4px;}
      .nav{display:flex;gap:1.5rem;padding:0.6rem 0;margin-bottom:0.5rem;border-bottom:2px solid #1fa3ec;}
      .nav a{color:#1fa3ec;text-decoration:none;font-weight:bold;font-size:1.2rem;}
    </style>
  </head>
  <body><div class="contain"><div class="center_div">
  <div class="nav"><a href="/">Home</a><a href="/settings">Settings</a></div>
)html";

static const char HTML_END[] PROGMEM = "</div></div></body></html>";

static const char INDEX_HTML[] PROGMEM = R"html(
  <h3>TV</h3>
  <button class="btn_pwr hold" type="button" data-cmd="power">&#x23FB;</button><br><br>
  <div class="row">
    <button class="hold" type="button" data-cmd="volume_down">Vol &minus;</button>
    <button class="hold" type="button" data-cmd="mute">Mute</button>
    <button class="hold" type="button" data-cmd="volume_up">Vol +</button>
  </div>
  <hr>
  <h3>Air conditioner</h3>
  <select id='am'><option value='cool'>Cool</option><option value='heat'>Heat</option><option value='auto'>Auto</option><option value='dry'>Dry</option><option value='fan'>Fan</option></select>
  <select id='at'></select>
  <select id='af'><option value='auto'>Fan auto</option><option value='low'>Low</option><option value='medium'>Medium</option><option value='high'>High</option></select><br><br>
  <div class="row">
    <button type="button" onclick="ac(1)">ON</button>
    <button type="button" onclick="ac(0)">OFF</button>
  </div>
  <p id='res'></p>
<script>
  function show(t){ document.getElementById('res').innerText=t; }
  /* TV buttons keep sending their command while held. */
  var held=null, n=0;
  function rep(){
    if(!held) return;
    var cmd=held;
    fetch('/tv?cmd='+cmd).then(r=>r.text()).then(t=>{show('TV '+cmd+' '+t+' x'+(++n));}).finally(rep);
  }
  document.querySelectorAll('.hold').forEach(function(b){
    b.onpointerdown=function(){ if(!held){ held=b.dataset.cmd; n=0; rep(); } };
    b.onpointerup=b.onpointerleave=b.onpointercancel=function(){ held=null; };
    b.oncontextmenu=function(){ return false; };
  });
  for(var t=17;t<=30;t++){ document.getElementById('at').innerHTML+='<option'+(t==24?' selected':'')+'>'+t+'</option>'; }
  function ac(p){
    var v=function(id){return document.getElementById(id).value;};
    fetch('/ac?power='+p+'&mode='+v('am')+'&temp='+v('at')+'&fan='+v('af')).then(r=>r.text()).then(t=>{show('AC '+t);});
  }
</script>
)html";

static const char SETTINGS_HTML[] PROGMEM = R"html(
  <h3>TV code</h3>
  <select id='tv'><option>Loading...</option></select><br><br>
  <div class="row">
    <button type="button" id="ta" onclick="tvAuto()">Scan</button>
    <button type="button" onclick="tvStep(-1)">Prev</button>
    <button type="button" onclick="tvStep(0)">Test</button>
    <button type="button" onclick="tvStep(1)">Next</button>
  </div>
  <p>Scan sends the next code every 2s. Stop when the TV reacts, then use Prev/Test to confirm.</p>
  <p id='tres'></p>
  <hr>
  <h3>AC protocol</h3>
  <select id='ac'><option>Loading...</option></select><br><br>
  <div class="row">
    <button type="button" id="aa" onclick="acAuto()">Scan</button>
    <button type="button" onclick="acSend(1)">Test ON</button>
    <button type="button" onclick="acSend(0)">Test OFF</button>
  </div>
  <p>Scan: with the AC running, sends OFF in the next protocol every 3s. Stop when the AC turns off.</p>
  <p id='ares'></p>
  <hr>
  <button type="button" onclick="save()">Save TV code and AC protocol</button>
  <p id='sres'></p>
  <hr>
  <h3>Test any code</h3>
  <input id='pr' placeholder='protocol (NEC, RC5, RC6, SAMSUNG...)'><br>
  <input id='cd' placeholder='code (e.g. 0x20DF10EF)'><br>
  <input id='bt' placeholder='bits (empty = protocol default)'><br><br>
  <button type="button" onclick="tst()">Send</button>
  <p id='xres'></p>
  <hr>
  <h3>System</h3>
  <div class="row">
    <button type="button" onclick="location.href='/selectap';">Configure WiFi</button>
    <button type="button" onclick="get('/update','sres')">Check for update</button>
  </div>
<script>
  function $(id){ return document.getElementById(id); }
  function get(url,out){
    return fetch(url).then(r=>r.text().then(t=>{ $(out).innerText=t; return r.ok; }));
  }
  fetch('/tvcodes').then(r=>r.text()).then(t=>{
    var h=''; t.trim().split('\n').forEach(function(d,i){ h+='<option value='+i+(i==tvCur?' selected':'')+'>'+i+': '+d+'</option>'; });
    $('tv').innerHTML=h;
  });
  fetch('/acprotocols').then(r=>r.text()).then(t=>{
    var h=''; t.split('|').forEach(function(p){ h+='<option'+(p==acCur?' selected':'')+'>'+p+'</option>'; });
    $('ac').innerHTML=h;
  });
  /* TV code scan moves through the dropdown. */
  var tvScanning=false;
  function tvSend(i){
    var s=$('tv');
    if(i<0 || i>=s.length) return Promise.resolve(false);
    s.selectedIndex=i;
    return get('/scan?i='+i,'tres');
  }
  function tvStep(d){
    tvScanning=false; $('ta').innerText='Scan';
    tvSend($('tv').selectedIndex+d);
  }
  function tvAuto(){
    tvScanning=!tvScanning;
    $('ta').innerText=tvScanning?'Stop':'Scan';
    if(tvScanning) tvSend($('tv').selectedIndex).then(tvNext);
  }
  function tvNext(ok){
    if(!ok){ tvScanning=false; $('ta').innerText='Scan'; }
    if(!tvScanning) return;
    setTimeout(function(){
      if(tvScanning) tvSend($('tv').selectedIndex+1).then(tvNext);
    },2000);
  }
  /* AC protocol scan moves through the dropdown. */
  var acScanning=false;
  function acSend(p){
    var s=$('ac');
    return fetch('/ac?protocol='+s.value+'&power='+p+'&mode=cool&temp=24&fan=auto').then(r=>r.text()).then(t=>{
      $('ares').innerText=s.selectedIndex+'/'+(s.length-1)+': '+s.value+' '+t;
    });
  }
  function acAuto(){
    acScanning=!acScanning;
    $('aa').innerText=acScanning?'Stop':'Scan';
    if(acScanning) acSend(0).then(acNext);
  }
  function acNext(){
    setTimeout(function(){
      var s=$('ac');
      if(!acScanning) return;
      if(s.selectedIndex>=s.length-1){ acAuto(); return; }
      s.selectedIndex++;
      acSend(0).then(acNext);
    },3000);
  }
  function save(){
    get('/save?tv='+$('tv').value+'&ac='+$('ac').value,'sres');
  }
  function tst(){
    var v=function(id){return encodeURIComponent($(id).value);};
    get('/send?protocol='+v('pr')+'&code='+v('cd')+'&bits='+v('bt'),'xres');
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
  sendPage(FPSTR(INDEX_HTML));
}

static void showSettings(void){
  String body = "<script>var tvCur=" + String(IR_getTvCode()) + ",acCur='" + IR_getAcProtocol() + "';</script>";
  body += FPSTR(SETTINGS_HTML);
  body += "<p>" + WIFIC_getStatus() + "<br>" + OTA_getStatus() + "</p>";
  sendPage(body);
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
  webServer.on("/settings", showSettings);
  webServer.on("/tv", tvCommand);
  webServer.on("/send", sendCode);
  webServer.on("/scan", scanCode);
  webServer.on("/tvcodes", tvCodes);
  webServer.on("/save", saveSettings);
  webServer.on("/ac", airCon);
  webServer.on("/acprotocols", acProtocols);
  webServer.on("/update", checkUpdate);
  webServer.on("/selectap", selectAP);
  webServer.on("/aplist", apList);
  webServer.on("/wifisave", saveWiFi);
  webServer.onNotFound(showStartPage);

  webServer.begin();
}
