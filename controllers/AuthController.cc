#include "AuthController.h"

#include "Helpers.h"
#include "Validation.h"

#include <drogon/HttpClient.h>
#include <drogon/orm/DbClient.h>
#include <drogon/orm/Exception.h>

#include <exception>
#include <string>

using namespace helpers;

namespace
{
// Обращение к микросервису hasher. method = "/hash" или "/verify".
Task<HttpResponsePtr> callHasher(const std::string &path, const Json::Value &body)
{
    auto client =
        HttpClient::newHttpClient(envOr("HASHER_URL", "http://hasher:5005"));
    auto req = HttpRequest::newHttpJsonRequest(body);
    req->setMethod(Post);
    req->setPath(path);
    co_return co_await client->sendRequestCoro(req);
}

void setLoggedIn(const HttpRequestPtr &req,
                 const std::string &uuid,
                 const std::string &login)
{
    auto session = req->session();
    session->insert("user_uuid", uuid);
    session->insert("login", login);
}
}  // namespace

Task<> AuthController::registerUser(
    HttpRequestPtr req, std::function<void(const HttpResponsePtr &)> callback)
{
    auto json = req->getJsonObject();
    if (!json || !json->isMember("login") || !json->isMember("password"))
    {
        callback(jsonError(k400BadRequest, "Нужны поля login и password"));
        co_return;
    }
    const std::string login = (*json)["login"].asString();
    const std::string password = (*json)["password"].asString();
    if (!util::isValidLogin(login))
    {
        callback(jsonError(k400BadRequest,
                           "Логин: 3-32 символа, латиница/цифры/подчёркивание"));
        co_return;
    }
    if (!util::isValidPassword(password))
    {
        callback(jsonError(k400BadRequest, "Пароль: от 4 до 128 символов"));
        co_return;
    }

    // 1. Хэшируем пароль через микросервис.
    std::string stored, salt;
    try
    {
        Json::Value hbody;
        hbody["password"] = password;
        auto hresp = co_await callHasher("/hash", hbody);
        auto hjson = hresp->getJsonObject();
        if (hresp->statusCode() != k200OK || !hjson)
        {
            callback(jsonError(k502BadGateway, "Сервис хэширования недоступен"));
            co_return;
        }
        stored = (*hjson)["stored"].asString();
        salt = (*hjson)["salt"].asString();
    }
    catch (const std::exception &e)
    {
        callback(jsonError(k502BadGateway,
                           std::string("Ошибка хэширования: ") + e.what()));
        co_return;
    }

    // Сохраняем пользователя в БД.
    try
    {
        auto db = app().getDbClient();
        auto result = co_await db->execSqlCoro(
            "INSERT INTO users(login, password, salt) VALUES($1,$2,$3) "
            "RETURNING uuid",
            login, stored, salt);
        const std::string uuid = result[0]["uuid"].as<std::string>();

        setLoggedIn(req, uuid, login);  // сразу логиним

        Json::Value out;
        out["uuid"] = uuid;
        out["login"] = login;
        callback(HttpResponse::newHttpJsonResponse(out));
    }
    catch (const orm::DrogonDbException &e)
    {
        const std::string what = e.base().what();
        if (what.find("duplicate") != std::string::npos ||
            what.find("unique") != std::string::npos)
            callback(jsonError(k409Conflict, "Такой логин уже занят"));
        else
            callback(jsonError(k500InternalServerError,
                               std::string("Ошибка БД: ") + what));
    }
    co_return;
}

Task<> AuthController::login(
    HttpRequestPtr req, std::function<void(const HttpResponsePtr &)> callback)
{
    auto json = req->getJsonObject();
    if (!json || !json->isMember("login") || !json->isMember("password"))
    {
        callback(jsonError(k400BadRequest, "Нужны поля login и password"));
        co_return;
    }
    const std::string login = (*json)["login"].asString();
    const std::string password = (*json)["password"].asString();

    try
    {
        auto db = app().getDbClient();
        auto result = co_await db->execSqlCoro(
            "SELECT uuid, password FROM users WHERE login=$1", login);
        if (result.empty())
        {
            callback(jsonError(k401Unauthorized, "Неверный логин или пароль"));
            co_return;
        }
        const std::string uuid = result[0]["uuid"].as<std::string>();
        const std::string stored = result[0]["password"].as<std::string>();

        // Проверяем пароль через микросервис.
        Json::Value vbody;
        vbody["password"] = password;
        vbody["stored"] = stored;
        auto vresp = co_await callHasher("/verify", vbody);
        auto vjson = vresp->getJsonObject();
        const bool valid = vjson && (*vjson)["valid"].asBool();
        if (!valid)
        {
            callback(jsonError(k401Unauthorized, "Неверный логин или пароль"));
            co_return;
        }

        setLoggedIn(req, uuid, login);

        Json::Value out;
        out["uuid"] = uuid;
        out["login"] = login;
        callback(HttpResponse::newHttpJsonResponse(out));
    }
    catch (const std::exception &e)
    {
        callback(jsonError(k500InternalServerError,
                           std::string("Ошибка входа: ") + e.what()));
    }
    co_return;
}

Task<> AuthController::logout(
    HttpRequestPtr req, std::function<void(const HttpResponsePtr &)> callback)
{
    req->session()->clear();
    Json::Value out;
    out["status"] = "ok";
    callback(HttpResponse::newHttpJsonResponse(out));
    co_return;
}

Task<> AuthController::me(
    HttpRequestPtr req, std::function<void(const HttpResponsePtr &)> callback)
{
    Json::Value out;
    auto session = req->session();
    if (session && session->find("user_uuid"))
    {
        out["authenticated"] = true;
        out["uuid"] = session->get<std::string>("user_uuid");
        out["login"] =
            session->find("login") ? session->get<std::string>("login") : "";
    }
    else
    {
        out["authenticated"] = false;
    }
    callback(HttpResponse::newHttpJsonResponse(out));
    co_return;
}
