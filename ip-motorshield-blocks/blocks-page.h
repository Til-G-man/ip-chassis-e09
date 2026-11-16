#pragma once
// Block-Baukasten (http://192.168.4.1:8080/blocks). Die Seite erzeugt Text in
// der Ablauf-Sprache (program.h) und speichert ihn als Skript im Roboter.
// Kein Internet nötig: alles steckt in dieser Datei.

static const char blocksPage[] PROGMEM = R"HTML(<!doctype html>
<html lang="de"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Roboter-Baukasten</title>
<style>
:root{--bg:#eef0f4;--card:#fff;--line:#d9dde5;--text:#111827;--muted:#6b7280;--accent:#2563eb;--danger:#dc2626;--ok:#16a34a;
--c-flow:#f59e0b;--c-move:#2563eb;--c-cmd:#0d9488;--c-fn:#9333ea;--c-var:#db2777;--c-raw:#6b7280}
*{box-sizing:border-box}body{margin:0;font:14px system-ui,sans-serif;background:var(--bg);color:var(--text)}
header{position:sticky;top:0;z-index:10;display:flex;gap:8px;align-items:center;flex-wrap:wrap;padding:8px 16px;background:#111827;color:#fff}
header h1{font-size:17px;margin:0;flex:1}header a{color:#93c5fd}
.pill{font-size:12px;padding:3px 8px;border-radius:999px;background:#374151}.pill.ok{background:var(--ok)}.pill.bad{background:var(--danger)}
#stopBtn{background:var(--danger);font-size:16px;padding:8px 22px}
button{background:var(--accent);color:#fff;border:0;border-radius:6px;padding:6px 11px;font:600 13px system-ui;cursor:pointer}
button.grey{background:#6b7280}button.light{background:#e5e7eb;color:#111}
input,select,textarea{font:inherit;border:1px solid #cbd2dc;border-radius:5px;padding:3px 6px;background:#fff;color:#111}
.wrap{display:grid;grid-template-columns:230px minmax(0,1fr) 280px;gap:12px;padding:12px 16px}
@media(max-width:1100px){.wrap{grid-template-columns:200px minmax(0,1fr)}.side{grid-column:1/-1}}
@media(max-width:700px){.wrap{grid-template-columns:minmax(0,1fr)}}
.card{background:var(--card);border:1px solid var(--line);border-radius:10px;padding:10px;min-width:0}
h2{font-size:13px;margin:10px 0 6px;color:var(--muted);text-transform:uppercase;letter-spacing:.04em}h2:first-child{margin-top:0}
.pal{display:flex;flex-direction:column;gap:4px}
.pi{border-radius:6px;padding:5px 8px;color:#fff;font-weight:600;cursor:pointer;user-select:none;font-size:13px}
.pi:hover{filter:brightness(1.1)}
.bar{display:flex;gap:6px;flex-wrap:wrap;align-items:center;margin-bottom:10px}
#ws{min-height:300px;padding:6px;border:2px dashed transparent;border-radius:8px}
.blk{border-radius:7px;margin:4px 0;color:#fff;border-left:6px solid rgba(0,0,0,.25)}
.bh{display:flex;flex-wrap:wrap;align-items:center;gap:6px;padding:5px 6px}
.bh input,.bh select{padding:2px 5px;font-size:13px}
.bh .t{font-weight:700}
.pk{width:24px;padding:2px 0 !important;margin-left:1px;cursor:pointer;background:rgba(255,255,255,.85)}
.grip{cursor:grab;opacity:.7;padding:0 2px}
.tools{margin-left:auto;display:flex;gap:2px}
.tools span{cursor:pointer;opacity:.75;padding:0 4px;font-weight:700}.tools span:hover{opacity:1}
.slot{margin:0 0 6px 18px;padding:2px 4px 2px 8px;border-left:3px solid rgba(255,255,255,.6);background:rgba(255,255,255,.18);border-radius:0 6px 6px 0;min-height:26px}
.lbl{padding:0 8px;font-weight:700;font-size:12px}
.add{color:inherit;opacity:.8;font-size:12px;padding:3px 6px;cursor:pointer;border-radius:5px}
#ws>.add{color:var(--muted)}
.add.target{outline:2px solid #facc15;opacity:1;font-weight:700}
.drop{box-shadow:0 -3px 0 #facc15}
.f-flow{background:var(--c-flow)}.f-move{background:var(--c-move)}.f-cmd{background:var(--c-cmd)}.f-fn{background:var(--c-fn)}.f-var{background:var(--c-var)}.f-raw{background:var(--c-raw)}
.help{color:var(--muted);font-size:12px;line-height:1.4}
#log{background:#0b1020;color:#d1fae5;font:12px ui-monospace,monospace;height:260px;overflow:auto;white-space:pre-wrap;padding:6px;border-radius:6px;margin:0}
#vars{font:12px ui-monospace,monospace;white-space:pre-wrap;margin:0}
#txt{width:100%;height:260px;font:13px ui-monospace,monospace;display:none;margin-top:8px}
code{background:#eef0f4;padding:0 3px;border-radius:3px}
</style></head><body>
<header><h1>Roboter-Baukasten</h1><a href="/">&larr; Werkbank</a><a href="/hw">Hardware</a><span class="pill" id="busy">...</span><span class="pill" id="conn">...</span><button id="stopBtn" onclick="stopAll()">STOP</button></header>
<div class="wrap">
<div class="card"><div id="palette"></div></div>
<div class="card">
 <div class="bar">
  <select id="sel" onchange="openScript(this.value)"></select>
  <input id="name" placeholder="Name (max. 15)" size="14" maxlength="15" oninput="dirty=true">
  <button onclick="saveScript()">Speichern</button>
  <button style="background:var(--ok)" onclick="runCurrent()">&#9654; Ausführen</button>
  <button class="light" onclick="toggleText()">Text</button>
  <button class="light" onclick="newScript()">Neu</button>
  <button class="light" onclick="deleteScript()">Löschen</button>
  <span id="dirty" class="help"></span>
 </div>
 <div id="ws"></div>
 <textarea id="txt" spellcheck="false" oninput="textEdited=true"></textarea>
 <div id="txtBar" style="display:none" class="bar"><button onclick="applyText()">Text übernehmen</button><span class="help">Text bearbeiten und übernehmen, oder einfach wieder auf "Text" klicken.</span></div>
 <p class="help">Klick auf einen Block links fügt ihn an der <b>gelb markierten</b> Stelle ein (auf "+ hier einfügen" klicken, um die Stelle zu wählen). Blöcke lassen sich am &#8942;&#8942; ziehen oder mit &uarr; &darr; verschieben. Jedes gespeicherte Skript ist eine <b>Funktion</b> und erscheint links unter "Meine Funktionen".</p>
</div>
<div class="card side">
 <h2>Log</h2><pre id="log"></pre>
 <h2>Variablen</h2><pre id="vars">-</pre>
 <h2>Bedingungen</h2>
 <div class="help"><code>checkpoints &gt;= 2</code> Querlinien im Block<br><code>time &gt; 3000</code> ms seit Block-Start<br><code>lineCenter</code> Mitte sieht Linie<br><code>gpio(0) == 0</code> Taster an B0<br><code>enc(1) &gt; 500</code> Encoder<br><code>qtr(2) &gt; 600</code> Sensor 2<br><code>cube</code> Würfel im Austritt (Sortierrad)<br>verknüpfen mit <code>and</code> <code>or</code> <code>not</code>; rechnen mit <code>+ - * / ( )</code>; Parameter (z.B. <code>speed</code>) und Variablen sind erlaubt. In Funktionen: <code>arg1</code>, <code>arg2</code>, ...</div>
</div>
</div>
<script>
const $=id=>document.getElementById(id);
let info={commands:[],scripts:[],params:[]},prog=[],target=null,current='',dirty=false,textEdited=false,from=0,drag=null;
const META=['help','params','save','defaults','reboot','show','vars','eval','run'];
const CONDS=['checkpoints >= 1','cube','not cube','time > 2000','lineCenter','not lineCenter','gpio(0) == 0','pin(6) == 1','analog(6) > 2000','enc(1) > 500','qtr(2) > 600','battery < 11.5'];
const ACTS={follow:['Linie folgen',[]],drive:['Fahren',['links','rechts']],turn:['Drehen',['1=rechts -1=links']],motor:['Motor',['nr','speed']],wait:['Warten',[]]};
function post(u,d){return fetch(u,{method:'POST',body:new URLSearchParams(d||{})}).then(r=>r.text())}
function send(l){return post('/cmd',{line:l})}
function stopAll(){post('/stop')}
document.addEventListener('keydown',e=>{if(e.key==='Escape')stopAll()});

// ---------- Text <-> Blöcke ----------
function splitArgs(s){const out=[];let cur='',d=0;for(const c of s){if(c=='(')d++;if(c==')')d--;if(/\s/.test(c)&&d<=0){if(cur)out.push(cur);cur='';continue}cur+=c}if(cur)out.push(cur);return out}
function arg(x){x=String(x??'').trim();if(/\s/.test(x)&&!(x[0]=='('&&x.endsWith(')')))x='('+x+')';return x}
function slotsOf(name){const c=info.commands.find(c=>c.name==name);if(!c)return[];const u=c.usage.split(' - ')[0];return[...u.matchAll(/[<\[]([^>\]]+)[>\]]/g)].map(m=>m[1])}
function isCmd(n){return info.commands.some(c=>c.name.toLowerCase()==n.toLowerCase())}
function isScript(n){return info.scripts.some(s=>s.name.toLowerCase()==n.toLowerCase())}
function toText(list,ind=''){let o='';for(const b of list){const I=ind;
 switch(b.t){
 case'cmd':{const a=b.args.map(arg);while(a.length&&a[a.length-1]==='')a.pop();o+=I+[b.name,...a.map(x=>x===''?'0':x)].join(' ')+'\n';break}
 case'call':o+=I+(b.name+' '+(b.args||'')).trim()+'\n';break;
 case'repeat':o+=I+'repeat '+(b.n||'1')+'\n'+toText(b.body,I+'  ')+I+'end\n';break;
 case'while':o+=I+'while '+(b.cond||'false')+'\n'+toText(b.body,I+'  ')+I+'end\n';break;
 case'if':o+=I+'if '+(b.cond||'false')+'\n'+toText(b.body,I+'  ')+(b.els?I+'else\n'+toText(b.els,I+'  '):'')+I+'end\n';break;
 case'until':{const a=(b.a||[]).slice(0,ACTS[b.act][1].length).map(x=>arg(x)||'0');o+=I+[b.act,...a,'until',b.cond||'true'].join(' ')+'\n';break}
 case'let':o+=I+'let '+(b.v||'x')+' = '+(b.e||'0')+'\n';break;
 case'print':o+=I+'print '+(b.text||'')+'\n';break;
 default:o+=I+(b.text||'')+'\n'}}return o}
function fromText(text){
 const L=text.split(/\n|;/).map(s=>s.trim()).filter(s=>s);let i=0;
 function seq(stops){const out=[];while(i<L.length){const line=L[i],w=line.split(/\s+/)[0].toLowerCase(),rest=line.slice(w.length).trim();
  if(stops.includes(w))return out;i++;
  if(line[0]=='#'){out.push({t:'raw',text:line});continue}
  if(w=='repeat'){const b={t:'repeat',n:rest,body:seq(['end'])};i++;out.push(b);continue}
  if(w=='while'){const b={t:'while',cond:rest,body:seq(['end'])};i++;out.push(b);continue}
  if(w=='if'){const b={t:'if',cond:rest,body:seq(['else','end']),els:null};if(i<L.length&&L[i].split(/\s+/)[0].toLowerCase()=='else'){i++;b.els=seq(['end'])}i++;out.push(b);continue}
  if(w=='let'){const m=rest.match(/^(\w+)\s*=\s*(.*)$/);out.push(m?{t:'let',v:m[1],e:m[2]}:{t:'raw',text:line});continue}
  if(w=='print'){out.push({t:'print',text:rest});continue}
  const u=line.toLowerCase().indexOf(' until ');
  if(u>0&&ACTS[w]){const parts=splitArgs(line.slice(0,u)).slice(1);out.push({t:'until',act:w,a:parts,cond:line.slice(u+7).trim()});continue}
  if(isCmd(w)){out.push({t:'cmd',name:w,args:splitArgs(rest)});continue}
  if(isScript(w)){out.push({t:'call',name:line.split(/\s+/)[0],args:rest});continue}
  out.push({t:'raw',text:line})}return out}
 return seq([])}

// ---------- Neue Blöcke ----------
const MAKE={
 repeat:()=>({t:'repeat',n:'3',body:[]}),while:()=>({t:'while',cond:'lineCenter',body:[]}),
 if:()=>({t:'if',cond:'gpio(0) == 0',body:[],els:null}),ifelse:()=>({t:'if',cond:'gpio(0) == 0',body:[],els:[]}),
 wait:()=>({t:'cmd',name:'wait',args:['1000']}),waituntil:()=>({t:'until',act:'wait',a:[],cond:'gpio(0) == 0'}),
 let:()=>({t:'let',v:'zaehler',e:'0'}),inc:()=>({t:'let',v:'zaehler',e:'zaehler + 1'}),print:()=>({t:'print',text:'Wert: {zaehler}'}),raw:()=>({t:'raw',text:'# Kommentar'}),
 follow:()=>({t:'until',act:'follow',a:[],cond:'checkpoints >= 1'}),drive:()=>({t:'until',act:'drive',a:['speed','speed'],cond:'time > 1000'}),
 turn:()=>({t:'until',act:'turn',a:['1'],cond:'time > 300 and lineCenter'}),motor:()=>({t:'until',act:'motor',a:['3','1500'],cond:'time > 2000'})};
function cmdBlock(n){return{t:'cmd',name:n,args:slotsOf(n).map(()=>'')}}

// ---------- Palette ----------
function renderPalette(){const p=$('palette');p.innerHTML='';
 const group=(title,items)=>{const h=document.createElement('h2');h.textContent=title;p.appendChild(h);const d=document.createElement('div');d.className='pal';
  for(const[label,cls,make,tip]of items){const e=document.createElement('div');e.className='pi '+cls;e.textContent=label;if(tip)e.title=tip;e.draggable=true;
   e.onclick=()=>insertNew(make());e.ondragstart=ev=>{drag={make};ev.dataTransfer.setData('text','x')};d.appendChild(e)}p.appendChild(d)};
 group('Ablauf',[['wiederhole n-mal','f-flow',MAKE.repeat],['solange ...','f-flow',MAKE.while],['wenn ... dann','f-flow',MAKE.if],['wenn ... sonst','f-flow',MAKE.ifelse],['warte ms','f-flow',MAKE.wait],['warte bis ...','f-flow',MAKE.waituntil]]);
 group('Bewegung bis ...',[['Linie folgen bis','f-move',MAKE.follow],['Fahren bis','f-move',MAKE.drive],['Drehen bis','f-move',MAKE.turn],['Motor bis','f-move',MAKE.motor]]);
 group('Variablen',[['setze Variable','f-var',MAKE.let],['erhöhe Variable','f-var',MAKE.inc],['Ausgabe ins Log','f-var',MAKE.print],['Kommentar','f-raw',MAKE.raw]]);
 group('Befehle',info.commands.filter(c=>!META.includes(c.name)).map(c=>[c.name,'f-cmd',()=>cmdBlock(c.name),c.usage]));
 group('Meine Funktionen',info.scripts.map(s=>[s.name,'f-fn',()=>({t:'call',name:s.name,args:''}),s.text]))}

// ---------- Vorschläge (▾ neben jedem Feld) ----------
let liveVars={};
const FREEPINS=[6,14,48,10,11,12,13,39,40];
function devs(k){return(info.devices||[]).filter(d=>d.k==k)}
function allVars(){const s=new Set(Object.keys(liveVars));const t=[toText(prog),...info.scripts.map(x=>x.text)].join('\n');for(const m of t.matchAll(/\blet\s+(\w+)/g))s.add(m[1]);return[...s].sort()}
function sensors(){return(info.devices||[]).filter(d=>(d.k=='gpio'||d.k=='pin')&&d.mode!=1)}
const G=(t,a)=>[t,a.map(x=>Array.isArray(x)?x:[String(x),String(x)])];
function varGroup(){return G('Variablen',allVars())}
function SUG(kind){switch(kind){
 case'motor':return[G('Benannt',devs('motor').map(d=>[d.name,d.name+' (Motor '+d.n+')'])),G('Anschluss',[1,2,3,4,5,6].map(n=>[n+'','Motor '+n])),varGroup()];
 case'servo':return[G('Benannt',devs('servo').map(d=>[d.name,d.name+' (Servo '+d.n+')'])),G('Anschluss',[1,2,3,4,5,6,7,8].map(n=>[n+'','Servo '+n])),varGroup()];
 case'mcp':return[G('Benannt',devs('gpio').map(d=>[d.name,d.name+' (B'+d.n+')'])),G('Pin',[0,1,2,3,4,5,6].map(n=>[n+'','B'+n]))];
 case'pin':return[G('Benannt',devs('pin').map(d=>[d.name,d.name+' (GPIO '+d.n+')'])),G('Freie GPIOs',FREEPINS.map(n=>[n+'','GPIO '+n]))];
 case'script':return[G('Funktionen',info.scripts.map(s=>s.name))];
 case'param':return[G('Parameter',info.params.map(p=>[p.name,p.name+' = '+p.value]))];
 case'var':return[varGroup()];
 case'printvar':return[G('Variable einfügen',allVars().map(v=>['{'+v+'}',v])),G('Sensor einfügen',sensors().map(d=>['{'+d.name+'}',d.name])),G('Wert einfügen',['{time}','{battery}','{linePos}'])];
 case'speed':return[G('Tempo',[['speed','speed (Parameter)'],['-speed','-speed'],'800','1500','-1500',['4095','4095 = voll']]),varGroup()];
 case'ms':return[G('Zeit (ms)',['300','500','1000','2000','5000']),varGroup()];
 case'angle':return[G('Winkel',['0','45','90','135','180']),varGroup()];
 case'us':return[G('Puls (µs)',['500','1000','1500','2000','2500'])];
 case'dir':return[G('Richtung',[['1','1 = rechts'],['-1','-1 = links']])];
 case'bool':return[G('Wert',[['1','1 = an / zu'],['0','0 = aus / auf']])];
 case'cond':{const s=sensors();return[G('Vorlagen',CONDS),
  G('Meine Sensoren',s.flatMap(d=>d.mode==3?[d.name+' < 400',d.name+' > 1000']:d.mode==2?[d.name+' > 2000']:[d.name+' == 0',d.name+' == 1'])),
  G('Variablen',allVars().flatMap(v=>[v+' > 0',v+' == 0',v+' >= 3']))]}
 default:return[varGroup(),G('Sensoren',sensors().map(d=>d.name)),G('Werte',['time','checkpoints','linePos','battery','cube','arg1','arg2','qtr(2)','random(1, 6)']),G('Parameter',info.params.map(p=>p.name))]}}
function kindOf(cmd,ph){ph=(ph||'').toLowerCase();
 if(ph=='1-6'||ph=='1-2'||ph=='nr')return'motor';if(ph=='1-8')return'servo';if(ph=='0-6')return'mcp';if(ph=='gpio')return'pin';
 if(ph=='skript')return'script';if(ph=='name'&&cmd=='set')return'param';if(ph.includes('1|-1')||ph.startsWith('1=rechts'))return'dir';
 if(ph=='0|1'||ph=='1|0')return'bool';if(ph=='winkel')return'angle';if(ph=='us')return'us';if(/ms$/.test(ph))return'ms';
 if(/speed|links|rechts|4095/.test(ph))return'speed';return'expr'}
function withPick(i,kind,append){const w=el('span');w.style.whiteSpace='nowrap';const s=el('select','pk');s.title='Vorschläge';
 const fill=()=>{s.innerHTML='<option value="">&#9662;</option>'+SUG(kind).filter(g=>g[1].length).map(([t,a])=>`<optgroup label="${t}">${a.map(([v,l])=>`<option value="${v.replace(/"/g,'&quot;')}">${l}</option>`).join('')}</optgroup>`).join('')};
 fill();s.onfocus=s.onmousedown=fill;
 s.onchange=()=>{if(s.value){i.value=append&&i.value.trim()?i.value.replace(/\s*$/,' ')+s.value:s.value;i.dispatchEvent(new Event('input'))}s.value=''};
 w.append(i,s);return w}

// ---------- Arbeitsfläche ----------
function el(tag,cls,text){const e=document.createElement(tag);if(cls)e.className=cls;if(text!=null)e.textContent=text;return e}
function field(obj,key,ph,size,idx){const i=el('input');const v=idx==null?obj[key]:obj[key][idx];i.value=v??'';i.placeholder=ph||'';i.size=Math.max(size||4,String(i.value).length+1,(ph||'').length);
 i.oninput=()=>{if(idx==null)obj[key]=i.value;else obj[key][idx]=i.value;i.size=Math.max(size||4,i.value.length+1);changed()};return i}
function condField(b,key){return withPick(field(b,key,'Bedingung',16),'cond')}
function contains(b,arr){if(b.body===arr||b.els===arr)return true;return[...(b.body||[]),...(b.els||[])].some(c=>contains(c,arr))}
function slot(arr,cls){const s=el('div',cls);arr.forEach((b,k)=>s.appendChild(renderBlock(b,arr,k)));const a=el('div','add'+(target===arr?' target':''),'+ hier einfügen');
 a.onclick=()=>{target=arr;render()};a.ondragover=e=>{e.preventDefault();a.classList.add('drop')};a.ondragleave=()=>a.classList.remove('drop');a.ondrop=e=>{e.preventDefault();e.stopPropagation();dropInto(arr,arr.length)};s.appendChild(a);return s}
function renderBlock(b,arr,k){
 const fam={cmd:'f-cmd',call:'f-fn',repeat:'f-flow',while:'f-flow',if:'f-flow',until:'f-move',let:'f-var',print:'f-var',raw:'f-raw'}[b.t]||'f-raw';
 const d=el('div','blk '+fam);const h=el('div','bh');d.appendChild(h);
 const g=el('span','grip','⋮⋮');g.draggable=true;g.ondragstart=e=>{drag={block:b,arr};e.dataTransfer.setData('text','x');e.stopPropagation()};h.appendChild(g);
 h.ondragover=e=>{e.preventDefault();e.stopPropagation();d.classList.add('drop')};h.ondragleave=()=>d.classList.remove('drop');h.ondrop=e=>{e.preventDefault();e.stopPropagation();dropInto(arr,arr.indexOf(b))};
 const T=t=>h.appendChild(el('span','t',t));
 switch(b.t){
 case'cmd':{const sl=slotsOf(b.name);T(b.name);const n=Math.max(sl.length,b.args.length);while(b.args.length<n)b.args.push('');for(let j=0;j<n;j++)h.appendChild(withPick(field(b,'args',sl[j]||'',4,j),kindOf(b.name,sl[j])));
  const c=info.commands.find(c=>c.name==b.name);if(c)h.title=c.usage;break}
 case'call':T('ƒ '+b.name);h.appendChild(withPick(field(b,'args','Argumente',6),'expr',true));break;
 case'repeat':T('wiederhole');h.appendChild(withPick(field(b,'n','Anzahl',3),'expr'));T('mal');break;
 case'while':T('solange');h.appendChild(condField(b,'cond'));break;
 case'if':T('wenn');h.appendChild(condField(b,'cond'));T('dann');break;
 case'until':{const s=el('select');for(const a in ACTS){const o=el('option',null,ACTS[a][0]);o.value=a;if(a==b.act)o.selected=true;s.appendChild(o)}
  s.onchange=()=>{b.act=s.value;b.a=ACTS[b.act][1].map((x,j)=>b.a[j]||'');changed();render()};h.appendChild(s);
  b.a=b.a||[];ACTS[b.act][1].forEach((ph,j)=>{if(b.a[j]==null)b.a[j]='';h.appendChild(withPick(field(b,'a',ph,5,j),kindOf(b.act,ph)))});T('bis');h.appendChild(condField(b,'cond'));break}
 case'let':T('setze');h.appendChild(withPick(field(b,'v','name',6),'var'));T('=');h.appendChild(withPick(field(b,'e','Wert',8),'expr',true));break;
 case'print':T('Ausgabe');h.appendChild(withPick(field(b,'text','Text, {variable}',18),'printvar',true));break;
 default:h.appendChild(field(b,'text','Text',24))}
 const tools=el('span','tools');const tool=(t,tip,f)=>{const s=el('span',null,t);s.title=tip;s.onclick=f;tools.appendChild(s)};
 tool('↑','nach oben',()=>{if(k>0){arr.splice(k,1);arr.splice(k-1,0,b);changed();render()}});
 tool('↓','nach unten',()=>{if(k<arr.length-1){arr.splice(k,1);arr.splice(k+1,0,b);changed();render()}});
 tool('⧉','kopieren',()=>{arr.splice(k+1,0,JSON.parse(JSON.stringify(b)));changed();render()});
 if(b.t=='if')tool(b.els?'−sonst':'+sonst','sonst-Teil',()=>{b.els=b.els?null:[];changed();render()});
 tool('✕','löschen',()=>{arr.splice(k,1);changed();render()});h.appendChild(tools);
 if(b.body){d.appendChild(slot(b.body,'slot'))}
 if(b.els){d.appendChild(el('div','lbl','sonst'));d.appendChild(slot(b.els,'slot'))}
 return d}
function render(){if(!target||(target!==prog&&!prog.some(b=>contains(b,target))))target=prog;const ws=$('ws');ws.innerHTML='';const s=slot(prog,'');while(s.firstChild)ws.appendChild(s.firstChild)}
function insertNew(b){(target||prog).push(b);changed();render()}
function dropInto(arr,idx){if(!drag)return;let b;
 if(drag.make)b=drag.make();else{b=drag.block;if(b.body&&(contains(b,arr)))return drag=null;const old=drag.arr.indexOf(b);drag.arr.splice(old,1);if(drag.arr===arr&&old<idx)idx--}
 arr.splice(Math.max(0,idx),0,b);drag=null;changed();render()}
function changed(){dirty=true;$('dirty').textContent='● nicht gespeichert';if($('txt').style.display=='block'&&!textEdited)$('txt').value=toText(prog)}

// ---------- Skripte ----------
async function loadInfo(){info=await(await fetch('/info',{cache:'no-store'})).json();renderPalette();fillSelect()}
function fillSelect(){const s=$('sel');s.innerHTML=info.scripts.map(x=>`<option>${x.name}</option>`).join('')+'<option value="">(neu)</option>';s.value=current}
function openScript(name){if(dirty&&!confirm('Änderungen verwerfen?')){$('sel').value=current;return}
 const sc=info.scripts.find(s=>s.name==name);current=name;$('name').value=name;prog=sc?fromText(sc.text):[];target=prog;dirty=false;$('dirty').textContent='';textEdited=false;$('txt').value=toText(prog);render()}
function newScript(){if(dirty&&!confirm('Änderungen verwerfen?'))return;dirty=false;current='';$('sel').value='';$('name').value='';prog=[];target=prog;render();$('name').focus()}
async function saveScript(){const name=$('name').value.trim();if(!/^\w{1,15}$/.test(name)){alert('Name: 1-15 Zeichen, nur Buchstaben, Zahlen, _');return false}
 if(isCmd(name)){alert('"'+name+'" ist schon ein Befehl. Bitte anderen Namen wählen.');return false}
 if($('txt').style.display=='block'&&textEdited)applyText();
 const text=toText(prog);if(!text.trim()){alert('Skript ist leer');return false}if(text.length>3900){alert('Skript zu lang (max. ca. 3900 Zeichen). In Funktionen aufteilen.');return false}
 await post('/script',{name,text});current=name;dirty=false;$('dirty').textContent='gespeichert';await loadInfo();return true}
async function runCurrent(){if(dirty||!current){if(!await saveScript())return}send('run '+current)}
async function deleteScript(){const name=$('name').value.trim();if(!name||!confirm('Gespeicherte Version von "'+name+'" löschen? (Eine Version aus dem Code bleibt erhalten.)'))return;await post('/script',{name,text:''});dirty=false;await loadInfo();openScript(info.scripts.some(s=>s.name==name)?name:'')}
function toggleText(){const t=$('txt'),show=t.style.display!='block';if(!show&&textEdited)applyText();t.style.display=show?'block':'none';$('txtBar').style.display=show?'flex':'none';if(show){t.value=toText(prog);textEdited=false}}
function applyText(){prog=fromText($('txt').value);target=prog;textEdited=false;changed();render()}

// ---------- Live-Daten ----------
async function pollLog(){try{const r=await fetch('/log?from='+from,{cache:'no-store'});from=+r.headers.get('X-Next');const t=await r.text();
 if(t){const o=$('log');const btm=o.scrollTop+o.clientHeight>=o.scrollHeight-30;o.textContent+=t;if(o.textContent.length>60000)o.textContent=o.textContent.slice(-40000);if(btm)o.scrollTop=o.scrollHeight}
 $('conn').textContent='verbunden';$('conn').className='pill ok'}catch(e){$('conn').textContent='getrennt';$('conn').className='pill bad'}setTimeout(pollLog,400)}
async function pollState(){try{const s=await(await fetch('/state',{cache:'no-store'})).json();$('busy').textContent=s.running?'läuft: '+s.running:'bereit';
 const v=s.vars||{};liveVars=v;$('vars').textContent=Object.keys(v).length?Object.entries(v).map(([k,x])=>k+' = '+x).join('\n'):'-'}catch(e){}setTimeout(pollState,600)}
window.onbeforeunload=()=>dirty?'':undefined;
pollLog();pollState();loadInfo().then(()=>openScript(info.scripts.length?info.scripts[0].name:'')).catch(()=>setTimeout(()=>location.reload(),3000));
</script></body></html>
)HTML";
