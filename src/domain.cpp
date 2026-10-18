#include "task/domain.hpp"
#include <algorithm>
#include <charconv>
#include <stdexcept>
#include <system_error>

namespace task {
std::optional<std::string> title_error(std::string_view title) {
    if (title.empty() || title.size() > 200) return "title must contain 1..200 UTF-8 bytes";
    if (std::ranges::any_of(title, [](unsigned char c) { return c < 32 || c == 127; }))
        return "title must not contain ASCII control characters";
    if (std::ranges::all_of(title, [](char c) { return c == ' '; }))
        return "title must not contain only ASCII spaces";
    return std::nullopt;
}
std::optional<std::int64_t> parse_id(std::string_view text) {
    if (text.empty() || text.front() == '+' || text.front() == '-') return std::nullopt;
    std::int64_t value{};
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (error != std::errc{} || end != text.data() + text.size() || value <= 0) return std::nullopt;
    return value;
}
void validate_patch(const TaskPatch& patch) {
    if (!patch.title && !patch.completed) throw std::invalid_argument("patch must contain title or completed");
    if (patch.title) {
        if (auto error = title_error(*patch.title)) throw std::invalid_argument(*error);
    }
}
} // namespace task
