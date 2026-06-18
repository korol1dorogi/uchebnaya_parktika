# CLAUDE.md

Учебная практика. Веб-сервис на **Drogon (C++20)**: генерирует позитивные
хокку через Groq, с регистрацией/входом, хранением в PostgreSQL и отдельным
Python-микросервисом хэширования паролей.

## Архитектура (docker compose, одна сеть)

- **praktika** — основной сервис на Drogon. В контейнере слушает `8081`,
  наружу опубликован на хост-порт **8082** (`8082:8081`), т.к. `8080/8081`
  на машине разработки заняты (EDB PEM и др.).
- **hasher** — Python/Flask микросервис хэширования (`hasher/app.py`).
  Намеренно НЕ классический хэш: своя `simple_kdf`. Формат хранимой строки
  `salt_hex:iterations:hash_hex`. Эндпоинты: `POST /hash`, `POST /verify`.
- **db** — PostgreSQL 16. Схема в `db/init.sql` (выполняется при первом старте
  тома): таблицы `users(uuid, login, password, salt)` и
  `haikus(id, text, user_uuid NULL, comment NULL, created_at)`.

praktika сам пароли не хэширует — ходит в `hasher`. Авторизация — серверные
сессии Drogon (cookie `SESSION`, in-memory, сбрасывается при рестарте).

## Сборка и запуск

```bash
docker compose up -d --build      # весь стек
docker logs -f praktika           # логи
docker compose down               # стоп
```
Открыть: <http://127.0.0.1:8082/>. Нужен запущенный Docker Desktop.

**Секреты — только в `.env`** (он в `.gitignore`, шаблон — `.env.example`):
`GROQ_API_KEY` и `DB_PASSWORD`. compose читает `.env` автоматически.

> Локальная сборка без Docker: mingw-сборка Drogon на машине разработки собрана
> **без** PostgreSQL, поэтому функции БД работают только в Docker. Чистая
> локальная сборка: `cmake -S . -B build -G Ninja && cmake --build build`.

## Карта кода

- `main.cc` — точка входа; грузит `config.json`, отключает экранирование
  юникода в JSON (`setUnicodeEscapingInJson(false)`), создаёт DB-клиент из env
  (`DB_HOST/PORT/NAME/USER/PASSWORD` — пароль НЕ в config.json).
- `config.json` — листенер 8081, логи в stdout (`log_path:""`), сессии вкл.
- `controllers/` (подхватываются `aux_source_directory` автоматически):
  - `ApiController` — `GET /api/health`.
  - `HappyController` — `GET /api/happy`: хокку от Groq + автосейв в БД
    (на пользователя из сессии или анонимно). Корутина.
  - `AuthController` — `/api/register|login|logout|me`. Корутины.
  - `HaikuController` — CRUD: `GET /api/haikus`, `GET /api/haikus/my`,
    `PUT /api/haikus/{id}/comment`, `DELETE /api/haikus/{id}`. Корутины.
  - `PageController` — HTML-страницы `/`, `/happy`, `/login`, `/register`
    (HTML встроен строками; без сложного CSS).
  - `Helpers.h` — `envOr`, `currentUserUuid`, `jsonError`.
- `models/User.h`, `models/Haiku.h` — простые структуры-модели.
- `hasher/` — Python-микросервис (`app.py`, `Dockerfile`, `requirements.txt`).
- `db/init.sql` — схема БД.

## Доступ к БД (Drogon, C++20 корутины)

```cpp
auto db = app().getDbClient();
auto r = co_await db->execSqlCoro("SELECT ... WHERE x=$1", val);
// r[i]["col"].as<std::string>(), r.affectedRows(), field.isNull()
```
HTTP к hasher/Groq — `co_await client->sendRequestCoro(req)`.

## Конвенции

- **Коммиты на русском, без трейлера `Co-Authored-By`.**
- Секреты не коммитить (только через `.env`).
- Drogon-конвенция: контроллеры в `controllers/`, регистрируются сами.
- `simple_kdf` — учебная, НЕ криптостойкая; для реального прод-кода нужен
  argon2/scrypt.

## Прочее

- Groq: модель `llama-3.1-8b-instant`, системный промт — генерация позитивного
  хокку; «соль» (текущее время) добавляется только при повторе ответа.
- Порт хоста для praktika — `8082` (см. выше, почему не 8081).
