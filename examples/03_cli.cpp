#include "task/domain.hpp"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

int main() {
    std::vector<task::Task> tasks;
    std::int64_t next_id = 1;
    std::cout << "Commands: add TITLE | list | done ID | delete ID | quit\n";
    for (std::string line; std::cout << "> " && std::getline(std::cin, line);) {
        std::istringstream input(line);
        std::string command;
        input >> command;
        if (command == "quit") break;
        if (command == "add") {
            std::string title;
            std::getline(input >> std::ws, title);
            if (auto error = task::title_error(title)) { std::cout << *error << '\n'; continue; }
            tasks.push_back({next_id++, title, false});
            std::cout << "created " << tasks.back().id << '\n';
        } else if (command == "list") {
            for (const auto& t : tasks) std::cout << t.id << " [" << (t.completed ? 'x' : ' ') << "] " << t.title << '\n';
        } else if (command == "done" || command == "delete") {
            std::string raw, extra;
            input >> raw;
            auto id = task::parse_id(raw);
            if (!id || (input >> extra)) { std::cout << "invalid id\n"; continue; }
            auto it = std::ranges::find(tasks, *id, &task::Task::id);
            if (it == tasks.end()) { std::cout << "not found\n"; continue; }
            if (command == "done") it->completed = true; else tasks.erase(it);
            std::cout << "ok\n";
        } else if (!command.empty()) std::cout << "unknown command\n";
    }
}
