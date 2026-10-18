#include "task/http_api.hpp"
#include "task/repository.hpp"
#include <httplib.h>
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

namespace {
std::atomic<bool> stopping{false};
static_assert(std::atomic<bool>::is_always_lock_free);
extern "C" void handle_signal(int) { stopping.store(true, std::memory_order_relaxed); }
std::string environment(const char* name, const char* fallback) {
    const char* value = std::getenv(name);
    return value && *value ? value : fallback;
}
}
int main() {
    try {
        const auto host = environment("APP_HOST", "127.0.0.1");
        const auto port_value = task::parse_id(environment("APP_PORT", "8080"));
        if (!port_value || *port_value > 65535) throw std::invalid_argument("APP_PORT must be 1..65535");
        const auto path = environment("APP_DB", "tasks.db");
        const auto token = environment("APP_TOKEN", "");
        if (host != "127.0.0.1" && host != "localhost" && host != "::1" && token.size() < 32)
            throw std::invalid_argument("non-loopback APP_HOST requires APP_TOKEN of at least 32 bytes");
        task::TaskRepository repository(path);
        httplib::Server server;
        server.new_task_queue = [] { return new httplib::ThreadPool(4, 4, 64); };
        server.set_payload_max_length(16 * 1024);
        server.set_read_timeout(std::chrono::seconds(5));
        server.set_write_timeout(std::chrono::seconds(5));
        server.set_keep_alive_max_count(20);
        task::register_routes(server, repository, token);
        std::signal(SIGINT, handle_signal);
        std::signal(SIGTERM, handle_signal);
        if (!server.bind_to_port(host, static_cast<int>(*port_value))) {
            std::cerr << "Cannot bind server address; check APP_HOST and APP_PORT\n";
            return 1;
        }
        std::jthread shutdown_watcher([&server](std::stop_token stop) {
            while (!stop.stop_requested()) {
                if (stopping.load(std::memory_order_relaxed) && server.is_running()) { server.stop(); return; }
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        });
        std::cout << "Task server: http://" << host << ':' << *port_value
                  << "  (Ctrl+C to stop)\n" << std::flush;
        const bool ok = server.listen_after_bind();
        return ok ? 0 : 1;
    } catch (const std::exception& error) {
        std::cerr << "Startup failed: " << error.what() << '\n';
        return 1;
    }
}
