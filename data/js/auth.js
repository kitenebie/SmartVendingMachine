// ============================================================
//  auth.js -- Shared authentication utilities
//  Used by all protected pages and login.html
// ============================================================

const TOKEN_KEY = 'inv_token';
const USERNAME_KEY = 'inv_username';

/** Retrieve the stored session token (or null). */
function getToken() {
  return sessionStorage.getItem(TOKEN_KEY);
}

/** Persist a session token. */
function setToken(token) {
  sessionStorage.setItem(TOKEN_KEY, token);
}

/** Remove the session token. */
function clearToken() {
  sessionStorage.removeItem(TOKEN_KEY);
  sessionStorage.removeItem(USERNAME_KEY);
}

/**
 * Guard function for protected pages.
 * Verifies the token with the server.  If invalid, redirects to login.
 * Returns the username on success.
 */
async function checkAuth() {
  const token = getToken();
  if (!token) {
    window.location.href = '/login.html';
    return null;
  }
  try {
    const r = await fetch('/api/session', {
      headers: { 'X-Session-Token': token }
    });
    const d = await r.json();
    if (!d.success) {
      clearToken();
      window.location.href = '/login.html';
      return null;
    }
    return d.username;
  } catch (_) {
    clearToken();
    window.location.href = '/login.html';
    return null;
  }
}

/**
 * Log out: notify server, clear token, redirect to login.
 */
async function logout() {
  const token = getToken();
  if (token) {
    try {
      await fetch('/api/logout', {
        method: 'POST',
        headers: { 'X-Session-Token': token }
      });
    } catch (_) {}
  }
  clearToken();
  window.location.href = '/login.html';
}

/** Build default fetch headers with the session token. */
function authHeaders() {
  return {
    'Content-Type': 'application/json',
    'X-Session-Token': getToken() || ''
  };
}

// ---- Sidebar / hamburger wiring (shared by all app pages) ---
document.addEventListener('DOMContentLoaded', () => {
  const hamburger = document.getElementById('hamburger');
  const sidebar   = document.getElementById('sidebar');
  const overlay   = document.getElementById('sidebarOverlay');
  const closeBtn  = document.getElementById('sidebarClose');
  const logoutBtn = document.getElementById('logoutBtn');

  function openSidebar() {
    sidebar && sidebar.classList.add('open');
    overlay && overlay.classList.add('active');
  }
  function closeSidebar() {
    sidebar && sidebar.classList.remove('open');
    overlay && overlay.classList.remove('active');
  }

  hamburger && hamburger.addEventListener('click', openSidebar);
  overlay   && overlay.addEventListener('click', closeSidebar);
  closeBtn  && closeBtn.addEventListener('click', closeSidebar);
  logoutBtn && logoutBtn.addEventListener('click', () => logout());

  // Password toggle buttons with data-toggle-pwd attribute
  document.querySelectorAll('[data-toggle-pwd]').forEach(btn => {
    btn.addEventListener('click', () => {
      const input = document.getElementById(btn.dataset.togglePwd);
      if (!input) return;
      input.type = input.type === 'password' ? 'text' : 'password';
    });
  });
});
