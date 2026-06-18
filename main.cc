#include <drogon/drogon.h>

int main()
{
    // Load listeners, logging, database and other settings from config.json.
    // Controllers in the controllers/ folder register themselves automatically.
    drogon::app().loadConfigFile("config.json");

    LOG_INFO << "praktika service starting on http://127.0.0.1:8080";

    drogon::app().run();
    return 0;
}
