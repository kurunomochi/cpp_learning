#include <charconv>
#include <iostream>
#include <optional>
#include <string_view>
#include <system_error>
std::optional<int> parse_port(std::string_view text) {
    if (text.empty()) return std::nullopt;
    int value{};
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (error != std::errc{} || end != text.data() + text.size()) return std::nullopt;
    if (value < 1 || value > 65535) return std::nullopt;
    return value;
}
int main() {
    for (const auto input : {"8080", "80x", "0", "65536", "", "99999999999999999999"}) {
        const auto value = parse_port(input);
        std::cout << '[' << input << "] -> ";
        if (value) std::cout << *value << '\n';
        else std::cout << "invalid\n";
    }
    return parse_port("8080") == 8080 && parse_port("65535") == 65535 &&
        !parse_port("80x") && !parse_port("0") && !parse_port("65536") &&
        !parse_port("") && !parse_port("99999999999999999999") ? 0 : 1;
}
