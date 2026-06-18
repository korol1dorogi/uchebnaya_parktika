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

## Установка Drogon

Drogon в системе пока не установлен. Выберите один из вариантов.

### Вариант 1 — MSYS2 (подходит, т.к. установлен g++ от MSYS2)

```bash
pacman -S mingw-w64-x86_64-drogon
```

### Вариант 2 — vcpkg

```bash
git clone https://github.com/microsoft/vcpkg
./vcpkg/bootstrap-vcpkg.bat
./vcpkg/vcpkg install drogon
```

## Сборка

```bash
cmake -S . -B build
cmake --build build
```

Для vcpkg добавьте toolchain-файл:

```bash
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=<путь к vcpkg>/scripts/buildsystems/vcpkg.cmake
cmake --build build
```

## Запуск

```bash
cd build
./praktika        # Windows: praktika.exe
```

Сервис стартует на `http://127.0.0.1:8080`.

## Проверка

```bash
curl http://127.0.0.1:8080/api/health
# {"service":"praktika","status":"ok"}
```

## Добавление нового роута

1. Создайте контроллер в `controllers/`, унаследовав
   `drogon::HttpController<YourController>`.
2. Опишите методы в блоке `METHOD_LIST_BEGIN ... METHOD_LIST_END`
   через `ADD_METHOD_TO(...)`.
3. Пересоберите проект — новый файл подхватится автоматически.
