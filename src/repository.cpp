#include "task/repository.hpp"
#include <sqlite3.h>
#include <utility>

namespace task {
namespace {
void check(sqlite3* db, int code) {
    if (code != SQLITE_OK) throw DatabaseError(code, sqlite3_errmsg(db));
}
class Statement {
public:
    Statement(sqlite3* db, const char* sql) : db_(db) {
        sqlite3_stmt* raw{};
        const int code = sqlite3_prepare_v2(db, sql, -1, &raw, nullptr);
        stmt_.reset(raw);
        check(db, code);
    }
    void bind(int index, const std::string& value) {
        check(db_, sqlite3_bind_text(stmt_.get(), index, value.data(),
                                     static_cast<int>(value.size()), SQLITE_TRANSIENT));
    }
    void bind(int index, std::int64_t value) { check(db_, sqlite3_bind_int64(stmt_.get(), index, value)); }
    void bind_null(int index) { check(db_, sqlite3_bind_null(stmt_.get(), index)); }
    bool row() {
        const int code = sqlite3_step(stmt_.get());
        if (code == SQLITE_ROW) return true;
        if (code == SQLITE_DONE) return false;
        throw DatabaseError(code, sqlite3_errmsg(db_));
    }
    Task task() const {
        const auto* text = sqlite3_column_text(stmt_.get(), 1);
        if (!text) throw DatabaseError(SQLITE_NOMEM, "cannot read task title");
        const auto size = sqlite3_column_bytes(stmt_.get(), 1);
        return {sqlite3_column_int64(stmt_.get(), 0),
                std::string(reinterpret_cast<const char*>(text), static_cast<std::size_t>(size)),
                sqlite3_column_int(stmt_.get(), 2) != 0};
    }
private:
    struct Finalize { void operator()(sqlite3_stmt* p) const noexcept { sqlite3_finalize(p); } };
    sqlite3* db_; // Non-owning; repository outlives every statement.
    std::unique_ptr<sqlite3_stmt, Finalize> stmt_;
};
void execute(sqlite3* db, const char* sql) { check(db, sqlite3_exec(db, sql, nullptr, nullptr, nullptr)); }
} // namespace

void DatabaseCloser::operator()(sqlite3* db) const noexcept { sqlite3_close_v2(db); }
TaskRepository::TaskRepository(const std::string& path) {
    sqlite3* raw{};
    const int code = sqlite3_open_v2(path.c_str(), &raw,
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr);
    db_.reset(raw); // Also closes a non-null handle when open failed.
    if (code != SQLITE_OK) throw DatabaseError(code, raw ? sqlite3_errmsg(raw) : "SQLite allocation failure");
    check(raw, sqlite3_busy_timeout(raw, 3000));
    execute(raw, "PRAGMA journal_mode=WAL;");
    execute(raw, "PRAGMA foreign_keys=ON;");
    execute(raw, "CREATE TABLE IF NOT EXISTS tasks("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "title TEXT NOT NULL CHECK(length(CAST(title AS BLOB)) BETWEEN 1 AND 200),"
        "completed INTEGER NOT NULL DEFAULT 0 CHECK(completed IN (0,1))); ");
}
Task TaskRepository::create(const std::string& title) {
    if (auto error = title_error(title)) throw std::invalid_argument(*error);
    std::lock_guard lock(mutex_);
    Statement stmt(db_.get(), "INSERT INTO tasks(title) VALUES(?1) RETURNING id,title,completed;");
    stmt.bind(1, title);
    if (!stmt.row()) throw DatabaseError(SQLITE_ERROR, "insert returned no task");
    auto result = stmt.task();
    (void)stmt.row(); // Finish the write before returning; commit failures surface here.
    return result;
}
std::vector<Task> TaskRepository::list(int limit, int offset) {
    if (limit < 1 || limit > 100 || offset < 0 || offset > 1000000)
        throw std::invalid_argument("invalid pagination");
    std::lock_guard lock(mutex_);
    Statement stmt(db_.get(), "SELECT id,title,completed FROM tasks ORDER BY id LIMIT ?1 OFFSET ?2;");
    stmt.bind(1, static_cast<std::int64_t>(limit));
    stmt.bind(2, static_cast<std::int64_t>(offset));
    std::vector<Task> result;
    while (stmt.row()) result.push_back(stmt.task());
    return result;
}
std::optional<Task> TaskRepository::get(std::int64_t id) {
    std::lock_guard lock(mutex_);
    Statement stmt(db_.get(), "SELECT id,title,completed FROM tasks WHERE id=?1;");
    stmt.bind(1, id);
    if (!stmt.row()) return std::nullopt;
    return stmt.task();
}
std::optional<Task> TaskRepository::update(std::int64_t id, const TaskPatch& patch) {
    validate_patch(patch);
    std::lock_guard lock(mutex_);
    Statement stmt(db_.get(), "UPDATE tasks SET title=COALESCE(?1,title),"
        "completed=COALESCE(?2,completed) WHERE id=?3 RETURNING id,title,completed;");
    if (patch.title) stmt.bind(1, *patch.title); else stmt.bind_null(1);
    if (patch.completed) stmt.bind(2, static_cast<std::int64_t>(*patch.completed)); else stmt.bind_null(2);
    stmt.bind(3, id);
    if (!stmt.row()) return std::nullopt;
    auto result = stmt.task();
    (void)stmt.row();
    return result;
}
bool TaskRepository::erase(std::int64_t id) {
    std::lock_guard lock(mutex_);
    Statement stmt(db_.get(), "DELETE FROM tasks WHERE id=?1;");
    stmt.bind(1, id);
    (void)stmt.row();
    return sqlite3_changes(db_.get()) == 1;
}
} // namespace task
