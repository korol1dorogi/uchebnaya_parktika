#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

// HTML-страницы (без сложного CSS). Логику данных тянут через fetch к /api/*.
//   /          и /happy  — главная: генерация + «мои хокку» + «все хокку»
//   /login               — вход
//   /register            — регистрация
class PageController : public drogon::HttpController<PageController>
{
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(PageController::index, "/", Get);
    ADD_METHOD_TO(PageController::index, "/happy", Get);
    ADD_METHOD_TO(PageController::loginPage, "/login", Get);
    ADD_METHOD_TO(PageController::registerPage, "/register", Get);
    METHOD_LIST_END

    void index(const HttpRequestPtr &req,
               std::function<void(const HttpResponsePtr &)> &&callback) const;
    void loginPage(const HttpRequestPtr &req,
                   std::function<void(const HttpResponsePtr &)> &&callback) const;
    void registerPage(const HttpRequestPtr &req,
                      std::function<void(const HttpResponsePtr &)> &&callback) const;
};
