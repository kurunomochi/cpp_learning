#include "taskkit/title.hpp"
#include <iostream>
#include <string>
int main() {
    int failures{};
    const auto check = [&failures](bool ok, const char* name) {
        if (!ok) { ++failures; std::cerr << "FAIL: " << name << '\n'; }
    };
    check(taskkit::valid_title("learn"), "ordinary title");
    check(!taskkit::valid_title(""), "empty title");
    check(!taskkit::valid_title(" \t"), "whitespace title");
    check(taskkit::valid_title(std::string(200, 'a')), "200-byte boundary");
    check(!taskkit::valid_title(std::string(201, 'a')), "201-byte boundary");
    check(taskkit::version() == "1.0.0", "configured version");
    return failures == 0 ? 0 : 1;
}
