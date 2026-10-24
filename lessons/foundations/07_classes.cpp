#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
class Task {
public:
    explicit Task(std::string title) : title_(checked(std::move(title))) {}
    const std::string& title() const noexcept { return title_; }
    bool completed() const noexcept { return completed_; }
    void complete() noexcept { completed_ = true; }
    void rename(std::string title) { title_ = checked(std::move(title)); }
private:
    static std::string checked(std::string title) {
        if (title.empty()) throw std::invalid_argument("empty title");
        return title;
    }
    std::string title_;
    bool completed_{false};
};
int main() {
    Task task{"learn"};
    task.complete();
    bool rejected{false};
    try { task.rename(""); }
    catch (const std::invalid_argument&) { rejected = true; }
    std::cout << task.title() << ": " << std::boolalpha << task.completed() << '\n';
    return rejected && task.title() == "learn" && task.completed() ? 0 : 1;
}
