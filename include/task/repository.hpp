#pragma once
#include "task/domain.hpp"
#include <memory>
#include <mutex>
#include <stdexcept>
#include <vector>

struct sqlite3;
namespace task {
class DatabaseError : public std::runtime_error {
public:
    DatabaseError(int code, const std::string& message) : std::runtime_error(message), code_(code) {}
    [[nodiscard]] int code() const noexcept { return code_; }
private:
    int code_;
};
struct DatabaseCloser { void operator()(sqlite3* db) const noexcept; };
class TaskRepository {
public:
    explicit TaskRepository(const std::string& path);
    TaskRepository(const TaskRepository&) = delete;
    TaskRepository& operator=(const TaskRepository&) = delete;
    [[nodiscard]] Task create(const std::string& title);
    [[nodiscard]] std::vector<Task> list(int limit = 20, int offset = 0);
    [[nodiscard]] std::optional<Task> get(std::int64_t id);
    [[nodiscard]] std::optional<Task> update(std::int64_t id, const TaskPatch& patch);
    [[nodiscard]] bool erase(std::int64_t id);
private:
    std::unique_ptr<sqlite3, DatabaseCloser> db_;
    std::mutex mutex_;
};
} // namespace task
