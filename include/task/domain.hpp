#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace task {
struct Task {
    std::int64_t id{};
    std::string title;
    bool completed{false};
};
struct TaskPatch {
    std::optional<std::string> title;
    std::optional<bool> completed;
};
// Length is measured in UTF-8 bytes, not displayed characters.
[[nodiscard]] std::optional<std::string> title_error(std::string_view title);
[[nodiscard]] std::optional<std::int64_t> parse_id(std::string_view text);
void validate_patch(const TaskPatch& patch);
} // namespace task
