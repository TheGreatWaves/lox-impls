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

    // This probably is a bad idea
    void write(std::size_t constant, std::size_t line) noexcept
    {
        write(static_cast<uint8_t>(constant), line);
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
    [[nodiscard]] std::size_t simpleInstruction(std::string_view name, std::size_t offset) const noexcept
    {
        std::cout << name << '\n';
        return offset + 1;
    }

    // A constant instruction (Some literal)
    [[nodiscard]] std::size_t constantInstruction(std::string_view name, std::size_t offset) const noexcept
    {
        auto constant = this->code[offset + 1];
        
        std::cout << name;
        printf("%4d '", constant);
        std::cout << this->constants[constant] << "'\n";
        return offset + 2;
    }

    [[nodiscard]] std::size_t byteInstruction(std::string_view name, std::size_t offset) const noexcept
    {
        auto slot = this->code[offset + 1];
        printf("%-16s %4d\n", name.data(), slot);
        return offset + 2;
    }


    [[nodiscard]] std::size_t jumpInstruction(std::string_view name, int sign, std::size_t offset) const noexcept
    {
        auto jump = static_cast<uint16_t>(this->code.at(offset + 1) << 8);
        jump |= this->code.at(offset + 2);
        printf("%-16s %4d -> %d\n", name.data(), static_cast<int>(offset),
            static_cast<int>(offset) + 3 + sign * jump);
        return offset + 3;
    }

    // Disassembles the given instruction
    std::size_t disassembleInstruction(std::size_t offset) noexcept
    {
        printf("%04d ", static_cast<int>(offset));

        // Fancy printing, subsequent instructions which is on the
        // same line prints | instead of the line number.
        if (offset > 0 && lines.at(offset) == lines.at(offset - 1))
        {
            std::cout << "  |  ";
        } 
        else
        {
            printf("%04d ", static_cast<int>(lines.at(offset)));
        }

        // No need for bounds checking since disassemble chunk
        // will only input valid offsets
        auto instr = static_cast<OpCode>(code.at(offset));
       

        // Switch for the instruction
        switch(instr)
        {
            case OpCode::ADD:
            case OpCode::SUBTRACT:
            case OpCode::DIVIDE:
            case OpCode::MULTIPLY:
            case OpCode::NEGATE:
            case OpCode::RETURN:
            case OpCode::NIL:
            case OpCode::TRUE:
            case OpCode::FALSE:
            case OpCode::NOT:
            case OpCode::EQUAL:
            case OpCode::GREATER:
            case OpCode::LESS:
            case OpCode::PRINT:
            case OpCode::POP:
                return simpleInstruction(nameof(instr), offset);
            case OpCode::CONSTANT:
            case OpCode::DEFINE_GLOBAL:
            case OpCode::GET_GLOBAL:
            case OpCode::SET_GLOBAL:
                return constantInstruction(nameof(instr), offset);
            case OpCode::SET_LOCAL:
            case OpCode::GET_LOCAL:
                return byteInstruction(nameof(instr), offset);
            case OpCode::JUMP:
            case OpCode::JUMP_IF_FALSE:
                return jumpInstruction(nameof(instr), 1, offset);
            default:
                std::cout << "Unknown opcode " << static_cast<uint8_t>(instr) << '\n';
                return offset + 1;
        }
        
    }    

    
    // Return size of the inputs
    [[nodiscard]] constexpr std::size_t count() const noexcept { return code.size(); }


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