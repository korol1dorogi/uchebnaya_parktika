#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

// Example HTTP API controller. Drogon discovers and instantiates it
// automatically at startup — no manual registration needed.
class ApiController : public drogon::HttpController<ApiController>
{
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(ApiController::health, "/api/health", Get);
    METHOD_LIST_END

    // GET /api/health -> {"status":"ok", ...}
    void health(const HttpRequestPtr &req,
                std::function<void(const HttpResponsePtr &)> &&callback) const;
};
