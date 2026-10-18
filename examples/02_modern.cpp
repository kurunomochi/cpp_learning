#include <algorithm>
#include <concepts>
#include <iostream>
#include <memory>
#include <mutex>
#include <ranges>
#include <span>
#include <string>
#include <thread>
#include <utility>
#include <vector>

struct Note { int id; std::string title; bool done{false}; };
template<std::integral T>
T twice(T value) { return value + value; }
int sum(std::span<const int> values) {
    int result{};
    for (int n : values) result += n;
    return result;
}
int main() {
    std::vector<Note> notes{{1, "Learn RAII", false}, {2, "Learn CMake", true}};
    auto found = std::ranges::find_if(notes, [](const Note& n) { return n.id == 1; });
    if (found != notes.end()) found->done = true;
    auto active = notes | std::views::filter([](const Note& n) { return n.done; });
    for (const auto& n : active) std::cout << n.title << '\n';
    auto owner = std::make_unique<Note>(Note{3, "Unique ownership"});
    auto moved = std::move(owner);
    if (owner || !moved) return 1;
    const int values[]{1, 2, 3};
    if (sum(values) != 6 || twice(4) != 8) return 2;
    int count{};
    std::mutex mutex;
    {
        std::vector<std::jthread> workers;
        for (int i = 0; i < 4; ++i) {
            workers.emplace_back([&] {
                for (int j = 0; j < 1000; ++j) {
                    std::lock_guard lock(mutex);
                    ++count;
                }
            });
        }
    } // jthread joins before count is examined.
    std::cout << "count=" << count << '\n';
    return count == 4000 ? 0 : 3;
}
