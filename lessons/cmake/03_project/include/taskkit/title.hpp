#pragma once
#include <string>
#include <string_view>
namespace taskkit {
bool valid_title(std::string_view text);
std::string version();
}
