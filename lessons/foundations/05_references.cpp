#include <iostream>
#include <string>
int main() {
    std::string title{"draft"};
    auto copy = title;                   // 一个新的 string
    auto& alias = title;                 // title 的别名
    const auto& view = title;            // 不允许通过 view 修改 title
    copy += " copy";
    alias += " ready";
    std::string* pointer = &title;       // 保存地址，不拥有对象
    if (pointer != nullptr) pointer->append("!");
    pointer = nullptr;                  // 不会销毁 title
    std::cout << "copy=" << copy << '\n';
    std::cout << "original=" << view << '\n';
    return copy == "draft copy" && title == "draft ready!" && pointer == nullptr ? 0 : 1;
}
