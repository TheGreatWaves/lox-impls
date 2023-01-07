#pragma once

#include "common.hpp"

#define OpCodes(DO) \
 DO(CONSTANT) \
 DO(NIL) \
 DO(TRUE) \
 DO(FALSE) \
 DO(POP) \
 DO(DEFINE_GLOBAL) \
 DO(GET_GLOBAL) \
 DO(SET_GLOBAL) \
 DO(SET_LOCAL) \
 DO(GET_LOCAL) \
 DO(EQUAL) \
 DO(GREATER) \
 DO(LESS) \
 DO(ADD) \
 DO(SUBTRACT) \
 DO(MULTIPLY) \
 DO(DIVIDE) \
 DO(NEGATE) \
 DO(NOT) \
 DO(PRINT) \
 DO(JUMP_IF_FALSE) \
 DO(JUMP) \
 DO(LOOP) \
 DO(CALL) \
 DO(RETURN)

// Enums of instructions supported

#define DEFINE_ENUMERATION(opcode) opcode,

enum class OpCode : uint8_t
{
    OpCodes( DEFINE_ENUMERATION )
};

#define STR_MAKER(X) #X
#define STR_HELPER(X) STR_MAKER(OP_##X)
#define DEF_OPCODE_TO_STR(opcode) case OpCode::##opcode: return STR_HELPER( opcode ); 

[[nodiscard]] inline std::string_view nameof(OpCode code)
{
    switch (code)
    {
        OpCodes ( DEF_OPCODE_TO_STR )
        default:                throw std::invalid_argument("UNEXPECTED OUTPUT");
    }
}

std::ostream& operator<<(std::ostream& out, OpCode code);
