#pragma once
#include <vector>
#include <iostream>
#include <array>

#include "opcode.hpp"
#include "value.hpp"
#include "common.hpp"

// Defining aliases
using ValueVector = std::vector<Value>;
using LineVector = std::vector<std::size_t>;
using CodeVector = std::vector<uint8_t>;


// A chunk represents a "chunk" of instructions
class Chunk
{
public:
    // Write instruction to chunk
    void write(uint8_t byte, std::size_t line) noexcept
    {
        code.push_back(byte);
        lines.push_back(line);
    }

    // Write opcode instruction to chunk
    void write(OpCode opcode, std::size_t line)
    {
        write(static_cast<uint8_t>(opcode), line);
    }


    // Disassembles a given chunk
    void disassembleChunk(std::string_view name) noexcept
    {
        std::cout << "== " << name << " ==" << '\n';

        for (std::size_t offset = 0; offset < code.size();)
        {
            offset = this->disassembleInstruction(offset);
        }
    }


    /////////////////
    /// Instructions
    /////////////////

    // A simple instruction
    [[nodiscard]] std::size_t simpleInstruction(std::string_view name, std::size_t offset) noexcept
    {
        std::cout << name << '\n';
        return offset + 1;
    }

    // A constant instruction (Some literal)
    [[nodiscard]] std::size_t constantInstruction(std::string_view name, std::size_t offset) noexcept
    {
        auto constant = this->code[offset + 1];
        
        std::cout << name;
        printf("%4d '", constant);
        std::cout << this->constants[constant] << "'\n";
        return offset + 2;
    }

    // Disassembles the given instruction
    std::size_t disassembleInstruction(std::size_t offset) noexcept
    {
        printf("%04d ", offset);

        // Fancy printing, subsequent instructions which is on the
        // same line prints | instead of the line number.
        if (offset > 0 && lines[offset] == lines[offset - 1])
        {
            std::cout << "  |  "; 
        } 
        else
        {
            printf("%04d ", lines[offset]);
        }

        // No need for bounds checking since disassemble chunk
        // will only input valid offsets
        auto instr = static_cast<OpCode>(code[offset]);

        // Switch for the instruction
        switch(instr)
        {
            case OpCode::RETURN:
                return simpleInstruction("OP_RETURN", offset);
            case OpCode::CONSTANT:
                return constantInstruction("OP_CONSTANT", offset);
            default:
                std::cout << "Unknown opcode " << static_cast<uint8_t>(instr) << '\n';
                return offset + 1;
        }
    }    

    
    // Return size of the inputs
    [[nodiscard]] constexpr std::size_t count() const noexcept { return code.size(); }


    // Push multiple constants
    template <typename... Args>
    [[nodiscard]] std::size_t addConstant(Args &&... args) noexcept
    {
        constants.emplace_back(std::forward<Args>(args)...);
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