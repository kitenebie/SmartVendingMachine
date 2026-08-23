// ============================================================
//  products.js -- Full product CRUD management (no price)
// ============================================================

let allProducts = [];
let editingId   = null;
let deletingId  = null;

// ---- Utilities ----------------------------------------------

function escapeHtml(str) {
  return String(str)
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;');
}

function formatNum(n) { return Number(n).toLocaleString('en-PH'); }

// ---- Toast --------------------------------------------------

function showToast(msg, type = 'success') {
  const container = document.getElementById('toastContainer');
  const toast = document.createElement('div');
  toast.className = 'toast toast-' + type;
  toast.textContent = msg;
  container.appendChild(toast);
  setTimeout(() => toast.remove(), 3000);
}

// ---- Render table -------------------------------------------

function renderTable(products) {
  const tbody = document.getElementById('productBody');
  if (!products || products.length === 0) {
    tbody.innerHTML = '<tr><td colspan="5" class="text-center text-muted" style="padding:32px">No products found. Click "+ Add Product" to get started.</td></tr>';
    return;
  }
  tbody.innerHTML = products.map((p, i) => `
    <tr>
      <td class="text-muted text-sm">${i + 1}</td>
      <td>
        <strong>${escapeHtml(p.name)}</strong>
        ${p.stocks <= 10 ? '<span class="badge-low">Low</span>' : ''}
      </td>
      <td class="text-right">${formatNum(p.stocks)}</td>
      <td class="text-right">${formatNum(p.price)} pcs</td>
      <td class="text-center">
        <div class="action-btns">
          <button class="btn-icon btn-icon-edit" onclick="openEditModal(${p.id})" title="Edit">
            <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M11 4H4a2 2 0 00-2 2v14a2 2 0 002 2h14a2 2 0 002-2v-7"/><path d="M18.5 2.5a2.121 2.121 0 013 3L12 15l-4 1 1-4 9.5-9.5z"/></svg>
          </button>
          <button class="btn-icon btn-icon-delete" onclick="openDeleteModal(${p.id}, '${escapeHtml(p.name)}')" title="Delete">
            <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polyline points="3 6 5 6 21 6"/><path d="M19 6l-1 14a2 2 0 01-2 2H8a2 2 0 01-2-2L5 6"/><path d="M10 11v6M14 11v6"/><path d="M9 6V4a1 1 0 011-1h4a1 1 0 011 1v2"/></svg>
          </button>
        </div>
      </td>
    </tr>
  `).join('');
}

// ---- Load products ------------------------------------------

async function loadProducts() {
  try {
    const r = await fetch('/api/products', { headers: authHeaders() });
    if (r.status === 401) { clearToken(); window.location.href = '/login.html'; return; }
    const data = await r.json();
    if (!data.success) { showToast('Failed to load products', 'error'); return; }
    allProducts = data.products || [];
    renderTable(allProducts);
  } catch (err) {
    showToast('Network error loading products', 'error');
    console.error(err);
  }
}

// ---- Modal helpers ------------------------------------------

function openModal(title) {
  document.getElementById('modalTitle').textContent = title;
  document.getElementById('productModal').classList.add('open');
  document.getElementById('productName').focus();
  document.getElementById('modalAlert').className = 'alert hidden';
  clearModalErrors();
}

function closeModal() {
  document.getElementById('productModal').classList.remove('open');
  document.getElementById('productForm').reset();
  document.getElementById('productId').value = '';
  editingId = null;
}

function clearModalErrors() {
  ['nameErr','stocksErr','priceErr'].forEach(id => {
    const el = document.getElementById(id);
    if (el) el.textContent = '';
  });
}

function showModalError(msg) {
  const el = document.getElementById('modalAlert');
  el.textContent = msg;
  el.className = 'alert alert-error';
}

// ---- Add product --------------------------------------------

function openAddModal() {
  editingId = null;
  document.getElementById('productId').value = '';
  document.getElementById('productName').value  = '';
  document.getElementById('productStocks').value = '';
  document.getElementById('productPrice').value  = '';
  openModal('Add Product');
  document.getElementById('saveProductBtn').textContent = 'Save Product';
}

// ---- Edit product -------------------------------------------

function openEditModal(id) {
  const product = allProducts.find(p => p.id === id);
  if (!product) return;
  editingId = id;
  document.getElementById('productId').value      = product.id;
  document.getElementById('productName').value    = product.name;
  document.getElementById('productStocks').value  = product.stocks;
  document.getElementById('productPrice').value   = product.price;
  openModal('Edit Product');
  document.getElementById('saveProductBtn').textContent = 'Update Product';
}

// ---- Delete modal -------------------------------------------

function openDeleteModal(id, name) {
  deletingId = id;
  document.getElementById('deleteProductName').textContent = name;
  document.getElementById('deleteModal').classList.add('open');
}

function closeDeleteModal() {
  document.getElementById('deleteModal').classList.remove('open');
  deletingId = null;
}

// ---- Validate form ------------------------------------------

function validateProductForm() {
  clearModalErrors();
  let valid = true;

  const name   = document.getElementById('productName').value.trim();
  const stocks = document.getElementById('productStocks').value;
  const price  = document.getElementById('productPrice').value;

  if (!name) {
    document.getElementById('nameErr').textContent = 'Product name is required';
    valid = false;
  }
  if (stocks === '' || isNaN(parseInt(stocks)) || parseInt(stocks) < 0) {
    document.getElementById('stocksErr').textContent = 'Stocks must be a non-negative integer';
    valid = false;
  }
  if (price === '' || isNaN(parseInt(price)) || parseInt(price) < 0) {
    document.getElementById('priceErr').textContent = 'Required bottles must be a non-negative integer';
    valid = false;
  }
  return valid;
}

// ---- Save product -------------------------------------------

async function saveProduct() {
  if (!validateProductForm()) return;

  const btn = document.getElementById('saveProductBtn');
  btn.disabled = true;
  btn.textContent = editingId ? 'Updating...' : 'Saving...';

  const payload = {
    name:   document.getElementById('productName').value.trim(),
    stocks: parseInt(document.getElementById('productStocks').value),
    price:  parseInt(document.getElementById('productPrice').value)
  };

  try {
    let r;
    if (editingId) {
      r = await fetch('/api/product?id=' + editingId, {
        method: 'PUT',
        headers: authHeaders(),
        body: JSON.stringify(payload)
      });
    } else {
      r = await fetch('/api/products', {
        method: 'POST',
        headers: authHeaders(),
        body: JSON.stringify(payload)
      });
    }

    const data = await r.json();
    if (data.success) {
      closeModal();
      showToast(editingId ? 'Product updated!' : 'Product added!', 'success');
      await loadProducts();
    } else {
      showModalError(data.message || 'Operation failed');
    }
  } catch (err) {
    showModalError('Network error. Please try again.');
    console.error(err);
  } finally {
    btn.disabled = false;
    btn.textContent = editingId ? 'Update Product' : 'Save Product';
  }
}

// ---- Delete product -----------------------------------------

async function confirmDelete() {
  if (!deletingId) return;

  const btn = document.getElementById('confirmDeleteBtn');
  btn.disabled = true;
  btn.textContent = 'Deleting...';

  try {
    const r = await fetch('/api/product?id=' + deletingId, {
      method: 'DELETE',
      headers: authHeaders()
    });
    const data = await r.json();
    if (data.success) {
      closeDeleteModal();
      showToast('Product deleted', 'success');
      await loadProducts();
    } else {
      showToast(data.message || 'Delete failed', 'error');
      closeDeleteModal();
    }
  } catch (err) {
    showToast('Network error', 'error');
    closeDeleteModal();
    console.error(err);
  } finally {
    btn.disabled = false;
    btn.textContent = 'Delete';
  }
}

// ---- Event listeners ----------------------------------------

document.addEventListener('DOMContentLoaded', async () => {
  const username = await checkAuth();
  if (!username) return;

  const usernameEl = document.getElementById('topbarUsername');
  if (usernameEl) usernameEl.textContent = username;

  loadProducts();

  document.getElementById('addProductBtn').addEventListener('click', openAddModal);
  document.getElementById('saveProductBtn').addEventListener('click', saveProduct);
  document.getElementById('cancelModalBtn').addEventListener('click', closeModal);
  document.getElementById('modalClose').addEventListener('click', closeModal);

  document.getElementById('confirmDeleteBtn').addEventListener('click', confirmDelete);
  document.getElementById('cancelDeleteBtn').addEventListener('click', closeDeleteModal);
  document.getElementById('deleteModalClose').addEventListener('click', closeDeleteModal);

  // Close modals on backdrop click
  document.getElementById('productModal').addEventListener('click', (e) => {
    if (e.target === e.currentTarget) closeModal();
  });
  document.getElementById('deleteModal').addEventListener('click', (e) => {
    if (e.target === e.currentTarget) closeDeleteModal();
  });

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
