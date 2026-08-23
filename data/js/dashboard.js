// ============================================================
//  dashboard.js -- Dashboard statistics and product summary
// ============================================================

/** Format a number as plastic bottle pieces. */
function formatBottles(value) {
  return Number(value).toLocaleString('en-PH', {
    minimumFractionDigits: 0,
    maximumFractionDigits: 0
  }) + ' pcs';
}

function formatNumber(n) {
  return Number(n).toLocaleString('en-PH');
}

async function loadDashboard() {
  const token = getToken();
  try {
    const r = await fetch('/api/products', {
      headers: { 'X-Session-Token': token }
    });
    if (r.status === 401) { clearToken(); window.location.href = '/login.html'; return; }
    const data = await r.json();
    if (!data.success) return;

    const s = data.summary;
    document.getElementById('statProducts').textContent = formatNumber(s.totalProducts);
    document.getElementById('statStocks').textContent   = formatNumber(s.totalStocks);
    document.getElementById('statValue').textContent    = formatBottles(s.totalValue);
    document.getElementById('statLow').textContent      = formatNumber(s.lowStock);

    // Show low stock warning
    if (s.lowStock > 0) {
      document.getElementById('lowStockAlert').classList.remove('hidden');
    }

    // Render product summary table
    const tbody = document.getElementById('summaryBody');
    if (!data.products || data.products.length === 0) {
      tbody.innerHTML = '<tr><td colspan="4" class="text-center text-muted" style="padding:24px">No products yet. <a href="/products.html">Add one</a></td></tr>';
      return;
    }

    // Sort by stocks descending
    const sorted = [...data.products].sort((a, b) => b.stocks - a.stocks);
    tbody.innerHTML = sorted.map(p => `
      <tr>
        <td>${escapeHtml(p.name)}
          ${p.stocks <= 10 ? '<span class="badge-low">Low</span>' : ''}
        </td>
        <td class="text-right">${formatNumber(p.stocks)}</td>
        <td class="text-right">${formatBottles(p.price)}</td>
        <td class="text-right">${formatBottles(p.totalValue)}</td>
      </tr>
    `).join('');
  } catch (err) {
    console.error('Dashboard load error:', err);
  }
}

function escapeHtml(str) {
  return String(str)
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;');
}

// ---- Init ---------------------------------------------------
document.addEventListener('DOMContentLoaded', async () => {
  const username = await checkAuth();
  if (!username) return;

  const usernameEl = document.getElementById('topbarUsername');
  if (usernameEl) usernameEl.textContent = username;

  loadDashboard();

  // Sidebar toggle (mobile)
  const hamburger = document.getElementById('hamburger');
  const sidebar   = document.getElementById('sidebar');
  const overlay   = document.getElementById('sidebarOverlay');
  const sideClose = document.getElementById('sidebarClose');

  hamburger.addEventListener('click', () => {
    sidebar.classList.add('open');
    overlay.classList.add('open');
  });
  sideClose.addEventListener('click', () => {
    sidebar.classList.remove('open');
    overlay.classList.remove('open');
  });
  overlay.addEventListener('click', () => {
    sidebar.classList.remove('open');
    overlay.classList.remove('open');
  });

  // Logout
  document.getElementById('logoutBtn').addEventListener('click', () => {
    clearToken();
    window.location.href = '/login.html';
  });
});
