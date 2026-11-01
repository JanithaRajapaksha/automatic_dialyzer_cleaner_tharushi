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

  .live-dot {
    width: 8px;
    height: 8px;
    background-color: #10b981;
    border-radius: 50%;
    display: inline-block;
    box-shadow: 0 0 0 0 rgba(16, 185, 129, 0.7);
    animation: pulse-dot 1.5s infinite;
  }
  @keyframes pulse-dot {
    0% { transform: scale(0.95); box-shadow: 0 0 0 0 rgba(16, 185, 129, 0.7); }
    70% { transform: scale(1); box-shadow: 0 0 0 6px rgba(16, 185, 129, 0); }
    100% { transform: scale(0.95); box-shadow: 0 0 0 0 rgba(16, 185, 129, 0); }
  }

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

  <!-- HX710B Air Pressure Sensor Card with Real-Time Waveform -->
  <div class="fsm-panel" style="margin-bottom:18px;border-left:5px solid #0284c7;">
    <div class="fsm-header">
      <div style="display:flex;align-items:center;gap:8px;">
        <span class="fsm-title">&#9881; Pneumatic Air Pressure (HX710B)</span>
        <span style="font-size:0.75rem;color:#64748b;font-weight:600;">(GPIO 32 DOUT, GPIO 14 SCLK)</span>
      </div>
      <div style="display:flex;align-items:center;gap:6px;">
        <span class="live-dot" id="p-live-dot" title="Live 10Hz Stream"></span>
        <span class="badge badge-connected" id="sensor-badge">ONLINE</span>
      </div>
    </div>

    <div style="display:flex;justify-content:space-between;align-items:flex-end;margin-top:6px;flex-wrap:wrap;gap:12px;">
      <div>
        <div style="font-size:2.5rem;font-weight:800;color:#0369a1;line-height:1;letter-spacing:-1px;">
          <span id="p-mmhg">0.0</span> <span style="font-size:1.1rem;color:#64748b;font-weight:600;">mmHg</span>
        </div>
        <div style="font-size:0.95rem;color:#334155;font-weight:600;margin-top:6px;">
          <span id="p-kpa">0.00</span> kPa &nbsp;|&nbsp; <span id="p-psi">0.00</span> psi
        </div>
        <div style="font-size:0.75rem;color:#64748b;margin-top:4px;">
          Kalman Stabilized &nbsp;|&nbsp; Raw: <span id="p-raw-mmhg">0.0</span> mmHg &nbsp;|&nbsp; Gain: <span id="p-gain">0.00</span>
        </div>
      </div>

      <!-- Live Statistical Badges -->
      <div style="display:flex;gap:12px;background:#f8fafc;padding:8px 12px;border-radius:8px;border:1px solid #e2e8f0;font-size:0.78rem;">
        <div><span style="color:#64748b;">Min:</span> <strong id="p-min" style="color:#0f172a;">--</strong> mmHg</div>
        <div><span style="color:#64748b;">Max:</span> <strong id="p-max" style="color:#0f172a;">--</strong> mmHg</div>
        <div><span style="color:#64748b;">&Delta;P/s:</span> <strong id="p-rate" style="color:#0284c7;">0.0</strong> mmHg/s</div>
      </div>

      <div style="text-align:right;">
        <div style="font-size:0.8rem;color:#64748b;font-weight:600;">Raw: <span id="p-raw">0</span> counts</div>
        <div style="font-size:0.8rem;color:#64748b;font-weight:600;">Tare: <span id="p-tare">0</span> &nbsp;(&Delta;: <span id="p-delta">0</span>)</div>
        <div style="display:flex;gap:6px;margin-top:6px;justify-content:flex-end;">
          <button class="btn-sync" style="background:#f0f9ff;border-color:#38bdf8;color:#0369a1;" onclick="cmdTarePressure()">
            Tare (0 mmHg)
          </button>
          <button class="btn-sync" style="background:#f8fafc;border-color:#cbd5e1;color:#475569;" onclick="clearPressureChart()">
            Clear Graph
          </button>
        </div>
      </div>
    </div>

    <!-- Live Real-Time Oscilloscope / Strip-Chart Canvas -->
    <div style="margin-top:14px;background:#0f172a;border-radius:8px;padding:8px;position:relative;overflow:hidden;">
      <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:4px;padding:0 4px;font-size:0.72rem;color:#94a3b8;font-weight:600;">
        <span>REAL-TIME PRESSURE WAVEFORM (mmHg)</span>
        <div style="display:flex;align-items:center;gap:10px;">
          <span>Window: 
            <select id="chart-window-sel" onchange="changeChartWindow(this.value)" style="background:#1e293b;color:#38bdf8;border:1px solid #475569;border-radius:4px;font-size:0.7rem;padding:1px 4px;">
              <option value="15">15 sec</option>
              <option value="30" selected>30 sec</option>
              <option value="60">60 sec</option>
              <option value="120">2 min</option>
            </select>
          </span>
          <span style="color:#34d399;">10 Hz LIVE</span>
        </div>
      </div>
      <canvas id="pressureCanvas" style="width:100%;height:150px;display:block;border-radius:4px;"></canvas>
    </div>
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

// Real-time Pressure Waveform Chart State
let pressureHistory = [];
let chartWindowSec = 30;
let minSeenPressure = null;
let maxSeenPressure = null;

function clearPressureChart() {
  pressureHistory = [];
  minSeenPressure = null;
  maxSeenPressure = null;
  const minEl = document.getElementById('p-min');
  const maxEl = document.getElementById('p-max');
  const rateEl = document.getElementById('p-rate');
  if (minEl) minEl.textContent = '--';
  if (maxEl) maxEl.textContent = '--';
  if (rateEl) { rateEl.textContent = '0.0'; rateEl.style.color = '#0284c7'; }
  drawPressureChart();
}

function changeChartWindow(sec) {
  chartWindowSec = parseInt(sec, 10) || 30;
  drawPressureChart();
}

function drawPressureChart() {
  const canvas = document.getElementById('pressureCanvas');
  if (!canvas) return;
  const ctx = canvas.getContext('2d');
  if (!ctx) return;

  const dpr = window.devicePixelRatio || 1;
  const rect = canvas.getBoundingClientRect();
  const width = rect.width || 600;
  const height = rect.height || 150;

  if (canvas.width !== Math.floor(width * dpr) || canvas.height !== Math.floor(height * dpr)) {
    canvas.width = Math.floor(width * dpr);
    canvas.height = Math.floor(height * dpr);
  }

  ctx.save();
  ctx.scale(dpr, dpr);

  // Background
  ctx.fillStyle = '#0f172a';
  ctx.fillRect(0, 0, width, height);

  const now = Date.now();
  const cutoff = now - (chartWindowSec * 1000);
  while (pressureHistory.length > 0 && pressureHistory[0].t < cutoff) {
    pressureHistory.shift();
  }

  // Determine dynamic Y bounds
  let yMin = 0;
  let yMax = 30;
  if (pressureHistory.length > 0) {
    let curMin = pressureHistory[0].val;
    let curMax = pressureHistory[0].val;
    for (let i = 1; i < pressureHistory.length; i++) {
      const v = pressureHistory[i].val;
      if (v < curMin) curMin = v;
      if (v > curMax) curMax = v;
    }
    yMin = Math.min(0, Math.floor(curMin - 3));
    yMax = Math.max(15, Math.ceil(curMax + 5));
  }
  if (yMax - yMin < 10) yMax = yMin + 10;

  const plotLeft = 36;
  const plotRight = width - 8;
  const plotTop = 10;
  const plotBottom = height - 16;
  const plotWidth = plotRight - plotLeft;
  const plotHeight = plotBottom - plotTop;

  // Grid Lines & Labels
  ctx.strokeStyle = '#1e293b';
  ctx.lineWidth = 1;
  ctx.fillStyle = '#64748b';
  ctx.font = '10px monospace';
  ctx.textAlign = 'right';

  const numGridLines = 4;
  for (let i = 0; i <= numGridLines; i++) {
    const yVal = yMin + (yMax - yMin) * (i / numGridLines);
    const yPx = plotBottom - (i / numGridLines) * plotHeight;
    ctx.beginPath();
    ctx.moveTo(plotLeft, yPx);
    ctx.lineTo(plotRight, yPx);
    ctx.stroke();
    ctx.fillText(yVal.toFixed(0), plotLeft - 4, yPx + 3);
  }

  // Plot Data
  if (pressureHistory.length >= 2) {
    ctx.beginPath();
    let firstX = plotLeft;
    let lastX = plotRight;

    for (let i = 0; i < pressureHistory.length; i++) {
      const pt = pressureHistory[i];
      const ageMs = now - pt.t;
      const x = plotRight - (ageMs / (chartWindowSec * 1000)) * plotWidth;
      const normY = Math.max(0, Math.min(1, (pt.val - yMin) / (yMax - yMin)));
      const y = plotBottom - (normY * plotHeight);

      if (i === 0) {
        firstX = x;
        ctx.moveTo(x, y);
      } else {
        ctx.lineTo(x, y);
      }
      if (i === pressureHistory.length - 1) {
        lastX = x;
      }
    }

    // Trace line stroke
    ctx.strokeStyle = '#38bdf8';
    ctx.lineWidth = 2;
    ctx.stroke();

    // Area fill under curve
    ctx.lineTo(lastX, plotBottom);
    ctx.lineTo(firstX, plotBottom);
    ctx.closePath();
    const grad = ctx.createLinearGradient(0, plotTop, 0, plotBottom);
    grad.addColorStop(0, 'rgba(56, 189, 248, 0.28)');
    grad.addColorStop(1, 'rgba(56, 189, 248, 0.0)');
    ctx.fillStyle = grad;
    ctx.fill();

    // Latest glowing data point
    const newestPt = pressureHistory[pressureHistory.length - 1];
    const newestNormY = Math.max(0, Math.min(1, (newestPt.val - yMin) / (yMax - yMin)));
    const newestY = plotBottom - (newestNormY * plotHeight);
    ctx.beginPath();
    ctx.arc(lastX, newestY, 4, 0, 2 * Math.PI);
    ctx.fillStyle = '#34d399';
    ctx.fill();
  }

  ctx.restore();
}

function renderPressureTelemetry(data) {
  if (!data) return;
  const badge = document.getElementById('sensor-badge');
  const pMmhg = document.getElementById('p-mmhg');
  const pKpa = document.getElementById('p-kpa');
  const pPsi = document.getElementById('p-psi');
  const pRaw = document.getElementById('p-raw');
  const pTare = document.getElementById('p-tare');
  const pDelta = document.getElementById('p-delta');

  if (badge) {
    if (data.connected) {
      badge.className = 'badge badge-connected';
      badge.textContent = 'ONLINE';
    } else {
      badge.className = 'badge badge-disconnected';
      badge.textContent = 'DISCONNECTED';
    }
  }

  const val = (typeof data.mmhg === 'number') ? data.mmhg : parseFloat(data.mmhg) || 0;

  const pRawMmhg = document.getElementById('p-raw-mmhg');
  const pGain = document.getElementById('p-gain');

  if (pMmhg) pMmhg.textContent = val.toFixed(1);
  if (pKpa) pKpa.textContent = (typeof data.kpa === 'number') ? data.kpa.toFixed(2) : data.kpa;
  if (pPsi) pPsi.textContent = (typeof data.psi === 'number') ? data.psi.toFixed(2) : data.psi;
  if (pRaw) pRaw.textContent = data.raw;
  if (pTare) pTare.textContent = data.tare;
  if (pDelta) pDelta.textContent = data.delta;
  if (pRawMmhg && typeof data.rawMmhg !== 'undefined') pRawMmhg.textContent = (typeof data.rawMmhg === 'number') ? data.rawMmhg.toFixed(1) : data.rawMmhg;
  if (pGain && typeof data.gain !== 'undefined') pGain.textContent = (typeof data.gain === 'number') ? data.gain.toFixed(2) : data.gain;

  // Track live history for waveform chart
  const now = Date.now();
  pressureHistory.push({ t: now, val: val });

  // Update Min/Max tracking
  if (minSeenPressure === null || val < minSeenPressure) minSeenPressure = val;
  if (maxSeenPressure === null || val > maxSeenPressure) maxSeenPressure = val;

  const minEl = document.getElementById('p-min');
  const maxEl = document.getElementById('p-max');
  const rateEl = document.getElementById('p-rate');
  if (minEl && minSeenPressure !== null) minEl.textContent = minSeenPressure.toFixed(1);
  if (maxEl && maxSeenPressure !== null) maxEl.textContent = maxSeenPressure.toFixed(1);

  // Calculate Rate of Pressure Change (Delta P / Delta t over ~1 second window)
  if (pressureHistory.length >= 4) {
    const pastIdx = Math.max(0, pressureHistory.length - 10);
    const pastPt = pressureHistory[pastIdx];
    const dt = (now - pastPt.t) / 1000.0;
    if (dt >= 0.25) {
      const dP = (val - pastPt.val) / dt;
      if (rateEl) {
        rateEl.textContent = (dP >= 0 ? '+' : '') + dP.toFixed(1);
        rateEl.style.color = dP < -1.5 ? '#f87171' : (dP > 1.5 ? '#38bdf8' : '#0284c7');
      }
    }
  }

  // Draw chart frame
  drawPressureChart();
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
      } else if (data.type === 'pressure') {
        renderPressureTelemetry(data);
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

function cmdTarePressure() {
  sendWsOrHttp('TARE', '/pressure/tare');
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
    const resPress = await fetch('/pressure');
    if (resPress.ok) {
      const data = await resPress.json();
      renderPressureTelemetry(data);
    }
  } catch (e) {}
}

// Startup
window.addEventListener('resize', drawPressureChart);
drawPressureChart();
forceSync();
connectWebSocket();
setInterval(forceSync, 4000);
</script>
</body>
</html>
)rawliteral";

#endif // WEBPAGE_H
