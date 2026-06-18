#pragma once

#include <drogon/HttpController.h>
#include <drogon/utils/coroutine.h>

using namespace drogon;

// Регистрация / вход / выход / «кто я». Пароли хэшируются внешним
// микросервисом (hasher), сессия хранится средствами Drogon (cookie SESSION).
class AuthController : public drogon::HttpController<AuthController>
{
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(AuthController::registerUser, "/api/register", Post);
    ADD_METHOD_TO(AuthController::login, "/api/login", Post);
    ADD_METHOD_TO(AuthController::logout, "/api/logout", Post);
    ADD_METHOD_TO(AuthController::me, "/api/me", Get);
    METHOD_LIST_END

    Task<> registerUser(HttpRequestPtr req,
                        std::function<void(const HttpResponsePtr &)> callback);
    Task<> login(HttpRequestPtr req,
                 std::function<void(const HttpResponsePtr &)> callback);
    Task<> logout(HttpRequestPtr req,
                  std::function<void(const HttpResponsePtr &)> callback);
    Task<> me(HttpRequestPtr req,
              std::function<void(const HttpResponsePtr &)> callback);
};
