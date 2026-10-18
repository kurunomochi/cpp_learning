#include "task/http_api.hpp"
#include "task/repository.hpp"
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <atomic>
#include <charconv>
#include <cctype>
#include <iostream>
#include <memory>
#include <syncstream>

namespace task {
namespace {
using Json = nlohmann::json;
class HttpError : public std::runtime_error {
public:
    HttpError(int status_value, std::string code_value, std::string message)
        : std::runtime_error(std::move(message)), status(status_value), code(std::move(code_value)) {}
    int status;
    std::string code;
};
void respond(httplib::Response& response, int status, const Json& body) {
    response.status = status;
    response.set_content(body.dump(), "application/json; charset=utf-8");
}
void fail(httplib::Response& response, int status, const std::string& code, const std::string& message) {
    respond(response, status, {{"error", {{"code", code}, {"message", message}}}});
}
Json as_json(const Task& value) {
    return {{"id", value.id}, {"title", value.title}, {"completed", value.completed}};
}
Json body_object(const httplib::Request& request) {
    auto type = request.get_header_value("Content-Type");
    type = type.substr(0, type.find(';'));
    while (!type.empty() && type.back() == ' ') type.pop_back();
    std::ranges::transform(type, type.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (type != "application/json") throw HttpError(415, "unsupported_media_type", "Content-Type must be application/json");
    auto body = Json::parse(request.body, nullptr, false);
    if (body.is_discarded()) throw HttpError(400, "invalid_json", "request body is not valid JSON");
    if (!body.is_object()) throw HttpError(422, "validation_error", "request body must be a JSON object");
    return body;
}
std::string read_title(const Json& body) {
    if (!body.contains("title") || !body.at("title").is_string())
        throw HttpError(422, "validation_error", "title must be a string");
    auto title = body.at("title").get<std::string>();
    if (auto error = title_error(title)) throw HttpError(422, "validation_error", *error);
    return title;
}
std::int64_t request_id(const httplib::Request& request) {
    auto id = parse_id(request.path_params.at("id"));
    if (!id) throw HttpError(400, "invalid_id", "id must be a positive 64-bit integer");
    return *id;
}
int query_int(const httplib::Request& request, const char* name, int fallback, int minimum, int maximum) {
    if (!request.has_param(name)) return fallback;
    const auto text = request.get_param_value(name);
    int result{};
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), result);
    if (request.get_param_value_count(name) != 1 || error != std::errc{} ||
        end != text.data() + text.size() || result < minimum || result > maximum)
        throw HttpError(400, "invalid_query", std::string(name) + " is out of range or not an integer");
    return result;
}
} // namespace

void register_routes(httplib::Server& server, TaskRepository& repository, std::string api_token) {
    auto sequence = std::make_shared<std::atomic<std::uint64_t>>(0);
    server.set_pre_routing_handler([token = std::move(api_token), sequence](const auto& request, auto& response) {
        response.set_header("X-Request-Id", std::to_string(sequence->fetch_add(1, std::memory_order_relaxed) + 1));
        response.set_header("Cache-Control", "no-store");
        if (request.path.starts_with("/api/") && !token.empty() &&
            request.get_header_value("Authorization") != "Bearer " + token) {
            response.set_header("WWW-Authenticate", "Bearer");
            fail(response, 401, "unauthorized", "a valid bearer token is required");
            return httplib::Server::HandlerResponse::Handled;
        }
        return httplib::Server::HandlerResponse::Unhandled;
    });
    server.set_exception_handler([](const auto&, auto& response, std::exception_ptr error) {
        try {
            std::rethrow_exception(error);
        } catch (const HttpError& e) {
            fail(response, e.status, e.code, e.what());
        } catch (const DatabaseError& e) {
            // SQLite primary error codes: BUSY=5, LOCKED=6.
            const bool busy = (e.code() & 0xff) == 5 || (e.code() & 0xff) == 6;
            fail(response, busy ? 503 : 500, busy ? "database_busy" : "internal_error",
                 busy ? "database is busy; retry later" : "an internal error occurred");
            std::osyncstream(std::cerr) << Json{{"event", "database_error"}, {"sqlite_code", e.code()}}.dump() << '\n';
        } catch (const std::exception&) {
            fail(response, 500, "internal_error", "an internal error occurred");
            std::osyncstream(std::cerr) << "{\"event\":\"unhandled_exception\"}\n";
        } catch (...) {
            fail(response, 500, "internal_error", "an internal error occurred");
        }
    });
    server.set_error_handler([](const auto&, auto& response) {
        if (response.body.empty()) fail(response, response.status, "http_error", "request could not be handled");
    });
    server.set_logger([](const auto& request, const auto& response) {
        // JSON escaping prevents log injection. Never log token or request body.
        std::osyncstream(std::cout) << Json{{"method", request.method}, {"path", request.path},
            {"status", response.status}, {"request_id", response.get_header_value("X-Request-Id")}}.dump() << '\n';
    });
    server.Get("/healthz", [](const auto&, auto& response) { respond(response, 200, {{"status", "ok"}}); });
    server.Get("/api/tasks", [&repository](const auto& request, auto& response) {
        const int limit = query_int(request, "limit", 20, 1, 100);
        const int offset = query_int(request, "offset", 0, 0, 1000000);
        auto items = Json::array();
        for (const auto& value : repository.list(limit, offset)) items.push_back(as_json(value));
        respond(response, 200, {{"items", items}, {"limit", limit}, {"offset", offset}});
    });
    server.Post("/api/tasks", [&repository](const auto& request, auto& response) {
        const Json body = body_object(request);
        if (body.size() != 1 || !body.contains("title"))
            throw HttpError(422, "validation_error", "creation accepts exactly one field: title");
        const auto value = repository.create(read_title(body));
        response.set_header("Location", "/api/tasks/" + std::to_string(value.id));
        respond(response, 201, as_json(value));
    });
    server.Get("/api/tasks/:id", [&repository](const auto& request, auto& response) {
        auto value = repository.get(request_id(request));
        if (!value) throw HttpError(404, "not_found", "task does not exist");
        respond(response, 200, as_json(*value));
    });
    server.Patch("/api/tasks/:id", [&repository](const auto& request, auto& response) {
        const auto id = request_id(request);
        const Json body = body_object(request);
        TaskPatch patch;
        if (body.empty()) throw HttpError(422, "validation_error", "patch must contain title or completed");
        for (auto it = body.begin(); it != body.end(); ++it) {
            if (it.key() != "title" && it.key() != "completed")
                throw HttpError(422, "validation_error", "unknown patch field");
        }
        if (body.contains("title")) patch.title = read_title(body);
        if (body.contains("completed")) {
            if (!body.at("completed").is_boolean()) throw HttpError(422, "validation_error", "completed must be boolean");
            patch.completed = body.at("completed").get<bool>();
        }
        auto value = repository.update(id, patch);
        if (!value) throw HttpError(404, "not_found", "task does not exist");
        respond(response, 200, as_json(*value));
    });
    server.Delete("/api/tasks/:id", [&repository](const auto& request, auto& response) {
        if (!repository.erase(request_id(request))) throw HttpError(404, "not_found", "task does not exist");
        response.status = 204;
    });
}
} // namespace task
