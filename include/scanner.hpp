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

    [[nodiscard]] Token errorToken(std::string_view message) const noexcept
    {
        return Token(TokenType::Error, message, line);
    }

    // Create token of the specified type,
    // the string will automatically be assigned
	[[nodiscard]] Token makeToken(TokenType type) const noexcept
    {
        std::size_t length = current - start;
        return Token(type, source.substr(start, length), line);
    }

    // Advances and returns the previous value
    char advance() noexcept
    {
        ++current;
        return source[current-1];
    }

    // Returns true if the current character matches the expected char
    [[nodiscard]] bool match(char expected)
    {
        if (isAtEnd() || source.at(current) != expected) return false;
        ++current;
        return true;
    }

    // read value without advancing
    // default to 0 to peek at current character
    char peek(std::size_t offset = 0) const noexcept
    {
        if (current + offset >= source.length()) return '\0'; 
        return source[current + offset];
    }

    // Advance and skip all whitespace
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


    // Create string token, scans from opening '"' to ending
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

    // Check the rest of the current word and return the 
    // expected token type if string matches a reserved keyword
    [[nodiscard]] TokenType checkKeyword(size_t begin, std::string_view rest, TokenType type) noexcept
    {
        size_t wordLength = current - start; // Current word length
        auto restLength = rest.length();

        // Makesure the word length matches and the string matches
        if (wordLength == begin + restLength
            && source.substr(start + begin, restLength) == rest)
        {
            // If it does, speicified type is returned (reserved keyword)
            return type;
        }
        // Otherwise, it's an identifier
        return TokenType::Identifier;
    }

    [[nodiscard]] TokenType identiferType()
    {
        switch(source[start])
        {
            case 'a': return checkKeyword(1, "nd", TokenType::And);
            case 'c': return checkKeyword(1, "lass", TokenType::And);
            case 'e': return checkKeyword(1, "lse", TokenType::Else);
            case 'i': return checkKeyword(1, "f", TokenType::If);
            case 'n': return checkKeyword(1, "il", TokenType::Nil);
            case 'o': return checkKeyword(1, "r", TokenType::Or);
            case 'p': return checkKeyword(1, "rint", TokenType::Print);
            case 'r': return checkKeyword(1, "eturn", TokenType::Return);
            case 's': return checkKeyword(1, "uper", TokenType::Super);
            case 'v': return checkKeyword(1, "ar", TokenType::Var);
            case 'w': return checkKeyword(1, "hile", TokenType::While);
            case 'f':
                if (current - start > 1)
                {
                    switch (source[start + 1])
                    {
                        case 'a': return checkKeyword(2, "lse", TokenType::False);
                        case 'o': return checkKeyword(2, "r", TokenType::For);
                        case 'u': return checkKeyword(2, "n", TokenType::Fun); 
                    }
                }
                break;
            default:
                break;
            case 't':
                if (current - start > 1)
                {
                    switch (source[start + 1])
                    {
                        case 'h': return checkKeyword(2, "is", TokenType::This);
                        case 'r': return checkKeyword(2, "ue", TokenType::True);
                    }
                }
                break;
        }
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
    constexpr explicit Scanner(std::string_view pSource) noexcept
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