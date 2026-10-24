#include <iostream>
#include <mutex>
#include <thread>
#include <vector>
int main() {
    int count{};
    std::mutex mutex;
    {
        std::vector<std::jthread> workers;
        for (int n = 0; n < 4; ++n) {
            workers.emplace_back([&count, &mutex] {
                for (int i = 0; i < 1000; ++i) {
                    std::lock_guard lock{mutex};
                    ++count;
                }
            });
        }
    }                                   // jthread 析构并 join，之后才能读取 count
    std::cout << "count=" << count << '\n';
    return count == 4000 ? 0 : 1;
}
