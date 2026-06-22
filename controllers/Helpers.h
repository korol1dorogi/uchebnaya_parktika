#pragma once

#include <drogon/HttpController.h>

#include <string>

#include "Env.h"

// Небольшие общие помощники для контроллеров.
namespace helpers
{
// envOr вынесен в utils/Env.h; реэкспортируем для вызовов через helpers::.
using util::envOr;

// UUID текущего пользователя из сессии ("" если аноним / не вошёл).
inline std::string currentUserUuid(const drogon::HttpRequestPtr &req)
{
    auto session = req->session();
    if (session && session->find("user_uuid"))
        return session->get<std::string>("user_uuid");
    return {};
}

// Ответ-ошибка в JSON.
inline drogon::HttpResponsePtr jsonError(drogon::HttpStatusCode code,
                                         const std::string &message)
{
    Json::Value j;
    j["error"] = message;
    auto resp = drogon::HttpResponse::newHttpJsonResponse(j);
    resp->setStatusCode(code);
    return resp;
}
}  // namespace helpers
