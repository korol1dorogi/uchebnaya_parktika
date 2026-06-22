#include "HelloController.h"

void HelloController::hello(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) const
{
    auto resp = HttpResponse::newHttpResponse();
    resp->setContentTypeCode(CT_TEXT_PLAIN);  // text/plain; charset=utf-8
    resp->setBody("Hello, World!");
    callback(resp);
}
