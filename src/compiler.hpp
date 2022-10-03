#pragma once

#include <string_view>
#include <string>
#include <functional>

#include "scanner.hpp"
#include "common.hpp"
#include "value.hpp"

struct Parser;
struct ParseRule;
struct Compilation;
struct Compiler;

void errorAt(Parser& parser, const Token token, std::string_view message) noexcept;
void errorAtCurrent(Parser& parser, std::string_view message) noexcept;
void error(Parser& parser, std::string_view message) noexcept;

enum class FunctionType
{
    FUNCTION,
    SCRIPT
};


enum class Precedence : uint8_t
{
    None,
    Assignment,     // =
    Or,             // ||
    And,            // &&
    Equality,       // == !=
    Comparison,     // < > <= >=
    Term,           // + -
    Factor,         // * /
    Unary,          // ! -
    Call,           // . ()
    Primary,
};

using ParseFn = std::function<void(bool canAssign)>;
struct ParseRule
{
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
};


struct Parser
{
    Token current, previous;
    Scanner scanner;
    bool hadError, panicMode;

    Parser() = delete;

    constexpr explicit Parser(std::string_view source) noexcept
        : scanner(source), hadError(false), panicMode(false)
    {}

    void advance() noexcept;

    void consume(TokenType type, std::string_view message) noexcept;

    // Check the current token's type
    // returns true if it is of expected type.
    [[nodiscard]] bool check(TokenType type) const noexcept;
};

// Used for local variables.
// - We store the name of the variable
// 
// - When resolving an identifer, we 
//   compare the identifier' lexeme
//   with each local's name to find
//   a match.
// 
// - The depth field records the scope 
//   depth of the block where the local
//   variable was declared.
struct Local
{
    std::string_view name;    // Name of local
    int              depth{}; // The depth level of local
};

// The compiler will help us resolve local variables.
struct Compiler
{
	explicit Compiler(Parser& parser, FunctionType type = FunctionType::SCRIPT, std::unique_ptr<Compiler> enclosing = nullptr);

    void markInitialized();

    Function function = nullptr;
    FunctionType funcType;

    std::array<Local, UINT8_COUNT>  locals;
    int                             localCount = 0;
    size_t                          scopeDepth = 0;

    std::unique_ptr<Compiler>       mEnclosing;
};

// This is the struct containing
// all the functions which will 
// used to compile the source code.
struct Compilation
{

    std::unique_ptr<Compiler>   compiler = nullptr;
    std::unique_ptr<Parser>     parser = nullptr;  // 

    [[nodiscard]] Chunk& currentChunk() noexcept;

    // Write an OpCode byte to the chunk.
    void emitByte(OpCode byte);

    // Write a byte to the chunk.
    void emitByte(uint8_t byte);

    // Write the OpCode for return to the chunk.
    void emitReturn();

    // Write two bytes to the chunk.
    void emitByte(uint8_t byte1, uint8_t byte2);

    // Write two OpCode bytes to the chunk.
    void emitByte(OpCode byte1, OpCode byte2);

    // Jump backwards by a given offset.
    void emitLoop(std::size_t loopStart);


    [[nodiscard]] int emitJump(OpCode instruction);

    [[nodiscard]] int emitJump(uint8_t instruction);

    std::shared_ptr<FunctionObject> compile(std::string_view code);

    [[nodiscard]] Function endCompiler() noexcept;

    // Error sychronization -
    // If we hit a compile-error parsing the
    // previous statement, we enter panic mode.
    // When that happens, after the statement 
    // we start synchronizing.
    void synchronize();

    // Parse expression.
    void expression();

    // Variable declaration (A 'var' token found)
    void varDeclaration();

    void funDeclaration();

    void and_(bool);
    void or_(bool);
    void call(bool);

    void expressionStatement();

    void returnStatement();

    void ifStatement();

    void whileStatement();

    void forStatement();

    // Declaring statements or variables.
    void declaration();

    void block();

    void function(FunctionType type);

    // Parse statements.
    void statement();

    // Parse a print statement.
    void printStatement() noexcept;

    // Check if current token matches the current token,
    // if it does, consume it.
    [[nodiscard]] bool match(TokenType type) noexcept;

    void number(bool) noexcept;

    void emitConstant(Value value) noexcept;

    void patchJump(const int offset);

    // Create a new constant and add it to the chunk.
    [[nodiscard]] uint8_t makeConstant(const Value& value) noexcept;

    void grouping(bool) noexcept;

    void unary(bool);

    void binary(bool);

    void literal(bool);

    void string(bool);

    void variable(bool canAssign);

    [[nodiscard]] int resolveLocal() const;


    void namedVariable(bool canAssign);

    // Precedence
    void parsePrecedence(Precedence precedence);

    // Create a new value with the previous token's lexeme
    // and return the index at which is is added in the constant table.
    [[nodiscard]] uint8_t identifierConstant() noexcept;

    [[nodiscard]] static bool identifiersEqual(std::string_view str1, std::string_view str2) noexcept;

    void declareVariable();

    // Parses the variable's name.
    [[nodiscard]] uint8_t parseVariable(std::string_view message) noexcept;

    // Mark the latest variable initialized's scope to
    // the current scope.
    void markInitialized() const;

    // Define the variable.
    // Global refers to the index of the name
    // in the chunk's constant collection.
    void defineVariable(uint8_t global);

    uint8_t argumentList();

    // By incrementing the
    // depth, we declare that 
    // a new block has begun.
    void beginScope() const noexcept;

    // By decrementing the 
    // depth, we declare that
    // a block is out of scope,
    // so we simply return to
    // the previous layer.
    void endScope() noexcept;

    // Add local variable to the compiler->
    void addLocal(const std::string_view name, Parser& targetParser) const;

    // get rule
    [[nodiscard]] const ParseRule& getRule(TokenType type) noexcept;
};




