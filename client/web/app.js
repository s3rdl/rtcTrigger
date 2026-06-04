const SERVICE_UUID = '7a4b1001-4b85-4cc0-98f6-5cb3d5bdb001';
const COMMAND_UUID = '7a4b1002-4b85-4cc0-98f6-5cb3d5bdb001';
const STATUS_UUID = '7a4b1003-4b85-4cc0-98f6-5cb3d5bdb001';
const BATTERY_SERVICE_UUID = 0x180f;
const BATTERY_LEVEL_UUID = 0x2a19;

const connectionStatus = document.querySelector('#connectionStatus');
const authStatus = document.querySelector('#authStatus');
const statusView = document.querySelector('#status');
const batterySummaryView = document.querySelector('#batterySummary');
const nextTriggerView = document.querySelector('#nextTrigger');
const rtcNowView = document.querySelector('#rtcNow');
const connectionStateValue = document.querySelector('#connectionStateValue');
const authStateValue = document.querySelector('#authStateValue');
const dailyScheduleValue = document.querySelector('#dailyScheduleValue');
const oneShotValue = document.querySelector('#oneShotValue');
const bleWindowValue = document.querySelector('#bleWindowValue');
const bleWakeScheduleValue = document.querySelector('#bleWakeScheduleValue');
const bleOneShotValue = document.querySelector('#bleOneShotValue');
const lastEventValue = document.querySelector('#lastEventValue');
const speakerRunsValue = document.querySelector('#speakerRunsValue');
const connectButton = document.querySelector('#connect');
const disconnectButton = document.querySelector('#disconnect');
const unlockButton = document.querySelector('#unlock');
const enableAuthButton = document.querySelector('#enableAuth');
const disableAuthButton = document.querySelector('#disableAuth');
const setPinButton = document.querySelector('#setPin');
const commandButtons = [...document.querySelectorAll('.cmd')];
const setScheduleButton = document.querySelector('#setSchedule');
const setOneShotButton = document.querySelector('#setOneShot');
const clearOneShotButton = document.querySelector('#clearOneShot');
const fillOneShotSoonButton = document.querySelector('#fillOneShotSoon');
const setBleOneShotButton = document.querySelector('#setBleOneShot');
const clearBleOneShotButton = document.querySelector('#clearBleOneShot');
const fillBleOneShotSoonButton = document.querySelector('#fillBleOneShotSoon');
const setTimingButton = document.querySelector('#setTiming');
const setBatteryButton = document.querySelector('#setBattery');
const setRtcButton = document.querySelector('#setRtc');
const syncRtcFromBrowserButton = document.querySelector('#syncRtcFromBrowser');

let device;
let server;
let commandCharacteristic;
let statusCharacteristic;
let batteryCharacteristic;
let latestStatusText = 'No data';
let latestBatteryLevel = null;
let isAuthenticated = false;
let isAuthEnabled = true;
let isSavingTiming = false;

function setInputValueIfIdle(selector, value) {
  const input = document.querySelector(selector);
  if (!input) {
    return;
  }

  if (document.activeElement === input) {
    return;
  }

  input.value = value;
}

function setCheckboxValueIfIdle(selector, checked) {
  const input = document.querySelector(selector);
  if (!input) {
    return;
  }

  if (isSavingTiming && selector === '#bleFixedWakeEnabled') {
    return;
  }
  if (isSavingTiming && selector === '#bleIntervalWakeEnabled') {
    return;
  }

  if (document.activeElement === input) {
    return;
  }

  input.checked = checked;
}

function setConnected(connected) {
  connectionStateValue.textContent = connected ? 'Connected' : 'Disconnected';
  disconnectButton.disabled = !connected;
  unlockButton.disabled = !connected;
  enableAuthButton.disabled = !connected;
  disableAuthButton.disabled = !connected;
  setPinButton.disabled = !connected;
  commandButtons.forEach((button) => {
    button.disabled = !connected || (isAuthEnabled && !isAuthenticated);
  });
  setScheduleButton.disabled = !connected || (isAuthEnabled && !isAuthenticated);
  fillOneShotSoonButton.disabled = !connected || (isAuthEnabled && !isAuthenticated);
  setOneShotButton.disabled = !connected || (isAuthEnabled && !isAuthenticated);
  clearOneShotButton.disabled = !connected || (isAuthEnabled && !isAuthenticated);
  fillBleOneShotSoonButton.disabled = !connected || (isAuthEnabled && !isAuthenticated);
  setBleOneShotButton.disabled = !connected || (isAuthEnabled && !isAuthenticated);
  clearBleOneShotButton.disabled = !connected || (isAuthEnabled && !isAuthenticated);
  setTimingButton.disabled = !connected || (isAuthEnabled && !isAuthenticated);
  setBatteryButton.disabled = !connected || (isAuthEnabled && !isAuthenticated);
  setRtcButton.disabled = !connected || (isAuthEnabled && !isAuthenticated);
  syncRtcFromBrowserButton.disabled = !connected || (isAuthEnabled && !isAuthenticated);
}

function browserDateParts() {
  const now = new Date();
  const year = now.getFullYear();
  const month = String(now.getMonth() + 1).padStart(2, '0');
  const day = String(now.getDate()).padStart(2, '0');
  const hour = String(now.getHours()).padStart(2, '0');
  const minute = String(now.getMinutes()).padStart(2, '0');

  return {
    date: `${year}-${month}-${day}`,
    time: `${hour}:${minute}`,
  };
}

function browserDatePartsPlusMinutes(minutesAhead) {
  const then = new Date(Date.now() + minutesAhead * 60 * 1000);
  const year = then.getFullYear();
  const month = String(then.getMonth() + 1).padStart(2, '0');
  const day = String(then.getDate()).padStart(2, '0');
  const hour = String(then.getHours()).padStart(2, '0');
  const minute = String(then.getMinutes()).padStart(2, '0');

  return {
    date: `${year}-${month}-${day}`,
    time: `${hour}:${minute}`,
  };
}

function fillRtcFromBrowserTime() {
  const { date, time } = browserDateParts();
  document.querySelector('#rtcDate').value = date;
  document.querySelector('#rtcTime').value = time;
}

function fillOneShotFromSoon() {
  const { date, time } = browserDatePartsPlusMinutes(5);
  document.querySelector('#oneShotDate').value = date;
  document.querySelector('#oneShotTime').value = time;
}

function fillBleOneShotFromSoon() {
  const { date, time } = browserDatePartsPlusMinutes(5);
  document.querySelector('#bleOneShotDate').value = date;
  document.querySelector('#bleOneShotTime').value = time;
}

function bleWakeTimesCompact(value) {
  return (value || '').replaceAll(':', '').replaceAll(',', '');
}

function setStatus(text) {
  latestStatusText = text;
  renderStatus();
  hydrateForm(text);
}

function renderStatus() {
  const suffix = latestBatteryLevel === null ? '' : `, batteryServicePct=${latestBatteryLevel}`;
  statusView.textContent = `${latestStatusText}${suffix}`.split(',').join('\n');
}

function parseStatusPairs(text) {
  return text.split(',').reduce((pairs, part) => {
    const [key, value] = part.split('=');
    if (key && value) {
      pairs[key.trim()] = value.trim();
    }
    return pairs;
  }, {});
}

function decodeBleWakeTimes(value) {
  return (value || '').replaceAll('|', ',');
}

function decodeDataView(dataView) {
  const bytes = new Uint8Array(dataView.buffer, dataView.byteOffset, dataView.byteLength);
  return new TextDecoder().decode(bytes);
}

function hydrateForm(text) {
  const pairs = parseStatusPairs(text);
  isAuthEnabled = pairs.auth !== 'off';
  isAuthenticated = pairs.authenticated === 'yes' || !isAuthEnabled;
  authStatus.textContent = isAuthEnabled
    ? (isAuthenticated ? 'Unlocked' : 'Locked')
    : 'PIN disabled';
  authStateValue.textContent = authStatus.textContent;
  setConnected(!!device?.gatt?.connected);
  const batteryPct = pairs.batteryPct || '?';
  const batteryVoltage = pairs.battery || '?';
  batterySummaryView.textContent = `${batteryPct}% (${batteryVoltage})`;
  nextTriggerView.textContent = pairs.nextTrigger || 'unknown';
  rtcNowView.textContent = pairs.rtcNow || 'unknown';
  dailyScheduleValue.textContent = pairs.schedule === 'on'
    ? `On at ${pairs.time || 'unknown'}`
    : 'Off';
  oneShotValue.textContent = pairs.oneShot === 'on'
    ? (pairs.oneShotAt || 'On')
    : 'Off';
  const bleWakeTimes = decodeBleWakeTimes(pairs.bleWakeTimes || '');
  bleWindowValue.textContent = pairs.bleFixedWake === 'on'
    ? `${bleWakeTimes || 'unknown'} / ${pairs.bleWindowMs || '?'} ms`
    : (pairs.bleIntervalWake === 'on'
      ? `${pairs.bleWindowMs || '?'} ms / ${pairs.bleWakeSec || '?'} s wake`
      : `${pairs.bleWindowMs || '?'} ms / interval off`);
  bleWakeScheduleValue.textContent = pairs.bleFixedWake === 'on'
    ? (bleWakeTimes || 'unknown')
    : (pairs.bleIntervalWake === 'on' ? `Every ${pairs.bleWakeSec || '?'} s` : 'Disabled');
  bleOneShotValue.textContent = pairs.bleOneShotAt || 'Off';
  lastEventValue.textContent = pairs.lastEventAt
    ? `${pairs.lastEvent || 'none'} @ ${pairs.lastEventAt}`
    : (pairs.lastEvent || 'none');
  speakerRunsValue.textContent = pairs.speakerRuns || '0';
  if (pairs.time) {
    setInputValueIfIdle('#scheduleTime', pairs.time);
  }
  if (pairs.oneShotAt) {
    const [date, time] = pairs.oneShotAt.split('T');
    if (date) {
      setInputValueIfIdle('#oneShotDate', date);
    }
    if (time) {
      setInputValueIfIdle('#oneShotTime', time);
    }
  }
  if (pairs.bleOneShotAt) {
    const [date, time] = pairs.bleOneShotAt.split('T');
    if (date) {
      setInputValueIfIdle('#bleOneShotDate', date);
    }
    if (time) {
      setInputValueIfIdle('#bleOneShotTime', time);
    }
  }
  if (pairs.rtcNow && pairs.rtcNow.includes('T')) {
    const [date, time] = pairs.rtcNow.split('T');
    setInputValueIfIdle('#rtcDate', date);
    setInputValueIfIdle('#rtcTime', time.slice(0, 5));
  }
  if (pairs.powerMs) {
    setInputValueIfIdle('#powerMs', pairs.powerMs);
  }
  if (pairs.bootMs) {
    setInputValueIfIdle('#bootMs', pairs.bootMs);
  }
  if (pairs.modeMs) {
    setInputValueIfIdle('#modeMs', pairs.modeMs);
  }
  if (pairs.bleWindowMs) {
    setInputValueIfIdle('#bleWindowMs', pairs.bleWindowMs);
  }
  if (pairs.bleWakeSec) {
    setInputValueIfIdle('#bleWakeSec', pairs.bleWakeSec);
  }
  setCheckboxValueIfIdle('#bleIntervalWakeEnabled', pairs.bleIntervalWake !== 'off');
  setCheckboxValueIfIdle('#bleFixedWakeEnabled', pairs.bleFixedWake === 'on');
  if (pairs.bleWakeTimes) {
    setInputValueIfIdle('#bleWakeTimes', bleWakeTimes);
  }
  if (pairs.batteryDividerX100) {
    setInputValueIfIdle('#batteryDivider', pairs.batteryDividerX100);
  }
  if (pairs.batteryEmptyMv) {
    setInputValueIfIdle('#batteryEmpty', pairs.batteryEmptyMv);
  }
  if (pairs.batteryFullMv) {
    setInputValueIfIdle('#batteryFull', pairs.batteryFullMv);
  }
}

async function sendCommand(command) {
  const data = new TextEncoder().encode(command);
  await commandCharacteristic.writeValue(data);
}

async function refreshStatusFromDevice() {
  const statusValue = await statusCharacteristic.readValue();
  setStatus(decodeDataView(statusValue));

  if (batteryCharacteristic) {
    const batteryValue = await batteryCharacteristic.readValue();
    latestBatteryLevel = batteryValue.getUint8(0);
    renderStatus();
  }
}

async function onStatusChanged(event) {
  const text = decodeDataView(event.target.value);
  setStatus(text);
}

async function onBatteryChanged(event) {
  latestBatteryLevel = event.target.value.getUint8(0);
  renderStatus();
}

async function connect() {
  device = await navigator.bluetooth.requestDevice({
    filters: [{ services: [SERVICE_UUID] }],
    optionalServices: [BATTERY_SERVICE_UUID],
  });

  device.addEventListener('gattserverdisconnected', handleDisconnect);
  server = await device.gatt.connect();

  const controlService = await server.getPrimaryService(SERVICE_UUID);
  commandCharacteristic = await controlService.getCharacteristic(COMMAND_UUID);
  statusCharacteristic = await controlService.getCharacteristic(STATUS_UUID);

  const batteryService = await server.getPrimaryService(BATTERY_SERVICE_UUID);
  batteryCharacteristic = await batteryService.getCharacteristic(BATTERY_LEVEL_UUID);

  await statusCharacteristic.startNotifications();
  statusCharacteristic.addEventListener('characteristicvaluechanged', onStatusChanged);

  await batteryCharacteristic.startNotifications();
  batteryCharacteristic.addEventListener('characteristicvaluechanged', onBatteryChanged);

  await refreshStatusFromDevice();

  connectionStatus.textContent = `Connected to ${device.name || 'device'}`;
  connectionStateValue.textContent = `Connected to ${device.name || 'device'}`;
  setConnected(true);
}

function handleDisconnect() {
  connectionStatus.textContent = 'Disconnected';
  isAuthenticated = false;
  authStateValue.textContent = 'Locked';
  setConnected(false);
}

async function disconnect() {
  if (device?.gatt?.connected) {
    device.gatt.disconnect();
  }
}

connectButton.addEventListener('click', async () => {
  try {
    await connect();
  } catch (error) {
    connectionStatus.textContent = error.message;
  }
});

disconnectButton.addEventListener('click', disconnect);

unlockButton.addEventListener('click', async () => {
  await sendCommand(`A ${document.querySelector('#authPin').value}`);
  await sendCommand('GET_STATUS');
  await refreshStatusFromDevice();
});

enableAuthButton.addEventListener('click', async () => {
  await sendCommand('ENABLE_AUTH');
  await sendCommand('GET_STATUS');
  await refreshStatusFromDevice();
});

disableAuthButton.addEventListener('click', async () => {
  await sendCommand('DISABLE_AUTH');
  await sendCommand('GET_STATUS');
  await refreshStatusFromDevice();
});

setPinButton.addEventListener('click', async () => {
  await sendCommand(`PIN ${document.querySelector('#newPin').value}`);
  await sendCommand('GET_STATUS');
  await refreshStatusFromDevice();
});

commandButtons.forEach((button) => {
  button.addEventListener('click', async () => {
    await sendCommand(button.dataset.command);
    await sendCommand('GET_STATUS');
    await refreshStatusFromDevice();
  });
});

setScheduleButton.addEventListener('click', async () => {
  const time = document.querySelector('#scheduleTime').value;
  await sendCommand(`SCH ${time}`);
  await sendCommand('ENABLE_SCHEDULE');
  await sendCommand('GET_STATUS');
  await refreshStatusFromDevice();
});

setOneShotButton.addEventListener('click', async () => {
  const date = document.querySelector('#oneShotDate').value;
  const time = document.querySelector('#oneShotTime').value;
  await sendCommand(`ONE ${date}T${time}`);
  await sendCommand('GET_STATUS');
  await refreshStatusFromDevice();
});

fillOneShotSoonButton.addEventListener('click', () => {
  fillOneShotFromSoon();
});

fillBleOneShotSoonButton.addEventListener('click', () => {
  fillBleOneShotFromSoon();
});

clearOneShotButton.addEventListener('click', async () => {
  await sendCommand('CLEAR_ONESHOT');
  await sendCommand('GET_STATUS');
  await refreshStatusFromDevice();
});

setBleOneShotButton.addEventListener('click', async () => {
  const date = document.querySelector('#bleOneShotDate').value;
  const time = document.querySelector('#bleOneShotTime').value;
  await sendCommand(`BWK ${date}T${time}`);
  await sendCommand('GET_STATUS');
  await refreshStatusFromDevice();
});

clearBleOneShotButton.addEventListener('click', async () => {
  await sendCommand('CBW');
  await sendCommand('GET_STATUS');
  await refreshStatusFromDevice();
});

setRtcButton.addEventListener('click', async () => {
  const date = document.querySelector('#rtcDate').value;
  const time = document.querySelector('#rtcTime').value;
  await sendCommand(`RTC ${date}T${time}`);
  await sendCommand('GET_STATUS');
  await refreshStatusFromDevice();
});

syncRtcFromBrowserButton.addEventListener('click', async () => {
  fillRtcFromBrowserTime();
  const date = document.querySelector('#rtcDate').value;
  const time = document.querySelector('#rtcTime').value;
  await sendCommand(`RTC ${date}T${time}`);
  await sendCommand('GET_STATUS');
  await refreshStatusFromDevice();
});

setTimingButton.addEventListener('click', async () => {
  isSavingTiming = true;
  try {
    await sendCommand(
      `SET_TIMING ${document.querySelector('#powerMs').value},${document.querySelector('#bootMs').value},${document.querySelector('#modeMs').value},${document.querySelector('#bleWindowMs').value},${document.querySelector('#bleWakeSec').value}`
    );
    await sendCommand(document.querySelector('#bleIntervalWakeEnabled').checked ? 'EBI' : 'DBI');
    await sendCommand(`FW${document.querySelector('#bleFixedWakeEnabled').checked ? '1' : '0'} ${bleWakeTimesCompact(document.querySelector('#bleWakeTimes').value)}`);
    await sendCommand('GET_STATUS');
    await refreshStatusFromDevice();
  } finally {
    isSavingTiming = false;
  }
});

setBatteryButton.addEventListener('click', async () => {
  await sendCommand(`BD ${document.querySelector('#batteryDivider').value}`);
  await sendCommand(`BE ${document.querySelector('#batteryEmpty').value}`);
  await sendCommand(`BF ${document.querySelector('#batteryFull').value}`);
  await sendCommand('GET_STATUS');
  await refreshStatusFromDevice();
});

setConnected(false);
fillRtcFromBrowserTime();
fillOneShotFromSoon();
fillBleOneShotFromSoon();
