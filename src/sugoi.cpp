#include "vm.hpp"

int main(int argc, char* argv[])
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
    auto constant = chunk.addConstant(1.2); 
    chunk.write(OpCode::CONSTANT, 1);
    chunk.write(static_cast<uint8_t>(constant), 1);    
    chunk.write(OpCode::RETURN, 1);

    auto val = vm.interpret(&chunk);
    val;

    return 0;
}