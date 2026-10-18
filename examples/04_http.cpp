#include <httplib.h>
#include <nlohmann/json.hpp>
#include <iostream>

int main() {
    httplib::Server server;
    server.Get("/healthz", [](const httplib::Request&, httplib::Response& response) {
        response.set_content(nlohmann::json{{"status", "ok"}}.dump(), "application/json");
    });
    std::cout << "http://127.0.0.1:8081/healthz\n" << std::flush;
    return server.listen("127.0.0.1", 8081) ? 0 : 1;
}
