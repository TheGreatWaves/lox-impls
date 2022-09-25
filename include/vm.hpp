#pragma once

#include <unordered_map>
#include <chrono>

#include "value.hpp"
#include "compiler.hpp"

// https://en.cppreference.com/w/cpp/utility/variant/visit
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };


class VM;
struct CallFrame;
enum class InterpretResult;

// Max number of elements in the stack
constexpr uint8_t FRAMES_MAX = 64;
constexpr std::size_t STACK_MAX = FRAMES_MAX * UINT8_COUNT;

struct CallFrame
{
    Function  function;
    std::size_t     valueOffset;           // Offset to the values
    std::size_t     ip;                    // Position in the code


    [[nodiscard]] uint8_t readByte()
    {
        return chunk().code.at(ip++);
    }

    [[nodiscard]] uint16_t readShort()
    {
        ip += 2;
        return static_cast<uint16_t>(chunk().code.at(ip - 2) | chunk().code.at(ip - 1));
    }

    [[nodiscard]] const Value& readConstant()
    {
        return chunk().constants.at(readByte());;
    }

    [[nodiscard]] const std::string& readString()
    {
        return std::get<std::string>(readConstant());
    }

    [[nodiscard]] const Chunk& chunk() noexcept
    {
        return function->mChunk;
    }
};

// Defining native functions
static Value clockNative(int, std::vector<Value>::iterator) 
{
    return (double)clock() / CLOCKS_PER_SEC;
}

static Value inputNative(int, std::vector<Value>::iterator)
{
    std::string input;
    std::getline(std::cin, input);

    if (std::isdigit(input.at(0)))
    {
        return std::stod(input);
    }

    return input;
}


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
    struct CallVisitor
    {
        CallVisitor(VM& vm, uint8_t argCount)
            : vm(&vm)
            , argc(argCount)
        {
        }

        [[nodiscard]] bool operator()(const Function& f) noexcept
        {
            return vm->call(f, argc);
        }

        [[nodiscard]] bool operator()(const NativeFunction& f) noexcept
        {
            auto result = f->fn(argc, vm->stack.end() - argc);
            vm->stack.resize(vm->stack.size() - 1 - argc);
            vm->stack.reserve(STACK_MAX);
            vm->push(result);
            return true;
        }


        [[nodiscard]] bool operator()(const auto& ) noexcept
        {
            vm->runTimeError("Can only call functions and classes.");
            return false;
        }

        VM* vm;
        uint8_t argc;
    };
public:
    // Ctor.
    VM() 
    {
        // Clean state.
        resetStack();

        // Define native functions.
        defineNativeFunctions();
    }


    // Interpret source
    InterpretResult interpret(std::string_view code)
    {
        const auto& func = cu.compile(code);
        if (func == nullptr) return InterpretResult::COMPILE_ERROR;

        push(func);
        call(func, 0);
 
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
                for (auto slot = 0; slot < stack.size(); ++slot)
                {
                    std::cout << "[ " << stack[slot] << " ]";

                }
                std::cout << '\n';

                frames.back().function->mChunk.disassembleInstruction(frames.back().ip);

                
                
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
            auto byte = frames.back().readByte();
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

                    // TEMP 
                    // Handle implicit number -> str
                    [this](double s1, std::string s2) -> bool
                    {
                        pop();
                        pop();


                        if (std::fmod(s1, 1.0) == 0)
                        {
                            push(std::to_string(static_cast<int>(s1)) + s2);
                        }
                        else
                        {
                            push(std::to_string(s1) + s2);
                        }
                        return true;
                    },

                    [this](std::string s1, double s2) -> bool
                    {
                        pop();
                        pop();

                        if (std::fmod(s2, 1.0) == 0)
                        {
                            push(s1 + std::to_string(static_cast<int>(s2)));
                        }
                        else
                        {
                            push(s1 + std::to_string(s2));
                        }
                        
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
                auto& constant = frames.back().readConstant();
                push(constant);
                break;
            }
            case OpCode::NIL:       push(std::monostate()); break;
            case OpCode::TRUE:      push(true); break;
            case OpCode::FALSE:     push(false); break;
            case OpCode::POP:       pop(); break;
            case OpCode::GET_GLOBAL:
            {
                auto name = frames.back().readString();

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
                auto& name = frames.back().readString();

                // Assign the global variable 
                // name with the value and pop 
                // it off the stack.
                globals[name] = pop();
                break;
            }
            case OpCode::SET_GLOBAL:
            {
                auto name = frames.back().readString();

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
                auto slot = frames.back().readByte();

                // Push the value onto 
                // the top of the stack.
                push(stack.at(slot + frames.back().valueOffset));
                break;
            }
            case OpCode::SET_LOCAL:
            {
                auto slot = frames.back().readByte();
                stack.at(slot + frames.back().valueOffset) = peek();
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
                auto offset = frames.back().readShort();
                if (isFalsey(peek(0))) 
                {
                    frames.back().ip += offset;
                }
                break;
            }
            case OpCode::JUMP:
            {
                auto offset = frames.back().readShort();
                frames.back().ip += offset;
                break;
            }
            case OpCode::LOOP:
            {
                // Read the offset (Beginning of the statement nested inside loop)
                auto offset = frames.back().readShort();

                // Jump to it
                frames.back().ip -= offset;
                break;
            }
            case OpCode::CALL:
            {
                auto argCount = static_cast<uint8_t>(frames.back().readByte());

                if (!callValue(peek(argCount), argCount))
                {
                    return InterpretResult::RUNTIME_ERROR;
                }

                break;
            }
            case OpCode::RETURN:
            {
                const auto& result = pop();
                
                auto latestOffset = frames.back().valueOffset;
              
                // This will implicitly
                // traverse back one callframe.
                frames.pop_back();

                
                if (frameCount() == 0)
                {
                    pop();
                    return InterpretResult::OK;
                }

                stack.resize(latestOffset);
                stack.reserve(STACK_MAX);
                push(result);
                break;
            }
            }
            
        }
        #undef BINARY_OP
    }

    // Stack related methods

    void resetStack() noexcept
    {
        // Reset frame
        frames.clear();
        stack.clear();
        stack.reserve(STACK_MAX);
    }

    void push(const Value& value) noexcept
    {
        stack.push_back(value);
    }

    Value pop() noexcept
    {
        auto value = std::move(stack.back());
        stack.pop_back();
        return value;
    }

    [[nodiscard]] const Value& peek(std::size_t offset = 0) const
    {
        return stack.at(stack.size() - 1 - offset);
    }

    [[nodiscard]] bool callValue(const Value& callee, uint8_t argCount)
    {
        return std::visit(CallVisitor(*this, argCount), callee);
    }

    bool call(Function function, uint8_t argCount)
    {
        // Check to see if the number of arguments passed in
        // is sufficient
        if (argCount != function->mArity)
        {
            runTimeError("Expected ", function->mArity, " arguments but got ", static_cast<int>(argCount), ".");
            return false;
        }

        // Handle stack overflow
        if (frameCount() == FRAMES_MAX)
        {
            runTimeError("Stack overflow.");
            return false;
        }

        auto offset = stack.size() - 1 - argCount;
        frames.emplace_back(function, offset, 0);

        return true;
    }

private:

    struct FalseyVistor
    {
        bool operator()(const bool b) const noexcept { return !b; }
        bool operator()(const std::monostate) const noexcept { return true; }

        // Handles any other case as true
        bool operator()(auto) const noexcept { return true; }
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

        for (size_t i = frameCount(); i-- > 0;)
        {
            const auto& frame = frames.at(i);
            auto& func = frame.function;
            auto line = func->mChunk.lines.at(frame.ip - 1);
            std::cerr << "[line " << line << "] in " << func->getName() << '\n';
        }
        resetStack();
    }

    void defineNative(const std::string& name, NativeFn fn)
    {
        // Create object
        auto nativeObj = std::make_shared<NativeFunctionObject>();

        // Attatch the native function
        nativeObj->fn = fn;
        
        // Assign the native function to global.
        globals[name] = nativeObj;
    }

    void defineNativeFunctions()
    {
        // Defining clock.
        defineNative("clock", clockNative);

        // Defining input
        defineNative("input", inputNative);

    }

    std::size_t frameCount() { return frames.size(); }

private:
    std::vector<CallFrame>      frames;
    
    std::vector<Value>          stack;                  

    Compilation                 cu;



    std::unordered_map<std::string, Value> globals; // Global variables;
};

