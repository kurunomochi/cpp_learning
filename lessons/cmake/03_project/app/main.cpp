#include "taskkit/title.hpp"
#include <iostream>
int main() {
    std::cout << "TaskKit " << taskkit::version() << '\n'
              << std::boolalpha << taskkit::valid_title("learn") << '\n';
}
