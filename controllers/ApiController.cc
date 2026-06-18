#include "ApiController.h"

void ApiController::health(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) const
{
    Json::Value json;
    json["status"]  = "ok";
    json["service"] = "praktika";

    auto resp = HttpResponse::newHttpJsonResponse(json);
    callback(resp);
}
