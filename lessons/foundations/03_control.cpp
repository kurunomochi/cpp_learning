#include <iostream>
#include <vector>
int main() {
    const std::vector<int> ids{1, 2, 3, 4, 5};
    int sum{};
    for (const int id : ids) {
        if (id == 2) continue;           // 跳过当前这轮
        if (id > 4) break;               // 结束整个循环
        sum += id;
    }
    int retries{3};
    while (retries > 0) {
        --retries;                       // 循环状态必须能够前进
    }
    const bool accepted = sum == 8 && retries == 0;
    std::cout << "sum=" << sum << ", retries=" << retries << '\n';
    if (accepted) std::cout << "accepted\n";
    return accepted ? 0 : 1;
}
