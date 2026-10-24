#include <concepts>
#include <iostream>
#include <string>
#include <utility>
template<class T> class Box {
public:
    explicit Box(T value) : value_(std::move(value)) {}
    const T& get() const noexcept { return value_; }
private:
    T value_;
};
template<std::integral T> constexpr bool positive(T value) {
    return value > T{0};
}
static_assert(positive(3));
static_assert(!positive(-1));
int main() {
    const Box<std::string> title{"templates"};
    const Box<int> id{7};
    std::cout << title.get() << ": " << id.get() << '\n';
    return positive(id.get()) ? 0 : 1;
}
