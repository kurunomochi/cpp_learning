#include <cmath>
#include <iostream>
#include <limits>
#include <optional>
std::optional<int> checked_add(int a, int b) {
    const int high = std::numeric_limits<int>::max();
    const int low = std::numeric_limits<int>::min();
    if ((b > 0 && a > high - b) || (b < 0 && a < low - b))
        return std::nullopt;
    return a + b;                        // 检查通过后才执行加法
}
int main() {
    const int done{2};
    const int total{3};
    const double ratio = static_cast<double>(done) / total;
    const auto sum = checked_add(7, 5);
    const auto overflow = checked_add(std::numeric_limits<int>::max(), 1);
    std::cout << "integer=" << done / total << '\n';
    std::cout << "ratio=" << ratio << '\n';
    std::cout << "sum=" << sum.value_or(-1) << '\n';
    std::cout << "overflow=" << std::boolalpha << !overflow.has_value() << '\n';
    return sum == 12 && !overflow && std::abs(ratio - 2.0 / 3.0) < 1e-9 ? 0 : 1;
}
