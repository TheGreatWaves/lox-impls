#pragma once

#include "opcode.hpp"
#include "common.hpp"

// Forward Declarations
struct FunctionObject;
struct NativeFunctionObject;
using Function = std::shared_ptr<FunctionObject>;
using NativeFunction = std::shared_ptr<NativeFunctionObject>;

// A value is simply a variant
using Value = std::variant<double, bool, std::monostate, std::string, Function, NativeFunction>;

// Defining aliases
using LineVector = std::vector<std::size_t>;
using CodeVector = std::vector<uint8_t>;
using ValueVector = std::vector<Value>;

// Func ptye
std::ostream& operator<<(std::ostream& os, const Value& v);

// A chunk represents a "chunk" of instructions
class Chunk
{
public:
    Chunk() noexcept;

    // Write instruction to chunk
    void write(uint8_t byte, std::size_t line) noexcept;

    // This probably is a bad idea
    void write(std::size_t constant, std::size_t line) noexcept;

    // Write opcode instruction to chunk
    void write(OpCode opcode, std::size_t line);

    // Disassembles a given chunk
    void disassembleChunk(std::string_view name) noexcept;


    /////////////////
    /// Instructions
    /////////////////

    // A simple instruction
    [[nodiscard]] static std::size_t simpleInstruction(std::string_view name, std::size_t offset) noexcept;

    // A constant instruction (Some literal)
    [[nodiscard]] std::size_t constantInstruction(std::string_view name, std::size_t offset) const noexcept;

    [[nodiscard]] std::size_t byteInstruction(std::string_view name, std::size_t offset) const noexcept;

    [[nodiscard]] std::size_t jumpInstruction(std::string_view name, int sign, std::size_t offset) const noexcept;

    // Disassembles the given instruction
    std::size_t disassembleInstruction(std::size_t offset) noexcept;

    // Return size of the inputs
    [[nodiscard]] constexpr std::size_t count() const noexcept { return code.size(); };

    // Push constant(s)
    template <typename... Args>
    [[nodiscard]] std::size_t addConstant(Args &&... args) noexcept
    {
        constants.emplace_back(std::forward<Args>(args)...);

        // return the index of the newly pushed constant.
        return constants.size() - 1;
    }

public: // Data members

    // The line of instructions
    CodeVector code;

    // The line numbers
    LineVector lines;

    // Literals pushed
    ValueVector constants;
};

////////////
/// OBJECTS
////////////

// Object 
struct FunctionObject
{
    FunctionObject(int arity, const std::string_view name);

    [[nodiscard]] std::string getName() const;


    int					mArity;
    std::string	        mName;
    Chunk				mChunk;
};

using NativeFn = std::function <Value(int argc, std::vector<Value>::iterator)>;

struct NativeFunctionObject
{
    NativeFn fn;
};

// Defined and overloaded for std::visit
struct OutputVisitor {
    void operator()(const double d) const { std::cout << d; }
    void operator()(const bool b) const { std::cout << (b ? "true" : "false"); }
    void operator()(const std::monostate) const { std::cout << "nil"; }
    void operator()(const std::string& str) const { std::cout << str; }
    void operator()(const Function& func) const { std::cout << func->getName(); }
    void operator()(const NativeFunction&) const { std::cout << "<native fn>"; }

};






