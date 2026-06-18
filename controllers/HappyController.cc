#include "HappyController.h"

#include "Helpers.h"

#include <drogon/HttpClient.h>
#include <drogon/orm/DbClient.h>
#include <drogon/orm/Result.h>
#include <trantor/utils/Date.h>

#include <exception>
#include <mutex>
#include <stdexcept>
#include <string>

using namespace helpers;

namespace
{
// Модель Groq (быстрая, бесплатная для генерации короткого текста).
constexpr const char *kGroqHost = "https://api.groq.com";
constexpr const char *kGroqPath = "/openai/v1/chat/completions";
constexpr const char *kGroqModel = "llama-3.1-8b-instant";
constexpr const char *kSystemPrompt =
    "Сгенерируй позитивное хокку желающее пользователю хорошего дня";

// Последнее выданное хокку — чтобы определять повтор.
std::mutex g_lastMutex;
std::string g_lastHaiku;

Json::Value buildBody(bool withSalt)
{
    Json::Value sys;
    sys["role"] = "system";
    sys["content"] = kSystemPrompt;

    Json::Value messages(Json::arrayValue);
    messages.append(sys);

    // Соль добавляем только при повторе: текущее время (с микросекундами).
    if (withSalt)
    {
        Json::Value usr;
        usr["role"] = "user";
        usr["content"] = "в качестве соли используй " +
                         trantor::Date::now().toFormattedString(true);
        messages.append(usr);
    }

    Json::Value body;
    body["model"] = kGroqModel;
    body["temperature"] = 1.0;
    body["max_tokens"] = 256;
    body["messages"] = messages;
    return body;
}

// Запрос хокку у Groq. Бросает исключение при ошибке.
Task<std::string> requestHaiku(std::string apiKey, bool withSalt)
{
    auto client = HttpClient::newHttpClient(kGroqHost);
    auto req = HttpRequest::newHttpJsonRequest(buildBody(withSalt));
    req->setMethod(Post);
    req->setPath(kGroqPath);
    req->addHeader("Authorization", std::string("Bearer ") + apiKey);

    auto resp = co_await client->sendRequestCoro(req);
    if (resp->statusCode() != k200OK)
        throw std::runtime_error("Groq вернул статус " +
                                 std::to_string(resp->statusCode()));
    auto json = resp->getJsonObject();
    if (!json || !json->isMember("choices") || (*json)["choices"].empty())
        throw std::runtime_error("Неожиданный ответ Groq");
    co_return (*json)["choices"][0]["message"]["content"].asString();
}
}  // namespace

Task<> HappyController::happy(
    HttpRequestPtr req, std::function<void(const HttpResponsePtr &)> callback)
{
    const std::string key = envOr("GROQ_API_KEY", "");
    if (key.empty())
    {
        callback(jsonError(k500InternalServerError,
                           "GROQ_API_KEY не задан (см. README)"));
        co_return;
    }

    std::string content;
    try
    {
        content = co_await requestHaiku(key, /*withSalt=*/false);

        bool repeated;
        {
            std::lock_guard<std::mutex> lock(g_lastMutex);
            repeated = (content == g_lastHaiku);
        }
        // Повтор — повторяем запрос с солью (текущее время).
        if (repeated)
            content = co_await requestHaiku(key, /*withSalt=*/true);

        {
            std::lock_guard<std::mutex> lock(g_lastMutex);
            g_lastHaiku = content;
        }
    }
    catch (const std::exception &e)
    {
        callback(jsonError(k502BadGateway, e.what()));
        co_return;
    }

    // Сохраняем хокку в БД (на пользователя или анонимно). Ошибку сохранения
    // не считаем фатальной — хокку всё равно возвращаем.
    std::string id;
    try
    {
        auto db = app().getDbClient();
        if (db)
        {
            const std::string me = currentUserUuid(req);
            orm::Result r =
                me.empty()
                    ? co_await db->execSqlCoro(
                          "INSERT INTO haikus(text) VALUES($1) RETURNING id",
                          content)
                    : co_await db->execSqlCoro(
                          "INSERT INTO haikus(text, user_uuid) VALUES($1,$2) "
                          "RETURNING id",
                          content, me);
            id = r[0]["id"].as<std::string>();
        }
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << "Не удалось сохранить хокку в БД: " << e.what();
    }

    Json::Value out;
    out["message"] = "Hello, World!";
    out["haiku"] = content;
    if (!id.empty())
        out["id"] = id;
    callback(HttpResponse::newHttpJsonResponse(out));
    co_return;
}
