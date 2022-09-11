#pragma once

#include <string_view>

// Enums of instructions supported
enum class OpCode : uint8_t
{
    CONSTANT,
    RETURN,
    NEGATE,
    ADD,
    SUBTRACT,
    MULTIPLY,
    DIVIDE,
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
        default:                throw std::invalid_argument("UNEXPECTED OUTPUT");
    }
}

std::ostream& operator<<(std::ostream& out, OpCode code)
{
	out << nameof(code);
	return out;
}