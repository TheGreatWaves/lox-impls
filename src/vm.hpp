#pragma once

#include <unordered_map>
#include <chrono>

#include "value.hpp"
#include "compiler.hpp"

// https://en.cppreference.com/w/cpp/utility/variant/visit
template <class... Ts>
struct overloaded : Ts...
{
	using Ts::operator()...;
};


class VM;
struct CallFrame;
struct Compilation;
enum class InterpretResult;

// Max number of elements in the stack
constexpr uint8_t     FRAMES_MAX = 64;
constexpr std::size_t STACK_MAX = FRAMES_MAX * UINT8_COUNT;

struct CallFrame
{
	Function    function;
	std::size_t valueOffset; // Offset to the values
	std::size_t ip;          // Position in the code


	[[nodiscard]] uint8_t readByte();

	[[nodiscard]] uint16_t readShort();

	[[nodiscard]] const Value& readConstant();

	[[nodiscard]] const std::string& readString();

	[[nodiscard]] const Chunk& chunk() const noexcept;
};

// Defining native functions
static Value clockNative(int, std::vector<Value>::iterator);

static Value inputNative(int, std::vector<Value>::iterator);


// The result of the interpretation
enum class InterpretResult
{
	OK,
	COMPILE_ERROR,
	RUNTIME_ERROR,
};

// Virtual Machine
class VM
{
private:
	struct CallVisitor
	{
		CallVisitor(VM& vm, uint8_t argCount);

		[[nodiscard]] bool operator()(const Function& f) noexcept;

		[[nodiscard]] bool operator()(const NativeFunction& f) noexcept;

		/**
		 *  Catch others.
		 */
		[[nodiscard]] bool operator()(const auto&) noexcept
		{
			vm->runTimeError("Can only call functions and classes.");
			return false;
		}

		VM*     vm;
		uint8_t argc;
	};

public:
	// Ctor.
	VM()
	{
		// Clean state.
		resetStack();

		// Define native functions.
		defineNativeFunctions();
	}


	// Interpret source
	InterpretResult interpret(std::string_view code);


	// Run the interpreter on the chunk
	[[nodiscard]] InterpretResult run();

	// Stack related methods

	void resetStack() noexcept;

	void push(const Value& value) noexcept;

	Value pop() noexcept;

	[[nodiscard]] const Value& peek(std::size_t offset = 0) const;

	[[nodiscard]] bool callValue(const Value& callee, uint8_t argCount);

	bool call(Function function, uint8_t argCount);

private:
	struct FalseyVistor
	{
		bool operator()(const bool b) const noexcept { return !b; }
		bool operator()(const std::monostate) const noexcept { return true; }

		// Handles any other case as true
		bool operator()(auto) const noexcept { return true; }
	};

	// Returns whether the value is false or not.
	[[nodiscard]] bool isFalsey(const Value& value) const;

	// F will be a lambda function
	template <typename Fn>
	bool binaryOp(Fn op)
	{
		try
		{
			auto a = std::get<double>(peek());
			auto b = std::get<double>(peek(1));
			pop();
			pop();
			push(op(a, b));
			return true;
		}
		catch (const std::exception&)
		{
			runTimeError("Operands must be numbers.");
			return false;
		}
	}

	// Error
	// Referenced from pksensei
	template <typename... Args>
	void runTimeError(Args&&...args)
	{
		(std::cerr << ... << std::forward<Args>(args));
		std::cerr << '\n';

		for (size_t i = frameCount(); i-- > 0;)
		{
			const auto& frame = frames.at(i);
			auto&       func = frame.function;
			const auto  line = func->mChunk.lines.at(frame.ip - 1);
			std::cerr << "[line " << line << "] in " << func->getName() << '\n';
		}
		resetStack();
	}

	void defineNative(const std::string& name, NativeFn fn);

	void defineNativeFunctions();

	std::size_t frameCount() const { return frames.size(); }

private:
	std::vector<CallFrame> frames;

	std::vector<Value> stack;

	Compilation cu;


	std::unordered_map<std::string, Value> globals; // Global variables;
};
