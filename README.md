# praktika

Веб-сервис на C++ с использованием фреймворка [Drogon](https://github.com/drogonframework/drogon).

## Структура проекта

```
praktika/
├── CMakeLists.txt        # сборка проекта
├── config.json           # конфигурация Drogon (порт, логи, БД)
├── main.cc               # точка входа
├── controllers/          # HTTP-контроллеры (роуты API)
│   ├── ApiController.h
│   └── ApiController.cc
├── filters/              # фильтры (middleware)
├── plugins/              # плагины
├── models/               # ORM-модели
└── views/                # CSP-шаблоны (HTML)
```

Файлы в `controllers/`, `filters/`, `plugins/`, `models/` подхватываются
сборкой автоматически (см. `aux_source_directory` в `CMakeLists.txt`).
Контроллеры регистрируются в Drogon самостоятельно — ручной регистрации не нужно.

## Окружение

Сборка ведётся тулчейном **MSYS2 / mingw64** (g++ 16.1, CMake, Ninja).
В репозиториях MSYS2 **нет** готового пакета Drogon, поэтому фреймворк
собирается из исходников один раз и устанавливается в префикс mingw64
(`C:/msys64/mingw64`), после чего `find_package(Drogon)` находит его сам.

### 1. Зависимости (через pacman)

```bash
# при необходимости сначала полностью обновить систему: pacman -Syu
pacman -S --needed \
  mingw-w64-x86_64-gcc \
  mingw-w64-x86_64-cmake \
  mingw-w64-x86_64-ninja \
  mingw-w64-x86_64-jsoncpp \
  mingw-w64-x86_64-c-ares \
  mingw-w64-x86_64-zlib \
  mingw-w64-x86_64-openssl \
  mingw-w64-x86_64-sqlite3 \
  mingw-w64-x86_64-brotli
```

### 2. Сборка и установка самого Drogon из исходников

```bash
git clone --recurse-submodules --depth 1 \
  https://github.com/drogonframework/drogon.git
cd drogon
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=C:/msys64/mingw64 \
  -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ \
  -DBUILD_EXAMPLES=OFF -DBUILD_TESTING=OFF -DBUILD_REDIS=OFF
cmake --build build --parallel
cmake --install build
```

> `-DBUILD_REDIS=OFF` обязателен, если в системе нет hiredis: иначе
> `FindHiredis.cmake` падает на этапе конфигурации.

## Сборка проекта

Mingw64 должен быть в `PATH` (`C:\msys64\mingw64\bin`).

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++
cmake --build build
```

## Запуск

```bash
cd build
./praktika.exe
```

Сервис стартует на `http://127.0.0.1:8081`.

> **Порт 8081, а не 8080.** На этой машине порт 8080 занят Apache из состава
> EDB PEM (`httpd.exe`). Если 8081 тоже окажется занят — поменяйте `port`
> в `config.json`. Проверить занятость: `netstat -ano | grep :8081`.

## Проверка

```bash
curl http://127.0.0.1:8081/api/health
# {"service":"praktika","status":"ok"}
```

## Docker

Проект собирается в образ multi-stage сборкой на официальном образе
`drogonframework/drogon` (в нём уже есть фреймворк и тулчейн).

```bash
# сборка и запуск в фоне
docker compose up -d --build

# логи / статус / остановка
docker logs -f praktika
docker ps
docker compose down

# либо вручную
docker build -t praktika:latest .
docker run --rm -t -p 8082:8081 praktika:latest
```

Сервис будет доступен на `http://127.0.0.1:8082/api/health`.

**Про порты:** внутри контейнера сервис слушает `8081`, а наружу публикуется
`8082` (`8082:8081` в compose) — на хосте `8080`/`8081` часто заняты (EDB PEM,
другие контейнеры). Свободный хостовый порт можно поменять в `docker-compose.yml`.

В контейнере настроен `HEALTHCHECK` (статус `healthy` виден в `docker ps`).
Логи Drogon выводятся в stdout; для их видимости в `docker logs` контейнеру
выделяется псевдо-TTY (`tty: true`), иначе вывод буферизуется.

> Требуется запущенный движок Docker (Docker Desktop). Первая сборка
> скачивает базовый образ Drogon (~2 ГБ) — это может занять время.

## Добавление нового роута

1. Создайте контроллер в `controllers/`, унаследовав
   `drogon::HttpController<YourController>`.
2. Опишите методы в блоке `METHOD_LIST_BEGIN ... METHOD_LIST_END`
   через `ADD_METHOD_TO(...)`.
3. Пересоберите проект — новый файл подхватится автоматически.

Утилита `drogon_ctl` (устанавливается вместе с Drogon) умеет генерировать
заготовки: `drogon_ctl create controller MyController`.
