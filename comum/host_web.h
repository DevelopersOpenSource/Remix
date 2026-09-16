#pragma once
// Host: a pagina que o celular abre (app web), o manifesto/service worker da PWA e
// a pagina "conectar" que o usuario manda pelo WhatsApp. Tudo embutido no executavel:
// nada e lido do disco, nada e montado a partir de dados do usuario sem escape.
// Regras de seguranca da pagina: CSP sem inline script, todo texto entra por
// textContent (nunca innerHTML com dados), POST sempre com o cabecalho X-Remix.
#include <string>

namespace hostweb {

// ---------------------------------------------------------------- index --
// @ACCENT@ e trocado pela cor do tema (#rrggbb) e @VER@ pela versao (o navegador
// guarda app.css/app.js por um dia; a versao na URL forca baixar de novo apos atualizar).
static const char* INDEX_HTML = R"~~~(<!doctype html>
<html lang="pt-BR">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">
<meta name="theme-color" content="#121212">
<title>Remix Player</title>
<link rel="manifest" href="/manifest.webmanifest">
<link rel="icon" href="/icon.png">
<link rel="apple-touch-icon" href="/icon.png">
<link rel="stylesheet" href="/app.css?v=@VER@">
<style>:root{--accent:@ACCENT@}</style>
</head>
<body>
<div id="pair" hidden>
  <div class="card">
    <h1>Remix Player</h1>
    <p class="dim">Conectar ao PC <b id="pairHost"></b></p>
    <label>Nome deste aparelho <input id="pairName" maxlength="40" placeholder="Meu celular" autocomplete="off"></label>
    <label>PIN do PC <input id="pairPin" inputmode="numeric" pattern="[0-9]*" maxlength="12" placeholder="4 a 12 números" autocomplete="one-time-code"></label>
    <button id="pairBtn" class="primary">Conectar</button>
    <p id="pairMsg" class="dim"></p>
  </div>
</div>
<div id="main" hidden>
  <header>
    <div class="brand">REMIX <span id="hostName" class="dim"></span></div>
    <button id="btnSair" class="ghost" title="Desconectar">sair</button>
  </header>
  <nav>
    <button data-tab="lib" class="on">Músicas</button>
    <button data-tab="pc">Playlists do PC</button>
    <button data-tab="mine">Minhas</button>
  </nav>
  <div id="search"><input id="q" placeholder="Buscar música ou artista" autocomplete="off"></div>
  <div id="toolbar" hidden><button id="btnBack" class="ghost">‹ voltar</button><span id="plTitle"></span><span class="grow"></span><button id="btnPlayAll" class="small">tocar tudo</button><button id="btnRename" class="small" hidden>renomear</button><button id="btnDelete" class="small danger" hidden>apagar</button></div>
  <div id="list"></div>
  <div id="empty" class="dim" hidden></div>
  <footer id="player">
    <div class="np"><img id="npCover" alt="" hidden><div class="npText"><div id="npTitle">Nada tocando</div><div id="npArtist" class="dim"></div></div></div>
    <input id="seek" type="range" min="0" max="1000" value="0">
    <div class="times"><span id="tPos">0:00</span><span id="tLen">0:00</span></div>
    <div class="controls">
      <button id="btnShuffle" class="ghost" title="Aleatório">⇄</button>
      <button id="btnPrev" class="ghost" title="Anterior">⏮</button>
      <button id="btnPlay" class="primary round" title="Tocar / pausar">▶</button>
      <button id="btnNext" class="ghost" title="Próxima">⏭</button>
      <button id="btnRepeat" class="ghost" title="Repetir">⟳</button>
    </div>
    <audio id="audio" preload="metadata"></audio>
  </footer>
</div>
<div id="picker" hidden><div class="sheet"><h2>Adicionar em</h2><div id="pickList"></div><button id="pickNew" class="small">+ nova playlist</button><button id="pickCancel" class="ghost">cancelar</button></div></div>
<div id="toast" hidden></div>
<script src="/app.js?v=@VER@"></script>
</body>
</html>
)~~~";

static const char* APP_CSS = R"~~~(
*{box-sizing:border-box}
[hidden]{display:none!important}
html,body{margin:0;background:#121212;color:#fff;font:15px/1.4 system-ui,-apple-system,Segoe UI,Roboto,sans-serif;-webkit-tap-highlight-color:transparent}
body{padding-bottom:env(safe-area-inset-bottom)}
button{font:inherit;color:#fff;background:#2a2a2a;border:0;border-radius:6px;padding:10px 14px;cursor:pointer}
button.primary{background:var(--accent);color:#121212;font-weight:700}
button.ghost{background:transparent;color:#b3b3b3}
button.small{padding:6px 10px;font-size:13px}
button.danger{color:#ff8a94}
button.round{width:56px;height:56px;border-radius:50%;font-size:22px;padding:0}
.dim{color:#b3b3b3}
.grow{flex:1}
#pair{min-height:100vh;display:flex;align-items:center;justify-content:center;padding:24px}
.card{background:#1c1c1c;border-radius:10px;padding:24px;width:100%;max-width:380px}
.card h1{margin:0 0 4px;font-size:22px;letter-spacing:.5px}
.card label{display:block;margin:14px 0 0;font-size:13px;color:#b3b3b3}
.card input{display:block;width:100%;margin-top:6px;padding:12px;border-radius:6px;border:1px solid #333;background:#121212;color:#fff;font-size:18px}
.card button{width:100%;margin-top:18px;padding:14px}
header{display:flex;align-items:center;padding:14px 16px 6px;background:#0a0a0a;position:sticky;top:0;z-index:2}
.brand{font-weight:800;letter-spacing:1px;color:var(--accent);flex:1}
.brand span{font-weight:400;letter-spacing:0;margin-left:8px;font-size:13px}
nav{display:flex;gap:6px;padding:6px 12px;background:#0a0a0a;border-bottom:1px solid #282828;position:sticky;top:46px;z-index:2}
nav button{flex:1;background:#1c1c1c;color:#b3b3b3;padding:9px 6px;font-size:13px;font-weight:600;text-transform:uppercase;letter-spacing:.3px}
nav button.on{background:#2a2a2a;color:#fff;box-shadow:inset 0 -2px 0 var(--accent)}
#search{padding:10px 12px 0}
#search input{width:100%;padding:11px 14px;border-radius:6px;border:0;background:#1c1c1c;color:#fff;font-size:15px}
#toolbar{display:flex;align-items:center;gap:8px;padding:10px 12px 0}
#plTitle{font-weight:700}
#list{padding:8px 12px 190px}
.row{display:flex;align-items:center;gap:12px;padding:8px;border-radius:6px;background:#181818;margin-bottom:6px}
.row.cur{background:#2a2a2a}
.row .cv{width:46px;height:46px;border-radius:4px;background:#0a0a0a;object-fit:cover;flex:none}
.row .tx{flex:1;min-width:0}
.row .t{font-weight:600;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
.row .a{font-size:13px;color:#b3b3b3;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
.row .d{font-size:12px;color:#8a8a8a}
.row button{padding:8px 10px}
.pl{display:flex;align-items:center;gap:12px;padding:14px;border-radius:6px;background:#181818;margin-bottom:8px}
.pl .n{font-weight:700;flex:1}
.pl .c{font-size:13px;color:#b3b3b3}
#empty{padding:40px 20px;text-align:center}
footer{position:fixed;left:0;right:0;bottom:0;background:#181818;border-top:1px solid #282828;padding:10px 14px calc(10px + env(safe-area-inset-bottom))}
.np{display:flex;align-items:center;gap:10px;margin-bottom:4px}
.np img{width:44px;height:44px;border-radius:4px;object-fit:cover}
.npText{min-width:0}
#npTitle{font-weight:700;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
#npArtist{font-size:13px;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
#seek{width:100%;accent-color:var(--accent);margin:4px 0 0}
.times{display:flex;justify-content:space-between;font-size:11px;color:#8a8a8a}
.controls{display:flex;justify-content:center;align-items:center;gap:18px;margin-top:2px}
.controls .ghost{font-size:22px;padding:6px 10px}
.controls .ghost.on{color:var(--accent)}
#picker{position:fixed;inset:0;background:rgba(0,0,0,.6);display:flex;align-items:flex-end;z-index:5}
.sheet{background:#1c1c1c;width:100%;border-radius:12px 12px 0 0;padding:16px 16px calc(16px + env(safe-area-inset-bottom));max-height:70vh;overflow:auto}
.sheet h2{margin:0 0 10px;font-size:16px}
.sheet .pl{cursor:pointer}
.sheet button{margin-top:8px;margin-right:8px}
#toast{position:fixed;left:50%;bottom:200px;transform:translateX(-50%);background:#2a2a2a;padding:10px 16px;border-radius:6px;font-size:14px;z-index:9;max-width:90vw}
)~~~";

static const char* APP_JS = R"~~~('use strict';
const $=s=>document.querySelector(s);
const el=(tag,cls,text)=>{const e=document.createElement(tag);if(cls)e.className=cls;if(text!=null)e.textContent=text;return e;};
const S={tab:'lib',tracks:[],byId:new Map(),pc:[],mine:[],q:'',view:null,queue:[],idx:-1,shuffle:false,repeat:false,host:'',dev:''};
function toast(m){const t=$('#toast');t.textContent=m;t.hidden=false;clearTimeout(toast.h);toast.h=setTimeout(()=>t.hidden=true,2500);}
async function api(path,body){
  const o={method:body?'POST':'GET',credentials:'same-origin',headers:{}};
  if(body){o.headers['Content-Type']='application/json';o.headers['X-Remix']='1';o.body=JSON.stringify(body);}
  const r=await fetch(path,o);
  if(r.status===401){showPair();throw new Error('nao pareado');}
  if(r.status===429){toast('Muitas tentativas. Espere um pouco.');throw new Error('429');}
  const j=await r.json().catch(()=>({}));
  if(!r.ok){throw new Error(j.erro||('erro '+r.status));}
  return j;
}
function fmt(s){s=Math.max(0,Math.floor(s||0));return Math.floor(s/60)+':'+String(s%60).padStart(2,'0');}
// ---------------- pareamento ----------------
function showPair(){$('#main').hidden=true;$('#pair').hidden=false;}
async function pair(){
  const nome=$('#pairName').value.trim()||'Meu celular', pin=$('#pairPin').value.trim();
  if(!/^[0-9]{4,12}$/.test(pin)){$('#pairMsg').textContent='PIN: de 4 a 12 números.';return;}
  $('#pairBtn').disabled=true;$('#pairMsg').textContent='Pedindo permissão ao PC...';
  try{
    const r=await api('/api/parear',{pin,nome});
    localStorage.setItem('remix_nome',nome);
    const t0=Date.now();
    while(Date.now()-t0<180000){
      await new Promise(r=>setTimeout(r,2000));
      const e=await fetch('/api/parear/estado?req='+encodeURIComponent(r.req),{credentials:'same-origin'}).then(x=>x.json());
      if(e.estado==='aceito'){$('#pairMsg').textContent='Aceito! Abrindo...';location.reload();return;}
      if(e.estado==='recusado'){$('#pairMsg').textContent='O PC recusou.';break;}
      if(e.estado==='expirado'){$('#pairMsg').textContent='O pedido expirou. Tente de novo.';break;}
      $('#pairMsg').textContent='Aguardando o PC aceitar... (olhe a tela do Remix)';
    }
  }catch(err){$('#pairMsg').textContent=err.message==='nao pareado'?'PIN errado.':err.message;}
  $('#pairBtn').disabled=false;
}
// ---------------- dados ----------------
async function load(){
  const est=await api('/api/estado');
  S.host=est.host;S.dev=est.dispositivo;$('#hostName').textContent=est.host;
  const b=await api('/api/biblioteca');
  S.tracks=b.faixas;S.byId=new Map(S.tracks.map(t=>[t.id,t]));
  await loadPls();
  $('#pair').hidden=true;$('#main').hidden=false;
  render();
}
async function loadPls(){const p=await api('/api/playlists');S.pc=p.pc;S.mine=p.minhas;}
// ---------------- listas ----------------
function tracksOf(pl){return pl.ids.map(id=>S.byId.get(id)).filter(Boolean);}
function filtered(list){const q=S.q.toLowerCase();return q?list.filter(t=>(t.t+' '+t.a).toLowerCase().includes(q)):list;}
function render(){
  document.querySelectorAll('nav button').forEach(b=>b.classList.toggle('on',b.dataset.tab===S.tab));
  const list=$('#list'),tb=$('#toolbar'),empty=$('#empty');list.textContent='';empty.hidden=true;tb.hidden=true;
  $('#search').hidden=false;
  if(S.view){ // playlist aberta
    tb.hidden=false;$('#plTitle').textContent=S.view.nome;
    const mine=S.view.mine;$('#btnRename').hidden=!mine;$('#btnDelete').hidden=!mine;
    const ts=filtered(tracksOf(S.view));
    if(!ts.length){empty.textContent=mine?'Playlist vazia: use o + nas músicas.':'Playlist vazia.';empty.hidden=false;}
    ts.forEach((t,i)=>list.appendChild(row(t,ts,i,mine?'−':'+')));
    return;
  }
  if(S.tab==='lib'){
    const ts=filtered(S.tracks);
    if(!ts.length){empty.textContent=S.tracks.length?'Nada encontrado.':'O PC não tem músicas na biblioteca.';empty.hidden=false;}
    ts.forEach((t,i)=>list.appendChild(row(t,ts,i,'+')));
  } else if(S.tab==='pc'){
    if(!S.pc.length){empty.textContent='O PC ainda não hosteou nenhuma playlist para este aparelho.';empty.hidden=false;}
    S.pc.forEach(p=>list.appendChild(plCard(p,false)));
  } else {
    const nb=el('button','small','+ nova playlist');nb.onclick=newMine;list.appendChild(nb);
    if(!S.mine.length){empty.textContent='Suas playlists ficam aqui (só neste aparelho; o PC consegue ver).';empty.hidden=false;}
    S.mine.forEach(p=>list.appendChild(plCard(p,true)));
  }
}
function row(t,ctx,i,btnLabel){
  const r=el('div','row'+(S.queue[S.idx]&&S.queue[S.idx].id===t.id?' cur':''));
  if(t.c){const img=el('img','cv');img.loading='lazy';img.src='/api/capa/'+encodeURIComponent(t.id);img.alt='';r.appendChild(img);} else r.appendChild(el('div','cv'));
  const tx=el('div','tx');tx.appendChild(el('div','t',t.t));tx.appendChild(el('div','a',t.a||'—'));r.appendChild(tx);
  r.appendChild(el('div','d',t.d?fmt(t.d):''));
  const b=el('button','small',btnLabel);
  b.onclick=e=>{e.stopPropagation();if(btnLabel==='+')pick(t);else removeFromMine(S.view,t);};
  r.appendChild(b);
  r.onclick=()=>playList(ctx,i);
  return r;
}
function plCard(p,mine){
  const c=el('div','pl');c.appendChild(el('div','n',p.nome));c.appendChild(el('div','c',p.ids.length+' faixa'+(p.ids.length===1?'':'s')));
  const b=el('button','small','▶');b.onclick=e=>{e.stopPropagation();const ts=tracksOf(p);if(ts.length)playList(ts,0);};c.appendChild(b);
  c.onclick=()=>{S.view={...p,mine};S.q='';$('#q').value='';render();window.scrollTo(0,0);};
  return c;
}
// ---------------- minhas playlists ----------------
async function newMine(){const nome=prompt('Nome da playlist');if(!nome)return;try{await api('/api/minhas',{acao:'criar',nome});await loadPls();render();}catch(e){toast(e.message);}}
function pick(t){
  const pl=$('#pickList');pl.textContent='';
  S.mine.forEach(p=>{const c=el('div','pl');c.appendChild(el('div','n',p.nome));c.appendChild(el('div','c',p.ids.length+''));c.onclick=async()=>{try{await api('/api/minhas',{acao:'add',slug:p.slug,id:t.id});await loadPls();toast('Adicionada em '+p.nome);}catch(e){toast(e.message);}$('#picker').hidden=true;render();};pl.appendChild(c);});
  $('#picker').hidden=false;
  $('#pickNew').onclick=async()=>{const nome=prompt('Nome da playlist');if(!nome)return;try{const r=await api('/api/minhas',{acao:'criar',nome});await api('/api/minhas',{acao:'add',slug:r.slug,id:t.id});await loadPls();toast('Criada e adicionada');}catch(e){toast(e.message);}$('#picker').hidden=true;render();};
}
async function removeFromMine(p,t){try{await api('/api/minhas',{acao:'remover',slug:p.slug,id:t.id});await loadPls();const np=S.mine.find(x=>x.slug===p.slug);if(np)S.view={...np,mine:true};render();}catch(e){toast(e.message);}}
// ---------------- player ----------------
const A=$('#audio');
function playList(list,i){S.queue=list.slice();S.idx=i;playIdx();}
function playIdx(){
  const t=S.queue[S.idx];if(!t)return;
  A.src='/api/faixa/'+encodeURIComponent(t.id);A.play().catch(()=>{});
  $('#npTitle').textContent=t.t;$('#npArtist').textContent=t.a||'';
  const img=$('#npCover');if(t.c){img.src='/api/capa/'+encodeURIComponent(t.id);img.hidden=false;}else img.hidden=true;
  if('mediaSession' in navigator){navigator.mediaSession.metadata=new MediaMetadata({title:t.t,artist:t.a||'',album:S.host,artwork:t.c?[{src:'/api/capa/'+encodeURIComponent(t.id),sizes:'512x512',type:'image/jpeg'}]:[]});}
  document.querySelectorAll('.row').forEach(r=>r.classList.remove('cur'));render();
}
function next(auto){
  if(!S.queue.length)return;
  if(S.repeat&&auto){playIdx();return;}
  if(S.shuffle){let n=Math.floor(Math.random()*S.queue.length);if(S.queue.length>1&&n===S.idx)n=(n+1)%S.queue.length;S.idx=n;}
  else{S.idx++;if(S.idx>=S.queue.length){if(!auto){S.idx=0;}else{S.idx=S.queue.length-1;return;}}}
  playIdx();
}
function prev(){if(!S.queue.length)return;if(A.currentTime>3){A.currentTime=0;return;}S.idx=(S.idx-1+S.queue.length)%S.queue.length;playIdx();}
A.addEventListener('ended',()=>next(true));
A.addEventListener('timeupdate',()=>{if(!seeking&&A.duration){$('#seek').value=Math.floor(A.currentTime/A.duration*1000);$('#tPos').textContent=fmt(A.currentTime);$('#tLen').textContent=fmt(A.duration);}});
A.addEventListener('play',()=>$('#btnPlay').textContent='❚❚');
A.addEventListener('pause',()=>$('#btnPlay').textContent='▶');
let seeking=false;
$('#seek').addEventListener('input',()=>{seeking=true;});
$('#seek').addEventListener('change',e=>{seeking=false;if(A.duration)A.currentTime=e.target.value/1000*A.duration;});
$('#btnPlay').onclick=()=>{if(!S.queue.length){const ts=filtered(S.tracks);if(ts.length)playList(ts,0);return;}if(A.paused)A.play();else A.pause();};
$('#btnNext').onclick=()=>next(false);$('#btnPrev').onclick=prev;
$('#btnShuffle').onclick=e=>{S.shuffle=!S.shuffle;e.currentTarget.classList.toggle('on',S.shuffle);};
$('#btnRepeat').onclick=e=>{S.repeat=!S.repeat;e.currentTarget.classList.toggle('on',S.repeat);};
if('mediaSession' in navigator){navigator.mediaSession.setActionHandler('play',()=>A.play());navigator.mediaSession.setActionHandler('pause',()=>A.pause());navigator.mediaSession.setActionHandler('nexttrack',()=>next(false));navigator.mediaSession.setActionHandler('previoustrack',prev);}
// ---------------- navegacao ----------------
document.querySelectorAll('nav button').forEach(b=>b.onclick=()=>{S.tab=b.dataset.tab;S.view=null;render();});
$('#q').addEventListener('input',e=>{S.q=e.target.value;render();});
$('#btnBack').onclick=()=>{S.view=null;render();};
$('#btnPlayAll').onclick=()=>{const ts=tracksOf(S.view);if(ts.length)playList(ts,0);};
$('#btnRename').onclick=async()=>{const nome=prompt('Novo nome',S.view.nome);if(!nome)return;try{await api('/api/minhas',{acao:'renomear',slug:S.view.slug,nome});await loadPls();S.view={...S.mine.find(x=>x.slug===S.view.slug),mine:true};render();}catch(e){toast(e.message);}};
$('#btnDelete').onclick=async()=>{if(!confirm('Apagar a playlist "'+S.view.nome+'"?'))return;try{await api('/api/minhas',{acao:'apagar',slug:S.view.slug});await loadPls();S.view=null;render();}catch(e){toast(e.message);}};
$('#pickCancel').onclick=()=>$('#picker').hidden=true;
$('#btnSair').onclick=async()=>{if(!confirm('Desconectar este aparelho do PC?'))return;try{await api('/api/sair',{});}catch(e){}location.reload();};
$('#pairBtn').onclick=pair;
$('#pairPin').addEventListener('keydown',e=>{if(e.key==='Enter')pair();});
$('#pairName').value=localStorage.getItem('remix_nome')||'';
fetch('/api/ping').then(r=>r.json()).then(j=>{$('#pairHost').textContent=j.nome||'';}).catch(()=>{});
if('serviceWorker' in navigator){navigator.serviceWorker.register('/sw.js').catch(()=>{});}
load().catch(()=>showPair());
)~~~";

static const char* MANIFEST_JSON = R"~~~({"name":"Remix Player","short_name":"Remix","start_url":"/","scope":"/","display":"standalone","background_color":"#121212","theme_color":"#121212","icons":[{"src":"/icon.png","sizes":"256x256","type":"image/png"}]})~~~";

static const char* SW_JS = R"~~~(self.addEventListener('install',e=>self.skipWaiting());self.addEventListener('activate',e=>self.clients.claim());self.addEventListener('fetch',e=>{});)~~~";

// -------------------------------------------------- pagina "conectar" ----
// Arquivo que o usuario manda pelo WhatsApp. @NOME@ = nome do PC, @TUNEL@ = URL do
// tunel (ou vazio), @LANS@ = lista JSON de URLs da rede local. So links: sem PIN,
// sem IP publico, sem nada que entregue onde a pessoa esta.
static const char* CONNECT_HTML = R"~~~(<!doctype html>
<html lang="pt-BR"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<title>Conectar ao Remix</title>
<style>
body{margin:0;background:#121212;color:#fff;font:16px/1.5 system-ui,-apple-system,Segoe UI,Roboto,sans-serif;display:flex;min-height:100vh;align-items:center;justify-content:center;padding:20px;box-sizing:border-box}
.c{background:#1c1c1c;border-radius:10px;padding:24px;width:100%;max-width:420px}
h1{margin:0 0 4px;font-size:22px;letter-spacing:.5px;color:#ACCENT}
p{color:#b3b3b3;margin:6px 0 14px}
a.b{display:block;text-align:center;background:#2a2a2a;color:#fff;text-decoration:none;padding:14px;border-radius:6px;margin:8px 0;font-weight:600}
a.b.p{background:#ACCENT;color:#121212}
small{color:#8a8a8a;display:block;margin-top:14px}
#st{color:#b3b3b3;font-size:14px}
</style></head><body><div class="c">
<h1>Remix Player</h1>
<p>Conectar ao PC <b>@NOME@</b></p>
<div id="st">Procurando o PC na sua rede...</div>
<div id="links"></div>
<small>Na primeira vez a página pede o PIN do PC e o PC precisa aceitar o aparelho. Se você está na mesma rede (Wi-Fi ou cabo do mesmo roteador), o link local é mais rápido; fora de casa use o link do túnel.</small>
</div>
<script>
(function(){
var tunel="@TUNEL@", lans=@LANS@;
var links=document.getElementById('links'), st=document.getElementById('st');
function add(url,label,primary){var a=document.createElement('a');a.className='b'+(primary?' p':'');a.href=url;a.textContent=label;links.appendChild(a);}
if(tunel)add(tunel,'Abrir pela internet (túnel seguro)',true);
lans.forEach(function(u){add(u,'Abrir na rede local ('+u.replace(/^https?:\/\//,'')+')',!tunel);});
if(!tunel&&!lans.length)st.textContent='O PC ainda não gerou nenhum link: abra o painel HOST no Remix.';
var done=false;
lans.forEach(function(u){
  var ctl=new AbortController();setTimeout(function(){ctl.abort();},1500);
  fetch(u+'/api/ping',{mode:'cors',signal:ctl.signal}).then(function(r){return r.json();}).then(function(j){
    if(!done&&j&&j.app==='remix'){done=true;st.textContent='PC encontrado na rede local. Abrindo...';location.href=u;}
  }).catch(function(){});
});
setTimeout(function(){if(!done)st.textContent=lans.length?'Não achei o PC na rede local: use um dos links.':'Escolha um link:';},2500);
})();
</script></body></html>
)~~~";

} // namespace hostweb
