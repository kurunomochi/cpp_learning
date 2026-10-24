#include "greeting/greeting.hpp"
namespace greeting {
std::string greet(std::string_view name) { return "Hello, " + std::string{name} + "!"; }
}
