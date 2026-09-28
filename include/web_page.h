#pragma once

#include <pgmspace.h>

// The phone control page. Fully self-contained -- the phone has no internet
// while it is joined to the disc's access point, so no CDN fonts or scripts.
//
// It talks to three endpoints (see src/web.cpp):
//   GET /api/state              -> JSON of every setting plus live motor values
//   GET /api/set?key=value&...  -> change settings, answers with the new state
//   GET /api/reset              -> back to defaults

static const char WEB_PAGE[] PROGMEM = R"rawliteral(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<meta name="theme-color" content="#101216">
<title>Moire disc</title>
<style>
:root{
  --bg:#101216;--card:#1a1d23;--line:#2a2e36;--text:#e8eaed;--dim:#8b919c;
  --accent:#d9ff00;--warn:#ff7543;--ok:#5fd38d;--radius:14px;
}
*{box-sizing:border-box;-webkit-tap-highlight-color:transparent}
body{margin:0;background:var(--bg);color:var(--text);
  font:16px/1.4 system-ui,-apple-system,Segoe UI,Roboto,sans-serif;
  padding:16px 16px 40px;max-width:520px;margin-inline:auto}
header{display:flex;align-items:center;justify-content:space-between;margin:4px 2px 16px}
h1{font-size:22px;margin:0;font-weight:650;letter-spacing:.2px}
#conn{font-size:13px;color:var(--dim);display:flex;align-items:center;gap:6px}
#conn i{width:9px;height:9px;border-radius:50%;background:var(--warn);display:inline-block}
#conn.ok i{background:var(--ok)}
.card{background:var(--card);border:1px solid var(--line);border-radius:var(--radius);
  padding:16px;margin-bottom:16px}
.card h2{font-size:13px;text-transform:uppercase;letter-spacing:1.2px;color:var(--dim);
  margin:0 0 14px;font-weight:600}
.seg{display:flex;flex-wrap:wrap;gap:6px;margin-bottom:16px}
.seg button{flex:1 1 0;min-width:84px;padding:12px 8px;border-radius:10px;border:1px solid var(--line);
  background:transparent;color:var(--text);font:inherit;font-size:15px}
.seg button.on{background:var(--accent);border-color:var(--accent);color:#0b1220;font-weight:600}
.row{margin:14px 0}
.row label{display:flex;justify-content:space-between;font-size:14px;color:var(--dim);margin-bottom:6px}
.row label b{color:var(--text);font-weight:600;font-variant-numeric:tabular-nums}
input[type=range]{width:100%;height:36px;margin:0;background:transparent;-webkit-appearance:none;appearance:none}
input[type=range]::-webkit-slider-runnable-track{height:8px;border-radius:4px;background:var(--line)}
input[type=range]::-moz-range-track{height:8px;border-radius:4px;background:var(--line)}
input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;width:28px;height:28px;margin-top:-10px;
  border-radius:50%;background:var(--text);border:3px solid var(--bg)}
input[type=range]::-moz-range-thumb{width:24px;height:24px;border-radius:50%;background:var(--text);border:3px solid var(--bg)}
#hue::-webkit-slider-runnable-track{background:linear-gradient(90deg,red,#ff0,lime,cyan,blue,#f0f,red)}
#hue::-moz-range-track{background:linear-gradient(90deg,red,#ff0,lime,cyan,blue,#f0f,red)}
.btns{display:flex;gap:8px;margin-top:6px}
.btn{flex:1;padding:14px;border-radius:10px;border:1px solid var(--line);background:transparent;
  color:var(--text);font:inherit;font-weight:600}
.btn.stop{background:var(--warn);border-color:var(--warn);color:#1a0b08}
.meter{position:relative;height:10px;border-radius:5px;background:var(--line);overflow:hidden;margin-top:4px}
.meter span{position:absolute;top:0;bottom:0;background:var(--accent);transition:all .2s linear}
.meter:after{content:"";position:absolute;left:50%;top:0;bottom:0;width:2px;background:var(--bg)}
.switch{display:flex;align-items:center;justify-content:space-between;margin:14px 0;font-size:15px}
.switch input{appearance:none;-webkit-appearance:none;width:50px;height:30px;border-radius:15px;
  background:var(--line);position:relative;transition:background .2s;margin:0}
.switch input:after{content:"";position:absolute;left:3px;top:3px;width:24px;height:24px;border-radius:50%;
  background:var(--text);transition:left .2s}
.switch input:checked{background:var(--accent)}
.switch input:checked:after{left:23px;background:#0b1220}
#swatch{width:18px;height:18px;border-radius:50%;display:inline-block;vertical-align:middle;margin-left:8px;border:1px solid var(--line)}
.hide{display:none}
.disabled{opacity:.4;pointer-events:none}
footer{text-align:center;margin-top:8px}
footer button{background:none;border:0;color:var(--dim);font:inherit;font-size:14px;text-decoration:underline;padding:10px}
</style>
</head>
<body>
<header>
  <h1>Moire disc</h1>
  <div id="conn"><i></i><span>connecting</span></div>
</header>

<section class="card">
  <h2>Motor</h2>
  <div class="seg" data-key="motorMode">
    <button data-v="0">Manual</button>
    <button data-v="1">Sweep</button>
  </div>

  <div id="manual">
    <div class="row"><label>Speed <b id="speedV"></b></label>
      <input type="range" id="speed" data-key="speed" min="-100" max="100" step="1" data-snap="3"></div>
    <div class="btns">
      <button class="btn" id="reverse">Reverse</button>
    </div>
  </div>

  <div id="sweep">
    <div class="row"><label>Lowest speed <b id="sweepMinV"></b></label>
      <input type="range" id="sweepMin" data-key="sweepMin" min="-100" max="100" step="1" data-snap="3"></div>
    <div class="row"><label>Highest speed <b id="sweepMaxV"></b></label>
      <input type="range" id="sweepMax" data-key="sweepMax" min="-100" max="100" step="1" data-snap="3"></div>
    <div class="row"><label>Cycle time <b id="sweepPeriodV"></b></label>
      <input type="range" id="sweepPeriod" data-key="sweepPeriod" min="4" max="120" step="1"></div>
  </div>

  <div class="row"><label>Actual speed <b id="actualV"></b></label>
    <div class="meter"><span id="actualBar"></span></div></div>
  <div class="btns"><button class="btn stop" id="stop">Stop</button></div>
</section>

<section class="card">
  <h2>Lights</h2>
  <div class="seg" data-key="effect">
    <button data-v="0">Gradient</button>
    <button data-v="1">Rainbow</button>
    <button data-v="2">Solid</button>
    <button data-v="3">Aurora</button>
    <button data-v="4">Off</button>
  </div>

  <div class="row" data-fx="0 2 3"><label><span>Colour<span id="swatch"></span></span><b id="hueV"></b></label>
    <input type="range" id="hue" data-key="hue" min="0" max="359" step="1"></div>
  <div class="row" data-fx="0 3"><label>Colour spread <b id="spreadV"></b></label>
    <input type="range" id="spread" data-key="spread" min="0" max="100" step="1"></div>
  <div class="row" data-fx="0 1 3" id="rotRow"><label>Rotation <b id="rotationV"></b></label>
    <input type="range" id="rotation" data-key="rotation" min="-100" max="100" step="1" data-snap="3"></div>
  <label class="switch" data-fx="0 1 3">Turn with the motor
    <input type="checkbox" id="syncMotor"></label>
  <div class="row" data-fx="0 1 2 3"><label>Brightness <b id="brightnessV"></b></label>
    <input type="range" id="brightness" data-key="brightness" min="0" max="100" step="1"></div>
</section>

<footer>
  <button id="reset">Reset to defaults</button>
</footer>

<script>
const $ = id => document.getElementById(id);
let S = {};
const touched = {};         // key -> time of last local change
const queue = {};           // params waiting to be sent
let timer = null, lastOk = 0;

const fmt = {
  speed: v => (v > 0 ? '+' : '') + v + ' %',
  sweepMin: v => fmt.speed(v), sweepMax: v => fmt.speed(v),
  sweepPeriod: v => v + ' s',
  hue: v => v + '°',
  spread: v => v + ' %',
  rotation: v => fmt.speed(v),
  brightness: v => v + ' %',
};

// Batches changes and sends at most ~10 per second while a slider is dragged.
function send(params, now) {
  Object.assign(queue, params);
  for (const k in params) touched[k] = Date.now();
  if (now) flush(); else if (!timer) timer = setTimeout(flush, 100);
}
function flush() {
  clearTimeout(timer); timer = null;
  const q = new URLSearchParams(queue).toString();
  for (const k in queue) delete queue[k];
  if (q) api('/api/set?' + q);
}
async function api(url) {
  try {
    const r = await fetch(url, {cache: 'no-store'});
    apply(await r.json());
    lastOk = Date.now();
  } catch (e) {}
  showConn();
}
function showConn() {
  const ok = Date.now() - lastOk < 4000;
  $('conn').className = ok ? 'ok' : '';
  $('conn').lastChild.textContent = ok ? (S.clients + (S.clients == 1 ? ' phone' : ' phones')) : 'offline';
}

// Takes a state from the disc. Keys the user touched in the last moment are
// left alone so a poll can't yank a slider out from under a finger.
function apply(s) {
  const fresh = k => !(touched[k] && Date.now() - touched[k] < 1500);
  for (const k in s) if (fresh(k)) S[k] = s[k];
  S.actual = s.actual; S.clients = s.clients;
  render();
}

function render() {
  document.querySelectorAll('input[type=range]').forEach(el => {
    const k = el.dataset.key;
    el.value = S[k];
    $(k + 'V').textContent = fmt[k](+el.value);
  });
  document.querySelectorAll('.seg').forEach(seg => {
    seg.querySelectorAll('button').forEach(b =>
      b.classList.toggle('on', +b.dataset.v === S[seg.dataset.key]));
  });
  $('manual').classList.toggle('hide', S.motorMode !== 0);
  $('sweep').classList.toggle('hide', S.motorMode !== 1);
  document.querySelectorAll('[data-fx]').forEach(el =>
    el.classList.toggle('hide', !el.dataset.fx.split(' ').includes(String(S.effect))));
  $('syncMotor').checked = !!S.syncMotor;
  $('rotRow').classList.toggle('disabled', !!S.syncMotor);
  $('swatch').style.background = 'hsl(' + $('hue').value + ',100%,50%)';

  const a = S.actual || 0;
  $('actualV').textContent = fmt.speed(Math.round(a));
  const bar = $('actualBar');
  bar.style.left = (a >= 0 ? 50 : 50 + a / 2) + '%';
  bar.style.width = Math.abs(a) / 2 + '%';
}

document.querySelectorAll('input[type=range]').forEach(el => {
  const k = el.dataset.key, snap = +(el.dataset.snap || 0);
  const val = () => { let v = +el.value; if (Math.abs(v) <= snap) { v = 0; el.value = 0; } return v; };
  el.addEventListener('input', () => { S[k] = val(); render(); send({[k]: S[k]}); });
  el.addEventListener('change', () => { S[k] = val(); render(); send({[k]: S[k]}, true); });
});
document.querySelectorAll('.seg').forEach(seg => {
  seg.addEventListener('click', e => {
    const b = e.target.closest('button'); if (!b) return;
    S[seg.dataset.key] = +b.dataset.v; render(); send({[seg.dataset.key]: +b.dataset.v}, true);
  });
});
$('syncMotor').addEventListener('change', e => {
  S.syncMotor = e.target.checked; render(); send({syncMotor: S.syncMotor ? 1 : 0}, true);
});
$('reverse').addEventListener('click', () => {
  S.speed = -S.speed; render(); send({speed: S.speed}, true);
});
$('stop').addEventListener('click', () => {
  S.motorMode = 0; S.speed = 0; render(); send({motorMode: 0, speed: 0}, true);
});
$('reset').addEventListener('click', () => {
  if (confirm('Reset all motor and light settings to defaults?')) {
    for (const k in touched) delete touched[k];
    api('/api/reset');
  }
});

api('/api/state');
setInterval(() => api('/api/state'), 1500);
</script>
</body>
</html>
)rawliteral";
