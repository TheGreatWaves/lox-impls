#include "value.hpp"

std::ostream& operator<<(std::ostream& os, const Value& v) {
    // Call the appropriate overload according to the parameter passed
    std::visit(OutputVisitor(), v);
    return os;
}

Chunk::Chunk() noexcept: code(), lines(), constants()
{
}

void Chunk::write(uint8_t byte, std::size_t line) noexcept
{
	code.push_back(byte);
	lines.push_back(line);
}

void Chunk::write(std::size_t constant, std::size_t line) noexcept
{
	write(static_cast<uint8_t>(constant), line);
}

void Chunk::write(OpCode opcode, std::size_t line)
{
	write(static_cast<uint8_t>(opcode), line);
}

void Chunk::disassembleChunk(std::string_view name) noexcept
{
	std::cout << "== " << name << " ==" << '\n';

	for (std::size_t offset = 0; offset < code.size();)
	{
		offset = this->disassembleInstruction(offset);
	}
}

std::size_t Chunk::simpleInstruction(std::string_view name, std::size_t offset) noexcept
{
	std::cout << name << '\n';
	return offset + 1;
}

std::size_t Chunk::constantInstruction(std::string_view name, std::size_t offset) const noexcept
{
	const auto constant = this->code[offset + 1];

	std::cout << name;
	printf("%4d '", constant);
	std::cout << this->constants[constant] << "'\n";
	return offset + 2;
}

std::size_t Chunk::byteInstruction(std::string_view name, std::size_t offset) const noexcept
{
	auto slot = this->code[offset + 1];
	printf("%-16s %4d\n", name.data(), slot);
	return offset + 2;
}

std::size_t Chunk::jumpInstruction(std::string_view name, int sign, std::size_t offset) const noexcept
{
	auto jump = static_cast<uint16_t>(this->code.at(offset + 1) << 8);
	jump |= this->code.at(offset + 2);
	printf("%-16s %4d -> %d\n", name.data(), static_cast<int>(offset),
	       static_cast<int>(offset) + 3 + sign * jump);
	return offset + 3;
}

std::size_t Chunk::disassembleInstruction(std::size_t offset) noexcept
{
	printf("%04d ", static_cast<int>(offset));

	// Fancy printing, subsequent instructions which is on the
	// same line prints | instead of the line number.
	if (offset > 0 && lines.at(offset) == lines.at(offset - 1))
	{
		std::cout << "  |  ";
	}
	else
	{
		printf("%04d ", static_cast<int>(lines.at(offset)));
	}

	// No need for bounds checking since disassemble chunk
	// will only input valid offsets
	auto instr = static_cast<OpCode>(code.at(offset));


	// Switch for the instruction
	switch (instr)
	{
	case OpCode::ADD:
	case OpCode::SUBTRACT:
	case OpCode::DIVIDE:
	case OpCode::MULTIPLY:
	case OpCode::NEGATE:
	case OpCode::RETURN:
	case OpCode::NIL:
	case OpCode::TRUE:
	case OpCode::FALSE:
	case OpCode::NOT:
	case OpCode::EQUAL:
	case OpCode::GREATER:
	case OpCode::LESS:
	case OpCode::PRINT:
	case OpCode::POP:
		return simpleInstruction(nameof(instr), offset);
	case OpCode::CONSTANT:
	case OpCode::DEFINE_GLOBAL:
	case OpCode::GET_GLOBAL:
	case OpCode::SET_GLOBAL:
		return constantInstruction(nameof(instr), offset);
	case OpCode::SET_LOCAL:
	case OpCode::GET_LOCAL:
	case OpCode::CALL:
		return byteInstruction(nameof(instr), offset);
	case OpCode::JUMP:
	case OpCode::JUMP_IF_FALSE:
		return jumpInstruction(nameof(instr), 1, offset);
	case OpCode::LOOP:
		return jumpInstruction(nameof(instr), -1, offset);
	default:
		std::cout << "Unknown opcode " << static_cast<uint8_t>(instr) << '\n';
		return offset + 1;
	}

}

FunctionObject::FunctionObject(int arity, const std::string_view name)
	: mArity(arity), mName(name), mChunk(Chunk())
{
}

std::string FunctionObject::getName() const
{
	if (mName.empty())
	{
		return "<script>";
	}
	else
	{
		std::stringstream ss;
		ss << "<fn " << std::string(mName) << ">";
		return ss.str();
	}
}
