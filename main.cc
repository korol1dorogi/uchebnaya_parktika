#include <drogon/drogon.h>

#include <string>

#include "Env.h"

int main()
{
    // Load listeners, logging, database and other settings from config.json.
    // Controllers in the controllers/ folder register themselves automatically.
    drogon::app().loadConfigFile("config.json");

    // Не экранировать не-ASCII в JSON: кириллица отдаётся как UTF-8,
    // а не как \uXXXX.
    drogon::app().setUnicodeEscapingInJson(false);

    const std::string dbHost = util::envOr("DB_HOST", "");
    if (!dbHost.empty())
    {
        drogon::app().createDbClient(
            "postgresql",
            dbHost,
            static_cast<unsigned short>(std::stoi(util::envOr("DB_PORT", "5432"))),
            util::envOr("DB_NAME", "praktika"),
            util::envOr("DB_USER", "praktika"),
            util::envOr("DB_PASSWORD", ""),
            1);  // число соединений
        LOG_INFO << "DB client configured for host " << dbHost;
    }
    else
    {
        LOG_WARN << "DB_HOST не задан — функции работы с БД будут недоступны";
    }

    LOG_INFO << "praktika service starting on http://127.0.0.1:8081";

    drogon::app().run();
    return 0;
}
