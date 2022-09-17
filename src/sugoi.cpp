#include "vm.hpp"
#include <string>
#include <fstream>
#include <sstream>

// Read-Evaluate-Print-Loop
void repl(VM& vm)
{
    std::string line;

    while(true)
    {
        // Read line.
        std::cout << "> ";
        if (!std::getline(std::cin, line))
        {
            // If no line is read, break.
            std::cout << '\n';
            break;
        }

        // Interpet the line.
        vm.interpret(line);

        // Flush the line.
        line.clear();
    }
}

// Retrive source code string from the
// file path.
std::string readFile(const std::string& path)
{   
    std::string sourceCode;

    try
    {
        // Check if we can open the file
        std::ifstream ifs(path);
        std::stringstream buffer;
        buffer << ifs.rdbuf();
        sourceCode = buffer.str();
    }
    catch (const std::exception&)
    {
        // Error opening file, throw error.
        throw("Could not open or read file");
    }

    // Return the string view because we
    // don't need the string itself.
    return sourceCode;
}

// Read the file, and interpret ALL the
// text inside the file.
void runFile(VM& vm, const std::string& path)
{
    // Get source code string.
    std::string src = readFile(path);

    // Interpret it.
    auto result = vm.interpret(src);
    
  
    // Analyse the result 
    // If there is a compiler error, quit.
    switch (result)
    {
    case InterpretResult::OK: break;
    case InterpretResult::COMPILE_ERROR: exit(65);
    case InterpretResult::RUNTIME_ERROR: exit(70);
    }
}

int main(int argc, const char* argv[])
{
    // If debug mode is actiavted,
    // signal the terminal.
    #ifdef DEBUG_TRACE_EXECUTION
    std::cout << "Compiling Mode: [ debug ]\n\n";
    #endif

    // Initialize the virtual machine
    VM vm;

    // If there is no argument passed it
    // we activate the REPL.
    if (argc == 1)
    {
        repl(vm);
    }
    // If there is an argument passed in,
    // read the file and interpret it.
    else if (argc == 2)
    {
        runFile(vm, argv[1]);
    }
    // Else it is false usage.
    else
    {
        std::cerr << "Usage: sugoi [path]\n";
        exit(64);
    }

    return 0;
}