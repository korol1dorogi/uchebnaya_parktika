#pragma once

#include <drogon/HttpController.h>
#include <drogon/utils/coroutine.h>

using namespace drogon;

// CRUD по хокку:
//   R  GET    /api/haikus          — все хокку (вкл. анонимные и чужие)
//   R  GET    /api/haikus/my       — хокку текущего пользователя
//   U  PUT    /api/haikus/{id}/comment — прокомментировать своё хокку
//   D  DELETE /api/haikus/{id}     — удалить своё хокку
// (Create происходит при генерации в HappyController.)
class HaikuController : public drogon::HttpController<HaikuController>
{
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(HaikuController::listAll, "/api/haikus", Get);
    ADD_METHOD_TO(HaikuController::listMine, "/api/haikus/my", Get);
    ADD_METHOD_TO(HaikuController::updateComment, "/api/haikus/{1}/comment", Put);
    ADD_METHOD_TO(HaikuController::remove, "/api/haikus/{1}", Delete);
    METHOD_LIST_END

    Task<> listAll(HttpRequestPtr req,
                   std::function<void(const HttpResponsePtr &)> callback);
    Task<> listMine(HttpRequestPtr req,
                    std::function<void(const HttpResponsePtr &)> callback);
    Task<> updateComment(HttpRequestPtr req,
                         std::function<void(const HttpResponsePtr &)> callback,
                         std::string id);
    Task<> remove(HttpRequestPtr req,
                  std::function<void(const HttpResponsePtr &)> callback,
                  std::string id);
};
