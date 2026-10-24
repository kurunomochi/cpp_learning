#include <array>
#include <iostream>
#include <string>
#include <vector>
int main() {
    const std::array<int, 3> ports{8080, 8081, 8082};
    std::vector<std::string> tasks{"read", "code"};
    tasks.push_back("test");
    for (auto& title : tasks) title += "!";
    for (const auto& title : tasks) std::cout << title << '\n';
    std::cout << "port=" << ports.at(0) << ", size=" << tasks.size() << '\n';
    return tasks.size() == 3 && tasks.at(1) == "code!" ? 0 : 1;
}
