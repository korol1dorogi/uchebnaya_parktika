#include "PageController.h"

namespace
{
HttpResponsePtr htmlResponse(const std::string &body)
{
    auto resp = HttpResponse::newHttpResponse();
    resp->setContentTypeCode(CT_TEXT_HTML);
    resp->setBody(body);
    return resp;
}

// ---- Главная страница ----
const char *kMainHtml = R"HTML(<!DOCTYPE html>
<html lang="ru">
<head><meta charset="utf-8"><title>praktika — хокку</title></head>
<body>
<div id="nav"></div>
<hr>
<h1>Генератор хокку</h1>
<button id="genBtn">Сгенерировать хокку</button>
<div id="gen"></div>
<hr>
<h2>Мои хокку</h2>
<div id="mine">—</div>
<hr>
<h2>Все хокку</h2>
<div id="all">—</div>

<script>
let ME = { authenticated: false };

async function api(path, opts) {
  const o = Object.assign({ headers: { 'Content-Type': 'application/json' } }, opts || {});
  const r = await fetch(path, o);
  let data = null;
  try { data = await r.json(); } catch (e) {}
  return { ok: r.ok, status: r.status, data };
}

async function loadMe() {
  const { data } = await api('/api/me');
  ME = data || { authenticated: false };
  const nav = document.getElementById('nav');
  nav.textContent = '';
  if (ME.authenticated) {
    nav.textContent = 'Вы вошли как ' + ME.login + '. ';
    const b = document.createElement('button');
    b.textContent = 'Выйти';
    b.onclick = logout;
    nav.appendChild(b);
  } else {
    nav.innerHTML = '<a href="/login">Войти</a> | <a href="/register">Регистрация</a>';
  }
}

async function logout() {
  await api('/api/logout', { method: 'POST' });
  location.reload();
}

async function generate() {
  const g = document.getElementById('gen');
  g.textContent = 'Генерируем…';
  const { ok, data } = await api('/api/happy');
  if (!ok) { g.textContent = 'Ошибка: ' + (data && data.error || ''); return; }
  g.innerHTML = '';
  const pre = document.createElement('pre');
  pre.textContent = data.haiku;
  g.appendChild(pre);
  loadLists();
}

function renderList(container, items) {
  container.innerHTML = '';
  if (!items || !items.length) { container.textContent = 'пусто'; return; }
  for (const h of items) {
    const div = document.createElement('div');
    const pre = document.createElement('pre');
    pre.textContent = h.text;
    div.appendChild(pre);
    const meta = document.createElement('small');
    meta.textContent = 'автор: ' + (h.author || 'аноним') + ', ' + h.created_at;
    div.appendChild(meta);
    if (h.comment) {
      const c = document.createElement('div');
      c.textContent = 'комментарий: ' + h.comment;
      div.appendChild(c);
    }
    if (h.is_mine) {
      div.appendChild(document.createElement('br'));
      const inp = document.createElement('input');
      inp.placeholder = 'комментарий';
      inp.value = h.comment || '';
      const save = document.createElement('button');
      save.textContent = 'Сохранить';
      save.onclick = () => saveComment(h.id, inp.value);
      const del = document.createElement('button');
      del.textContent = 'Удалить';
      del.onclick = () => removeHaiku(h.id);
      div.appendChild(inp);
      div.appendChild(save);
      div.appendChild(del);
    }
    div.appendChild(document.createElement('hr'));
    container.appendChild(div);
  }
}

async function loadLists() {
  const all = await api('/api/haikus');
  renderList(document.getElementById('all'), all.data);
  const mine = document.getElementById('mine');
  if (ME.authenticated) {
    const my = await api('/api/haikus/my');
    renderList(mine, my.data);
  } else {
    mine.textContent = 'Войдите, чтобы видеть свои хокку.';
  }
}

async function saveComment(id, value) {
  const { ok, data } = await api('/api/haikus/' + id + '/comment',
    { method: 'PUT', body: JSON.stringify({ comment: value }) });
  if (!ok) { alert('Ошибка: ' + (data && data.error || '')); return; }
  loadLists();
}

async function removeHaiku(id) {
  if (!confirm('Удалить хокку?')) return;
  const { ok, data } = await api('/api/haikus/' + id, { method: 'DELETE' });
  if (!ok) { alert('Ошибка: ' + (data && data.error || '')); return; }
  loadLists();
}

document.getElementById('genBtn').onclick = generate;
(async function () { await loadMe(); await loadLists(); })();
</script>
</body>
</html>
)HTML";

// ---- Страница входа ----
const char *kLoginHtml = R"HTML(<!DOCTYPE html>
<html lang="ru">
<head><meta charset="utf-8"><title>Вход</title></head>
<body>
<h1>Вход</h1>
<form id="f">
  <p>Логин: <input id="login"></p>
  <p>Пароль: <input id="password" type="password"></p>
  <button type="submit">Войти</button>
</form>
<p id="msg"></p>
<p><a href="/register">Регистрация</a> | <a href="/">На главную</a></p>
<script>
document.getElementById('f').addEventListener('submit', async function (e) {
  e.preventDefault();
  const body = JSON.stringify({
    login: document.getElementById('login').value,
    password: document.getElementById('password').value
  });
  const r = await fetch('/api/login', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body });
  const data = await r.json().catch(() => ({}));
  if (r.ok) { location.href = '/'; }
  else { document.getElementById('msg').textContent = 'Ошибка: ' + (data.error || r.status); }
});
</script>
</body>
</html>
)HTML";

// ---- Страница регистрации ----
const char *kRegisterHtml = R"HTML(<!DOCTYPE html>
<html lang="ru">
<head><meta charset="utf-8"><title>Регистрация</title></head>
<body>
<h1>Регистрация</h1>
<form id="f">
  <p>Логин: <input id="login"></p>
  <p>Пароль: <input id="password" type="password"></p>
  <button type="submit">Зарегистрироваться</button>
</form>
<p id="msg"></p>
<p><a href="/login">Вход</a> | <a href="/">На главную</a></p>
<script>
document.getElementById('f').addEventListener('submit', async function (e) {
  e.preventDefault();
  const body = JSON.stringify({
    login: document.getElementById('login').value,
    password: document.getElementById('password').value
  });
  const r = await fetch('/api/register', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body });
  const data = await r.json().catch(() => ({}));
  if (r.ok) { location.href = '/'; }
  else { document.getElementById('msg').textContent = 'Ошибка: ' + (data.error || r.status); }
});
</script>
</body>
</html>
)HTML";
}  // namespace

void PageController::index(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) const
{
    callback(htmlResponse(kMainHtml));
}

void PageController::loginPage(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) const
{
    callback(htmlResponse(kLoginHtml));
}

void PageController::registerPage(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) const
{
    callback(htmlResponse(kRegisterHtml));
}
