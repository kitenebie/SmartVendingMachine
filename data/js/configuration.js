// ============================================================
//  configuration.js -- Account & GPIO Pin Management
//  Updated for 20x4 I2C LCD (SDA & SCL)
// ============================================================

const DEFAULT_PINS = {
  sensorA: 34, sensorB: 35,
  btn1: 13, btn2: 14, btn3: 27, btn4: 26,
  ir1: 16, ir2: 17, ir3: 32, ir4: 33,
  servo1: 18, servo2: 19, servo3: 21, servo4: 25,
  lcdSda: 23, lcdScl: 22
};

// ---- Account Settings ---------------------------------------

async function loadConfig() {
  try {
    const r = await fetch('/api/configuration', { headers: authHeaders() });
    if (r.status === 401) { clearToken(); window.location.href = '/login.html'; return; }
    const data = await r.json();
    if (data.success) {
      document.getElementById('currentUsername').value = data.username || '';
    }
  } catch (err) {
    console.error('Config load error:', err);
  }
}

function showConfigAlert(msg, type = 'error') {
  const el = document.getElementById('configAlert');
  el.textContent = msg;
  el.className = 'alert alert-' + type;
}

function clearErrors() {
  ['newUsernameErr','currentPasswordErr','newPasswordErr','confirmPasswordErr'].forEach(id => {
    const el = document.getElementById(id);
    if (el) el.textContent = '';
  });
  document.getElementById('configAlert').className = 'alert hidden';
}

function validateConfigForm() {
  clearErrors();
  let valid = true;

  const newUsername     = document.getElementById('newUsername').value.trim();
  const currentPassword = document.getElementById('currentPassword').value;
  const newPassword     = document.getElementById('newPassword').value;
  const confirmPassword = document.getElementById('confirmPassword').value;

  if (!newUsername) {
    document.getElementById('newUsernameErr').textContent = 'New username is required';
    valid = false;
  }
  if (!currentPassword) {
    document.getElementById('currentPasswordErr').textContent = 'Current password is required';
    valid = false;
  }
  if (!newPassword) {
    document.getElementById('newPasswordErr').textContent = 'New password is required';
    valid = false;
  } else if (newPassword.length < 6) {
    document.getElementById('newPasswordErr').textContent = 'Password must be at least 6 characters';
    valid = false;
  }
  if (newPassword && confirmPassword !== newPassword) {
    document.getElementById('confirmPasswordErr').textContent = 'Passwords do not match';
    valid = false;
  }
  return valid;
}

// ---- GPIO Pin Settings --------------------------------------

function showPinAlert(msg, type = 'error') {
  const el = document.getElementById('pinAlert');
  el.textContent = msg;
  el.className = 'alert alert-' + type;
}

async function loadPins() {
  try {
    const r = await fetch('/api/pins', { headers: authHeaders() });
    if (r.status === 401) return;
    const data = await r.json();
    if (data.success && data.pins) {
      populatePinInputs(data.pins);
    }
  } catch (err) {
    console.error('Pins load error:', err);
  }
}

function populatePinInputs(pins) {
  for (const [k, v] of Object.entries(pins)) {
    const inputId = 'pin' + k.charAt(0).toUpperCase() + k.slice(1);
    const input = document.getElementById(inputId);
    if (input) input.value = v;
  }
}

function getPinFormData() {
  return {
    sensorA: parseInt(document.getElementById('pinSensorA').value),
    sensorB: parseInt(document.getElementById('pinSensorB').value),
    btn1:    parseInt(document.getElementById('pinBtn1').value),
    btn2:    parseInt(document.getElementById('pinBtn2').value),
    btn3:    parseInt(document.getElementById('pinBtn3').value),
    btn4:    parseInt(document.getElementById('pinBtn4').value),
    ir1:     parseInt(document.getElementById('pinIr1').value),
    ir2:     parseInt(document.getElementById('pinIr2').value),
    ir3:     parseInt(document.getElementById('pinIr3').value),
    ir4:     parseInt(document.getElementById('pinIr4').value),
    servo1:  parseInt(document.getElementById('pinServo1').value),
    servo2:  parseInt(document.getElementById('pinServo2').value),
    servo3:  parseInt(document.getElementById('pinServo3').value),
    servo4:  parseInt(document.getElementById('pinServo4').value),
    lcdSda:  parseInt(document.getElementById('pinLcdSda').value),
    lcdScl:  parseInt(document.getElementById('pinLcdScl').value)
  };
}

// ---- Init & Event Listeners ---------------------------------

document.addEventListener('DOMContentLoaded', async () => {
  const username = await checkAuth();
  if (!username) return;

  const usernameEl = document.getElementById('topbarUsername');
  if (usernameEl) usernameEl.textContent = username;

  loadConfig();
  loadPins();

  // Pin form submission
  const pinForm = document.getElementById('pinForm');
  const savePinsBtn = document.getElementById('savePinsBtn');
  const savePinsText = document.getElementById('savePinsText');
  const savePinsSpinner = document.getElementById('savePinsSpinner');

  pinForm.addEventListener('submit', async (e) => {
    e.preventDefault();
    savePinsBtn.disabled = true;
    savePinsText.classList.add('hidden');
    savePinsSpinner.classList.remove('hidden');
    document.getElementById('pinAlert').className = 'alert hidden';

    const payload = getPinFormData();

    try {
      const r = await fetch('/api/pins', {
        method: 'PUT',
        headers: authHeaders(),
        body: JSON.stringify(payload)
      });
      const data = await r.json();

      if (data.success) {
        showPinAlert(data.message || 'GPIO Pins updated and 20x4 LCD reinitialized live!', 'success');
      } else {
        showPinAlert(data.message || 'Failed to update GPIO pins');
      }
    } catch (err) {
      showPinAlert('Network error while saving pins.');
      console.error(err);
    } finally {
      savePinsBtn.disabled = false;
      savePinsText.classList.remove('hidden');
      savePinsSpinner.classList.add('hidden');
    }
  });

  // Reset pins to defaults button
  document.getElementById('resetPinsBtn').addEventListener('click', () => {
    if (confirm('Reset all GPIO pins to factory default pinout?')) {
      populatePinInputs(DEFAULT_PINS);
      showPinAlert('Reset to default values in form. Click "Save Pin Configuration" to apply.', 'success');
    }
  });

  // Account form submission
  const form    = document.getElementById('configForm');
  const saveBtn = document.getElementById('saveConfigBtn');
  const saveTxt = document.getElementById('saveText');
  const spinner = document.getElementById('saveSpinner');

  form.addEventListener('submit', async (e) => {
    e.preventDefault();
    if (!validateConfigForm()) return;

    saveBtn.disabled = true;
    saveTxt.classList.add('hidden');
    spinner.classList.remove('hidden');

    const payload = {
      newUsername:     document.getElementById('newUsername').value.trim(),
      currentPassword: document.getElementById('currentPassword').value,
      newPassword:     document.getElementById('newPassword').value,
      confirmPassword: document.getElementById('confirmPassword').value
    };

    try {
      const r = await fetch('/api/configuration', {
        method: 'PUT',
        headers: authHeaders(),
        body: JSON.stringify(payload)
      });
      const data = await r.json();

      if (data.success) {
        showConfigAlert('Credentials updated! Redirecting to login...', 'success');
        clearToken();
        setTimeout(() => { window.location.href = '/login.html'; }, 2000);
      } else {
        showConfigAlert(data.message || 'Update failed');
        saveBtn.disabled = false;
        saveTxt.classList.remove('hidden');
        spinner.classList.add('hidden');
      }
    } catch (err) {
      showConfigAlert('Network error. Please try again.');
      saveBtn.disabled = false;
      saveTxt.classList.remove('hidden');
      spinner.classList.add('hidden');
      console.error(err);
    }
  });
});
