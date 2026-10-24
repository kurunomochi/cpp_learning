#include <iostream>
#include <memory>
#include <string>
#include <utility>
struct Trace {
    explicit Trace(std::string label) : label(std::move(label)) {
        std::cout << "open " << this->label << '\n';
    }
    ~Trace() { std::cout << "close " << label << '\n'; }
    Trace(const Trace&) = delete;
    Trace& operator=(const Trace&) = delete;
    std::string label;
};
int main() {
    Trace outer{"outer"};
    {
        auto first = std::make_unique<Trace>("resource");
        auto second = std::move(first);  // 转移指针的所有权，不移动 Trace 对象
        if (first || !second) return 1;
        std::cout << "owner moved\n";
    }                                   // second 析构，释放 Trace
    std::cout << "leaving main\n";
}                                       // outer 析构
