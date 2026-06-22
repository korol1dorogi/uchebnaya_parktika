#pragma once

#include <cctype>
#include <string>

namespace util
{
// Логин: 3..32 символа, латиница / цифры / подчёркивание.
inline bool isValidLogin(const std::string &login)
{
    if (login.size() < 3 || login.size() > 32)
        return false;
    for (unsigned char c : login)
        if (!(std::isalnum(c) || c == '_'))
            return false;
    return true;
}

// Пароль: 4..128 символов.
inline bool isValidPassword(const std::string &password)
{
    return password.size() >= 4 && password.size() <= 128;
}
}  // namespace util
