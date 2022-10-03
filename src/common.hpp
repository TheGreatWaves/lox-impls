#pragma once

// Common things which will be used are defined here

#include <iostream>
#include <vector>
#include <cassert>
#include <array>
#include <string>
#include <string_view>
#include <variant>
#include <memory>
#include <sstream>
#include <functional>
#include <stdexcept>

// Comment/Uncomment to toggle debug mode.
// #define DEBUG_TRACE_EXECUTION
// #define DEBUG_PRINT_CODE

constexpr auto UINT8_COUNT = std::numeric_limits<uint8_t>::max() + 1;
