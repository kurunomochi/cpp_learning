#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
struct Task { int id; std::string title; bool done; };
int main() {
    std::vector<Task> tasks{{3, "test", false}, {1, "read", true}, {2, "code", false}};
    std::ranges::sort(tasks, {}, &Task::id);
    const int wanted{2};
    const auto it = std::ranges::find_if(tasks, [wanted](const Task& task) {
        return task.id == wanted;
    });
    if (it == tasks.end()) return 1;
    it->done = true;
    const auto removed = std::erase_if(tasks, [](const Task& task) { return task.done; });
    // erase 后不再使用 it，它可能已经失效。
    std::cout << "removed=" << removed << ", left=" << tasks.size() << '\n';
    return removed == 2 && tasks.front().id == 3 ? 0 : 1;
}
