#include "wifi-debug.h"

#if ENABLE_WIFI_DEBUG
#if !ENABLE_WEB_INTERFACE
#error "Set ENABLE_WEB_INTERFACE to 1 when using ENABLE_WIFI_DEBUG."
#endif

#include <ArduinoOTA.h>
#include <WebServer.h>

#include "blocks-page.h"
#include "devices.h"
#include "hw-page.h"
#include "commands.h"
#include "program.h"
#include "params.h"
#include "shared-globals.h"
#include "src/motorshield/motorshield.h"

DebugOutput Debug;
static WebServer debugServer(8080);
static Motorshield *debugShield = nullptr;
static volatile bool motionLocked = false;

// Ringpuffer für das Log. logTotal zählt alle je geschriebenen Zeichen,
// damit der Browser nur Neues abholt (/log?from=...).
static char logBuffer[8192];
static uint32_t logTotal = 0;
static portMUX_TYPE logMutex = portMUX_INITIALIZER_UNLOCKED;

static const char workbenchPage[] PROGMEM = R"HTML(<!doctype html>
<html lang="de"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Roboter-Werkbank</title>
<style>
:root{--bg:#f3f4f6;--card:#fff;--line:#e5e7eb;--text:#111827;--muted:#6b7280;--accent:#2563eb;--danger:#dc2626;--ok:#16a34a}
*{box-sizing:border-box}body{margin:0;font:15px system-ui,sans-serif;background:var(--bg);color:var(--text)}
header{position:sticky;top:0;z-index:5;display:flex;gap:10px;align-items:center;flex-wrap:wrap;padding:10px 16px;background:#111827;color:#fff}
header h1{font-size:18px;margin:0;flex:1}
#conn,#busy{font-size:13px;padding:3px 8px;border-radius:999px;background:#374151}
#conn.ok{background:var(--ok)}#conn.bad{background:var(--danger)}
#stopBtn{background:var(--danger);font-size:18px;padding:10px 26px}
main{display:grid;gap:14px;padding:14px 16px;grid-template-columns:minmax(0,3fr) minmax(0,2fr)}
@media(max-width:900px){main{grid-template-columns:minmax(0,1fr)}}
section{background:var(--card);border:1px solid var(--line);border-radius:10px;padding:12px}
h2{font-size:15px;margin:0 0 8px}
button{background:var(--accent);color:#fff;border:0;border-radius:6px;padding:7px 12px;font-weight:600;cursor:pointer}
button.grey{background:#6b7280}
input,select,textarea{font:inherit;border:1px solid #d1d5db;border-radius:6px;padding:6px}
#log{background:#0b1020;color:#d1fae5;font:12.5px ui-monospace,monospace;height:340px;overflow:auto;white-space:pre-wrap;padding:8px;border-radius:6px;margin:0}
.row{display:flex;gap:8px;margin-top:8px;flex-wrap:wrap}.row>input{flex:1;min-width:0}
textarea{width:100%;height:170px;font:13px ui-monospace,monospace}
table{width:100%;border-collapse:collapse}td{padding:4px 4px;border-top:1px solid var(--line);vertical-align:middle}
td.n{font-family:ui-monospace,monospace;font-size:13px}td input[type=number]{width:100px}
.help{color:var(--muted);font-size:12px}
.changed td.n{color:var(--accent);font-weight:700}
#cmds{columns:2 260px;font:12.5px ui-monospace,monospace;margin:0;padding:0;list-style:none}
#cmds li{cursor:pointer;padding:2px 0;break-inside:avoid}#cmds li:hover{color:var(--accent)}
.quick button{background:#e5e7eb;color:#111827}
</style></head><body>
<header><h1>Roboter-Werkbank</h1><a href="/hw" style="color:#93c5fd;font-weight:600">Hardware</a><a href="/blocks" style="color:#93c5fd;font-weight:600">Baukasten &rarr;</a><span id="busy">...</span><span id="conn">...</span><button id="stopBtn" onclick="stopAll()">STOP</button></header>
<main>
<div style="display:grid;gap:14px;min-width:0">
<section><h2>Log &amp; Konsole</h2>
<pre id="log"></pre>
<div class="row"><input id="cmd" placeholder="Befehl, z.B.  drive 1000 1000 500   (Pfeil hoch = letzter Befehl)" autocomplete="off"><button onclick="sendInput()">Senden</button><button class="grey" onclick="$('log').textContent=''">Leeren</button></div>
<div class="row quick"><button onclick="send('help')">help</button><button onclick="send('calibrate')">calibrate</button><button onclick="send('sensors 3000')">sensors</button><button onclick="send('follow 1')">follow 1</button><button onclick="send('battery')">battery</button><button onclick="send('i2cscan')">i2cscan</button></div>
</section>
<section><h2>Skripte (Missionen)</h2>
<div class="row" style="margin-top:0"><select id="scriptSel" onchange="showScript()"></select><input id="scriptName" placeholder="Name"><button onclick="runScript()">&#9654; Ausführen</button></div>
<textarea id="scriptText" spellcheck="false"></textarea>
<div class="row"><button onclick="saveScript()">Speichern</button><button class="grey" onclick="deleteScript()">Gespeicherte Version löschen</button></div>
<div class="help">Ein Befehl pro Zeile (oder mit ; getrennt), # = Kommentar. Gespeichert wird im Roboter – kein Neu-Kompilieren nötig.</div>
</section>
</div>
<div style="display:grid;gap:14px;align-content:start;min-width:0">
<section><h2>Parameter</h2>
<div class="help">Änderungen wirken sofort. Mit "Speichern" bleiben sie nach einem Neustart erhalten. Blau = anders als im Code.</div>
<table id="params"></table>
<div class="row"><button onclick="post('/save').then(loadInfo)">Speichern</button><button class="grey" onclick="if(confirm('Alle Parameter auf die Werte aus dem Code zurücksetzen?'))send('defaults').then(()=>setTimeout(loadInfo,300))">Standardwerte</button></div>
</section>
<section><h2>Befehle</h2><div class="help">Klicken übernimmt den Befehl in die Konsole.</div><ul id="cmds"></ul></section>
</div>
</main>
<script>
const $=id=>document.getElementById(id);
let from=0,hist=[],histPos=0,scripts=[];
function post(url,data){return fetch(url,{method:'POST',body:new URLSearchParams(data||{})}).then(r=>r.text())}
function send(line){line=line.trim();if(!line)return Promise.resolve();hist.push(line);histPos=hist.length;return post('/cmd',{line})}
function sendInput(){send($('cmd').value);$('cmd').value=''}
function stopAll(){post('/stop')}
$('cmd').addEventListener('keydown',e=>{if(e.key==='Enter')sendInput();if(e.key==='ArrowUp'&&histPos>0){$('cmd').value=hist[--histPos];e.preventDefault()}if(e.key==='ArrowDown'){histPos=Math.min(hist.length,histPos+1);$('cmd').value=hist[histPos]||''}});
document.addEventListener('keydown',e=>{if(e.key==='Escape')stopAll()});
async function pollLog(){
 try{const r=await fetch('/log?from='+from,{cache:'no-store'});from=+r.headers.get('X-Next');const t=await r.text();
  if(t){const o=$('log');const bottom=o.scrollTop+o.clientHeight>=o.scrollHeight-30;o.textContent+=t;if(o.textContent.length>80000)o.textContent=o.textContent.slice(-50000);if(bottom)o.scrollTop=o.scrollHeight}
  $('conn').textContent='verbunden';$('conn').className='ok'}
 catch(e){$('conn').textContent='getrennt';$('conn').className='bad'}
 setTimeout(pollLog,400)}
async function pollState(){try{const s=await(await fetch('/state',{cache:'no-store'})).json();$('busy').textContent=s.running?('läuft: '+s.running):'bereit'}catch(e){}setTimeout(pollState,700)}
function esc(s){return String(s).replace(/[&<>"]/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[c]))}
async function loadInfo(){
 const info=await(await fetch('/info',{cache:'no-store'})).json();
 $('params').innerHTML=info.params.map(p=>{
  const input=p.type==='bool'?`<input type="checkbox" ${p.value?'checked':''} onchange="setP('${p.name}',this.checked?1:0,this)">`
   :`<input type="number" step="${p.type==='int'?1:'any'}" min="${p.min}" max="${p.max}" value="${p.value}" onchange="setP('${p.name}',this.value,this)">`;
  return `<tr class="${p.value!=p.def?'changed':''}" title="Standard: ${p.def}"><td class="n">${esc(p.name)}</td><td>${input}</td><td class="help">${esc(p.help)}</td></tr>`}).join('');
 $('cmds').innerHTML=info.commands.map(c=>`<li onclick="$('cmd').value='${esc(c.name)} ';$('cmd').focus()">${esc(c.usage)}</li>`).join('');
 scripts=info.scripts;const sel=$('scriptSel'),old=sel.value;
 sel.innerHTML=scripts.map(s=>`<option>${esc(s.name)}</option>`).join('')+'<option value="">+ neues Skript</option>';
 if(old!==undefined&&[...sel.options].some(o=>o.value===old))sel.value=old;showScript()}
function setP(name,value,el){post('/set',{name,value}).then(()=>loadInfo())}
function showScript(){const s=scripts.find(x=>x.name===$('scriptSel').value);$('scriptName').value=s?s.name:'';$('scriptText').value=s?s.text:''}
function saveScript(){const name=$('scriptName').value.trim();if(!name)return alert('Name fehlt');post('/script',{name,text:$('scriptText').value}).then(()=>{$('scriptSel').value=name;loadInfo().then(()=>{$('scriptSel').value=name;showScript()})})}
function deleteScript(){const name=$('scriptName').value.trim();if(name&&confirm('Gespeicherte Version von "'+name+'" löschen?'))post('/script',{name,text:''}).then(loadInfo)}
function runScript(){const name=$('scriptName').value.trim();if(name)send('run '+name)}
pollLog();pollState();loadInfo().catch(()=>setTimeout(loadInfo,2000));
</script></body></html>
)HTML";

size_t DebugOutput::write(uint8_t value)
{
    return write(&value, 1);
}

size_t DebugOutput::write(const uint8_t *data, size_t length)
{
    // normale USB-Ausgabe zusätzlich, falls ein Kabel steckt
    if (Serial)
    {
        Serial.write(data, length);
    }

    // beide Kerne schreiben ins Log -> Puffer schützen
    portENTER_CRITICAL(&logMutex);
    for (size_t i = 0; i < length; i++)
    {
        logBuffer[logTotal % sizeof(logBuffer)] = data[i];
        logTotal++;
    }
    portEXIT_CRITICAL(&logMutex);
    return length;
}

static void sendLog()
{
    static char chunk[sizeof(logBuffer) + 1];
    uint32_t from = debugServer.hasArg("from") ? strtoul(debugServer.arg("from").c_str(), nullptr, 10) : 0;

    portENTER_CRITICAL(&logMutex);
    const uint32_t total = logTotal;
    if (from > total)
    {
        from = 0; // Roboter wurde neu gestartet
    }
    if (total - from > sizeof(logBuffer))
    {
        from = total - sizeof(logBuffer);
    }
    const size_t count = total - from;
    for (size_t i = 0; i < count; i++)
    {
        chunk[i] = logBuffer[(from + i) % sizeof(logBuffer)];
    }
    portEXIT_CRITICAL(&logMutex);
    chunk[count] = '\0';

    debugServer.sendHeader("Cache-Control", "no-store");
    debugServer.sendHeader("X-Next", String(total));
    debugServer.send(200, "text/plain; charset=utf-8", chunk);
}

static void sendState()
{
    String json = "{\"running\":\"";
    String line = currentCommandLine();
    line.replace("\\", "\\\\");
    line.replace("\"", "\\\"");
    json += line;
    json += "\",\"vars\":";
    variablesToJson(json);
    json += '}';
    debugServer.sendHeader("Cache-Control", "no-store");
    debugServer.send(200, "application/json", json);
}

static void sendInfo()
{
    String json;
    json.reserve(8192);
    json += "{\"params\":";
    paramsToJson(json);
    json += ",\"commands\":";
    commandsToJson(json);
    json += ",\"scripts\":";
    scriptsToJson(json);
    json += ",\"devices\":";
    deviceNamesToJson(json);
    json += '}';
    debugServer.sendHeader("Cache-Control", "no-store");
    debugServer.send(200, "application/json", json);
}

static void handleCommand()
{
    String line = debugServer.arg("line");
    line.trim();
    if (line.equalsIgnoreCase("stop"))
    {
        requestStop();
    }
    queueCommand(line.c_str());
    debugServer.send(200, "text/plain", "ok");
}

static void handleStop()
{
    requestStop();
    queueCommand("stop");
    debugServer.send(200, "text/plain", "ok");
}

static void handleSet()
{
    const String name = debugServer.arg("name");
    const float value = debugServer.arg("value").toFloat();
    if (setParam(name.c_str(), value))
    {
        float applied = 0;
        getParam(name.c_str(), applied);
        Debug.printf("%s = %g\n", name.c_str(), applied);
        debugServer.send(200, "text/plain", "ok");
    }
    else
    {
        debugServer.send(404, "text/plain", "unknown param");
    }
}

static void handleSave()
{
    saveParams();
    debugServer.send(200, "text/plain", "ok");
}

static void handleScript()
{
    const bool ok = storeScript(debugServer.arg("name").c_str(), debugServer.arg("text").c_str());
    debugServer.send(ok ? 200 : 400, "text/plain", ok ? "ok" : "bad name");
}

static void sendHardware()
{
    String json;
    hardwareToJson(json, debugServer.hasArg("amps"));
    debugServer.sendHeader("Cache-Control", "no-store");
    debugServer.send(200, "application/json", json);
}

static void handleHardwareName()
{
    const String k = debugServer.arg("k");
    const DeviceKind kind = k == "m" ? DevMotor : k == "s" ? DevServo : k == "b" ? DevMcp : DevPin;
    String error;
    if (setDevice(kind, debugServer.arg("n").toInt(), debugServer.arg("name"), debugServer.arg("mode").toInt(), error))
    {
        debugServer.send(200, "text/plain", "ok");
    }
    else
    {
        debugServer.send(400, "text/plain; charset=utf-8", error);
    }
}

static void stopForUpload()
{
    // ab jetzt darf nichts mehr fahren, auch wenn der Upload fehlschlägt
    motionLocked = true;
    robotMotionEnabled = false;
    requestStop();
    for (int motor = 1; motor <= 6; motor++)
    {
        debugShield->motor(motor).stop();
    }
}

void wifiDebugBegin(Motorshield &shield)
{
    debugShield = &shield;

    debugServer.on("/", HTTP_GET, []()
                   { debugServer.send_P(200, "text/html; charset=utf-8", workbenchPage); });
    debugServer.on("/blocks", HTTP_GET, []()
                   { debugServer.send_P(200, "text/html; charset=utf-8", blocksPage); });
    debugServer.on("/hw", HTTP_GET, []()
                   { debugServer.send_P(200, "text/html; charset=utf-8", hwPage); });
    debugServer.on("/hwdata", HTTP_GET, sendHardware);
    debugServer.on("/hwname", HTTP_POST, handleHardwareName);
    debugServer.on("/log", HTTP_GET, sendLog);
    debugServer.on("/state", HTTP_GET, sendState);
    debugServer.on("/info", HTTP_GET, sendInfo);
    debugServer.on("/cmd", HTTP_POST, handleCommand);
    debugServer.on("/stop", HTTP_POST, handleStop);
    debugServer.on("/set", HTTP_POST, handleSet);
    debugServer.on("/save", HTTP_POST, handleSave);
    debugServer.on("/script", HTTP_POST, handleScript);
    debugServer.begin();

    // Name erscheint in der Arduino IDE unter Werkzeuge > Port > Netzwerk-Ports.
    // Upload-Passwort = WLAN-Passwort.
    ArduinoOTA.setHostname(otaHostname);
    ArduinoOTA.setPassword(accessPointPassword);
    ArduinoOTA.setMdnsEnabled(true);

    ArduinoOTA.onStart([]()
                       {
        stopForUpload();
        Debug.println("OTA: Upload startet, Motoren gestoppt."); });
    ArduinoOTA.onProgress([](unsigned int done, unsigned int total)
                          {
        static unsigned int lastPercent = 101;
        const unsigned int percent = total ? done * 100 / total : 0;
        if (percent / 10 != lastPercent / 10) {
            Serial.printf("OTA: %u%%\n", percent);
            lastPercent = percent;
        } });
    ArduinoOTA.onEnd([]()
                     { Debug.println("OTA: fertig, Neustart."); });
    ArduinoOTA.onError([](ota_error_t error)
                       {
        stopForUpload();
        Debug.printf("OTA-Fehler %u. Motoren bleiben aus; nochmal hochladen oder neu starten.\n",
                     (unsigned int)error); });
    ArduinoOTA.begin();

    Debug.println("Werkbank: http://192.168.4.1:8080   Baukasten: /blocks   Hardware: /hw");
}

void wifiDebugWebLoop()
{
    debugServer.handleClient();
}

void wifiDebugOtaLoop()
{
    ArduinoOTA.handle();
}

bool wifiDebugMotionLocked()
{
    return motionLocked;
}
#endif
