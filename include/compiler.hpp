#pragma once

#include <string_view>
#include <string>
#include <functional>

#include "scanner.hpp"
#include "vm.hpp"

struct Parser;
struct ParseRule;
struct Compiler;
struct Compilation;

void errorAt(Parser& parser, const Token token, std::string_view message) noexcept;
void errorAtCurrent(Parser& parser, std::string_view message) noexcept;
void error(Parser& parser, std::string_view message) noexcept;

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

using ParseFn = std::function<void()>;
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

  
    void advance() noexcept
    {
        // Store the current token
        previous = current;

        // If valid simply reuturn, else output error and scan next
        while(true)
        {
            current = scanner.scanToken();
            if (current.type != TokenType::Error) break;
            errorAtCurrent(*this, current.text);
        }
    }

    void consume(TokenType type, std::string_view message) noexcept
    {
        if (current.type == type)
        {
            advance();
            return;
        }

        // Type wasn't expected, output error.
        errorAtCurrent(*this, message);
    }

    [[nodiscard]] bool check(TokenType type) const noexcept
    {
        return current.type == type;
    }

};


void errorAt(Parser& parser, const Token token, std::string_view message) noexcept
{
    if (parser.panicMode) return;
    parser.panicMode = true;

    std::cerr << "[line " << token.line << "] Error";

    if (token.type == TokenType::Eof)
    {
        std::cerr << " at end";
    }
    else if (token.type == TokenType::Error)
    {
        // Nothing
    }
    else
    {
        std::cerr << " at " << token.text;
    }
    std::cerr << ": " << message << '\n';
    parser.hadError = true;
}

void errorAtCurrent(Parser& parser, std::string_view message) noexcept
{
    errorAt(parser, parser.current, message);
}

void error(Parser& parser, std::string_view message) noexcept
{
    errorAt(parser, parser.previous, message);
}


struct Compiler
{
    Chunk* compilingChunk = nullptr;
};

struct Compilation
{
    Compiler compiler;
    std::unique_ptr<Parser> parser = nullptr;

    [[nodiscard]] Chunk& currentChunk() noexcept { return *compiler.compilingChunk; }

    void emitByte(uint8_t byte)
    {
        currentChunk().write(byte, parser->previous.line);
    }

     void emitByte(OpCode byte)
    {
        currentChunk().write(byte, parser->previous.line);
    }

    bool compile(std::string_view code, Chunk& chunk) 
    {
        // Setup
        this->parser = std::make_unique<Parser>(code);
        compiler.compilingChunk = &chunk;

        // Compiling
        parser->advance();


        while (!match(TokenType::Eof))
        {
            declaration();
        }
        
        endCompiler();
        return !parser->hadError;
    }

    void endCompiler() noexcept
    {
        emitReturn();
        #ifdef DEBUG_PRINT_CODE
        if (!parser->hadError) 
        {
            currentChunk().disassembleChunk("code");
        }
        #endif
    }

    void emitReturn() 
    {
        emitByte(OpCode::RETURN);
    }

    void emitBytes(uint8_t byte1, uint8_t byte2)
    {
        emitByte(byte1);
        emitByte(byte2);
    }

    void emitByte(OpCode byte1, OpCode byte2)
    {
        emitByte(byte1);
        emitByte(byte2);
    }

    // Expressions

    void expression()
    {
        parsePrecedence(Precedence::Assignment);
    }

    void declaration()
    {
        statement();
    }

    void statement()
    {
        if (match(TokenType::Print))
        {
            printStatement();
        }
    }

    void printStatement() noexcept
    {
        expression();
        parser->consume(TokenType::Semicolon, "Expected ';' after value.");
        emitByte(OpCode::PRINT);
    }
    
    [[nodiscard]] bool match(TokenType type) noexcept
    {
        // If the current token is not the expected type
        // return false.
        if (!parser->check(type)) return false;
        
        // If it was expected, consume it.
        parser->advance();
        return true;
    }

    void number() noexcept
    {
        Value value = std::stod(std::string(parser->previous.text));
        emitConstant(value);
    }

    void emitConstant(Value value) noexcept
    {
        emitBytes(static_cast<uint8_t>(OpCode::CONSTANT), makeConstant(value));
    }

    [[nodiscard]] uint8_t makeConstant(Value value)
    {
        int constant = currentChunk().addConstant(value);
        if (constant > UINT8_COUNT)
        {
            error(*parser.get(), "Too many constants in one chunk");
            return 0;
        }

        return static_cast<uint8_t>(constant);
    }

    void grouping() noexcept
    {
        expression();
        parser->consume(TokenType::RightParen, "Expect ')' after expression.");
    }

    void unary()
    {
        // Remember the operator
        auto operatorType = parser->previous.type;

        // Compile the operand
        parsePrecedence(Precedence::Unary);

        // Emit the operator instruction
        switch(operatorType)
        {
            case TokenType::Minus:  emitByte(OpCode::NEGATE); break;
            case TokenType::Bang:   emitByte(OpCode::NOT); break;
            default: return; // Unreachable
        }
    }

    void binary()
    {
        // Remember the operator
        auto operatorType = parser->previous.type;

        auto rule = getRule(operatorType);
        parsePrecedence(Precedence(static_cast<int>(rule.precedence) + 1));

        // Emit the corresponding opcode
        switch (operatorType)
        {
            case TokenType::BangEqual:      emitByte(OpCode::EQUAL, OpCode::NOT); break;
            case TokenType::EqualEqual:     emitByte(OpCode::EQUAL); break;
            case TokenType::Greater:        emitByte(OpCode::GREATER); break;
            case TokenType::GreaterEqual:   emitByte(OpCode::LESS, OpCode::NOT); break;
            case TokenType::Less:           emitByte(OpCode::LESS); break;
            case TokenType::LessEqual:      emitByte(OpCode::GREATER, OpCode::NOT); break;

            case TokenType::Plus:           emitByte(OpCode::ADD); break;
            case TokenType::Minus:          emitByte(OpCode::SUBTRACT); break;
            case TokenType::Star:           emitByte(OpCode::MULTIPLY); break;
            case TokenType::Slash:          emitByte(OpCode::DIVIDE); break;
            default: return; // Unreachable
        }
    }

    void literal()
    {
        switch(parser->previous.type)
        {
            case TokenType::False: emitByte(OpCode::FALSE); break;
            case TokenType::Nil: emitByte(OpCode::NIL); break;
            case TokenType::True: emitByte(OpCode::TRUE); break;
            default: return; // Unreachable
        }
    }

    void string()
    {
        // Retrive the text in the form: "str"
        auto str = parser->previous.text;

        // Get rid of quotation marks
        str.remove_prefix(1);
        str.remove_suffix(1);

        // Construct string object
        emitConstant(std::string(str));
    }


    // Precedence
    void parsePrecedence(Precedence precedence)
    {
        parser->advance();
        
        auto type = parser->previous.type;
        auto prefixRule = getRule(type).prefix;

        if (prefixRule == nullptr)
        {
            error(*parser, "Expected expression.");
            return;
        }

        // Invoke function
       prefixRule();

        while (precedence <= getRule(parser->current.type).precedence)
        {
            parser->advance();
            ParseFn infixRule = getRule(parser->previous.type).infix;
            infixRule();
        }
    }

    // get rule
    [[nodiscard]] const ParseRule& getRule(TokenType type) noexcept
    {
        auto grouping = [this]() { this->grouping(); };
        auto unary = [this]() { this->unary(); };
        auto binary = [this]() { this->binary(); };
        auto number = [this]() { this->number(); };
        auto literal = [this]() { this->literal(); };
        auto string = [this] () { this->string(); };
        
        static ParseRule rls[] = 
        {
            {grouping,       nullptr,    Precedence::None},     // TokenType::LEFT_PAREN
            {nullptr,       nullptr,    Precedence::None},      // TokenType::RIGHT_PAREN
            {nullptr,       nullptr,    Precedence::None},      // TokenType::LEFT_BRACE
            {nullptr,       nullptr,    Precedence::None},      // TokenType::RIGHT_BRACE
            {nullptr,       nullptr,    Precedence::None},      // TokenType::COMMA
            {nullptr,       nullptr,    Precedence::None},      // TokenType::DOT
            {unary,         binary,     Precedence::Term},      // TokenType::MINUS
            {nullptr,       binary,     Precedence::Term},      // TokenType::PLUS
            {nullptr,       nullptr,    Precedence::None},      // TokenType::SEMICOLON
            {nullptr,       binary,     Precedence::Factor},    // TokenType::SLASH
            {nullptr,       binary,     Precedence::Factor},    // TokenType::STAR
            {unary,         nullptr,    Precedence::None},      // TokenType::BANG
            {nullptr,       binary,     Precedence::Equality},  // TokenType::BANG_EQUAL
            {nullptr,       nullptr,    Precedence::None},      // TokenType::EQUAL
            {nullptr,       binary,     Precedence::Equality},  // TokenType::EQUAL_EQUAL
            {nullptr,       binary,     Precedence::Comparison},// TokenType::GREATER
            {nullptr,       binary,     Precedence::Comparison},// TokenType::GREATER_EQUAL
            {nullptr,       binary,     Precedence::Comparison},// TokenType::LESS
            {nullptr,       binary,     Precedence::Comparison},// TokenType::LESS_EQUAL
            {nullptr,       nullptr,    Precedence::None},      // TokenType::IDENTIFIER
            {string,        nullptr,    Precedence::None},      // TokenType::STRING
            {number,        nullptr,    Precedence::None},      // TokenType::NUMBER
            {nullptr,       nullptr,    Precedence::None},      // TokenType::AND
            {nullptr,       nullptr,    Precedence::None},      // TokenType::CLASS
            {nullptr,       nullptr,    Precedence::None},      // TokenType::ELSE
            {literal,       nullptr,    Precedence::None},      // TokenType::FALSE
            {nullptr,       nullptr,    Precedence::None},      // TokenType::FOR
            {nullptr,       nullptr,    Precedence::None},      // TokenType::FUN
            {nullptr,       nullptr,    Precedence::None},      // TokenType::IF
            {literal,       nullptr,    Precedence::None},      // TokenType::NIL
            {nullptr,       nullptr,    Precedence::None},      // TokenType::OR
            {nullptr,       nullptr,    Precedence::None},      // TokenType::PRINT
            {nullptr,       nullptr,    Precedence::None},      // TokenType::RETURN
            {nullptr,       nullptr,    Precedence::None},      // TokenType::SUPER
            {nullptr,       nullptr,    Precedence::None},      // TokenType::THIS
            {literal,       nullptr,    Precedence::None},      // TokenType::TRUE
            {nullptr,       nullptr,    Precedence::None},      // TokenType::VAR
            {nullptr,       nullptr,    Precedence::None},      // TokenType::WHILE
            {nullptr,       nullptr,    Precedence::None},      // TokenType::ERROR
            {nullptr,       nullptr,    Precedence::None},      // TokenType::EOF
        };

        return rls[static_cast<int>(type)];
    }
};




