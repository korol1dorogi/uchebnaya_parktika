#pragma once

#include <drogon/HttpController.h>
#include <drogon/utils/coroutine.h>

using namespace drogon;

// GET /api/happy -> {"message":"Hello, World!", "haiku":"...", "id":"..."}
// Хокку генерирует Groq, затем оно сохраняется в БД: на текущего пользователя
// (если вошёл) или как анонимное (user_uuid = NULL).
class HappyController : public drogon::HttpController<HappyController>
{
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(HappyController::happy, "/api/happy", Get);
    METHOD_LIST_END

    Task<> happy(HttpRequestPtr req,
                 std::function<void(const HttpResponsePtr &)> callback);
};
