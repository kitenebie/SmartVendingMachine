// ============================================================
//  dashboard.js -- Dashboard statistics and product summary
// ============================================================

/** Format a number as Philippine Peso currency. */
function formatPeso(value) {
  return '\u20B1' + Number(value).toLocaleString('en-PH', {
    minimumFractionDigits: 2,
    maximumFractionDigits: 2
  });
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
    document.getElementById('statValue').textContent    = formatPeso(s.totalValue);
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

    // Sort by total value descending
    const sorted = [...data.products].sort((a, b) => b.totalValue - a.totalValue);
    tbody.innerHTML = sorted.map(p => `
      <tr>
        <td>${escapeHtml(p.name)}
          ${p.stocks <= 10 ? '<span class="badge-low">Low</span>' : ''}
        </td>
        <td class="text-right">${formatNumber(p.stocks)}</td>
        <td class="text-right">${formatPeso(p.price)}</td>
        <td class="text-right">${formatPeso(p.totalValue)}</td>
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
});
