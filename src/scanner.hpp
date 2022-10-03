#pragma once

#include <string_view>
#include <cctype>

enum class TokenType : uint8_t
{
    // Single-character tokens.
	LeftParen, RightParen, LeftBrace, RightBrace,
	Comma, Dot, Minus, Plus,
	Semicolon, Slash, Star,

	// One or two character tokens.
	Bang, BangEqual, Equal, EqualEqual,
	Greater, GreaterEqual, Less, LessEqual,

	// Literals.
	Identifier, String, Number,

	// Keywords.
	And, Class, Else, False,
	Fun, For, If, Nil,
	Or, Print, Return, Super,
	This, True, Var, While,

	Error, Eof,
};

struct Token
{
    TokenType           type;
    std::string_view    text;
    std::size_t         line;

    constexpr Token(TokenType pType, std::string_view pText, std::size_t pLine) noexcept
        : type(pType), text(pText), line(pLine)
    {}

    constexpr Token() noexcept
        : type(TokenType::Eof)
        , line(0) 
    {}
};

class Scanner
{
public:
    // Constructor
    constexpr explicit Scanner(std::string_view pSource) noexcept
        : source(pSource)
    {}

    [[nodiscard]] Token scanToken() noexcept;

private: // Methods
    [[nodiscard]] bool isAtEnd() const noexcept;

    [[nodiscard]] Token errorToken(std::string_view message) const noexcept;

    // Create token of the specified type,
    // the string will automatically be assigned
	[[nodiscard]] Token makeToken(TokenType type) const noexcept;

    // Advances and returns the previous value
    char advance() noexcept;

    // Returns true if the current character matches the expected char
    [[nodiscard]] bool match(char expected);

    // read value without advancing
    // default to 0 to peek at current character
    [[nodiscard]] char peek(std::size_t offset = 0) const noexcept;

    // Advance and skip all whitespace
    void skipWhiteSpace() noexcept;

    // Create string token, scans from opening '"' to ending
    [[nodiscard]] Token string() noexcept;

    [[nodiscard]] Token number() noexcept;

    // Check the rest of the current word and return the 
    // expected token type if string matches a reserved keyword
    [[nodiscard]] TokenType checkKeyword(size_t begin, std::string_view rest, TokenType type) const noexcept;

    [[nodiscard]] TokenType identiferType() const;

    [[nodiscard]] Token identifier() noexcept;

private:
    std::size_t start = 0;
    std::size_t current = start;
    std::size_t line = 1;
    std::string_view source;
};