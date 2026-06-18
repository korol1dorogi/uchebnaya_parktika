#include "HaikuController.h"

#include "Helpers.h"

#include <drogon/orm/DbClient.h>
#include <drogon/orm/Exception.h>
#include <drogon/orm/Result.h>
#include <drogon/orm/Row.h>

#include <exception>
#include <string>

using namespace helpers;

namespace
{
// Преобразование строки результата в JSON-объект хокку.
// me — uuid текущего пользователя (для пометки is_mine).
Json::Value rowToJson(const orm::Row &row, const std::string &me)
{
    Json::Value o;
    o["id"] = row["id"].as<std::string>();
    o["text"] = row["text"].as<std::string>();
    o["created_at"] = row["created_at"].as<std::string>();
    o["comment"] =
        row["comment"].isNull() ? Json::Value() : Json::Value(row["comment"].as<std::string>());

    bool mine = false;
    if (!row["user_uuid"].isNull())
    {
        const std::string owner = row["user_uuid"].as<std::string>();
        o["author"] = row["author"].isNull() ? Json::Value("аноним")
                                              : Json::Value(row["author"].as<std::string>());
        mine = (!me.empty() && owner == me);
    }
    else
    {
        o["author"] = "аноним";
    }
    o["is_mine"] = mine;
    return o;
}
}  // namespace

Task<> HaikuController::listAll(
    HttpRequestPtr req, std::function<void(const HttpResponsePtr &)> callback)
{
    try
    {
        auto db = app().getDbClient();
        auto result = co_await db->execSqlCoro(
            "SELECT h.id, h.text, h.comment, h.created_at, h.user_uuid, "
            "u.login AS author "
            "FROM haikus h LEFT JOIN users u ON u.uuid = h.user_uuid "
            "ORDER BY h.created_at DESC LIMIT 200");

        const std::string me = currentUserUuid(req);
        Json::Value arr(Json::arrayValue);
        for (const auto &row : result)
            arr.append(rowToJson(row, me));
        callback(HttpResponse::newHttpJsonResponse(arr));
    }
    catch (const std::exception &e)
    {
        callback(jsonError(k500InternalServerError,
                           std::string("Ошибка БД: ") + e.what()));
    }
    co_return;
}

Task<> HaikuController::listMine(
    HttpRequestPtr req, std::function<void(const HttpResponsePtr &)> callback)
{
    const std::string me = currentUserUuid(req);
    if (me.empty())
    {
        callback(jsonError(k401Unauthorized, "Требуется вход"));
        co_return;
    }
    try
    {
        auto db = app().getDbClient();
        auto result = co_await db->execSqlCoro(
            "SELECT h.id, h.text, h.comment, h.created_at, h.user_uuid, "
            "u.login AS author "
            "FROM haikus h LEFT JOIN users u ON u.uuid = h.user_uuid "
            "WHERE h.user_uuid = $1 ORDER BY h.created_at DESC",
            me);
        Json::Value arr(Json::arrayValue);
        for (const auto &row : result)
            arr.append(rowToJson(row, me));
        callback(HttpResponse::newHttpJsonResponse(arr));
    }
    catch (const std::exception &e)
    {
        callback(jsonError(k500InternalServerError,
                           std::string("Ошибка БД: ") + e.what()));
    }
    co_return;
}

Task<> HaikuController::updateComment(
    HttpRequestPtr req,
    std::function<void(const HttpResponsePtr &)> callback,
    std::string id)
{
    const std::string me = currentUserUuid(req);
    if (me.empty())
    {
        callback(jsonError(k401Unauthorized, "Требуется вход"));
        co_return;
    }
    auto json = req->getJsonObject();
    if (!json || !json->isMember("comment"))
    {
        callback(jsonError(k400BadRequest, "Нужно поле comment"));
        co_return;
    }
    const std::string comment = (*json)["comment"].asString();

    try
    {
        auto db = app().getDbClient();
        // Обновляем только своё хокку (user_uuid = me).
        auto result = co_await db->execSqlCoro(
            "UPDATE haikus SET comment = $1 WHERE id = $2 AND user_uuid = $3",
            comment, id, me);
        if (result.affectedRows() == 0)
        {
            callback(jsonError(k404NotFound, "Хокку не найдено или оно не ваше"));
            co_return;
        }
        Json::Value out;
        out["status"] = "ok";
        out["id"] = id;
        out["comment"] = comment;
        callback(HttpResponse::newHttpJsonResponse(out));
    }
    catch (const std::exception &e)
    {
        callback(jsonError(k400BadRequest,
                           std::string("Не удалось обновить: ") + e.what()));
    }
    co_return;
}

Task<> HaikuController::remove(
    HttpRequestPtr req,
    std::function<void(const HttpResponsePtr &)> callback,
    std::string id)
{
    const std::string me = currentUserUuid(req);
    if (me.empty())
    {
        callback(jsonError(k401Unauthorized, "Требуется вход"));
        co_return;
    }
    try
    {
        auto db = app().getDbClient();
        auto result = co_await db->execSqlCoro(
            "DELETE FROM haikus WHERE id = $1 AND user_uuid = $2", id, me);
        if (result.affectedRows() == 0)
        {
            callback(jsonError(k404NotFound, "Хокку не найдено или оно не ваше"));
            co_return;
        }
        Json::Value out;
        out["status"] = "ok";
        callback(HttpResponse::newHttpJsonResponse(out));
    }
    catch (const std::exception &e)
    {
        callback(jsonError(k400BadRequest,
                           std::string("Не удалось удалить: ") + e.what()));
    }
    co_return;
}
