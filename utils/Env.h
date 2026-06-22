#pragma once

#include <cstdlib>
#include <string>

namespace util
{
// Значение переменной окружения или дефолт (если не задана/пустая).
inline std::string envOr(const char *name, const std::string &def)
{
    const char *v = std::getenv(name);
    return (v != nullptr && *v != '\0') ? std::string(v) : def;
}
}  // namespace util
