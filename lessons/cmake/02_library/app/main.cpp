#include "greeting/greeting.hpp"
#include <iostream>
int main() {
    const auto message = greeting::greet("CMake");
    std::cout << message << '\n';
    return message == "Hello, CMake!" ? 0 : 1;
}
