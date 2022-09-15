#pragma once

#include <string_view>

// Enums of instructions supported
enum class OpCode : uint8_t
{
    // Constant literal
    CONSTANT,

    // Values of literals
    NIL,
    TRUE,
    FALSE,

    // Value comparison ops
    EQUAL,
    GREATER,
    LESS,

    // Binary ops
    ADD,
    SUBTRACT,
    MULTIPLY,
    DIVIDE,

    NEGATE,
    NOT,

    // Return op
    RETURN,
};

[[nodiscard]] std::string_view nameof(OpCode code)
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
        default:                throw std::invalid_argument("UNEXPECTED OUTPUT");
    }
}

std::ostream& operator<<(std::ostream& out, OpCode code)
{
	out << nameof(code);
	return out;
}