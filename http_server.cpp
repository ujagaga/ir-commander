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
<html lang="en">
  <head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <meta name="theme-color" content="#152923">
    <title>IR Commander</title>
    <style>
      :root{color-scheme:light;--ink:#152923;--muted:#60716a;--line:#dce4dc;--green:#236349;--paper:#fffefa}
      *{box-sizing:border-box}body{margin:0;background:#eff2eb;color:var(--ink);font:16px/1.5 system-ui,-apple-system,sans-serif}
      .center_div{max-width:860px;margin:auto;padding:32px 24px 48px}.brand{display:flex;align-items:center;gap:12px;font-size:19px;font-weight:750;letter-spacing:-.5px}.brand-mark{display:grid;place-items:center;background:var(--ink);color:#e6f1b5;border-radius:13px;width:42px;height:42px;font-size:24px}.brand small{display:block;font-size:11px;letter-spacing:1.8px;color:var(--muted);font-weight:600;text-transform:uppercase}
      .nav{display:flex;gap:6px;background:#e2e8df;padding:5px;border-radius:14px;margin:26px 0 30px}.nav svg{width:24px;height:24px;display:block}.nav a{display:flex;align-items:center;justify-content:center;min-height:48px;flex:1;text-align:center;padding:10px;border-radius:10px;text-decoration:none;color:var(--muted);font-weight:650}.nav a[aria-current]{background:var(--paper);color:var(--ink);box-shadow:0 2px 5px #1529230a}
      h1{font-size:30px;line-height:1.2;letter-spacing:-1px;margin:0 0 8px}h3 label{font:inherit;color:inherit;margin:0}h2,h3{font-size:20px;letter-spacing:-.5px;margin:0 0 18px}p{color:var(--muted);font-size:14px} .intro{margin:0 0 26px}.eyebrow{font-size:11px;letter-spacing:1.6px;font-weight:700;text-transform:uppercase;color:var(--muted);margin-bottom:6px}
      .devices{display:grid;grid-template-columns:1fr 1fr;gap:20px}.card{background:var(--paper);border:1px solid var(--line);border-radius:24px;padding:24px;margin-bottom:20px}.devices .card{margin:0}.card-head{display:flex;align-items:center;justify-content:space-between}.device-icon{color:var(--green);font-size:25px}.hint{font-size:12px;color:var(--muted)}
      button{font:inherit;font-weight:650;cursor:pointer;min-height:48px;width:100%;border:1px solid var(--line);border-radius:12px;color:var(--ink);background:#edf1e9;padding:10px 12px;touch-action:manipulation;transition:background .15s,transform .15s}button:hover{background:#e1e8dc}button:active,.hold.pressed{transform:scale(.97);background:#d4dfce}button:disabled{opacity:.45;cursor:default}button.primary{background:var(--green);color:white;border-color:var(--green)}button.primary:hover{background:#194e38}button:focus-visible,a:focus-visible,input:focus-visible,select:focus-visible{outline:3px solid #b17a21;outline-offset:3px}
      .btn_pwr{display:flex;flex-direction:column;align-items:center;justify-content:center;width:124px;height:124px;border-radius:50%;margin:22px auto 26px;background:#f8e7df;border:1px solid #efd4c7;color:#a0442e;font-size:14px}.btn_pwr svg{display:block;width:40px;height:44px;margin-bottom:6px;pointer-events:none}.btn_pwr:hover{background:#f0d6c9}.hold{user-select:none;-webkit-user-select:none;touch-action:none}.row{display:flex;gap:8px}.row>*{flex:1;min-width:0}.volume button{height:64px}.volume .symbol{font-size:26px}.tv-hint{text-align:center;margin:18px 0 0}
      label{display:block;font-size:12px;font-weight:650;color:var(--muted);margin:14px 0 6px}input,select{font:inherit;width:100%;max-width:100%;padding:12px;border:1px solid var(--line);border-radius:10px;background:var(--paper);color:var(--ink);min-height:46px}input{margin-bottom:8px}.temperature{display:flex;align-items:center;gap:12px;margin:20px 0}.temperature button{width:48px;flex-shrink:0;font-size:24px}.temperature div{flex:1;text-align:center}.temperature output{font-size:56px;letter-spacing:-3px;font-weight:600;line-height:1.2}.temperature .unit{font-size:23px;color:var(--muted);vertical-align:top}.temperature small{display:block;font-size:11px;color:var(--muted)}.ac-actions{margin-top:24px}
      .status{min-height:48px;padding:14px 18px;border:1px solid var(--line);border-radius:14px;background:#e6edde;margin:20px 0 10px;font-size:13px;color:var(--ink)}.status.error{background:#fae7df;color:#933c28}.footnote{text-align:center;font-size:12px}hr{border:0;border-top:1px solid var(--line);margin:24px 0}a{color:var(--green)}[id$="res"]{overflow-wrap:anywhere}#vm a{display:inline-block;padding:10px 0}summary{cursor:pointer;font-weight:650}
      @media(max-width:600px){.center_div{padding:20px 16px 32px}.devices{grid-template-columns:1fr}.card{padding:20px;border-radius:20px}.nav{margin:22px 0}h1{font-size:27px}.btn_pwr{margin:12px auto 20px;width:110px;height:110px}.temperature{margin:16px 0}.row button{padding:10px 8px}}
      @media(prefers-reduced-motion:reduce){button{transition:none}}
    </style>
  </head>
  <body><div class="contain"><div class="center_div">
  <header class="brand"><span class="brand-mark" aria-hidden="true">&#x2301;</span><div>IR Commander</div></header>
  <nav class="nav" aria-label="Main navigation"><a href="/" aria-label="Home" title="Home"><svg aria-hidden="true" focusable="false" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 512 512"><!--! Font Awesome Free 7.3.0 by @fontawesome - https://fontawesome.com License - https://fontawesome.com/license/free (Icons: CC BY 4.0, Fonts: SIL OFL 1.1, Code: MIT License) Copyright 2026 Fonticons, Inc. --><path fill="currentColor" d="M277.8 8.6c-12.3-11.4-31.3-11.4-43.5 0l-224 208c-9.6 9-12.8 22.9-8 35.1S18.8 272 32 272l16 0 0 176c0 35.3 28.7 64 64 64l288 0c35.3 0 64-28.7 64-64l0-176 16 0c13.2 0 25-8.1 29.8-20.3s1.6-26.2-8-35.1l-224-208zM240 320l32 0c26.5 0 48 21.5 48 48l0 96-128 0 0-96c0-26.5 21.5-48 48-48z"/></svg></a><a href="/settings" aria-label="Settings" title="Settings"><svg aria-hidden="true" focusable="false" xmlns="http://www.w3.org/2000/svg" viewBox="-16 -16 544 544"><!--! Font Awesome Free 7.3.0 by @fontawesome - https://fontawesome.com License - https://fontawesome.com/license/free (Icons: CC BY 4.0, Fonts: SIL OFL 1.1, Code: MIT License) Copyright 2026 Fonticons, Inc. --><path fill="currentColor" d="M195.1 9.5C198.1-5.3 211.2-16 226.4-16l59.8 0c15.2 0 28.3 10.7 31.3 25.5L332 79.5c14.1 6 27.3 13.7 39.3 22.8l67.8-22.5c14.4-4.8 30.2 1.2 37.8 14.4l29.9 51.8c7.6 13.2 4.9 29.8-6.5 39.9L447 233.3c.9 7.4 1.3 15 1.3 22.7s-.5 15.3-1.3 22.7l53.4 47.5c11.4 10.1 14 26.8 6.5 39.9l-29.9 51.8c-7.6 13.1-23.4 19.2-37.8 14.4l-67.8-22.5c-12.1 9.1-25.3 16.7-39.3 22.8l-14.4 69.9c-3.1 14.9-16.2 25.5-31.3 25.5l-59.8 0c-15.2 0-28.3-10.7-31.3-25.5l-14.4-69.9c-14.1-6-27.2-13.7-39.3-22.8L73.5 432.3c-14.4 4.8-30.2-1.2-37.8-14.4L5.8 366.1c-7.6-13.2-4.9-29.8 6.5-39.9l53.4-47.5c-.9-7.4-1.3-15-1.3-22.7s.5-15.3 1.3-22.7L12.3 185.8c-11.4-10.1-14-26.8-6.5-39.9L35.7 94.1c7.6-13.2 23.4-19.2 37.8-14.4l67.8 22.5c12.1-9.1 25.3-16.7 39.3-22.8L195.1 9.5zM256.3 336a80 80 0 1 0 -.6-160 80 80 0 1 0 .6 160z"/></svg></a></nav>
  <script>document.querySelectorAll('.nav a').forEach(function(a){if(a.getAttribute('href')===(location.pathname==='/'?'/':'/settings'))a.setAttribute('aria-current','page');});</script>
)html";

static const char HTML_END[] PROGMEM = "</div></div></body></html>";

static const char INDEX_HTML[] PROGMEM = R"html(
  <main class="devices">
    <section class="card" aria-labelledby="tv-title">
      <div class="card-head"><div><h2 id="tv-title">Television</h2></div><span class="device-icon" aria-hidden="true">&#9635;</span></div>
      <button class="btn_pwr hold" type="button" data-cmd="power" aria-label="Toggle TV power"><svg aria-hidden="true" focusable="false" xmlns="http://www.w3.org/2000/svg" viewBox="-16 -40 544 560"><!--! Font Awesome Free 7.3.0 by @fontawesome - https://fontawesome.com License - https://fontawesome.com/license/free (Icons: CC BY 4.0, Fonts: SIL OFL 1.1, Code: MIT License) Copyright 2026 Fonticons, Inc. --><path fill="currentColor" d="M288 0c0-17.7-14.3-32-32-32S224-17.7 224 0l0 256c0 17.7 14.3 32 32 32s32-14.3 32-32L288 0zM146.3 98.4c14.5-10.1 18-30.1 7.9-44.6s-30.1-18-44.6-7.9C43.4 92.1 0 169 0 256 0 397.4 114.6 512 256 512S512 397.4 512 256c0-87-43.4-163.9-109.7-210.1-14.5-10.1-34.4-6.6-44.6 7.9s-6.6 34.4 7.9 44.6c49.8 34.8 82.3 92.4 82.3 157.6 0 106-86 192-192 192S64 362 64 256c0-65.2 32.5-122.9 82.3-157.6z"/></svg>Power</button>
      <label>Volume</label>
      <div class="row volume">
        <button class="hold symbol" type="button" data-cmd="volume_down" aria-label="Volume down">&minus;</button>
        <button class="hold" type="button" data-cmd="mute">Mute</button>
        <button class="hold symbol" type="button" data-cmd="volume_up" aria-label="Volume up">+</button>
      </div>
      <p class="tv-hint hint">Tap to send &middot; Hold to repeat</p>
    </section>
    <section class="card" aria-labelledby="ac-title">
      <div class="card-head"><div><h2 id="ac-title">Air conditioner</h2></div><span class="device-icon" aria-hidden="true">&#10052;</span></div>
      <div class="temperature">
        <button type="button" id="less" onclick="temperature(-1)" aria-label="Decrease temperature">&minus;</button>
        <div><output id="at" aria-live="polite" aria-label="Target temperature">24</output><span class="unit">&deg;C</span><small>Target temperature</small></div>
        <button type="button" id="more" onclick="temperature(1)" aria-label="Increase temperature">+</button>
      </div>
      <div class="row"><div><label for="am">Mode</label><select id="am"><option value="cool">Cool</option><option value="heat">Heat</option><option value="auto">Auto</option><option value="dry">Dry</option><option value="fan">Fan only</option></select></div>
      <div><label for="af">Fan speed</label><select id="af"><option value="auto">Auto</option><option value="low">Low</option><option value="medium">Medium</option><option value="high">High</option></select></div></div>
      <div class="row ac-actions"><button class="primary" type="button" onclick="ac(1)">Turn on / Apply</button><button type="button" onclick="ac(0)">Turn off</button></div>
    </section>
  </main>
  <p id="res" class="status" role="status" aria-live="polite">Ready when you are. Choose a control to send a command.</p>
  <p class="footnote">Infrared sends commands; device power and room temperature aren’t reported back.</p>
<script>
  function show(t,error){var el=document.getElementById('res');el.textContent=t;el.classList.toggle('error',!!error);}
  async function send(url,message){
    var controller=new AbortController(),timeout=setTimeout(()=>controller.abort(),8000);
    try{var r=await fetch(url,{signal:controller.signal});var t=await r.text();if(!r.ok)throw new Error(t);show(message);return true;}
    catch(e){show('Could not send command. '+(e.name==='AbortError'?'Device timed out.':e.message)+' Try again.',true);return false;}finally{clearTimeout(timeout);}
  }
  var held=null,timer=null,busy=false;
  var names={power:'TV power command sent.',volume_up:'TV volume up sent.',volume_down:'TV volume down sent.',mute:'TV mute command sent.'};
  function stop(){clearTimeout(timer);if(held)held.classList.remove('pressed');held=null;}
  async function rep(){
    var b=held;if(!b||busy)return;busy=true;
    var ok=await send('/tv?cmd='+b.dataset.cmd,names[b.dataset.cmd]);busy=false;
    if(!ok){stop();return;}if(held===b)timer=setTimeout(rep,120);else if(held)rep();
  }
  document.querySelectorAll('.hold').forEach(function(b){
    b.onpointerdown=function(e){if(e.button!==0)return;e.preventDefault();stop();held=b;b.classList.add('pressed');b.setPointerCapture(e.pointerId);rep();};
    b.onpointerup=b.onpointercancel=b.onlostpointercapture=stop;
    b.onclick=function(e){if(e.detail===0&&!busy)send('/tv?cmd='+b.dataset.cmd,names[b.dataset.cmd]);};
    b.oncontextmenu=function(){return false;};
  });
  window.addEventListener('blur',stop);document.addEventListener('visibilitychange',function(){if(document.hidden)stop();});
  function temperature(d){var el=document.getElementById('at');var t=Math.max(17,Math.min(30,Number(el.value)+d));el.value=t;document.getElementById('less').disabled=t===17;document.getElementById('more').disabled=t===30;}
  async function ac(p){
    var v=function(id){return document.getElementById(id).value;};
    var buttons=document.querySelectorAll('.ac-actions button');buttons.forEach(b=>b.disabled=true);
    await send('/ac?power='+p+'&mode='+v('am')+'&temp='+v('at')+'&fan='+v('af'),p?'AC settings sent: '+v('am')+', '+v('at')+' °C, fan '+v('af')+'.':'AC off command sent.');
    buttons.forEach(b=>b.disabled=false);
  }
</script>
)html";

static const char SETTINGS_HTML[] PROGMEM = R"html(
  <section class="card"><h3><label for="tv">TV power code</label></h3>
  <select id='tv'><option>Loading...</option></select><br><br>
  <div class="row">
    <button type="button" id="ta" onclick="tvAuto()">Scan</button>
    <button type="button" onclick="tvStep(-1)">Prev</button>
    <button type="button" onclick="tvStep(0)">Test</button>
    <button type="button" onclick="tvStep(1)">Next</button>
  </div>
  <p>Scan sends the next code every 2s. Stop when the TV reacts, then use Prev/Test to confirm.</p>
  <p id='tres'></p>
  </section><section class="card">
  <h3><label for="ac">AC protocol</label></h3>
  <select id='ac'><option>Loading...</option></select><br><br>
  <div class="row">
    <button type="button" id="aa" onclick="acAuto()">Scan</button>
    <button type="button" onclick="acSend(1)">Test ON</button>
    <button type="button" onclick="acSend(0)">Test OFF</button>
  </div>
  <p>Scan: with the AC running, sends OFF in the next protocol every 3s. Stop when the AC turns off.</p>
  <p id='ares'></p>
  </section>
  <button class="primary" type="button" onclick="save()">Save TV code and AC protocol</button>
  <p id='sres'></p>
  <details class="card"><summary>Advanced: test any code</summary>
  <label for="pr">Protocol</label>
  <input id='pr' placeholder='protocol (NEC, RC5, RC6, SAMSUNG...)'><br>
  <label for='cd'>Hex code</label><input id='cd' placeholder='code (e.g. 0x20DF10EF)'><br>
  <label for='bt'>Bit count (optional)</label><input id='bt' placeholder='bits (empty = protocol default)'><br><br>
  <button type="button" onclick="tst()">Send</button>
  <p id='xres'></p>
  </details><section class="card">
  <h3>System</h3>
  <form action="/hostname" method="post">
    <label for="hostname">Hostname</label>
    <input id="hostname" name="hostname" maxlength="31" pattern="[A-Za-z0-9]([A-Za-z0-9-]{0,29}[A-Za-z0-9])?" autocapitalize="none" spellcheck="false" aria-describedby="hostname-help">
    <p id="hostname-help">Use letters, numbers and hyphens, without .local. Leave blank to restore the default. Saving restarts the device.</p>
    <button class="primary" type="submit">Save hostname and restart</button>
  </form><hr>
  <div class="row">
    <button type="button" onclick="location.href='/selectap';">Configure WiFi</button>
    <button type="button" onclick="get('/update','sres')">Check for update</button>
  </div>
<script>
  function $(id){ return document.getElementById(id); }
  $('hostname').value=hostCur;
  function get(url,out){
    return fetch(url).then(r=>r.text().then(t=>{ $(out).innerText=t; return r.ok; })).catch(()=>{$(out).innerText='Connection failed. Please try again.';return false;});
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
  <h1>WiFi setup</h1><p>Choose your network to connect IR Commander.</p><h3 id='ttl'>Available networks</h3>
  <div id='vm'>Please wait...</div>
  <form method='get' action='wifisave'>
    <button type='button' onclick='refresh();'>Rescan</button><br/><br/>
    <label for='s'>Network name (blank for access point mode)</label><input id='s' name='s' maxlength='32' placeholder='SSID (Leave blank for AP mode)'><br>
    <label for='p'>WiFi password</label><input id='p' name='p' type='password' maxlength='63' autocomplete='new-password' placeholder='Password'><br>
    <br><button class='primary' type='submit'>Save and connect</button>
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
  body += "<script>var hostCur='" + WIFIC_getHostname() + "';</script>";
  body += FPSTR(SETTINGS_HTML);
  body += "<p>" + WIFIC_getStatus() + "<br>" + OTA_getStatus() + "</p></section>";
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
    msg += "<br>Then open <a href='http://" + WIFIC_getHostname() + ".local'>http://" + WIFIC_getHostname() + ".local</a> or the IP your router assigned.";
  }else{
    msg = "Saving settings and switching to AP mode only.";
  }
  sendPage("<h1>Connection Status</h1><p>" + msg + "</p>");

  /* Let the response go out, then restart to apply.
   Note: ESP.restart may hang the first time after serial programming. Reset once manually after programming. */
  delay(1000);
  ESP.restart();
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
    sendPage("<h1>Invalid hostname</h1><p>Use 1–31 letters, numbers or hyphens. Start and end with a letter or number.</p><a href='/settings'>Back to Settings</a>");
    return;
  }
  if(!WIFIC_saveHostname(name)){
    webServer.send(500, "text/plain", "Could not save hostname. Please try again.");
    return;
  }
  if(name.isEmpty()) name = HOSTNAME;
  sendPage("<h1>Hostname saved</h1><p>Restarting. Reconnect at <a href='http://" + name + ".local'>http://" + name + ".local</a>, or use the device IP address.</p>");
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
  webServer.on("/hostname", HTTP_POST, saveHostname);
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
