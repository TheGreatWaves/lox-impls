#pragma once
#include <variant>
#include <memory>

// struct Obj;
// strct ObjStr;

// using ObjectPtr = std::shared_ptr<Obj>;

// A value is simply a variant
using Value = std::variant<double, bool, std::monostate, std::string>;

// Defined and overloaded for std::visit
struct OutputVisitor {
    void operator()(const double d) const { std::cout << d; }
    void operator()(const bool b) const { std::cout << (b ? "true" : "false"); }
    void operator()(const std::monostate) const { std::cout << "nil"; }
    void operator()(const std::string& str) const { std::cout << str; }
};


inline std::ostream& operator<<(std::ostream& os, const Value& v) {
    // Call the appropriate overload according to the parameter passed
    std::visit(OutputVisitor(), v);
    return os;
}