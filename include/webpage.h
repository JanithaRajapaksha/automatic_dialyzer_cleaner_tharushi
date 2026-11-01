#ifndef WEBPAGE_H
#define WEBPAGE_H

#include <pgmspace.h>

const char webpage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Automatic Dialyzer Reprocessor</title>
<style>
  :root {
    --primary: #0284c7;
    --primary-dark: #0369a1;
    --bg: #f1f5f9;
    --card-bg: #ffffff;
    --text-main: #0f172a;
    --text-muted: #64748b;
    --success: #10b981;
    --warning: #f59e0b;
    --danger: #ef4444;
    --neutral: #334155;
    --border: #e2e8f0;
  }
  * { box-sizing: border-box; margin: 0; padding: 0; }
  body {
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
    background: var(--bg);
    color: var(--text-main);
    padding: 20px 14px;
    max-width: 800px;
    margin: 0 auto;
  }
  .header {
    text-align: center;
    margin-bottom: 16px;
  }
  .header h1 {
    font-size: 1.6rem;
    color: #0369a1;
    letter-spacing: -0.5px;
  }
  .header p {
    font-size: 0.88rem;
    color: var(--text-muted);
    margin-top: 2px;
  }

  /* Status Bar */
  .status-bar {
    background: #e2e8f0;
    border-radius: 8px;
    padding: 8px 14px;
    display: flex;
    justify-content: space-between;
    align-items: center;
    font-size: 0.85rem;
    font-weight: 500;
    margin-bottom: 16px;
  }
  .badge {
    padding: 4px 10px;
    border-radius: 9999px;
    font-size: 0.75rem;
    font-weight: 700;
    letter-spacing: 0.5px;
  }
  .badge-connected { background: #dcfce7; color: #166534; }
  .badge-disconnected { background: #fee2e2; color: #991b1b; }
  .badge-updating { background: #fef3c7; color: #92400e; }
  .btn-sync {
    padding: 3px 10px;
    font-size: 0.75rem;
    cursor: pointer;
    border-radius: 4px;
    border: 1px solid #cbd5e1;
    background: #fff;
    font-weight: 600;
  }

  /* Reprocessor Process Panel */
  .fsm-panel {
    background: var(--card-bg);
    border-radius: 12px;
    padding: 18px;
    box-shadow: 0 4px 6px -1px rgba(0,0,0,0.06), 0 2px 4px -2px rgba(0,0,0,0.04);
    border: 1px solid var(--border);
    margin-bottom: 18px;
    text-align: left;
  }
  .fsm-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 10px;
  }
  .fsm-title {
    font-size: 1.15rem;
    font-weight: 700;
    color: #1e293b;
  }
  .fsm-stage-badge {
    padding: 4px 10px;
    border-radius: 6px;
    font-size: 0.8rem;
    font-weight: 700;
    background: #e0f2fe;
    color: #0369a1;
  }
  .fsm-stage-badge.running { background: #dcfce7; color: #15803d; }
  .fsm-stage-badge.paused { background: #fef3c7; color: #b45309; }
  .fsm-stage-badge.aborted { background: #fee2e2; color: #b91c1c; }

  .stage-main {
    font-size: 1.25rem;
    font-weight: 800;
    color: #0f172a;
    margin-bottom: 4px;
  }
  .stage-sub {
    font-size: 0.88rem;
    color: var(--primary-dark);
    font-weight: 600;
    margin-bottom: 12px;
  }

  /* Progress Bar */
  .progress-wrap {
    background: #f1f5f9;
    border-radius: 8px;
    height: 12px;
    overflow: hidden;
    margin-bottom: 8px;
    border: 1px solid #e2e8f0;
  }
  .progress-bar {
    height: 100%;
    width: 0%;
    background: linear-gradient(90deg, #0284c7, #10b981);
    transition: width 0.3s ease;
  }
  .fsm-timing {
    display: flex;
    justify-content: space-between;
    font-size: 0.8rem;
    color: var(--text-muted);
    font-weight: 600;
    margin-bottom: 12px;
  }
  .actuators-active-ribbon {
    background: #f8fafc;
    border: 1px dashed #cbd5e1;
    border-radius: 6px;
    padding: 8px 12px;
    font-size: 0.82rem;
    font-weight: 600;
    color: #334155;
    margin-bottom: 16px;
  }

  /* Main Action Buttons */
  .fsm-controls {
    display: grid;
    grid-template-columns: 2fr 1fr 1fr;
    gap: 10px;
  }
  .btn-cmd {
    padding: 12px;
    font-size: 0.92rem;
    font-weight: 700;
    border-radius: 8px;
    border: none;
    cursor: pointer;
    color: white;
    transition: opacity 0.15s ease, transform 0.1s ease;
    text-align: center;
  }
  .btn-cmd:active { transform: scale(0.98); }
  .btn-start-auto { background: #059669; }
  .btn-pause { background: #d97706; }
  .btn-estop { background: #dc2626; }

  /* Stage Quick-Run Selector */
  .section-title {
    font-size: 0.95rem;
    font-weight: 700;
    color: #475569;
    text-align: left;
    margin: 16px 0 8px 0;
    text-transform: uppercase;
    letter-spacing: 0.5px;
  }
  .stage-grid {
    display: grid;
    grid-template-columns: repeat(4, 1fr);
    gap: 8px;
    margin-bottom: 18px;
  }
  .btn-stage {
    padding: 9px 6px;
    font-size: 0.78rem;
    font-weight: 600;
    background: #ffffff;
    border: 1px solid #cbd5e1;
    border-radius: 6px;
    cursor: pointer;
    color: #1e293b;
    transition: all 0.15s ease;
  }
  .btn-stage:hover {
    background: #f0f9ff;
    border-color: #38bdf8;
    color: #0369a1;
  }
  .btn-stage:active { transform: scale(0.97); }

  /* Actuator Cards */
  .actuator-grid {
    display: grid;
    grid-template-columns: repeat(2, 1fr);
    gap: 12px;
    margin-bottom: 20px;
  }
  .act-card {
    background: var(--card-bg);
    border-radius: 10px;
    padding: 14px;
    box-shadow: 0 2px 4px -1px rgba(0,0,0,0.05);
    border-left: 4px solid #cbd5e1;
    text-align: left;
    display: flex;
    flex-direction: column;
    justify-content: space-between;
    transition: border-color 0.2s ease;
  }
  .act-card.active { border-left-color: var(--success); }
  .act-top {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 6px;
  }
  .act-id {
    font-weight: 800;
    font-size: 0.95rem;
    color: #0f172a;
    display: flex;
    align-items: center;
    gap: 6px;
  }
  .act-code-badge {
    background: #e2e8f0;
    color: #334155;
    padding: 2px 6px;
    border-radius: 4px;
    font-size: 0.75rem;
    font-weight: 700;
  }
  .act-card.active .act-code-badge {
    background: #dcfce7;
    color: #166534;
  }
  .act-name {
    font-size: 0.8rem;
    color: var(--text-muted);
    margin-bottom: 10px;
    line-height: 1.25;
  }
  .act-pill {
    font-size: 0.72rem;
    font-weight: 700;
    padding: 2px 7px;
    border-radius: 4px;
    text-transform: uppercase;
  }
  .pill-on { background: #dcfce7; color: #15803d; }
  .pill-off { background: #f1f5f9; color: #64748b; }

  .btn-act-toggle {
    width: 100%;
    padding: 8px;
    border: none;
    border-radius: 6px;
    font-size: 0.85rem;
    font-weight: 600;
    color: white;
    cursor: pointer;
    transition: background-color 0.2s ease;
  }
  .btn-act-on { background: var(--success); }
  .btn-act-off { background: var(--neutral); }

  .bulk-actions {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 10px;
    margin-bottom: 24px;
  }
  .btn-bulk {
    padding: 10px;
    border-radius: 6px;
    border: none;
    color: white;
    font-weight: 600;
    font-size: 0.85rem;
    cursor: pointer;
  }
  .btn-bulk-on { background: #0d9488; }
  .btn-bulk-off { background: #64748b; }

  @media (max-width: 600px) {
    .stage-grid { grid-template-columns: repeat(2, 1fr); }
    .actuator-grid { grid-template-columns: 1fr; }
    .fsm-controls { grid-template-columns: 1fr; }
  }
</style>
</head>

<body>
  <!-- Header -->
  <div class="header">
    <h1>Automatic Dialyzer Reprocessor</h1>
    <p>10-Channel Fluidic & Pneumatic Sequence Controller (v1.1.0)</p>
  </div>

  <!-- Status Bar -->
  <div class="status-bar">
    <span>System Link: <span id="msg-text">Connecting to controller...</span></span>
    <div style="display:flex;align-items:center;gap:8px;">
      <button class="btn-sync" onclick="forceSync()">Force Sync</button>
      <span class="badge badge-updating" id="conn-badge">CONNECTING</span>
    </div>
  </div>

  <!-- Reprocessor Process Stage Controller Panel -->
  <div class="fsm-panel">
    <div class="fsm-header">
      <span class="fsm-title">Process Stage Controller</span>
      <span class="fsm-stage-badge" id="fsm-badge">STANDBY</span>
    </div>

    <div class="stage-main" id="stage-name-display">System Idle</div>
    <div class="stage-sub" id="substep-name-display">All actuators de-energized (Safe State)</div>

    <div class="progress-wrap">
      <div class="progress-bar" id="fsm-progress-bar"></div>
    </div>

    <div class="fsm-timing">
      <span id="stage-timer-display">Stage Time: 0s / 0s</span>
      <span id="cycle-timer-display">Total Cycle: 0s (0%)</span>
    </div>

    <div class="actuators-active-ribbon" id="actuators-ribbon">
      Active Actuators: None (All OFF)
    </div>

    <div class="fsm-controls">
      <button class="btn-cmd btn-start-auto" id="btn-auto" onclick="cmdStartAuto()">
        &#9654; START AUTO CYCLE (1-7)
      </button>
      <button class="btn-cmd btn-pause" id="btn-pause" onclick="cmdTogglePause()">
        PAUSE
      </button>
      <button class="btn-cmd btn-estop" onclick="cmdEStop()">
        &#9888; E-STOP / ABORT
      </button>
    </div>
  </div>

  <!-- Individual Stage Selector -->
  <div class="section-title">Manual Process Stage Execution</div>
  <div class="stage-grid">
    <button class="btn-stage" onclick="cmdRunStage(1)">Stage 1: RO Wash</button>
    <button class="btn-stage" onclick="cmdRunStage(2)">Stage 2: H2O2 Flush</button>
    <button class="btn-stage" onclick="cmdRunStage(3)">Stage 3: RO Rinse</button>
    <button class="btn-stage" onclick="cmdRunStage(4)">Stage 4: Leak Test</button>
    <button class="btn-stage" onclick="cmdRunStage(5)">Stage 5: Blood Fill</button>
    <button class="btn-stage" onclick="cmdRunStage(6)">Stage 6: FBV Measure</button>
    <button class="btn-stage" onclick="cmdRunStage(7)">Stage 7: PAA Prime</button>
    <button class="btn-stage" style="background:#fef2f2;border-color:#fca5a5;color:#dc2626;" onclick="cmdAbort()">Abort Cycle</button>
  </div>

  <!-- Actuator Grid -->
  <div class="section-title">Actuator Hardware Control (R1 – R10)</div>
  <div class="actuator-grid" id="actuator-cards"></div>

  <div class="bulk-actions">
    <button class="btn-bulk btn-bulk-on" onclick="setAllChannels(1)">MANUAL OVERRIDE: ALL ON</button>
    <button class="btn-bulk btn-bulk-off" onclick="setAllChannels(0)">MANUAL OVERRIDE: ALL OFF</button>
  </div>

<script>
// Actuator Hardware Definitions matching Firmware Relay Control Specification
const actuators = [
  { code: "P1",  name: "Pump 1: Primary RO Water Pump", desc: "Reverse Osmosis Feed Source" },
  { code: "P2",  name: "Pump 2: Chemical / H2O2 Flush Pump", desc: "Peroxide Cleaning Circuit" },
  { code: "P3",  name: "Pump 3: PAA / Renalin Sterilant Pump", desc: "Sterilant Storage Delivery" },
  { code: "S1",  name: "Valve S1: RO / Fluid Input Control", desc: "RO / Inlet Fluid Routing" },
  { code: "S2",  name: "Valve S2: Intermediate Port / Chemical Path", desc: "Disinfectant Channel Switch" },
  { code: "S3",  name: "Valve S3: Recirculation Control", desc: "FBV Measure / PAA Routing" },
  { code: "S4",  name: "Valve S4: Dialyzer Blood / Dialyzate Port", desc: "Dual Compartment Isolation" },
  { code: "S5",  name: "Valve S5: Pressure Test Isolation Valve", desc: "Dialyzate Pneumatic Air Lock" },
  { code: "S6",  name: "Valve S6: Exhaust / Bleed / Drain Valve", desc: "Depressurize & Waste Drain" },
  { code: "AIR", name: "Air Pump: Pneumatic Pressure Source", desc: "Leak Test Air Compressor" }
];

const total = 10;
const states = Array(total).fill(false);
let isCycleRunning = false;
let isCyclePaused = false;

// Build Actuator Cards
const cardsContainer = document.getElementById('actuator-cards');
for (let i = 0; i < total; i++) {
  const act = actuators[i];
  cardsContainer.innerHTML += `
    <div class="act-card" id="card${i}">
      <div class="act-top">
        <div class="act-id">
          <span class="act-code-badge">${act.code}</span>
          <span>Relay ${i + 1}</span>
        </div>
        <span class="act-pill pill-off" id="pill${i}">OFF</span>
      </div>
      <div class="act-name"><strong>${act.name}</strong><br><span style="font-size:0.75rem;color:#64748b;">${act.desc}</span></div>
      <button id="btn${i}" type="button" class="btn-act-toggle btn-act-off" onclick="toggleRelay(${i})">
        Turn ON
      </button>
    </div>`;
}

function updateConnectionStatus(status, text) {
  const badge = document.getElementById('conn-badge');
  const msg = document.getElementById('msg-text');
  msg.textContent = text;
  badge.className = 'badge badge-' + status;
  badge.textContent = status.toUpperCase();
}

function renderRelay(i) {
  const isOn = states[i];
  const card = document.getElementById('card' + i);
  const pill = document.getElementById('pill' + i);
  const btn = document.getElementById('btn' + i);

  if (isOn) {
    card.classList.add('active');
    pill.textContent = 'ACTIVE';
    pill.className = 'act-pill pill-on';
    btn.textContent = 'Turn OFF';
    btn.className = 'btn-act-toggle btn-act-on';
  } else {
    card.classList.remove('active');
    pill.textContent = 'OFF';
    pill.className = 'act-pill pill-off';
    btn.textContent = 'Turn ON';
    btn.className = 'btn-act-toggle btn-act-off';
  }
}

function extractStates(data) {
  if (!data) return null;
  if (Array.isArray(data)) return data;
  if (Array.isArray(data.states)) return data.states;
  if (typeof data === 'object') {
    const arr = [];
    for (let i = 0; i < total; i++) {
      if (typeof data[i] !== 'undefined') arr.push(Boolean(data[i]));
      else if (typeof data['' + i] !== 'undefined') arr.push(Boolean(data['' + i]));
      else return null;
    }
    return arr;
  }
  return null;
}

function applyStates(statesArr) {
  if (!statesArr) return;
  for (let i = 0; i < total && i < statesArr.length; i++) {
    states[i] = Boolean(statesArr[i]);
    renderRelay(i);
  }
}

function renderReprocessorTelemetry(rep) {
  if (!rep) return;

  isCycleRunning = !!rep.isRunning;
  isCyclePaused = !!rep.isPaused;

  const badge = document.getElementById('fsm-badge');
  const stageDisplay = document.getElementById('stage-name-display');
  const substepDisplay = document.getElementById('substep-name-display');
  const pbar = document.getElementById('fsm-progress-bar');
  const stageTimer = document.getElementById('stage-timer-display');
  const cycleTimer = document.getElementById('cycle-timer-display');
  const ribbon = document.getElementById('actuators-ribbon');
  const btnPause = document.getElementById('btn-pause');

  stageDisplay.textContent = rep.stageName || 'System Idle';
  substepDisplay.textContent = rep.subStepName || (rep.message || 'Ready');

  if (rep.isAborted) {
    badge.className = 'fsm-stage-badge aborted';
    badge.textContent = 'ABORTED';
  } else if (rep.isPaused) {
    badge.className = 'fsm-stage-badge paused';
    badge.textContent = 'PAUSED';
    btnPause.textContent = 'RESUME';
  } else if (rep.isRunning) {
    badge.className = 'fsm-stage-badge running';
    badge.textContent = rep.isAuto ? 'AUTO RUNNING' : 'STAGE RUNNING';
    btnPause.textContent = 'PAUSE';
  } else {
    badge.className = 'fsm-stage-badge';
    badge.textContent = 'IDLE';
    btnPause.textContent = 'PAUSE';
  }

  const pct = Math.min(100, Math.max(0, rep.progress || 0));
  pbar.style.width = pct + '%';

  if (rep.stageTotal > 0) {
    stageTimer.textContent = `Stage: ${rep.stageElapsed}s / ${rep.stageTotal}s`;
  } else {
    stageTimer.textContent = 'Stage: Standby';
  }

  if (rep.cycleTotal > 0) {
    cycleTimer.textContent = `Total: ${rep.cycleElapsed}s / ${rep.cycleTotal}s (${pct}%)`;
  } else {
    cycleTimer.textContent = 'Total: Idle (0%)';
  }

  ribbon.textContent = `Active Actuators: ${rep.actuators || 'None (All OFF)'}`;
}

// WebSocket Connection Management
let ws = null;
let wsReconnectTimer = null;
const sendQueue = [];

function connectWebSocket() {
  if (ws && (ws.readyState === WebSocket.CONNECTING || ws.readyState === WebSocket.OPEN)) return;

  const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
  const wsUrl = `${protocol}//${window.location.hostname}:81/`;

  updateConnectionStatus('updating', 'Connecting Realtime Link...');

  try {
    ws = new WebSocket(wsUrl);
  } catch (err) {
    updateConnectionStatus('disconnected', 'WS Init Failed');
    return;
  }

  ws.onopen = function() {
    updateConnectionStatus('connected', 'Live Realtime (WebSocket)');
    if (wsReconnectTimer) {
      clearTimeout(wsReconnectTimer);
      wsReconnectTimer = null;
    }
    while (sendQueue.length > 0) {
      const msg = sendQueue.shift();
      if (ws.readyState === WebSocket.OPEN) {
        ws.send(msg);
      }
    }
  };

  ws.onmessage = function(event) {
    try {
      const data = JSON.parse(event.data);

      if (data.type === 'reprocessor') {
        renderReprocessorTelemetry(data);
      } else {
        const arr = extractStates(data);
        if (arr) {
          applyStates(arr);
          updateConnectionStatus('connected', 'Live Realtime (WebSocket)');
        }
      }
    } catch (err) {
      console.error('WS Parse Error', err);
    }
  };

  ws.onerror = function() {};

  ws.onclose = function() {
    updateConnectionStatus('disconnected', 'Link Disconnected. Reconnecting...');
    if (!wsReconnectTimer) {
      wsReconnectTimer = setTimeout(connectWebSocket, 2000);
    }
  };
}

function sendWsOrHttp(wsMsg, httpUrl) {
  if (ws && ws.readyState === WebSocket.OPEN) {
    ws.send(wsMsg);
  } else if (ws && ws.readyState === WebSocket.CONNECTING) {
    sendQueue.push(wsMsg);
  } else {
    fetch(httpUrl)
      .then(res => res.json())
      .then(data => {
        if (data.type === 'reprocessor') renderReprocessorTelemetry(data);
        else {
          const arr = extractStates(data);
          if (arr) applyStates(arr);
        }
      })
      .catch(() => updateConnectionStatus('disconnected', 'HTTP Error'));
  }
}

// Reprocessor Process Commands
function cmdStartAuto() {
  sendWsOrHttp('START', '/reprocessor/start');
}

function cmdRunStage(stageId) {
  sendWsOrHttp(`STAGE:${stageId}`, `/reprocessor/stage?id=${stageId}`);
}

function cmdTogglePause() {
  if (isCyclePaused) {
    sendWsOrHttp('RESUME', '/reprocessor/resume');
  } else {
    sendWsOrHttp('PAUSE', '/reprocessor/pause');
  }
}

function cmdAbort() {
  sendWsOrHttp('ABORT', '/reprocessor/abort');
}

function cmdEStop() {
  // Immediate Emergency Stop
  sendWsOrHttp('ESTOP', '/reprocessor/estop');
  // Local immediate optimistic clear
  for (let i = 0; i < total; i++) {
    states[i] = false;
    renderRelay(i);
  }
}

// Manual Actuator Toggle
function toggleRelay(i) {
  const targetState = !states[i];
  states[i] = targetState;
  renderRelay(i);

  const payload = `SET:${i}:${targetState ? 1 : 0}`;
  if (ws && ws.readyState === WebSocket.OPEN) {
    ws.send(payload);
  } else {
    fetch(`/relay?ch=${i}&state=${targetState ? 1 : 0}`)
      .then(res => res.json())
      .then(data => {
        if (typeof data.state !== 'undefined') {
          states[i] = Boolean(data.state);
          renderRelay(i);
        }
      })
      .catch(() => {
        states[i] = !targetState;
        renderRelay(i);
      });
  }
}

function setAllChannels(state) {
  const target = Boolean(state);
  for (let i = 0; i < total; i++) {
    states[i] = target;
    renderRelay(i);
  }
  const payload = `ALL:${target ? 1 : 0}`;
  if (ws && ws.readyState === WebSocket.OPEN) {
    ws.send(payload);
  } else {
    fetch(`/all?state=${target ? 1 : 0}`).then(() => forceSync());
  }
}

async function forceSync() {
  if (ws && ws.readyState === WebSocket.OPEN) {
    ws.send('STATUS');
    return;
  }
  try {
    const resRelays = await fetch('/status');
    if (resRelays.ok) {
      const data = await resRelays.json();
      const arr = extractStates(data);
      if (arr) applyStates(arr);
    }
    const resRep = await fetch('/reprocessor/status');
    if (resRep.ok) {
      const data = await resRep.json();
      renderReprocessorTelemetry(data);
    }
  } catch (e) {}
}

// Startup
forceSync();
connectWebSocket();
setInterval(forceSync, 4000);
</script>
</body>
</html>
)rawliteral";

#endif // WEBPAGE_H
