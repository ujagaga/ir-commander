#pragma once
#include <pgmspace.h>

// Static UI assets stay in flash. HTTP routes and streaming live in http_server.cpp.

static const char HTML_BEGIN[] PROGMEM = R"html(
<!DOCTYPE HTML>
<html lang="en">
  <head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <meta name="theme-color" content="#152923">

)html";

static const char HTML_LAYOUT[] PROGMEM = R"html(
    <style>
      :root{color-scheme:light;--ink:#152923;--muted:#60716a;--line:#dce4dc;--green:#236349;--paper:#fffefa}
      *{box-sizing:border-box}body{margin:0;background:#eff2eb;color:var(--ink);font:16px/1.5 system-ui,-apple-system,sans-serif}
      .center_div{max-width:860px;margin:auto;padding:32px 24px 48px}.brand{display:flex;align-items:center;gap:12px;font-size:19px;font-weight:750;letter-spacing:-.5px}.brand-mark{display:grid;place-items:center;background:var(--ink);color:#e6f1b5;border-radius:13px;width:42px;height:42px;font-size:24px}.brand small{display:block;font-size:11px;letter-spacing:1.8px;color:var(--muted);font-weight:600;text-transform:uppercase}
      .nav{display:flex;gap:6px;background:#e2e8df;padding:5px;border-radius:14px;margin:26px 0 30px}.nav img{width:24px;height:24px;display:block}.nav a{display:flex;align-items:center;justify-content:center;min-height:48px;flex:1;text-align:center;padding:10px;border-radius:10px;text-decoration:none;color:var(--muted);font-weight:650}.nav a[aria-current]{background:var(--paper);color:var(--ink);box-shadow:0 2px 5px #1529230a}
      h1{font-size:30px;line-height:1.2;letter-spacing:-1px;margin:0 0 8px}h3 label{font:inherit;color:inherit;margin:0}h2,h3{font-size:20px;letter-spacing:-.5px;margin:0 0 18px}p{color:var(--muted);font-size:14px} .intro{margin:0 0 26px}.eyebrow{font-size:11px;letter-spacing:1.6px;font-weight:700;text-transform:uppercase;color:var(--muted);margin-bottom:6px}
      .devices{display:grid;grid-template-columns:1fr 1fr;gap:20px}.card{background:var(--paper);border:1px solid var(--line);border-radius:24px;padding:24px;margin-bottom:20px}.devices .card{margin:0}.card-head{display:flex;align-items:center;justify-content:space-between}.device-icon{color:var(--green);font-size:25px}.device-icon img{display:block;width:28px;height:25px}.hint{font-size:12px;color:var(--muted)}
      button{font:inherit;font-weight:650;cursor:pointer;min-height:48px;width:100%;border:1px solid var(--line);border-radius:12px;color:var(--ink);background:#edf1e9;padding:10px 12px;touch-action:manipulation;transition:background .15s,transform .15s}button:hover{background:#e1e8dc}button:active,.hold.pressed{transform:scale(.97);background:#d4dfce}button:disabled{opacity:.45;cursor:default}button.primary{background:var(--green);color:white;border-color:var(--green)}button.primary:hover{background:#194e38}button:focus-visible,a:focus-visible,input:focus-visible,select:focus-visible{outline:3px solid #b17a21;outline-offset:3px}
      .btn_pwr{display:flex;flex-direction:column;align-items:center;justify-content:center;width:124px;height:124px;border-radius:50%;margin:22px auto 26px;background:#f8e7df;border:1px solid #efd4c7;color:#a0442e;font-size:14px}.btn_pwr img{display:block;width:40px;height:44px;margin-bottom:6px;pointer-events:none}.btn_pwr:hover{background:#f0d6c9}.hold{user-select:none;-webkit-user-select:none;touch-action:none}.row{display:flex;gap:8px}.row>*{flex:1;min-width:0}.volume button{height:64px}.volume .symbol{font-size:26px}.tv-hint{text-align:center;margin:18px 0 0}
      label{display:block;font-size:12px;font-weight:650;color:var(--muted);margin:14px 0 6px}input,select{font:inherit;width:100%;max-width:100%;padding:12px;border:1px solid var(--line);border-radius:10px;background:var(--paper);color:var(--ink);min-height:46px}input{margin-bottom:8px}.temperature{display:flex;align-items:center;gap:12px;margin:20px 0}.temperature button{width:48px;flex-shrink:0;font-size:24px}.temperature div{flex:1;text-align:center}.temperature output{font-size:56px;letter-spacing:-3px;font-weight:600;line-height:1.2}.temperature .unit{font-size:23px;color:var(--muted);vertical-align:top}.temperature small{display:block;font-size:11px;color:var(--muted)}.temperature #room{font-size:15px}.ac-actions{margin-top:24px}.temperature div{position:relative}.ac-on{position:absolute;left:50%;top:50%;width:130px;height:130px;margin:-65px 0 0 -65px;box-sizing:border-box;border:3px solid var(--line);border-top-color:var(--green);border-radius:50%;animation:spin 1.2s linear infinite;pointer-events:none}.ac-on[hidden]{display:none}@keyframes spin{to{transform:rotate(360deg)}}
      .status{min-height:48px;padding:14px 18px;border:1px solid var(--line);border-radius:14px;background:#e6edde;margin:20px 0 10px;font-size:13px;color:var(--ink)}.status.error{background:#fae7df;color:#933c28}.footnote{text-align:center;font-size:12px}hr{border:0;border-top:1px solid var(--line);margin:24px 0}a{color:var(--green)}[id$="res"]{overflow-wrap:anywhere}#vm a{display:inline-block;padding:10px 0}summary{cursor:pointer;font-weight:650}
      .visibility-option{display:flex;align-items:center;gap:12px;font-size:16px;color:var(--ink);min-height:48px}.visibility-option input{width:22px;height:22px;min-height:0;margin:0;accent-color:var(--green)}
      @media(max-width:600px){.center_div{padding:20px 16px 32px}.devices{grid-template-columns:1fr}.card{padding:20px;border-radius:20px}.nav{margin:22px 0}h1{font-size:27px}.btn_pwr{margin:12px auto 20px;width:110px;height:110px}.temperature{margin:16px 0}.row button{padding:10px 8px}}
      @media(prefers-reduced-motion:reduce){button{transition:none}.ac-on{animation-duration:4s}}
    </style>
  </head>
  <body><div class="contain"><div class="center_div">
  <header class="brand"><span class="brand-mark" aria-hidden="true">&#x2301;</span><div>IR Commander</div></header>
  <nav class="nav" aria-label="Main navigation"><a href="/" aria-label="Home" title="Home"><img src="/icons/home-e4ff8833c2f2.svg" width="24" height="24" alt="" draggable="false"></a><a href="/settings" aria-label="Settings" title="Settings"><img src="/icons/settings-08c94bb532ed.svg" width="24" height="24" alt="" draggable="false"></a></nav>
  <script>document.querySelectorAll('.nav a').forEach(function(a){if(a.getAttribute('href')===(location.pathname==='/'?'/':'/settings'))a.setAttribute('aria-current','page');});</script>
)html";

static const char HTML_END[] PROGMEM = "</div></div></body></html>";

static const char INDEX_HTML[] PROGMEM = R"html(
  <main class="devices">
    <section class="card" aria-labelledby="tv-title">
      <div class="card-head"><div><h2 id="tv-title">Television</h2></div><span class="device-icon" aria-hidden="true"><img src="/icons/television-724e5ba57c4f.svg" width="28" height="25" alt="" draggable="false"></span></div>
      <button class="btn_pwr hold" type="button" data-cmd="power" aria-label="Toggle TV power"><img src="/icons/power-f1123170991d.svg" width="40" height="44" alt="" draggable="false">Power</button>
      <label>Volume</label>
      <div class="row volume">
        <button class="hold symbol" type="button" data-cmd="volume_down" aria-label="Volume down">&minus;</button>
        <button class="hold" type="button" data-cmd="mute">Mute</button>
        <button class="hold symbol" type="button" data-cmd="volume_up" aria-label="Volume up">+</button>
      </div>
      <p class="tv-hint hint">Tap to send &middot; Hold to repeat</p>
    </section>
    <section class="card" aria-labelledby="ac-title">
      <div class="card-head"><div><h2 id="ac-title">Air conditioner</h2></div><span id="ac-mode-icon" class="device-icon" aria-hidden="true"><img src="/icons/cool-0da41409ccc3.svg" width="28" height="25" alt="" draggable="false"></span></div>
      <div class="temperature">
        <button type="button" id="less" onclick="temperature(-1)" aria-label="Decrease temperature">&minus;</button>
        <div><span id="ac-on" class="ac-on" role="img" aria-label="AC on" hidden></span><output id="at" aria-live="polite" aria-label="Target temperature">24</output><span class="unit">&deg;C</span><small id="room" hidden></small></div>
        <button type="button" id="more" onclick="temperature(1)" aria-label="Increase temperature">+</button>
      </div>
      <div class="row"><div><label for="am">Mode</label><select id="am"><option value="cool">Cool</option><option value="heat">Heat</option><option value="auto">Auto</option><option value="dry">Dry</option><option value="fan">Fan only</option></select></div>
      <div><label for="af">Fan speed</label><select id="af"><option value="auto">Auto</option><option value="low">Low</option><option value="medium">Medium</option><option value="high">High</option></select></div></div>
      <div class="row ac-actions"><button type="button" onclick="ac(1)">On / Apply</button><button type="button" onclick="ac(0)">Off</button></div>
    </section>
  </main>
<script>
  var modeIcons={"cool": "/icons/cool-0da41409ccc3.svg", "heat": "/icons/heat-721f0eb2cafc.svg", "auto": "/icons/auto-f2e9892497ab.svg", "dry": "/icons/dry-8dff9712e689.svg", "fan": "/icons/fan-1354feac9110.svg"};
  function updateModeIcon(){
    var url=modeIcons[document.getElementById('am').value] || modeIcons.cool;
    var icon=document.querySelector('#ac-mode-icon img');
    if(icon.getAttribute('src')!==url) icon.setAttribute('src',url);
  }
  document.getElementById('am').addEventListener('change',updateModeIcon);
  window.addEventListener('pageshow',updateModeIcon);
  updateModeIcon();
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
  function acSet(m,t,f,on){document.getElementById('ac-on').hidden=!on;document.getElementById('am').value=m;document.getElementById('af').value=f;document.getElementById('at').value=t;temperature(0);updateModeIcon();}
  function room(t){var el=document.getElementById('room');el.textContent='Room '+(t||'–')+' °C';el.hidden=false;}
  function roomStart(t){room(t);setInterval(function(){fetch('/temperature').then(r=>r.text()).then(room).catch(function(){});},30000);}
  async function ac(p){
    var v=function(id){return document.getElementById(id).value;};
    var buttons=document.querySelectorAll('.ac-actions button');buttons.forEach(b=>b.disabled=true);
    if(await send('/ac?power='+p+'&mode='+v('am')+'&temp='+v('at')+'&fan='+v('af'),p?'AC settings sent: '+v('am')+', '+v('at')+' °C, fan '+v('af')+'.':'AC off command sent.'))document.getElementById('ac-on').hidden=!p;
    buttons.forEach(b=>b.disabled=false);
  }
</script>
)html";

static const char SETTINGS_HTML[] PROGMEM = R"html(
  <section class="card">
    <h3>Home page controls</h3>
    <form id="visibility-form">
      <label class="visibility-option"><input type="checkbox" id="show-tv">Show TV controls</label>
      <label class="visibility-option"><input type="checkbox" id="show-ac">Show AC controls</label>
      <p>Choose which controls appear on Home for everyone using this device.</p>
      <button class="primary" type="submit">Save home page controls</button>
      <p id="visibility-result" role="status" aria-live="polite"></p>
    </form>
  </section>
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
  $('show-tv').checked=!!(visibleControls & 1);
  $('show-ac').checked=!!(visibleControls & 2);
  $('visibility-form').onsubmit=async function(e){
    e.preventDefault();
    var button=this.querySelector('button');button.disabled=true;
    try{
      var r=await fetch('/visibility',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'tv='+($('show-tv').checked?'1':'0')+'&ac='+($('show-ac').checked?'1':'0')});
      var t=await r.text();$('visibility-result').textContent=r.ok?'Saved. Open Home to see your controls.':t;
    }catch(e){$('visibility-result').textContent='Could not save. Check your connection and try again.';}
    finally{button.disabled=false;}
  };
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

// Small fragments used around dynamic settings and status values.
static const char UI_TITLE_OPEN[] PROGMEM = "<title>";
static const char UI_TITLE_CLOSE[] PROGMEM = "</title>";
static const char UI_STYLE_OPEN[] PROGMEM = "<style>";
static const char UI_STYLE_CLOSE[] PROGMEM = "</style>";
static const char UI_HIDE_TV[] PROGMEM = "[aria-labelledby='tv-title']{display:none}";
static const char UI_HIDE_AC[] PROGMEM = "[aria-labelledby='ac-title']{display:none}";
static const char UI_SINGLE_COLUMN[] PROGMEM = ".devices{grid-template-columns:1fr}";
static const char UI_HIDE_FEEDBACK[] PROGMEM = "#res,.footnote{display:none}";
static const char UI_EMPTY_HOME[] PROGMEM = "<section class='card'><h2>No controls selected</h2><p>Choose the controls you want to see in <a href='/settings'>Settings</a>.</p></section>";
static const char UI_TV_STATE[] PROGMEM = "<script>var tvCur=";
static const char UI_AC_STATE[] PROGMEM = ",acCur='";
static const char UI_STRING_SCRIPT_END[] PROGMEM = "';</script>";
static const char UI_AC_SET[] PROGMEM = "<script>acSet('";
static const char UI_QUOTE_COMMA[] PROGMEM = "',";
static const char UI_COMMA_QUOTE[] PROGMEM = ",'";
static const char UI_CALL_END[] PROGMEM = "',";
static const char UI_SCRIPT_CALL_END[] PROGMEM = ");</script>";
static const char UI_ROOM_START[] PROGMEM = "<script>roomStart('";
static const char UI_CALL_END_QUOTE[] PROGMEM = "');</script>";
static const char UI_HOST_STATE[] PROGMEM = "<script>var hostCur='";
static const char UI_VISIBILITY_STATE[] PROGMEM = "<script>var visibleControls=";
static const char UI_SCRIPT_END[] PROGMEM = ";</script>";
static const char UI_PARAGRAPH_OPEN[] PROGMEM = "<p>";
static const char UI_BREAK[] PROGMEM = "<br>";
static const char UI_SYSTEM_END[] PROGMEM = "</p></section>";
static const char UI_PARAGRAPH_CLOSE[] PROGMEM = "</p>";
static const char UI_WIFI_INVALID[] PROGMEM = "<p>Sorry, this module can only remember SSID up to 32 and a PASSWORD up to 63 bytes long.</p>";
static const char UI_WIFI_CONNECTING[] PROGMEM = "Saving settings and connecting to SSID: ";
static const char UI_WIFI_LINK[] PROGMEM = "<br>Then open <a href='http://";
static const char UI_LOCAL_LINK_TEXT[] PROGMEM = ".local'>http://";
static const char UI_WIFI_LINK_END[] PROGMEM = ".local</a> or the IP your router assigned.";
static const char UI_WIFI_AP[] PROGMEM = "Saving settings and switching to AP mode only.";
static const char UI_CONNECTION_TITLE[] PROGMEM = "<h1>Connection Status</h1><p>";
static const char UI_HOST_INVALID[] PROGMEM = "<h1>Invalid hostname</h1><p>Use 1–31 letters, numbers or hyphens. Start and end with a letter or number.</p><a href='/settings'>Back to Settings</a>";
static const char UI_HOST_SAVED[] PROGMEM = "<h1>Hostname saved</h1><p>Restarting. Reconnect at <a href='http://";
static const char UI_HOST_LINK_END[] PROGMEM = ".local</a>, or use the device IP address.</p>";

// SVG URLs include a content hash; update the hash and page references after edits.

static const char ICON_HOME[] PROGMEM = R"svg(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 512 512"><!--! Font Awesome Free 7.3.0 by @fontawesome - https://fontawesome.com License - https://fontawesome.com/license/free (Icons: CC BY 4.0, Fonts: SIL OFL 1.1, Code: MIT License) Copyright 2026 Fonticons, Inc. --><path fill="#152923" d="M277.8 8.6c-12.3-11.4-31.3-11.4-43.5 0l-224 208c-9.6 9-12.8 22.9-8 35.1S18.8 272 32 272l16 0 0 176c0 35.3 28.7 64 64 64l288 0c35.3 0 64-28.7 64-64l0-176 16 0c13.2 0 25-8.1 29.8-20.3s1.6-26.2-8-35.1l-224-208zM240 320l32 0c26.5 0 48 21.5 48 48l0 96-128 0 0-96c0-26.5 21.5-48 48-48z"/></svg>)svg";

static const char ICON_SETTINGS[] PROGMEM = R"svg(<svg xmlns="http://www.w3.org/2000/svg" viewBox="-16 -16 544 544"><!--! Font Awesome Free 7.3.0 by @fontawesome - https://fontawesome.com License - https://fontawesome.com/license/free (Icons: CC BY 4.0, Fonts: SIL OFL 1.1, Code: MIT License) Copyright 2026 Fonticons, Inc. --><path fill="#152923" d="M195.1 9.5C198.1-5.3 211.2-16 226.4-16l59.8 0c15.2 0 28.3 10.7 31.3 25.5L332 79.5c14.1 6 27.3 13.7 39.3 22.8l67.8-22.5c14.4-4.8 30.2 1.2 37.8 14.4l29.9 51.8c7.6 13.2 4.9 29.8-6.5 39.9L447 233.3c.9 7.4 1.3 15 1.3 22.7s-.5 15.3-1.3 22.7l53.4 47.5c11.4 10.1 14 26.8 6.5 39.9l-29.9 51.8c-7.6 13.1-23.4 19.2-37.8 14.4l-67.8-22.5c-12.1 9.1-25.3 16.7-39.3 22.8l-14.4 69.9c-3.1 14.9-16.2 25.5-31.3 25.5l-59.8 0c-15.2 0-28.3-10.7-31.3-25.5l-14.4-69.9c-14.1-6-27.2-13.7-39.3-22.8L73.5 432.3c-14.4 4.8-30.2-1.2-37.8-14.4L5.8 366.1c-7.6-13.2-4.9-29.8 6.5-39.9l53.4-47.5c-.9-7.4-1.3-15-1.3-22.7s.5-15.3 1.3-22.7L12.3 185.8c-11.4-10.1-14-26.8-6.5-39.9L35.7 94.1c7.6-13.2 23.4-19.2 37.8-14.4l67.8 22.5c12.1-9.1 25.3-16.7 39.3-22.8L195.1 9.5zM256.3 336a80 80 0 1 0 -.6-160 80 80 0 1 0 .6 160z"/></svg>)svg";

static const char ICON_TELEVISION[] PROGMEM = R"svg(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 576 512"><!--! Font Awesome Free 7.3.0 by @fontawesome - https://fontawesome.com License - https://fontawesome.com/license/free (Icons: CC BY 4.0, Fonts: SIL OFL 1.1, Code: MIT License) Copyright 2026 Fonticons, Inc. --><path fill="#236349" d="M64 96l0 240 448 0 0-240-448 0zM0 96C0 60.7 28.7 32 64 32l448 0c35.3 0 64 28.7 64 64l0 240c0 35.3-28.7 64-64 64L64 400c-35.3 0-64-28.7-64-64L0 96zM160 448l256 0c17.7 0 32 14.3 32 32s-14.3 32-32 32l-256 0c-17.7 0-32-14.3-32-32s14.3-32 32-32z"/></svg>)svg";

static const char ICON_POWER[] PROGMEM = R"svg(<svg xmlns="http://www.w3.org/2000/svg" viewBox="-16 -40 544 560"><!--! Font Awesome Free 7.3.0 by @fontawesome - https://fontawesome.com License - https://fontawesome.com/license/free (Icons: CC BY 4.0, Fonts: SIL OFL 1.1, Code: MIT License) Copyright 2026 Fonticons, Inc. --><path fill="#a0442e" d="M288 0c0-17.7-14.3-32-32-32S224-17.7 224 0l0 256c0 17.7 14.3 32 32 32s32-14.3 32-32L288 0zM146.3 98.4c14.5-10.1 18-30.1 7.9-44.6s-30.1-18-44.6-7.9C43.4 92.1 0 169 0 256 0 397.4 114.6 512 256 512S512 397.4 512 256c0-87-43.4-163.9-109.7-210.1-14.5-10.1-34.4-6.6-44.6 7.9s-6.6 34.4 7.9 44.6c49.8 34.8 82.3 92.4 82.3 157.6 0 106-86 192-192 192S64 362 64 256c0-65.2 32.5-122.9 82.3-157.6z"/></svg>)svg";

static const char ICON_COOL[] PROGMEM = R"svg(<svg xmlns="http://www.w3.org/2000/svg" viewBox="-32 -40 576 592"><!--! Font Awesome Free 7.3.0 by @fontawesome - https://fontawesome.com License - https://fontawesome.com/license/free (Icons: CC BY 4.0, Fonts: SIL OFL 1.1, Code: MIT License) Copyright 2026 Fonticons, Inc. --><path fill="#236349" d="M288.2 0c0-17.7-14.3-32-32-32s-32 14.3-32 32l0 62.1-15-15c-9.4-9.4-24.6-9.4-33.9 0s-9.4 24.6 0 33.9l49 49 0 70.6-61.2-35.3-17.9-66.9c-3.4-12.8-16.6-20.4-29.4-17S95.3 98 98.7 110.8l5.5 20.5-53.7-31C35.2 91.5 15.6 96.7 6.8 112s-3.6 34.9 11.7 43.7l53.7 31-20.5 5.5c-12.8 3.4-20.4 16.6-17 29.4s16.6 20.4 29.4 17l66.9-17.9 61.2 35.3-61.2 35.3-66.9-17.9c-12.8-3.4-26 4.2-29.4 17s4.2 26 17 29.4l20.5 5.5-53.7 31C3.2 365.1-2 384.7 6.8 400s28.4 20.6 43.7 11.7l53.7-31-5.5 20.5c-3.4 12.8 4.2 26 17 29.4s26-4.2 29.4-17l17.9-66.9 61.2-35.3 0 70.6-49 49c-9.4 9.4-9.4 24.6 0 33.9s24.6 9.4 33.9 0l15-15 0 62.1c0 17.7 14.3 32 32 32s32-14.3 32-32l0-62.1 15 15c9.4 9.4 24.6 9.4 33.9 0s9.4-24.6 0-33.9l-49-49 0-70.6 61.2 35.3 17.9 66.9c3.4 12.8 16.6 20.4 29.4 17s20.4-16.6 17-29.4l-5.5-20.5 53.7 31c15.3 8.8 34.9 3.6 43.7-11.7s3.6-34.9-11.7-43.7l-53.7-31 20.5-5.5c12.8-3.4 20.4-16.6 17-29.4s-16.6-20.4-29.4-17l-66.9 17.9-61.2-35.3 61.2-35.3 66.9 17.9c12.8 3.4 26-4.2 29.4-17s-4.2-26-17-29.4l-20.5-5.5 53.7-31c15.3-8.8 20.6-28.4 11.7-43.7s-28.4-20.5-43.7-11.7l-53.7 31 5.5-20.5c3.4-12.8-4.2-26-17-29.4s-26 4.2-29.4 17l-17.9 66.9-61.2 35.3 0-70.6 49-49c9.4-9.4 9.4-24.6 0-33.9s-24.6-9.4-33.9 0l-15 15 0-62.1z"/></svg>)svg";

static const char ICON_HEAT[] PROGMEM = R"svg(<svg xmlns="http://www.w3.org/2000/svg" viewBox="-8 -40 592 592"><!--! Font Awesome Free 7.3.0 by @fontawesome - https://fontawesome.com License - https://fontawesome.com/license/free (Icons: CC BY 4.0, Fonts: SIL OFL 1.1, Code: MIT License) Copyright 2026 Fonticons, Inc. --><path fill="#236349" d="M288-32c8.4 0 16.3 4.4 20.6 11.7L364.1 72.3 468.9 46c8.2-2 16.9 .4 22.8 6.3S500 67 498 75.1l-26.3 104.7 92.7 55.5c7.2 4.3 11.7 12.2 11.7 20.6s-4.4 16.3-11.7 20.6L471.7 332.1 498 436.8c2 8.2-.4 16.9-6.3 22.8S477 468 468.9 466l-104.7-26.3-55.5 92.7c-4.3 7.2-12.2 11.7-20.6 11.7s-16.3-4.4-20.6-11.7L211.9 439.7 107.2 466c-8.2 2-16.8-.4-22.8-6.3S76 445 78 436.8l26.2-104.7-92.6-55.5C4.4 272.2 0 264.4 0 256s4.4-16.3 11.7-20.6L104.3 179.9 78 75.1c-2-8.2 .3-16.8 6.3-22.8S99 44 107.2 46l104.7 26.2 55.5-92.6 1.8-2.6c4.5-5.7 11.4-9.1 18.8-9.1zm0 144a144 144 0 1 0 0 288 144 144 0 1 0 0-288zm0 240a96 96 0 1 1 0-192 96 96 0 1 1 0 192z"/></svg>)svg";

static const char ICON_AUTO[] PROGMEM = R"svg(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 512 512"><!--! Font Awesome Free 7.3.0 by @fontawesome - https://fontawesome.com License - https://fontawesome.com/license/free (Icons: CC BY 4.0, Fonts: SIL OFL 1.1, Code: MIT License) Copyright 2026 Fonticons, Inc. --><path fill="#236349" d="M65.9 228.5c13.3-93 93.4-164.5 190.1-164.5 53 0 101 21.5 135.8 56.2 .2 .2 .4 .4 .6 .6l7.6 7.2-47.9 0c-17.7 0-32 14.3-32 32s14.3 32 32 32l128 0c17.7 0 32-14.3 32-32l0-128c0-17.7-14.3-32-32-32s-32 14.3-32 32l0 53.4-11.3-10.7C390.5 28.6 326.5 0 256 0 127 0 20.3 95.4 2.6 219.5 .1 237 12.2 253.2 29.7 255.7s33.7-9.7 36.2-27.1zm443.5 64c2.5-17.5-9.7-33.7-27.1-36.2s-33.7 9.7-36.2 27.1c-13.3 93-93.4 164.5-190.1 164.5-53 0-101-21.5-135.8-56.2-.2-.2-.4-.4-.6-.6l-7.6-7.2 47.9 0c17.7 0 32-14.3 32-32s-14.3-32-32-32L32 320c-8.5 0-16.7 3.4-22.7 9.5S-.1 343.7 0 352.3l1 127c.1 17.7 14.6 31.9 32.3 31.7S65.2 496.4 65 478.7l-.4-51.5 10.7 10.1c46.3 46.1 110.2 74.7 180.7 74.7 129 0 235.7-95.4 253.4-219.5z"/></svg>)svg";

static const char ICON_DRY[] PROGMEM = R"svg(<svg xmlns="http://www.w3.org/2000/svg" viewBox="-8 -24 400 544"><!--! Font Awesome Free 7.3.0 by @fontawesome - https://fontawesome.com License - https://fontawesome.com/license/free (Icons: CC BY 4.0, Fonts: SIL OFL 1.1, Code: MIT License) Copyright 2026 Fonticons, Inc. --><path fill="#236349" d="M192 512C86 512 0 426 0 320 0 228.8 130.2 45.9 166.6-3.5 172.5-11.5 181.8-16 191.8-16l.4 0c10 0 19.3 4.5 25.2 12.5 36.4 49.4 166.6 232.3 166.6 323.5 0 106-86 192-192 192zM112 312c0-13.3-10.7-24-24-24s-24 10.7-24 24c0 75.1 60.9 136 136 136 13.3 0 24-10.7 24-24s-10.7-24-24-24c-48.6 0-88-39.4-88-88z"/></svg>)svg";

static const char ICON_FAN[] PROGMEM = R"svg(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 512 512"><!--! Font Awesome Free 7.3.0 by @fontawesome - https://fontawesome.com License - https://fontawesome.com/license/free (Icons: CC BY 4.0, Fonts: SIL OFL 1.1, Code: MIT License) Copyright 2026 Fonticons, Inc. --><path fill="#236349" d="M160 144c0-79.5 64.5-144 144-144 8.8 0 16 7.2 16 16l0 152.2c15-5.3 31.2-8.2 48-8.2 79.5 0 144 64.5 144 144 0 8.8-7.2 16-16 16l-152.2 0c5.3 15 8.2 31.2 8.2 48 0 79.5-64.5 144-144 144-8.8 0-16-7.2-16-16l0-152.2c-15 5.3-31.2 8.2-48 8.2-79.5 0-144-64.5-144-144 0-8.8 7.2-16 16-16l152.2 0c-5.3-15-8.2-31.2-8.2-48zm96 144a32 32 0 1 0 0-64 32 32 0 1 0 0 64z"/></svg>)svg";

struct IconAsset { const char* url; PGM_P content; };
static const IconAsset ICON_ASSETS[] = {
  {"/icons/home-e4ff8833c2f2.svg", ICON_HOME},
  {"/icons/settings-08c94bb532ed.svg", ICON_SETTINGS},
  {"/icons/television-724e5ba57c4f.svg", ICON_TELEVISION},
  {"/icons/power-f1123170991d.svg", ICON_POWER},
  {"/icons/cool-0da41409ccc3.svg", ICON_COOL},
  {"/icons/heat-721f0eb2cafc.svg", ICON_HEAT},
  {"/icons/auto-f2e9892497ab.svg", ICON_AUTO},
  {"/icons/dry-8dff9712e689.svg", ICON_DRY},
  {"/icons/fan-1354feac9110.svg", ICON_FAN},
};
