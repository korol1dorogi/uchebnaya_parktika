#pragma once

#include <string>

// Модель пользователя (таблица users).
// Пароль хранится не классическим хэшем, а строкой от микросервиса hasher
// в формате salt_hex:iterations:hash_hex (см. hasher/app.py).
struct User
{
    std::string uuid;      // PK
    std::string login;     // уникальный логин
    std::string password;  // хранимая строка salt:iters:hash
    std::string salt;      // соль отдельным полем (по ТЗ)
};
