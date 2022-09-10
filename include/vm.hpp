#pragma once

#include "chunk.hpp"

// Max number of elements in the stack
constexpr std::size_t STACK_MAX = 256;

// The result of the interpretation
enum class InterpretResult
{
    OK,
    COMPILE_ERROR,
    RUNTIME_ERROR,
};

// Virtual Machine
class VM
{

private:
    // Returns the current byte and increments position
    [[nodiscard]] uint8_t readByte() noexcept
    {
        return this->ip->at(pos++);
    }

    // Returns the current constant stored at the byte
    [[nodiscard]] Value readConstant() noexcept
    {
        return this->mChunk->constants[readByte()];
    }
    
public:
    // Constructor
    VM() 
        : pos(0)
    {

    }

    // Interpret the chunk
    [[nodiscard]] InterpretResult interpret(Chunk* chunk) noexcept
    {
        mChunk = chunk;
        this->ip = &chunk->code;
        return run();
    }

    // Run the interpreter on the chunk
    [[nodiscard]] InterpretResult run() noexcept
    {
        #ifdef DEBUG_TRACE_EXECUTION
            this->mChunk->disassembleInstruction(pos);
        #endif

        while(true)
        {
            auto instruction = static_cast<OpCode>(readByte());
            switch (instruction)
            {
            case OpCode::RETURN:
                return InterpretResult::OK;
            case OpCode::CONSTANT:
                auto constant = readConstant();
                std::cout << constant << '\n';
                break;
            }
        }
    }

private:

    Chunk*                          mChunk;                 // Chunk to interpret
    CodeVector*                     ip;                     // Instruction pointer
    std::size_t                     pos;                    // Position in the code
    std::array<Value, STACK_MAX>    stack;                  
    Value*                          stackTop = nullptr;
};