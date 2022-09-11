#pragma once

#include <string_view>

#include "scanner.hpp"

void compile(std::string_view code)
{
    std::size_t line = 0;
    while (true)
    {
        // Delete later
        Scanner scanner(code);
        Token token = scanner.scanToken();
        
        if (token.line != line)
        {
            printf("%4d ", token.line);
            line = token.line;
        }
        else
        {
            std::cout << "   | ";
        }

        std::cout << static_cast<uint8_t>(token.type) << " '" << token.text << "' ";
        if (token.type == TokenType::Eof) break;
    }
}