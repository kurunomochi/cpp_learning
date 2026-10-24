#include <taskkit/title.hpp>
#include <iostream>
int main() {
    std::cout << "Imported TaskKit " << taskkit::version() << '\n';
    return taskkit::valid_title("external app") && !taskkit::valid_title("") ? 0 : 1;
}
