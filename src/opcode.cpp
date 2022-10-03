#include "opcode.hpp"

std::ostream& operator<<(std::ostream& out, OpCode code)
{
	out << nameof(code);
	return out;
}
