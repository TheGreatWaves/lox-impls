#pragma once

#include "common.hpp"


// Enums of instructions supported
enum class OpCode : uint8_t
{
    // Constant literal
    CONSTANT,

    // Values of literals
    NIL,
    TRUE,
    FALSE,

    // Pop value off stack, for evaluating expr
    POP,

    // 1. Define global variable
    // 2. Get global variable
    DEFINE_GLOBAL,
    GET_GLOBAL,
    SET_GLOBAL,

    SET_LOCAL,
    GET_LOCAL,

    // Value comparison ops
    EQUAL,
    GREATER,
    LESS,

    // Binary ops
    ADD,
    SUBTRACT,
    MULTIPLY,
    DIVIDE,

    // Unary ops
    NEGATE,
    NOT,

    PRINT,
    
    // PROGRAM FLOW
    JUMP_IF_FALSE,
    JUMP,
    LOOP,

    CALL,


    // Return op
    RETURN,
};

[[nodiscard]] inline std::string_view nameof(OpCode code)
{
    switch (code)
    {
        case OpCode::CONSTANT:  return "OP_CONSTANT";
        case OpCode::RETURN:    return "OP_RETURN";
        case OpCode::NEGATE:    return "OP_NEGATE";
        case OpCode::ADD:       return "OP_ADD";
        case OpCode::SUBTRACT:  return "OP_SUBTRACT";
        case OpCode::MULTIPLY:  return "OP_MULTIPLY";
        case OpCode::DIVIDE:    return "OP_DIVIDE";
        case OpCode::TRUE:      return "OP_TRUE";
        case OpCode::FALSE:     return "OP_FALSE";
        case OpCode::NIL:       return "OP_NIL";
        case OpCode::NOT:       return "OP_NOT";
        case OpCode::EQUAL:     return "OP_EQUAL";
        case OpCode::GREATER:   return "OP_GREATER";
        case OpCode::LESS:      return "OP_LESS";
        case OpCode::PRINT:     return "OP_PRINT";
        case OpCode::POP:       return "OP_POP";
        case OpCode::DEFINE_GLOBAL: return "OP_DEFINE_GLOBAL";
        case OpCode::GET_GLOBAL: return "OP_GET_GLOBAL";
        case OpCode::SET_GLOBAL: return "OP_SET_GLOBAL";
        case OpCode::SET_LOCAL: return "OP_SET_LOCAL";
        case OpCode::GET_LOCAL: return "OP_GET_LOCAL";
        case OpCode::JUMP_IF_FALSE: return "OP_JUMP_IF_FALSE";
        case OpCode::JUMP:      return "OP_JUMP";
        case OpCode::LOOP:      return "OP_LOOP";
        case OpCode::CALL:      return "OP_CALL";
        default:                throw std::invalid_argument("UNEXPECTED OUTPUT");
    }
}

std::ostream& operator<<(std::ostream& out, OpCode code);
