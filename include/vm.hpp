#pragma once

#include <unordered_map>

#include "chunk.hpp"
#include "compiler.hpp"

// https://en.cppreference.com/w/cpp/utility/variant/visit
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };

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
    // Read methods

    // Returns the current byte and increments position.
    [[nodiscard]] uint8_t readByte() noexcept
    {
        return this->ip->at(pos++);
    }

    // Returns the current constant stored at the byte.
    [[nodiscard]] Value& readConstant() noexcept
    {
        // readByte() here is expected to
        // return an uint8_t value which represents
        // the index at which our constant sits in,
        // in the chunk's constant collection.
        return this->mChunk->constants.at(readByte());
    }
    
    // Read the current string constant stored at the byte.
    [[nodiscard]] std::string& readString() noexcept
    {
        // Return the current value and interpret it
        // as a string.
        return std::get<std::string>(readConstant());
    }

public:
    // Ctor.
    VM() 
        : pos(0)
    {
        // Clean state.
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
        return interpret(&chunk);
    }

    // Interpret the chunk
    [[nodiscard]] InterpretResult interpret(Chunk* chunk)
    {
        mChunk = chunk;
        this->ip = &chunk->code;
        this->pos = 0;
        return run();
    }

    // Run the interpreter on the chunk
    [[nodiscard]] InterpretResult run()
    {

        auto readShort = [this]() -> uint16_t {
            this->pos += 2;
            return static_cast<uint16_t>(this->ip->at(pos - 2) | this->ip->at(pos - 1));
        };


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
                    if (!binaryOp([](double b, double a) -> Value { return (a op b); })) \
                    { \
                        return InterpretResult::COMPILE_ERROR; \
                    } \
                } while (false)


         
            // read the current byte and increment the
            auto byte = readByte();
            auto instruction = static_cast<OpCode>(byte);
            switch (instruction)
            {
            case OpCode::ADD:       
            {
                auto success = std::visit(overloaded 
                {
                    // Handle number addition
                    [this](double d1, double d2) -> bool
                    {
                        pop();
                        pop();
                        push(d1 + d2);
                        return true;
                    },

                    // Handle string concat
                    [this](std::string s1, std::string s2) -> bool
                    {
                        pop();
                        pop();
                        push(s1 + s2);
                        return true;
                    },

                    // Handle any other type
                    [this](auto&, auto&) -> bool
                    {
                        runTimeError("Operands must be two numbers or two strings.");
                        return false;
                    }
                }, peek(1), peek(0));

                if (!success) return InterpretResult::RUNTIME_ERROR;
                break;
            }
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
            case OpCode::PRINT:
            {
        
                std::cout << pop() << '\n';
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
            case OpCode::POP:       pop(); break;
            case OpCode::GET_GLOBAL:
            {
                auto name = readString();

                if (auto found = globals.find(name); found == globals.end())
                {
                    // Not found - Variable undefined.
                    runTimeError("Undefined variable '", name.c_str(), "'.");
                    return InterpretResult::RUNTIME_ERROR;
                }
                else
                {
                    // Variable constant 
                    // found, push it.
                    push(found->second);
                }
          
                break;
            }
            case OpCode::DEFINE_GLOBAL:
            {

                // This will definitely be a
                // string because prior functions
                // which calls it will never
                // emit an instruction that 
                // refers to a non string constant.
                auto& name = readString();

                // Assign the global variable 
                // name with the value and pop 
                // it off the stack.
                globals[name] = pop();
                break;
            }
            case OpCode::SET_GLOBAL:
            {
                auto name = readString();

                if (auto found = globals.find(name); found == globals.end())
                {
                    // Not found - Variable undefined.
                    runTimeError("Undefined variable '", name.c_str(), "'.");
                    return InterpretResult::RUNTIME_ERROR;
                }
                else
                {
                    // Variable found, reassign the value
                    found->second = peek(0);    
                }
                break;
            }
            case OpCode::GET_LOCAL:
            {
                // Get the index of the local
                // variable called for
                auto slot = readByte();

                // Push the value onto 
                // the top of the stack.
                push(stack[slot]);
                break;
            }
            case OpCode::SET_LOCAL:
            {
                auto slot = readByte();
                stack[slot] = peek();
                break;
            }
            case OpCode::EQUAL:
            {
                const auto& a = pop();
                const auto& b = pop();
                push(a == b);
                break;
            }
            case OpCode::GREATER:   BINARY_OP(>); break;
            case OpCode::LESS:      BINARY_OP(<); break;
            case OpCode::JUMP_IF_FALSE:
            {
                auto offset = readShort();
                if (isFalsey(peek(0))) 
                {
                    this->pos += offset;
                }
                break;
            }
            case OpCode::JUMP:
            {
                auto offset = readShort();
                pos += offset;
                break;
            }
            case OpCode::LOOP:
            {
                // Read the offset (Beginning of the statement nested inside loop)
                auto offset = readShort();

                // Jump to it
                this->pos -= offset;
                break;
            }
            case OpCode::RETURN:
            {
                // Exit interpreter
                return InterpretResult::OK;
            }
            }
            
        }
        #undef BINARY_OP
    }

    // Stack related methods

    void resetStack() noexcept
    {
        // Reset the pointer
        stackTop = stack.data();

        // Reset Instructor Pointer
        ip = 0;
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

    // Returns whether the value is false or not.
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

       
        //const auto instruction = (*ip).at(pos - 1);
        const auto line = mChunk->lines.at(pos-1);
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

    std::unordered_map<std::string, Value> globals; // Global variables;
};