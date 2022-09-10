#pragma once
#include <variant>

using Value = std::variant<double, bool>;

struct OutputVisitor {
    void operator()(const double d) const { std::cout << d; }
    void operator()(const bool b) const { std::cout << (b ? "true" : "false"); }
};

inline std::ostream& operator<<(std::ostream& os, const Value& v) {
    std::visit(OutputVisitor(), v);
    return os;
}