#include "vm.hpp"
#include <string>
#include <fstream>
#include <sstream>

void repl(VM& vm)
{
    std::string line;

    while(true)
    {
        std::cout << "> ";
        if (!std::getline(std::cin, line))
        {
            std::cout << '\n';
            break;
        }

        vm;
        //vm.interpret(line);
        // Flush
        line.clear();
    }
}

// Retrive source code string
std::string_view readFile(const std::string& path)
{   
    std::string_view sourceCode;

    try
    {
        std::ifstream ifs(path);
        std::stringstream buffer;
        buffer << ifs.rdbuf();
        sourceCode = buffer.str();
    }
    catch (const std::exception&)
    {
        throw("Could not open or read file");
    }

    return sourceCode;
}

void runFile(VM& vm, const std::string& path)
{
    // Get source code string
    std::string_view src = readFile(path);

    // Interpret it
    auto result = vm.interpret(src);
    
    // Analyse the result 
    switch (result)
    {
    case InterpretResult::OK: break;
    case InterpretResult::COMPILE_ERROR: exit(65);
    case InterpretResult::RUNTIME_ERROR: exit(70);
    }
}

int main(int argc, const char* argv[])
{
    // Logging for debug purposes
    #ifdef DEBUG_TRACE_EXECUTION
    std::cout << "Compiling Mode: [ debug ]\n\n";
    #endif

    // To keep /WX happy (temp)
    argc;
    argv;

    VM vm;
    Chunk chunk;

    if (argc == 1)
    {
        repl(vm);
    }
    else if (argc == 2)
    {
        runFile(vm, argv[1]);
    }
    else
    {
        std::cerr << "Usage: sugoi [path]\n";
        exit(64);
    }

    return 0;
}