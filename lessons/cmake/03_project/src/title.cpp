#include "taskkit/title.hpp"
#include "taskkit/config.hpp"
namespace taskkit {
bool valid_title(std::string_view text) {
    if (text.empty() || text.size() > 200) return false;
    // 本实验只识别 ASCII 的六种空白，不是完整 Unicode 空白规则。
    return text.find_first_not_of(" \t\n\r\f\v") != std::string_view::npos;
}
std::string version() { return TASKKIT_VERSION; }
}
