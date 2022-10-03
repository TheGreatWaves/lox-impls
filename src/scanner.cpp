#include "scanner.hpp"

bool Scanner::isAtEnd() const noexcept
{ return current == source.length(); }

Token Scanner::errorToken(std::string_view message) const noexcept
{
    return { TokenType::Error, message, line };
}

Token Scanner::makeToken(const TokenType type) const noexcept
{
	const std::size_t length = current - start;
	return { type, source.substr(start, length), line };
}

char Scanner::advance() noexcept
{
	++current;
	return source[current-1];
}

bool Scanner::match(const char expected)
{
	if (isAtEnd() || source.at(current) != expected) return false;
	++current;
	return true;
}

char Scanner::peek(std::size_t offset) const noexcept
{
	if (current + offset >= source.length()) return '\0'; 
	return source.at(current + offset);
}

void Scanner::skipWhiteSpace() noexcept
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

Token Scanner::string() noexcept
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

Token Scanner::number() noexcept
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

TokenType Scanner::checkKeyword(size_t begin, std::string_view rest, TokenType type) const noexcept
{
	const size_t wordLength = current - start; // Current word length

	// Makesure the word length matches and the string matches
	if (const auto restLength = rest.length(); wordLength == begin + restLength
		&& source.substr(start + begin, restLength) == rest)
	{
		// If it does, speicified type is returned (reserved keyword)
		return type;
	}
	// Otherwise, it's an identifier
	return TokenType::Identifier;
}

TokenType Scanner::identiferType() const
{
	switch(source.at(start))
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
			switch (source.at(start + 1))
			{
			case 'a': return checkKeyword(2, "lse", TokenType::False);
			case 'o': return checkKeyword(2, "r", TokenType::For);
			case 'u': return checkKeyword(2, "n", TokenType::Fun);
			default: ;
			}
		}
		break;
	default:
		break;
	case 't':
		if (current - start > 1)
		{
			switch (source.at(start + 1))
			{
			case 'h': return checkKeyword(2, "is", TokenType::This);
			case 'r': return checkKeyword(2, "ue", TokenType::True);
			default: ;
			}
		}
		break;
	}
	return TokenType::Identifier;
}

Token Scanner::identifier() noexcept
{
	// Keep consuming as long as it is alphanum
	while (std::isalnum(peek()) || peek() == '_') advance();
	return makeToken(identiferType());
}

Token Scanner::scanToken() noexcept
{
	skipWhiteSpace();
	start = current;

	if (isAtEnd()) return makeToken(TokenType::Eof);

	const auto c = advance();

	if (std::isdigit(c))
	{
		return number(); 
	}
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
	default: ;
	}
	return errorToken("Unexpected character.");
}
