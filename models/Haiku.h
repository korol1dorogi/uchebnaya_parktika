#pragma once

#include <string>

// Модель хокку (таблица haikus).
struct Haiku
{
    std::string id;         // PK
    std::string text;       // текст хокку
    std::string userUuid;   // для кого сгенерировано ("" = аноним)
    std::string comment;    // комментарий пользователя (может отсутствовать)
    std::string createdAt;  // время создания
};
