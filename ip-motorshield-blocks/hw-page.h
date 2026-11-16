#pragma once
// Hardware-Übersicht (http://192.168.4.1:8080/hw): alle Anschlüsse mit Namen,
// Live-Werten und Test-Knöpfen. Daten von /hwdata, Namen speichern über /hwname.

static const char hwPage[] PROGMEM = R"HTML(<!doctype html>
<html lang="de"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Roboter-Hardware</title>
<style>
:root{--bg:#f3f4f6;--card:#fff;--line:#e5e7eb;--text:#111827;--muted:#6b7280;--accent:#2563eb;--danger:#dc2626;--ok:#16a34a;--warn:#d97706}
*{box-sizing:border-box}body{margin:0;font:14px system-ui,sans-serif;background:var(--bg);color:var(--text)}
header{position:sticky;top:0;z-index:5;display:flex;gap:10px;align-items:center;flex-wrap:wrap;padding:8px 16px;background:#111827;color:#fff}
header h1{font-size:17px;margin:0;flex:1}header a{color:#93c5fd;font-weight:600}
.pill{font-size:12px;padding:3px 8px;border-radius:999px;background:#374151}.pill.ok{background:var(--ok)}.pill.bad{background:var(--danger)}
#stopBtn{background:var(--danger);font-size:16px;padding:8px 22px}
main{display:grid;gap:12px;padding:12px 16px;grid-template-columns:repeat(auto-fit,minmax(min(100%,430px),1fr))}
section{background:var(--card);border:1px solid var(--line);border-radius:10px;padding:10px;min-width:0;overflow-x:auto}
h2{font-size:15px;margin:0 0 6px}
button{background:var(--accent);color:#fff;border:0;border-radius:6px;padding:4px 9px;font:600 13px system-ui;cursor:pointer}
button.light{background:#e5e7eb;color:#111}button.red{background:var(--danger)}
input,select{font:inherit;border:1px solid #d1d5db;border-radius:5px;padding:3px 5px;background:#fff;color:#111}
input.nm{width:110px}input.nm.saved{border-color:var(--ok)}input.nm.err{border-color:var(--danger)}
table{width:100%;border-collapse:collapse}td,th{padding:4px 4px;border-top:1px solid var(--line);vertical-align:middle;text-align:left;white-space:nowrap}
th{font-size:12px;color:var(--muted);font-weight:600;border-top:0}
.nr{font:600 13px ui-monospace,monospace}
.tag{font-size:11px;color:var(--warn);display:block}
.val{font:600 13px ui-monospace,monospace;min-width:52px;display:inline-block}
.on{color:var(--ok)}.off{color:var(--muted)}
.help{color:var(--muted);font-size:12px;line-height:1.45}
.bars{display:flex;gap:6px;align-items:flex-end;height:90px;margin:6px 0}
.bars div{flex:1;background:#111827;border-radius:3px 3px 0 0;min-height:2px;position:relative}
.bars span{position:absolute;top:-16px;left:0;right:0;text-align:center;font:11px ui-monospace,monospace;color:var(--text)}
details summary{cursor:pointer;font-weight:700}
.wire td{white-space:normal}
code{background:#eef0f4;padding:0 3px;border-radius:3px}
</style></head><body>
<header><h1>Roboter-Hardware</h1><a href="/">Werkbank</a><a href="/blocks">Baukasten</a><span class="pill" id="busy">...</span><span class="pill" id="bat">...</span><span class="pill" id="conn">...</span><button id="stopBtn" onclick="stopAll()">STOP</button></header>
<main>
<section style="grid-column:1/-1"><div class="help"><b>Namen</b> (Buchstaben, Zahlen, _ ; Enter = speichern) gelten sofort in Befehlen und im Baukasten:
Motor/Servo/Ausgang = Anschluss-Nummer, z.B. <code>motor rad 1500</code>, <code>servo klappe 90</code>.
Eingang/Sensor = Messwert, z.B. <code>wait until taster == 0</code>, <code>if austritt &lt; 400</code>.
Test-Knöpfe laufen über die Befehls-Warteschlange (STOP wirkt). Kommt nach dem Anschliessen kein Wert: Kabel, Modus und Pin prüfen.</div></section>

<section><h2>DC-Motoren</h2>
<table id="motors"><tr><th>Nr</th><th>Name</th><th>Test (0,5 s)</th><th>Strom</th><th>Encoder</th></tr></table>
<div class="help" style="margin-top:6px">Tempo <input id="mspeed" type="number" value="1500" style="width:80px"> <label><input type="checkbox" id="amps"> Strom messen</label> – dreht der Motor falsch herum: Kabel tauschen oder negatives Tempo.</div></section>

<section><h2>Servos</h2>
<table id="servos"><tr><th>Nr</th><th>Name</th><th>Winkel</th><th></th></tr></table></section>

<section><h2>MCP-Pins B0–B6 <span class="help">(Taster, Endschalter, LEDs)</span></h2>
<table id="mcp"><tr><th>Pin</th><th>Name</th><th>Modus</th><th>Wert</th><th></th></tr></table>
<div class="help" style="margin-top:6px">Eingang hat Pull-up: offen = 1, gegen GND gedrückt = 0.</div></section>

<section><h2>ESP32-Pins <span class="help">(Sensoren)</span></h2>
<table id="pins"><tr><th>GPIO</th><th>Name</th><th>Modus</th><th>Wert</th><th></th></tr></table>
<div class="help" style="margin-top:6px">RC-Sensor = Pololu QTR-…RC (Wert = µs, klein = hell). Analog nur GPIO 6 und 10. Max. 3,3 V an jedem Pin!</div></section>

<section><h2>Liniensensor (5×)</h2><div class="bars" id="bars"></div><div class="help" id="lineInfo"></div>
<button class="light" onclick="send('calibrate')">calibrate (dreht sich!)</button></section>

<section><h2>Sortierrad</h2>
<table>
<tr><td>Rad-Motor</td><td><select id="s_wheelMotor"></select></td></tr>
<tr><td>Rad-Sensor (Speichen)</td><td><select id="s_wheelEncPin"></select> <span class="val" id="wv"></span></td></tr>
<tr><td>Austritts-Sensor (Würfel)</td><td><select id="s_exitPin"></select> <span class="val" id="xv"></span></td></tr>
</table>
<div style="margin-top:6px;display:flex;gap:6px;flex-wrap:wrap"><button onclick="send('wheelcal')">wheelcal</button><button class="light" onclick="send('wheel 1')">wheel 1</button><button class="light" onclick="send('feed 10')">feed</button><button class="light" onclick="send('wheelsens 3000')">wheelsens</button></div>
<div class="help">Änderungen werden sofort gespeichert.</div></section>

<section style="grid-column:1/-1"><details><summary>Wo schliesse ich was an?</summary>
<table class="wire">
<tr><td><b>DC-Motor</b> (2 Kabel)</td><td>Schraubklemme <b>DC-Motor 1–6</b> (+ / −). 1 und 2 = Antrieb (mit Encoder), 3 = Sortierrad, 4–6 frei.</td></tr>
<tr><td><b>Servo</b> (3 Kabel)</td><td>Stiftleiste <b>Servo 1–8</b>: Signal (gelb/orange/weiss) auf die Signal-Reihe, Rot = 5V, Braun/Schwarz = GND. Stecker nicht verdreht aufstecken!</td></tr>
<tr><td><b>RC-Sensor</b> QTR-HD-01RC (3 Kabel)</td><td>VIN → <b>3V3</b>, GND → <b>GND</b>, OUT → freier ESP32-Pin. Sortierrad: Rad-Sensor <b>GPIO 14</b>, Austritt <b>GPIO 48</b>. Nicht GPIO 39/40 (Pull-up stört die Messung).</td></tr>
<tr><td><b>Taster / Endschalter</b> (2 Kabel)</td><td>Ein Bein an <b>MCP B0–B6</b> (Header-Reihen "I/O (MCP)"), das andere an <b>GND</b>. Kein Widerstand nötig (Pull-up eingebaut).</td></tr>
<tr><td><b>Analog-Sensor</b> (Poti, IR-Distanz)</td><td>Signal an <b>GPIO 6 oder 10</b>, Versorgung 3V3 + GND. Signal darf nie über 3,3 V gehen.</td></tr>
<tr><td><b>LED / Relais</b></td><td>MCP B0–B6 oder freier ESP32-Pin als Ausgang (LED immer mit Vorwiderstand ~220 Ω).</td></tr>
<tr><td><b>I²C-Sensor</b> (4 Kabel)</td><td>Qwiic-Buchse oder Header SDA = GPIO 8, SCL = GPIO 9, 3V3, GND. Mit <code>i2cscan</code> prüfen, ob die neue Adresse erscheint.</td></tr>
<tr><td><b>Belegt / Finger weg</b></td><td>GPIO 1, 2, 4, 5 (Motoren 1/2), 17, 18, 21, 47 (Encoder), 7, 15, 16, 41, 42 (Liniensensor), 8, 9 (I²C), 38 (RGB-LED), 0, 3, 45, 46 (Boot-Pins). MCP B7 nicht benutzen.</td></tr>
</table>
<div class="help">Die GPIO-Nummern stehen auf der Platine neben dem Header. Im Bild aus docs/Pinout_Motorshield_ESP32_IP26_V4 sieht man, welche Reihe 3V3 und welche GND ist. Erst Akku aus, dann umstecken.</div>
</details></section>
</main>
<script>
const $=id=>document.getElementById(id);let built=false,last=null;
const MODES=['Eingang','Ausgang','Analog','RC-Sensor'];
function post(u,d){return fetch(u,{method:'POST',body:new URLSearchParams(d||{})}).then(async r=>{const t=await r.text();if(!r.ok)throw new Error(t);return t})}
function send(l){return post('/cmd',{line:l})}
function stopAll(){post('/stop')}
document.addEventListener('keydown',e=>{if(e.key==='Escape')stopAll()});
function esc(s){return String(s??'').replace(/[&<>"]/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[c]))}
function nameInput(k,n,v,mode){return `<input class="nm" id="nm_${k}${n}" value="${esc(v)}" placeholder="Name" maxlength="15" onkeydown="if(event.key==='Enter')this.blur()" onchange="saveName('${k}',${n})">`}
function modeSel(k,n,mode,adc){const opts=k=='b'?[0,1]:[0,1,...(adc?[2]:[]),3];return `<select id="md_${k}${n}" onchange="saveName('${k}',${n})">${opts.map(m=>`<option value="${m}" ${m==mode?'selected':''}>${MODES[m]}</option>`).join('')}</select>`}
async function saveName(k,n){const i=$('nm_'+k+n),m=$('md_'+k+n);try{await post('/hwname',{k,n,name:i.value.trim(),mode:m?m.value:1});i.className='nm saved';setTimeout(()=>i.className='nm',1200)}catch(e){i.className='nm err';alert(e.message)}}
function tag(t){return t?`<span class="tag">${esc(t)}</span>`:''}
function build(d){
 $('motors').innerHTML+=d.motors.map(m=>`<tr><td class="nr">M${m.n}${tag(m.tag)}</td><td>${nameInput('m',m.n,m.name)}</td><td><button class="light" onclick="mt(${m.n},-1)">◀</button> <button class="light" onclick="send('brake ${m.n}')">■</button> <button class="light" onclick="mt(${m.n},1)">▶</button></td><td><span class="val" id="ma${m.n}">-</span></td><td><span class="val" id="me${m.n}">${m.enc??''}</span></td></tr>`).join('');
 $('servos').innerHTML+=d.servos.map(s=>`<tr><td class="nr">S${s.n}${tag(s.tag)}</td><td>${nameInput('s',s.n,s.name)}</td><td><input type="range" min="0" max="180" value="90" style="width:120px" oninput="$('sv${s.n}').textContent=this.value" onchange="send('servo ${s.n} '+this.value)"> <span class="val" id="sv${s.n}">90</span></td><td><button class="light" onclick="send('servooff ${s.n}')">aus</button></td></tr>`).join('');
 $('mcp').innerHTML+=d.mcp.map(p=>`<tr><td class="nr">B${p.n}${tag(p.tag)}</td><td>${nameInput('b',p.n,p.name)}</td><td>${modeSel('b',p.n,p.mode)}</td><td><span class="val" id="bv${p.n}"></span></td><td><button class="light" id="bt${p.n}" onclick="toggle('gpio',${p.n},'bv')">0/1</button></td></tr>`).join('');
 $('pins').innerHTML+=d.pins.map(p=>`<tr><td class="nr">${p.n}${tag(p.tag)}</td><td>${nameInput('g',p.n,p.name)}</td><td>${modeSel('g',p.n,p.mode,p.adc)}</td><td><span class="val" id="gv${p.n}"></span></td><td><button class="light" id="gt${p.n}" onclick="toggle('pin',${p.n},'gv')">0/1</button></td></tr>`).join('');
 const opt=(arr,cur)=>arr.map(([v,l])=>`<option value="${v}" ${v==cur?'selected':''}>${l}</option>`).join('');
 const pinOpts=[[-1,'keiner'],...d.pins.map(p=>[p.n,'GPIO '+p.n+(p.name?' ('+p.name+')':'')])];
 $('s_wheelMotor').innerHTML=opt([1,2,3,4,5,6].map(n=>[n,'Motor '+n]),d.sorter.wheelMotor);
 $('s_wheelEncPin').innerHTML=opt(pinOpts,d.sorter.wheelPin);$('s_exitPin').innerHTML=opt(pinOpts,d.sorter.exitPin);
 for(const p of['wheelMotor','wheelEncPin','exitPin'])$('s_'+p).onchange=async e=>{await post('/set',{name:p,value:e.target.value});await post('/save');built=false;rebuild()};
 $('bars').innerHTML=d.line.v.map((v,i)=>`<div id="lb${i}"><span id="lv${i}"></span></div>`).join('');built=true}
async function rebuild(){for(const t of['motors','servos','mcp','pins']){const tb=$(t);while(tb.rows.length>1)tb.deleteRow(1)}await poll(true)}
function mt(n,dir){send(`motor ${n} ${dir*(+$('mspeed').value||1500)} 500`)}
function toggle(cmd,n,pre){const cur=$(pre+n).textContent.trim()=='1'?1:0;const k=cmd=='gpio'?'b':'g';const m=$('md_'+k+n);if(m&&m.value!='1'){m.value='1'}send(`${cmd} ${n} ${1-cur}`)}
function showVal(id,v,mode){const e=$(id);if(!e)return;e.textContent=v;e.className='val '+((mode==0||mode==1)?(v==1?'on':'off'):'')}
async function poll(force){try{const d=await(await fetch('/hwdata'+($('amps').checked?'?amps=1':''),{cache:'no-store'})).json();
 if(!built||force)build(d);last=d;
 $('busy').textContent=d.running?'läuft: '+d.running:'bereit';$('bat').textContent=d.battery.toFixed(2)+' V';$('bat').className='pill '+(d.battery<10.5?'bad':'ok');
 for(const m of d.motors){$('ma'+m.n).textContent=m.a==null?'-':m.a.toFixed(2)+' A';if(m.enc!=null)$('me'+m.n).textContent=m.enc}
 for(const p of d.mcp){showVal('bv'+p.n,p.v,p.mode);$('bt'+p.n).style.visibility=p.mode==1?'visible':'hidden';const i=$('nm_b'+p.n);if(document.activeElement!==i&&i.className=='nm')i.value=p.name;const m=$('md_b'+p.n);if(document.activeElement!==m)m.value=p.mode}
 for(const p of d.pins){showVal('gv'+p.n,p.v??'…',p.mode);$('gt'+p.n).style.visibility=p.mode==1?'visible':'hidden';const i=$('nm_g'+p.n);if(document.activeElement!==i&&i.className=='nm')i.value=p.name;const m=$('md_g'+p.n);if(document.activeElement!==m)m.value=p.mode;
  if(p.n==d.sorter.wheelPin)$('wv').textContent=(p.v??'…')+' µs';if(p.n==d.sorter.exitPin)$('xv').textContent=(p.v??'…')+' µs'}
 const mx=d.line.cal?1000:Math.max(1000,...d.line.v);d.line.v.forEach((v,i)=>{$('lb'+i).style.height=Math.max(2,v/mx*90)+'px';$('lv'+i).textContent=v});
 $('lineInfo').textContent=d.line.cal?'kalibriert: 0 = weiss, 1000 = schwarz':'noch nicht kalibriert: Rohwerte in µs (gross = dunkel)';
 $('conn').textContent='verbunden';$('conn').className='pill ok'}catch(e){$('conn').textContent='getrennt';$('conn').className='pill bad'}}
async function loop(){await poll();setTimeout(loop,700)}loop();
</script></body></html>
)HTML";
