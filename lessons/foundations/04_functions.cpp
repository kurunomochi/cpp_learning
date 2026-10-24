#include <iostream>
#include <string>
#include <string_view>
int increment_copy(int value) { return ++value; }
void increment(int& value) { ++value; }
bool valid_title(std::string_view title) {
    return !title.empty() && title.size() <= 200;
}
std::string make_label(int id) {
    return "task-" + std::to_string(id);   // 按值返回，结果有自己的生命周期
}
int main() {
    int count{3};
    const int changed_copy = increment_copy(count);
    const bool copy_unchanged = count == 3;
    increment(count);
    const auto label = make_label(count);
    std::cout << label << '\n';
    return changed_copy == 4 && copy_unchanged && count == 4 && valid_title(label) && !valid_title("") ? 0 : 1;
}
