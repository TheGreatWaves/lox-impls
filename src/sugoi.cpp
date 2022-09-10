#include "chunk.hpp"

int main(int argc, char* argv[])
{
    // To keep /WX happy (temp)
    argc;
    argv;


    Chunk chunk;
    auto constant = chunk.addConstant(1.2); 
    chunk.write(OpCode::CONSTANT, 1);
    chunk.write(static_cast<uint8_t>(constant), 1);
    
    chunk.write(OpCode::RETURN, 1);

    chunk.disassembleChunk("test chunk");

    return 0;
}