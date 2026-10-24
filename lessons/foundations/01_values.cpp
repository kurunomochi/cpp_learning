#include <iostream>
#include <string>
int main() {
    int count{};                         // 初始化为 0
    const int limit{3};                  // 初始化后不再修改
    std::string title{"Learn C++"};      // 字符串对象拥有内容
    count = count + 1;                   // 给现有对象赋新值
    std::cout << title << '\n';
    std::cout << "count=" << count << ", limit=" << limit << '\n';
    return count == 1 ? 0 : 1;
}
