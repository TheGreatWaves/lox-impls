#pragma once

#include "chunk.hpp"
#include "compiler.hpp"

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
        resetStack();
    }

    // Interpret source
    InterpretResult interpret(std::string_view code)
    {
        Chunk chunk;

        if (!cu.compile(code, chunk))
        {
            return InterpretResult::COMPILE_ERROR;
        }
        return run();
    }

    // Interpret the chunk
    [[nodiscard]] InterpretResult interpret(Chunk* chunk)
    {
        mChunk = chunk;
        this->ip = &chunk->code;
        return run();
    }

    // Run the interpreter on the chunk
    [[nodiscard]] InterpretResult run()
    {
        while(true)
        {
            // Print instruction before executing it (debug)
            #ifdef DEBUG_TRACE_EXECUTION
                std::cout << "          ";
                for (auto slot = stack.data(); slot < stackTop; ++slot)
                {
                    std::cout << "[ " << *slot << " ]";

                }
                std::cout << '\n';

                this->mChunk->disassembleInstruction(pos);
            #endif

            // Exploiting macros, wrap a 'op' b into a lambda to be executed
            #define BINARY_OP(op) \
                do { \
                    if (!binaryOp([](double a, double b) -> Value { return a op b; })) \
                    { \
                        return InterpretResult::COMPILE_ERROR; \
                    } \
                } while (false)


        
            auto instruction = static_cast<OpCode>(readByte());
            switch (instruction)
            {
            case OpCode::RETURN:
                std::cout << pop() << '\n';
                return InterpretResult::OK;
            case OpCode::ADD:       BINARY_OP(+); break;
            case OpCode::SUBTRACT:  BINARY_OP(-); break;
            case OpCode::DIVIDE:    BINARY_OP(/); break;
            case OpCode::MULTIPLY:  BINARY_OP(*); break;
            case OpCode::NOT:       
            {
                push(isFalsey(pop()));
                break;
            }
            case OpCode::NEGATE:
            {
                try
                {
                    auto tryNegate = -std::get<double>(peek());
                    pop();
                    push(tryNegate);
                }
                catch(const std::exception&)
                {
                    runTimeError("Operand must be a number.");
                    return InterpretResult::RUNTIME_ERROR;
                }
                break;
            }
            case OpCode::CONSTANT:
            {
                auto constant = readConstant();
                push(constant);
                break;
            }
            case OpCode::NIL:       push(std::monostate()); break;
            case OpCode::TRUE:      push(true); break;
            case OpCode::FALSE:     push(false); break;
            case OpCode::EQUAL:
            {
                const auto& a = pop();
                const auto& b = pop();
                push(a == b);
                break;
            }
            case OpCode::GREATER:   BINARY_OP(>); break;
            case OpCode::LESS:      BINARY_OP(<); break;

            }
            
        }
        #undef BINARY_OP
    }

    // Stack related methods

    void resetStack() noexcept
    {
        // Reset the pointer
        stackTop = stack.data();
    }

    void push(Value value) noexcept
    {
        // Dereference and assign
        *stackTop = std::move(value);
        ++stackTop;
    }

    Value pop() noexcept
    {
        // Decrement and return 
        --stackTop;
        return *stackTop;
    }

    [[nodiscard]] const Value& peek(std::size_t offset = 0) const
    {
        return stackTop[-1 - offset];
    }

private:

    struct FalseyVistor
    {
        bool operator()(const bool b) const noexcept { return !b; }
        bool operator()(const std::monostate) const noexcept { return true; }

        // Handles any other case as true
        template <typename T>
        bool operator()(const T) const noexcept { return true; }
    };

    bool isFalsey(const Value& value) const
    {
        return std::visit(FalseyVistor(), value); 
    }

    // F will be a lambda function
    template <typename Fn>
    bool binaryOp(Fn op)
    {
        try
        {
            auto a = std::get<double>(peek());
            auto b = std::get<double>(peek(1));

            pop();
            pop();
            push(op(a, b));
            return true;
        }
        catch(const std::exception&)
        {
            runTimeError("Operands must be numbers.");
            return false;
        }
    }

    // Error
    // Referenced from pksensei
    template <typename... Args>
    void runTimeError(Args&&... args)
    {
        (std::cerr << ... << std::forward<Args>(args));
        std::cerr << '\n';

       
        const auto instruction = (*ip)[pos] - 1;
        const auto line = mChunk->lines[instruction];
        std::cerr << "[line " << line << "] in script\n";
        resetStack();
    }

private:

    Chunk*                          mChunk;                 // Chunk to interpret
    CodeVector*                     ip;                     // Instruction pointer
    std::size_t                     pos;                    // Position in the code
    std::array<Value, STACK_MAX>    stack;                  
    Value*                          stackTop = nullptr;

    Compilation                     cu;
};