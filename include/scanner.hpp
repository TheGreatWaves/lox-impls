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
    TokenType type;
    std::string_view text;
    std::size_t line;

    constexpr Token(TokenType pType, std::string_view pText, std::size_t pLine) noexcept
        : type(pType), text(pText), line(pLine)
    {}

    constexpr Token() noexcept
        : type(TokenType::Eof) {}
};

class Scanner
{
private:
    std::size_t start = 0;
    std::size_t current = start;
    std::size_t line = 1;
    std::string_view source;

private: // Methods
    [[nodiscard]] bool isAtEnd() const noexcept { return current == source.length(); }

    [[nodiscard]] Token errorToken(std::string_view message)const noexcept
    {
        return Token(TokenType::Error, message, line);
    }
	[[nodiscard]] Token makeToken(TokenType type)const noexcept
    {
        std::size_t length = current - start;
        return Token(type, source.substr(start, length), line);
    }

    char advance() noexcept
    {
        ++current;
        return source[current-1];
    }

    [[nodiscard]] bool match(char expected)
    {
        if (isAtEnd() || source.at(current) != expected) return false;
        ++current;
        return true;
    }

    char peek(std::size_t offset = 0) const noexcept
    {
        if (current + offset >= source.length()) return '\0'; 
        return source[current + offset];
    }

    void skipWhiteSpace() noexcept
    {
        while (true)
        {
            switch (peek()) 
            {
            case ' ':
            case '\r':
            case '\t':
                advance();
                break;
            case '\n':
                ++line;
                advance();
                break;
            case '/':
                if (peek(1) == '/') // is a comment
                {
                    while (peek() != '\n' && !isAtEnd())
                    {
                        advance();
                    }
                }
                else
                {
                    return;
                }
                break;
            default:
                return;
            }
        }
    }

    [[nodiscard]] Token string() noexcept
    {
        while (peek() != '"' && !isAtEnd())
        {
            if (peek() == '\n') ++line;
            advance();
        }

        if (isAtEnd()) return errorToken("Unterminated string.");

        // capture the closing quote
        advance();
        return makeToken(TokenType::String);
    }

    [[nodiscard]] Token number() noexcept
    {
        while (std::isdigit(peek())) advance();

        // Look for fractional part

        if (peek() == '.' && std::isdigit(peek(1)))
        {
            // Consume the dot
            advance();
        }

        while (std::isdigit(peek())) advance();

        return makeToken(TokenType::Number);
    }

    [[nodiscard]] TokenType identiferType()
    {
        return TokenType::Identifier;
    }

    [[nodiscard]] Token identifier() noexcept
    {
        // Keep consuming as long as it is alphanum
        while (std::isalnum(peek()) || peek() == '_') advance();
        return makeToken(identiferType());
    }

public:
    // Constructor
    explicit Scanner(std::string_view pSource) noexcept
        : source( pSource )
    {}


    [[nodiscard]] Token scanToken() noexcept
    {
        start = current;

        if (isAtEnd()) return makeToken(TokenType::Eof);

        auto c = advance();

        if (std::isdigit(c)) return number(); 
        if (std::isalpha(c)) return identifier();

        switch (c) 
        {
            case '(': return makeToken(TokenType::LeftParen);
            case ')': return makeToken(TokenType::RightParen);
            case '{': return makeToken(TokenType::LeftBrace);
            case '}': return makeToken(TokenType::RightBrace);
            case ';': return makeToken(TokenType::Semicolon);
            case ',': return makeToken(TokenType::Comma);
            case '.': return makeToken(TokenType::Dot);
            case '-': return makeToken(TokenType::Minus);
            case '+': return makeToken(TokenType::Plus);
            case '/': return makeToken(TokenType::Slash);
            case '*': return makeToken(TokenType::Star);
            case '!':
                return makeToken(match('=') ? TokenType::BangEqual : TokenType::Bang);
            case '=':
                return makeToken(match('=') ? TokenType::EqualEqual : TokenType::Equal);
            case '<':
                return makeToken(match('=') ? TokenType::LessEqual : TokenType::Less);
            case '>':
                return makeToken(match('=') ? TokenType::GreaterEqual : TokenType::Greater);
            case '"':
                return string();
        }
        return errorToken("Unexpected character.");
    }

    
};