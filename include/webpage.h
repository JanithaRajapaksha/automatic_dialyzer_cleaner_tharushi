#ifndef WEBPAGE_H
#define WEBPAGE_H

#include <pgmspace.h>

const char webpage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Automatic Dialyzer Cleaner</title>
<style>
  :root {
    --primary: #0284c7;
    --primary-dark: #0369a1;
    --bg: #f0f4f8;
    --card-bg: #ffffff;
    --text-main: #0f172a;
    --text-muted: #64748b;
    --success: #10b981;
    --danger: #ef4444;
    --neutral: #334155;
  }
  * { box-sizing: border-box; margin: 0; padding: 0; }
  body {
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
    background: var(--bg);
    color: var(--text-main);
    text-align: center;
    padding: 24px 16px;
  }
  .header {
    margin-bottom: 24px;
  }
  .header h1 {
    font-size: 1.75rem;
    color: #0369a1;
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 8px;
  }
  .header p {
    font-size: 0.95rem;
    color: var(--text-muted);
    margin-top: 4px;
  }
  .status-bar {
    max-width: 720px;
    margin: 0 auto 20px auto;
    padding: 10px 16px;
    border-radius: 8px;
    background: #e2e8f0;
    font-size: 0.9rem;
    font-weight: 500;
    display: flex;
    justify-content: space-between;
    align-items: center;
  }
  .badge {
    padding: 4px 10px;
    border-radius: 9999px;
    font-size: 0.8rem;
    font-weight: 600;
  }
  .badge-connected { background: #dcfce7; color: #166534; }
  .badge-disconnected { background: #fee2e2; color: #991b1b; }
  .badge-updating { background: #fef3c7; color: #92400e; }
  .actions {
    max-width: 720px;
    margin: 0 auto 20px auto;
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 12px;
  }
  .btn-action {
    padding: 12px 20px;
    font-size: 0.95rem;
    font-weight: 600;
    border-radius: 8px;
    border: none;
    cursor: pointer;
    color: white;
    transition: opacity 0.2s ease, transform 0.1s ease;
  }
  .btn-action:active { transform: scale(0.98); }
  .btn-all-on { background: #059669; }
  .btn-all-off { background: #dc2626; }
  .container {
    max-width: 720px;
    margin: auto;
    display: grid;
    grid-template-columns: repeat(2, 1fr);
    gap: 14px;
  }
  .card {
    background: var(--card-bg);
    border-radius: 12px;
    padding: 16px;
    box-shadow: 0 4px 6px -1px rgba(0, 0, 0, 0.07), 0 2px 4px -2px rgba(0, 0, 0, 0.05);
    display: flex;
    flex-direction: column;
    justify-content: space-between;
    text-align: left;
    border-left: 5px solid #cbd5e1;
    transition: border-color 0.2s ease;
  }
  .card.active { border-left-color: var(--success); }
  .card-top {
    display: flex;
    justify-content: space-between;
    align-items: flex-start;
    margin-bottom: 8px;
  }
  .card-title {
    font-size: 1rem;
    font-weight: 700;
    color: #1e293b;
  }
  .card-desc {
    font-size: 0.82rem;
    color: var(--text-muted);
    margin-bottom: 12px;
  }
  .state-pill {
    font-size: 0.75rem;
    font-weight: 700;
    padding: 3px 8px;
    border-radius: 6px;
    text-transform: uppercase;
  }
  .state-on { background: #dcfce7; color: #15803d; }
  .state-off { background: #f1f5f9; color: #64748b; }
  .btn-toggle {
    width: 100%;
    padding: 10px;
    border: none;
    border-radius: 6px;
    font-size: 0.9rem;
    font-weight: 600;
    color: white;
    cursor: pointer;
    transition: background-color 0.2s ease;
  }
  .btn-on { background: var(--success); }
  .btn-off { background: var(--neutral); }
  @media (max-width: 540px) {
    .container { grid-template-columns: 1fr; }
    .actions { grid-template-columns: 1fr; }
  }
</style>
</head>

<body>
  <div class="header">
    <h1>Automatic Dialyzer Cleaner</h1>
    <p>10-Channel Automated Valve & Pump Control System</p>
  </div>

  <div class="status-bar">
    <span>Status: <span id="msg-text">Connecting to system...</span></span>
    <span class="badge badge-updating" id="conn-badge">SYNCING</span>
  </div>

  <div class="actions">
    <button class="btn-action btn-all-on" onclick="setAllChannels(1)">ACTIVATE ALL VALVES</button>
    <button class="btn-action btn-all-off" onclick="setAllChannels(0)">SHUTDOWN ALL VALVES</button>
  </div>

  <div class="container" id="relays"></div>

<script>
const channelNames = [
  "Water Inlet Valve",
  "Disinfectant Valve",
  "Blood Port Flush Valve",
  "Dialysate Port Flush Valve",
  "Reverse Flow Valve",
  "Drain Release Valve",
  "Main Booster Pump",
  "Chemical Metering Pump",
  "Air Purge / Vent Valve",
  "Waste Discharge Valve"
];

const total = 10;
const states = Array(total).fill(false);
const container = document.getElementById('relays');

// Build UI cards
for (let i = 0; i < total; i++) {
  container.innerHTML += `
    <div class="card" id="card${i}">
      <div class="card-top">
        <span class="card-title">Channel ${i + 1}</span>
        <span class="state-pill state-off" id="pill${i}">OFF</span>
      </div>
      <div class="card-desc">${channelNames[i] || 'Relay ' + (i + 1)}</div>
      <button id="btn${i}" class="btn-toggle btn-off" onclick="toggleRelay(${i})">
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

function render(i) {
  const isOn = states[i];
  const card = document.getElementById('card' + i);
  const pill = document.getElementById('pill' + i);
  const btn = document.getElementById('btn' + i);

  if (isOn) {
    card.classList.add('active');
    pill.textContent = 'ACTIVE';
    pill.className = 'state-pill state-on';
    btn.textContent = 'Turn OFF';
    btn.className = 'btn-toggle btn-on';
  } else {
    card.classList.remove('active');
    pill.textContent = 'OFF';
    pill.className = 'state-pill state-off';
    btn.textContent = 'Turn ON';
    btn.className = 'btn-toggle btn-off';
  }
}

let ws = null;
let wsReconnectTimer = null;

function connectWebSocket() {
  if (ws && (ws.readyState === WebSocket.CONNECTING || ws.readyState === WebSocket.OPEN)) return;

  const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
  const wsUrl = `${protocol}//${window.location.hostname}:81/`;

  updateConnectionStatus('updating', 'Connecting Realtime Link...');

  ws = new WebSocket(wsUrl);

  ws.onopen = function() {
    updateConnectionStatus('connected', 'Live Realtime (WebSocket)');
    if (wsReconnectTimer) {
      clearTimeout(wsReconnectTimer);
      wsReconnectTimer = null;
    }
  };

  ws.onmessage = function(event) {
    try {
      const data = JSON.parse(event.data);
      if (data.type === 'status' && Array.isArray(data.states)) {
        for (let i = 0; i < total; i++) {
          states[i] = data.states[i];
          render(i);
        }
        updateConnectionStatus('connected', 'Live Realtime (WebSocket)');
      }
    } catch (err) {
      console.error('WS Parse Error', err);
    }
  };

  ws.onerror = function() {
    // onclose will handle reconnect
  };

  ws.onclose = function() {
    updateConnectionStatus('disconnected', 'WebSocket Disconnected. Reconnecting...');
    if (!wsReconnectTimer) {
      wsReconnectTimer = setTimeout(connectWebSocket, 2000);
    }
  };
}

function toggleRelay(i) {
  const previousState = states[i];
  // 1. Instant optimistic UI feedback (< 1ms)
  states[i] = !previousState;
  render(i);

  // 2. Transmit command instantly over WebSocket (< 10ms)
  if (ws && ws.readyState === WebSocket.OPEN) {
    ws.send(JSON.stringify({ cmd: 'set', ch: i, state: states[i] ? 1 : 0 }));
  } else {
    // Fallback to HTTP fetch if WebSocket is not open
    fetch(`/relay?ch=${i}&state=${states[i] ? 1 : 0}`)
      .then(res => {
        if (!res.ok) throw new Error();
        return res.json();
      })
      .then(data => {
        states[i] = data.state;
        render(i);
      })
      .catch(() => {
        // Revert on error
        states[i] = previousState;
        render(i);
        updateConnectionStatus('disconnected', 'Communication Error');
      });
  }
}

function setAllChannels(state) {
  for (let i = 0; i < total; i++) {
    states[i] = !!state;
    render(i);
  }

  if (ws && ws.readyState === WebSocket.OPEN) {
    ws.send(JSON.stringify({ cmd: 'all', state: state ? 1 : 0 }));
  } else {
    fetch(`/all?state=${state ? 1 : 0}`)
      .catch(() => {
        updateConnectionStatus('disconnected', 'Error sending bulk command');
        refreshStates();
      });
  }
}

async function refreshStates() {
  // If WebSocket is active, it handles live pushes automatically; skip HTTP polling
  if (ws && ws.readyState === WebSocket.OPEN) return;

  try {
    const response = await fetch('/status');
    if (!response.ok) throw new Error('Status poll failed');
    const data = await response.json();
    const statesArr = Array.isArray(data) ? data : data.states;
    if (statesArr) {
      for (let i = 0; i < total; i++) {
        states[i] = statesArr[i];
        render(i);
      }
      updateConnectionStatus('connected', 'System Online (HTTP Fallback)');
    }
  } catch (e) {
    updateConnectionStatus('disconnected', 'Dialyzer Cleaner Disconnected');
  }
}

// Initial state fetch & WebSocket startup
refreshStates();
connectWebSocket();

// Low-frequency fallback poll only if WebSocket is disconnected
setInterval(refreshStates, 5000);
</script>
</body>
</html>
)rawliteral";

#endif // WEBPAGE_H

