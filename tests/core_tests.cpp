#include "task/repository.hpp"
#include <iostream>
#include <stdexcept>
#include <thread>
#include <vector>

namespace {
int checks = 0;
void require(bool condition, const char* message) {
    ++checks;
    if (!condition) throw std::runtime_error(message);
}
}
int main() {
    try {
        require(task::parse_id("42") == 42, "valid id");
        for (const auto* value : {"", "0", "-1", "+1", " 1", "1x", "99999999999999999999999"})
            require(!task::parse_id(value), "reject invalid id");
        require(!task::title_error("Learn C++"), "valid title");
        require(!task::title_error(std::string(200, 'a')), "200-byte boundary");
        for (const auto& value : {std::string{}, std::string("   "), std::string("a\nb"), std::string(201, 'x')})
            require(task::title_error(value).has_value(), "reject invalid title");
        task::TaskRepository repo(":memory:");
        const auto a = repo.create("first");
        require(a.id > 0 && !a.completed, "create task");
        require(repo.get(a.id)->title == "first", "get task");
        require(!repo.get(99999), "missing task");
        require(repo.list().size() == 1, "list task");
        task::TaskPatch patch;
        patch.completed = true;
        require(repo.update(a.id, patch)->completed, "mark completed");
        patch.completed = false;
        require(!repo.update(a.id, patch)->completed, "optional false is present");
        patch.title = "new title";
        require(repo.update(a.id, patch)->title == "new title", "rename task");
        require(!repo.update(99999, patch), "update missing task");
        bool rejected = false;
        try { (void)repo.update(a.id, {}); } catch (const std::invalid_argument&) { rejected = true; }
        require(rejected, "empty patch rejected");
        rejected = false;
        try { (void)repo.list(101); } catch (const std::invalid_argument&) { rejected = true; }
        require(rejected, "pagination bounded");
        const auto injected = repo.create("x'); DROP TABLE tasks; --");
        require(repo.get(injected.id)->title == injected.title, "SQL text treated as data");
        require(repo.erase(a.id), "delete existing");
        require(!repo.erase(a.id), "delete missing");
        {
            std::vector<std::jthread> threads;
            for (int i = 0; i < 4; ++i) {
                threads.emplace_back([&repo] {
                    for (int n = 0; n < 10; ++n) (void)repo.create("parallel");
                });
            }
        }
        require(repo.list(100).size() == 41, "concurrent writes serialized");
        require(repo.list(1, 1).size() == 1, "pagination works");
        std::cout << "PASS: " << checks << " core checks\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL after check " << checks << ": " << error.what() << '\n';
        return 1;
    }
}
