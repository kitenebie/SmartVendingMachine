const tokenKey = 'rvm_token';
const token = () => sessionStorage.getItem(tokenKey) || '';
async function api(path, options = {}) { const headers = {'Content-Type':'application/json', ...(options.headers || {})}; if (token()) headers['X-Session-Token'] = token(); const response = await fetch(path, {...options, headers}); const data = await response.json().catch(() => ({})); if (!response.ok) throw new Error(data.error || 'Request failed'); return data; }
function logout() { api('/api/logout', {method:'POST'}).catch(()=>{}).finally(() => { sessionStorage.removeItem(tokenKey); location.href='/login.html'; }); }
