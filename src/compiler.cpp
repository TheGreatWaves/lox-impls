#include "compiler.hpp"

void errorAt(Parser& parser, const Token token, std::string_view message) noexcept
{
    if (parser.panicMode) return;
    parser.panicMode = true;

    std::cerr << "[line " << token.line << "] Error";

    if (token.type == TokenType::Eof)
    {
        std::cerr << " at end";
    }
    else if (token.type == TokenType::Error)
    {
        // Nothing
    }
    else
    {
        std::cerr << " at " << token.text;
    }
    std::cerr << ": " << message << '\n';
    parser.hadError = true;
}

void errorAtCurrent(Parser& parser, std::string_view message) noexcept
{
    errorAt(parser, parser.current, message);
}

void error(Parser& parser, std::string_view message) noexcept
{
    errorAt(parser, parser.previous, message);
}

void Parser::advance() noexcept
{
	// Store the current token.
	previous = current;

	// If valid simply return, else output error and scan next.
	while (true)
	{
		current = scanner.scanToken();
		if (current.type != TokenType::Error) break;
		errorAtCurrent(*this, current.text);
	}
}

void Parser::consume(TokenType type, std::string_view message) noexcept
{
	// If the current token's type
	// is the expected tpye,
	// consume it and return.
	if (current.type == type)
	{
		advance();
		return;
	}

	// Type wasn't expected, output error.
	errorAtCurrent(*this, message);
}

bool Parser::check(TokenType type) const noexcept
{
	return current.type == type;
}

Compiler::Compiler(Parser& parser, FunctionType type, std::unique_ptr<Compiler> enclosing): funcType(type)
                                                                                          , mEnclosing(std::move(enclosing))
{
	function = std::make_shared<FunctionObject>(0, "");


	if (type != FunctionType::SCRIPT)
	{
		function->mName = parser.previous.text;
	}

	auto& local = locals[localCount++];
	local.depth = 0;
	local.name = "";
}

void Compiler::markInitialized()
{
	if (scopeDepth == 0) return;
	locals.at(localCount).depth = static_cast<int>(scopeDepth);
}

Chunk& Compilation::currentChunk() noexcept
{
	return compiler->function->mChunk; 
}

void Compilation::emitByte(OpCode byte)
{
	currentChunk().write(byte, parser->previous.line);
}

void Compilation::emitByte(uint8_t byte)
{
	currentChunk().write(byte, parser->previous.line);
}

void Compilation::emitReturn()
{
	emitByte(OpCode::NIL);
	emitByte(OpCode::RETURN);
}

void Compilation::emitByte(uint8_t byte1, uint8_t byte2)
{
	emitByte(byte1);
	emitByte(byte2);
}

void Compilation::emitByte(OpCode byte1, OpCode byte2)
{
	emitByte(byte1);
	emitByte(byte2);
}

void Compilation::emitLoop(std::size_t loopStart)
{
	emitByte(OpCode::LOOP);

	// This offset is the size of the statement nested in the while loop.
	auto offset = currentChunk().count() - loopStart + 2;

	if (offset > UINT16_MAX)
	{
		error(*parser, "Loop body too large");
	}

	// Emit bytes containing the offset
	emitByte((offset >> 8) & 0xff);
	emitByte(offset & 0xff);
}

int Compilation::emitJump(OpCode instruction)
{
	return emitJump(static_cast<uint8_t>(instruction));
}

int Compilation::emitJump(uint8_t instruction)
{

	emitByte(instruction);

	// Emit place holder bytes
	emitByte(0xff);
	emitByte(0xff);

	return static_cast<int>(currentChunk().count() - 2);
}

std::shared_ptr<FunctionObject> Compilation::compile(std::string_view code)
{
	// Setup
	this->parser = std::make_unique<Parser>(code);
	this->compiler = std::make_unique<Compiler>(*parser);
        

	//// Compiling
	parser->advance();


	while (!match(TokenType::Eof))
	{
		declaration();
	}

	auto func = endCompiler();
	return parser->hadError ? nullptr : func;
}

Function Compilation::endCompiler() noexcept
{
	emitReturn();

	auto func = compiler->function;



#ifdef DEBUG_PRINT_CODE
        if (!parser->hadError)
        {
            currentChunk().disassembleChunk(compiler->function->getName().empty() ? compiler->function->getName() : "<script>");
        }
#endif

	// Get the enclosing compiler.
	this->compiler = std::move(this->compiler->mEnclosing);

	return func;
}

void Compilation::synchronize()
{
	// Reset flag.
	parser->panicMode = false;

	// Skip tokens indiscriminately, until we reach
	// something that looks like a statement boundary.
	// Like a preceding semi-colon (;) or subsquent
	// token which begins a new statement, usually
	// a control flow or declaration keywords.
	while (parser->current.type != TokenType::Eof)
	{
		// Preceding semi-colon.
		if (parser->previous.type == TokenType::Semicolon) return;

		// If it is one of the keywords listed, we stop.
		switch (parser->current.type)
		{
		case TokenType::Class:
		case TokenType::Fun:
		case TokenType::Var:
		case TokenType::For:
		case TokenType::If:
		case TokenType::While:
		case TokenType::Print:
		case TokenType::Return:
			return;
		default:
			; // Nothing
		}

		// No conditions met, keep advancing.
		parser->advance();
	}
}

void Compilation::expression()
{
	// Beginning of the Pratt parser.

	// Parse with lowest precedence first.
	parsePrecedence(Precedence::Assignment);
}

void Compilation::varDeclaration()
{
	// Parse the variable name and get back
	// the index of the newly pushed constant (the name)
	auto global = parseVariable("Expect variable name.");

	// We expect the next token to be
	// an assignment operator.
	if (match(TokenType::Equal))
	{
		// If there is an equal token,
		// consume it then evaluate
		// the following expression.
		// The result of the evaluation
		// will be the assigned value
		expression();
	}
	else // No equal token found. We imply unintialized declaration.
	{
		// The expression is declared,
		// but uninitialized, implicitly
		// init to nil.
		emitByte(OpCode::NIL);
	}

	// We expect statements to be 
	// terminated with a semi-colon.
	// Consume the final token to 
	// finalize the statement.
	parser->consume(TokenType::Semicolon, "Expect ';' after variable declaration.");

	// If everything went well then we 
	// can now just define the variable.
	defineVariable(global);
}

void Compilation::funDeclaration()
{
	auto global = parseVariable("Expected function name.");
	markInitialized();
	function(FunctionType::FUNCTION);
	defineVariable(global);
}

void Compilation::and_(bool)
{
	auto endJump = emitJump(OpCode::JUMP_IF_FALSE);

	emitByte(OpCode::POP);
	parsePrecedence(Precedence::And);

	patchJump(endJump);
}

void Compilation::or_(bool)
{
	auto elseJump = emitJump(OpCode::JUMP_IF_FALSE);
	auto endJump = emitJump(OpCode::JUMP);

	patchJump(elseJump);
	emitByte(OpCode::POP);

	parsePrecedence(Precedence::Or);
	patchJump(endJump);
}

void Compilation::call(bool)
{
	auto argCount = argumentList();
	emitByte(static_cast<uint8_t>(OpCode::CALL), argCount);
}

void Compilation::expressionStatement()
{
	expression();
	parser->consume(TokenType::Semicolon, "Expect ';' after expression.");
	emitByte(OpCode::POP);
}

void Compilation::returnStatement()
{
	if (compiler->funcType == FunctionType::SCRIPT)
	{
		error(*parser, "Can't return from top-level code.");
	}

	if (match(TokenType::Semicolon))
	{
		emitReturn();
	}
	else
	{
		expression();
		parser->consume(TokenType::Semicolon, "Expect ';' after return value.");
		emitByte(OpCode::RETURN);
	}
}

void Compilation::ifStatement()
{
	// Consume the if '(...)' part. Evaluate the '...' expression aswell.
	parser->consume(TokenType::LeftParen, "Expect '(' after 'if'.");
	expression();
	parser->consume(TokenType::RightParen, "Expect ')' after condition.");

	// Jump offset (if false we jump over the statement)
	auto thenJump = emitJump(static_cast<uint8_t>(OpCode::JUMP_IF_FALSE));
	emitByte(OpCode::POP);
	statement();

	int elseJump = emitJump(static_cast<uint8_t>(OpCode::JUMP));

	patchJump(thenJump);
	emitByte(OpCode::POP);

	if (match(TokenType::Else))
	{
		statement();
	}

	patchJump(elseJump);
}

void Compilation::whileStatement()
{
	// This is the starting position of the bytecode for the while loop statement.
	// We want to jump back to this position, if the expression is true.
	// Note that we jump back to before the condition, to re-evaluate it.
	auto loopStart = currentChunk().count();

	// Consume the while '(...)' part. Evaluate the '...' expression aswell.
	parser->consume(TokenType::LeftParen, "Expect '(' after 'while'.");
	expression();
	parser->consume(TokenType::RightParen, "Expect ')' after condition.");

	// Emit OpCode and place holder byte offset.
	int exitJump = emitJump(OpCode::JUMP_IF_FALSE);

	// Pop the expressson off the stack (value discarded)
	// (This is executed when the loop is true)
	emitByte(OpCode::POP);

	// Evaluate the statement {...}
	statement();

	emitLoop(loopStart);

	// By this point the bytecode offset is known, back patch.
	patchJump(exitJump);

	// Assuming it was false and we jumped over, we now have to pop the value outside.
	// Since the only pop code we have was executed during the loop.
	emitByte(OpCode::POP);
}

void Compilation::forStatement()
{
	// Begin a new scope, the variables will be scoped 
	// to the for loop body.
	beginScope();

	// We expect for ( init ; expr ; incr )
	parser->consume(TokenType::LeftParen, "Expect '(' after 'for'.");


	if (match(TokenType::Semicolon))
	{
		// No initializer : for(; ... ; ...)
		// NOTE: Semi-colon is consumed.
	}
	else if (match(TokenType::Var))
	{
		// We got a variable declaration : for(var i = 0; ... ; ...)
		// NOTE: Semi-colon is consumed.
		varDeclaration();
	}
	else
	{
		// We got an expression 
		// Note expressionStatement() 
		// will also consume a ';'
		// and will pop the value off. for( i = 0 ; ... ; ...)
		// NOTE: Semi-colon is consumed.
		expressionStatement();
	}

	// The beginning of our loop (expr eval)
	auto loopStart = currentChunk().count();

	// Evaluating the expression that can be used to exit the loop.
	auto exitJump = -1;

	// Check if condition clause was omitted, if it was, the next token MUST be a semi-colon.
	// and if it isn't then it won't be a semi-colon.
	if (!match(TokenType::Semicolon))
	{
		// Next token isn't a semicolon, therefore we must evaluate the expression.
		// Put expression on the stack for condition checking for the loop.
		expression();

		// Consume the semi colon after the expression.
		parser->consume(TokenType::Semicolon, "Expect ';' after loop condition.");


		// Jump out of the loop if the condition is false.
		exitJump = emitJump(OpCode::JUMP_IF_FALSE);

		// Pop the expression off the stack.
		emitByte(OpCode::POP);
	}

	// If the next token isn't right parenthesis ')', there is an increment clause.
	if (!match(TokenType::RightParen))
	{
		// There is an increment clause.

		// Offset for jumping to the start of the body
		auto bodyJump = emitJump(OpCode::JUMP);

		// This is where the expression for incrementing is.
		auto incrementStart = currentChunk().count();

		// Compile expression for the side effects.
		// We don't care about the returned value
		// so we simply pop if off the stack.
		expression();
		emitByte(OpCode::POP);

		// Consume the next token, which is expected to be ')'
		parser->consume(TokenType::RightParen, "Expect ')' after for clauses.");

		// Emit a loop instruction, this is the loop that will take us 
		// back to the top of the for loop, right before the condition
		// expression if there is one. The for loop executes after the
		// increment since the increment executes at the end of each
		// loop iteration.
		emitLoop(loopStart);

		// Change loopStart to point to the offset where the increment
		// expression begins. Later when we emit the loop instruction
		// after the body statement, this will cause it to jump up to
		// the increment expression instead of the top.
		loopStart = incrementStart;

		// Back patch the body jump.
		patchJump(bodyJump);
	}

	// Compile the statement.
	statement();

	// Jump back to the beginning (expr)
	emitLoop(loopStart);

	// Patch jump.
	// We do this only when there is a condition clause,
	// otherwise there is no jump to patch, and no condition
	// value on the stack to pop.
	if (exitJump != -1)
	{
		patchJump(static_cast<uint8_t>(exitJump));
		emitByte(OpCode::POP); // Pop condition off
	}

	// Once the whole for loop is evaluated, we have to end the scope.
	endScope();
}

void Compilation::declaration()
{
	// Match variable token.
	if (match(TokenType::Fun))
	{
		funDeclaration();
	}
	else if (match(TokenType::Var))
	{
		// After the var token is consumed, we need
		// to parse for the variable name and value
		varDeclaration();
	}
	else
	{
		// If it isn't a variable
		// it must be a statement.
		statement();
	}

	// Synchronize error after compile-error
	if (parser->panicMode) synchronize();
}

void Compilation::block()
{
	// While we we haven't reached reached the end of
	// the block, or reach the end, we parse the
	// declaration(s).
	while (!parser->check(TokenType::RightBrace) && !parser->check(TokenType::Eof))
	{
		declaration();
	}

	// The while loop ends when the
	// current token is the right brace
	// (or end of file).

	// We simply consume the right brace
	// to complete the process.
	parser->consume(TokenType::RightBrace, "Expect '}': no matching token found.");

}

void Compilation::function(FunctionType type)
{

	// Link member compiler to the new one.
	this->compiler = std::make_unique<Compiler>(*this->parser, type, std::move(std::move(this->compiler)));
        
	// Begin the new scope (function scope).
	beginScope();

	// Consume left paren after function name.
	parser->consume(TokenType::LeftParen, "Expect '(' after function name.");

	// Parameters
	if (!parser->check(TokenType::RightParen))
	{
		do
		{
			// Increment argc.
			compiler->function->mArity++;
			if (compiler->function->mArity > 255)
			{
				errorAtCurrent(*parser, "Can't have more than 255 parameters.");
			}

			// Declare variables and get a dummy constant.
			auto constant = parseVariable("Expect parameter name.");

			// Define variables local to function scope.
			defineVariable(constant);

		} while (match(TokenType::Comma));
	}

	parser->consume(TokenType::RightParen, "Expect ')' after parameters.");
	parser->consume(TokenType::LeftBrace, "Expect '{' before function body.");

	// Compile the function body.
	block();

	// Revert back to the previous compiler.
	auto func = endCompiler();

	emitByte(static_cast<uint8_t>(OpCode::CLOSURE), makeConstant(func));
}

void Compilation::statement()
{
	// Check if we match a print token,
	// if we are then the token will be
	// consumed, then we evaluate the
	// subsequent tokens, expecting them
	// to be expression statements.
	if (match(TokenType::Print))
	{
		printStatement();
	}
	else if (match(TokenType::LeftBrace))
	{
		beginScope();
		block();
		endScope();
	}
	else if (match(TokenType::For))
	{
		forStatement();
	}
	else if (match(TokenType::If))
	{
		ifStatement();
	}
	else if (match(TokenType::Return))
	{
		returnStatement();
	}
	else if (match(TokenType::While))
	{
		whileStatement();
	}
	// We're not looking at print,
	// we must be looking at an
	// expression statement.
	else
	{
		expressionStatement();
	}
}

void Compilation::printStatement() noexcept
{
	// Evalaute the expression
	expression();

	// If parsing and evaluating the expression
	// succeeded, we can then consume the ';'
	// concluding the process.
	parser->consume(TokenType::Semicolon, "Expected ';' after value.");

	// If everything succeeded, 
	// simply emit the bytecode
	// for print.
	emitByte(OpCode::PRINT);
}

bool Compilation::match(TokenType type) noexcept
{
	// If the current token is not the expected type
	// return false.
	if (!parser->check(type)) return false;

	// If it was expected, consume it.
	parser->advance();
	return true;
}

void Compilation::number(bool) noexcept
{
	Value value = std::stod(std::string(parser->previous.text));
	emitConstant(value);
}

void Compilation::emitConstant(const Value value) noexcept
{
	emitByte(static_cast<uint8_t>(OpCode::CONSTANT), makeConstant(value));
}

void Compilation::patchJump(const int offset)
{
	// -2 to adjust for the bytecode for the jump offset itself
	const auto jump = currentChunk().count() - offset - 2;

	if (jump > UINT16_MAX)
	{
		error(*parser, "Too much code to jump over.");
	}

	currentChunk().code.at(offset) = (jump >> 8) & 0xff;
	currentChunk().code.at(offset + 1) = jump & 0xff;
}

uint8_t Compilation::makeConstant(const Value& value) noexcept
{
	// Add the constant to the current chunk and retrieve the 
	// byte code which corresponds to it.
	const auto constant = static_cast<uint8_t>(currentChunk().addConstant(value));

	if (constant > UINT8_MAX)
	{
		error(*parser, "Too many constants in one chunk");
		return 0;
	}

	// Return the bytecode which correspond to 
	// the constant pushed onto the chunk.
	return constant;
}

void Compilation::grouping(bool) noexcept
{
	expression();
	parser->consume(TokenType::RightParen, "Expect ')' after expression.");
}

void Compilation::unary(bool)
{
	// Remember the operator
	const auto operatorType = parser->previous.type;

	// Compile the operand
	parsePrecedence(Precedence::Unary);

	// Emit the operator instruction
	switch (operatorType)
	{
	case TokenType::Minus:  emitByte(OpCode::NEGATE); break;
	case TokenType::Bang:   emitByte(OpCode::NOT); break;
	default: return; // Unreachable
	}
}

void Compilation::binary(bool)
{
	// Remember the operator
	const auto operatorType = parser->previous.type;

	auto rule = getRule(operatorType);
	parsePrecedence(Precedence(static_cast<int>(rule.precedence) + 1));

	// Emit the corresponding opcode
	switch (operatorType)
	{
	case TokenType::BangEqual:      emitByte(OpCode::EQUAL, OpCode::NOT); break;
	case TokenType::EqualEqual:     emitByte(OpCode::EQUAL); break;
	case TokenType::Greater:        emitByte(OpCode::GREATER); break;
	case TokenType::GreaterEqual:   emitByte(OpCode::LESS, OpCode::NOT); break;
	case TokenType::Less:           emitByte(OpCode::LESS); break;
	case TokenType::LessEqual:      emitByte(OpCode::GREATER, OpCode::NOT); break;

	case TokenType::Plus:           emitByte(OpCode::ADD); break;
	case TokenType::Minus:          emitByte(OpCode::SUBTRACT); break;
	case TokenType::Star:           emitByte(OpCode::MULTIPLY); break;
	case TokenType::Slash:          emitByte(OpCode::DIVIDE); break;
	default: return; // Unreachable
	}
}

void Compilation::literal(bool)
{
	switch (parser->previous.type)
	{
	case TokenType::False: emitByte(OpCode::FALSE); break;
	case TokenType::Nil: emitByte(OpCode::NIL); break;
	case TokenType::True: emitByte(OpCode::TRUE); break;
	default: return; // Unreachable
	}
}

void Compilation::string(bool)
{
	// Retrive the text in the form: "str"
	auto str = parser->previous.text;

	// Get rid of quotation marks
	str.remove_prefix(1);
	str.remove_suffix(1);

	// Construct string object
	emitConstant(std::string(str));
}

void Compilation::variable(bool canAssign)
{
	namedVariable(canAssign);
}

int Compilation::resolveLocal() const
{
	// Walk the list from the back,
	// returns the first local which
	// has the same name as the identifier
	// token.

	// The list is walked backward, starting from the
	// current deepest layer, because all locals 
	// only have access to local variables declared in
	// lower or equal depth.

	// The compiler's local array will mirror the vm's stack
	// which means that the index can be directly grabbed from
	// here.
	for (auto i = compiler->localCount - 1; i >= 0; i--)
	{
		auto& local = compiler->locals.at(i);
		if (identifiersEqual(parser->previous.text, local.name))
		{
			if (local.depth == -1)
			{
				error(*parser, "Can't read local variable in its own initializer.");
			}
			return i;
		}
	}

	// Variable with given name not found,
	// assumed to be global variable instead.
	return -1;
}

void Compilation::namedVariable(bool canAssign)
{
	OpCode getOp, setOp;
	int    arg = resolveLocal();
	if (arg != -1)
	{
		getOp = OpCode::GET_LOCAL;
		setOp = OpCode::SET_LOCAL;
	}
	else
	{
		arg = identifierConstant();
		getOp = OpCode::GET_GLOBAL;
		setOp = OpCode::SET_GLOBAL;
	}

	// Indiciates that the variable is
	// calling for a setter/ assignment.
	if (canAssign && match(TokenType::Equal))
	{
		// Evaluate the expression (on the right).
		expression();

		// Link variable name to it in the map.
		emitByte(static_cast<uint8_t>(setOp), static_cast<uint8_t>(arg));
	}
	else
	{
		// Calls for getter / access.
		emitByte(static_cast<uint8_t>(getOp), static_cast<uint8_t>(arg));
	}
}

void Compilation::parsePrecedence(Precedence precedence)
{
	// Consume the first token.
	parser->advance();

	// Get the type of the token.
	auto type = parser->previous.type;

	// Get the precedence rule which applies 
	// to the given token.
	auto prefixRule = getRule(type).prefix;

	if (prefixRule == nullptr)
	{
		error(*parser, "Expected expression.");
		return;
	}

	// Invoke function
	bool canAssign = precedence <= Precedence::Assignment;
	prefixRule(canAssign);

	while (precedence <= getRule(parser->current.type).precedence)
	{
		parser->advance();
		ParseFn infixRule = getRule(parser->previous.type).infix;
		infixRule(canAssign);
	}

	if (canAssign && match(TokenType::Equal))
	{
		error(*this->parser, "Invalid assignment target.");
	}
}

uint8_t Compilation::identifierConstant() noexcept
{
	return makeConstant({ std::string(parser->previous.text) });
}

bool Compilation::identifiersEqual(std::string_view str1, std::string_view str2) noexcept
{
	return str1 == str2;
}

void Compilation::declareVariable()
{
	// If we are in global scope return.
	// This is only for local variables.
	if (compiler->scopeDepth == 0) return;

	const auto name = parser->previous.text;

	for (auto i = compiler->localCount - 1; i >= 0; i--)
	{
		assert(i < static_cast<int>(compiler->locals.size()));
		auto& local = compiler->locals[i];
		if (local.depth != -1 && local.depth < static_cast<int>(compiler->scopeDepth))
		{
			break;
		}

		if (identifiersEqual(name, local.name))
		{
			error(*parser, "Re-definition of an existing variable in this scope.");
		}
	}

	// Add the local variable to the compiler->
	// This makes sure the compiler keeps track
	// of the existence of the variable.
	addLocal(name, *parser.get());
}

uint8_t Compilation::parseVariable(std::string_view message) noexcept
{
	// We expect the token after 'var' to be an identifer.
	parser->consume(TokenType::Identifier, message);

	// Declare the variable
	declareVariable();

	// Check if we are in scope (Not in global)
	// At runtime, locals aren't looked up by name,
	// meaning that there is no need to stuff them
	// int the constant table, if declaration is in 
	// scope, we just return a dummy table index.
	if (compiler->scopeDepth > 0) return 0;

	// If we made it here, it meant that we successfully
	// consumed an identifer token. We now want to add
	// the token lexeme as a new constant, then return
	// the index that it was added at the constant table.
	return identifierConstant();
}

void Compilation::markInitialized() const
{
	if (compiler->scopeDepth == 0)
	{
		return;
	}
	compiler->locals.at(compiler->localCount - 1).depth = static_cast<int>(compiler->scopeDepth);
}

void Compilation::defineVariable(uint8_t global)
{
	// If we are in a scope, we do not want to define global.
	if (compiler->scopeDepth > 0)
	{
		markInitialized();
		return;
	}

	// emit the OpCode and the index of the name. (in chunk's constants)
	emitByte(static_cast<uint8_t>(OpCode::DEFINE_GLOBAL), global);
}

uint8_t Compilation::argumentList()
{
	uint8_t argCount = 0;
	if (!parser->check(TokenType::RightParen))
	{
		do 
		{
			expression();
			if (argCount == 255)
			{
				error(*parser, "Can't have more than 255 arguments.");
			}
			argCount++;
		} while (match(TokenType::Comma));
	}
	parser->consume(TokenType::RightParen, "Expect ')' after arguments.");
	return argCount;
}

void Compilation::beginScope() const noexcept
{
	compiler->scopeDepth++;
}

void Compilation::endScope() noexcept
{
	// End of scope.
	// We go back one scope.
	compiler->scopeDepth--;

	// Pop all the variables which are now out of scope.
	// The while loop crawls backwards on locals and
	// keeps popping the variables off the stack until
	// it reaches a local variable which has the same 
	// depth as the current depth being evaluated.

	// This works beautifully thanks to the fact that
	// variables in the 'locals' array are nicely grouped 
	// together, and that the depth attribute is incrementing
	// uniformly with each subsequent group.

	/* i.e:

            var greeting;
            var message;
            {
                var firstName;           ===     [{greeting, 0}, {message, 0}, {firstName, 1}, {middleName, 1}, {lastName, 1}, {deepestLevel, 2}]
                var middleName;          ===       |_______________________|   |                                            |  |                |
                var lastName;            ===                   0               |____________________________________________|  |                |
                {                                                                                     1                        |________________|
                    var deepestLevel;                                                                                                  2
                }
            }
        */

	// Check that variable count isn't 0.
	while (compiler->localCount > 0
		// Check that the current target variable has deeper depth than the current
		// deepest scope.
		&& compiler->locals[compiler->localCount - 1].depth > static_cast<int>(compiler->scopeDepth))
	{
		// Pop the value off the stack.
		emitByte(OpCode::POP);

		// One less variable.
		compiler->localCount--;
	}

	/* for (auto slot = 0; slot < compiler->localCount; ++slot)
         {
             std::cout << "{ " << compiler->locals[slot].name << " }";
         }
         std::cout << '\n';*/
}

void Compilation::addLocal(const std::string_view name, Parser& targetParser) const
{

	// Since our indexs are stored in a single byte,
	// it means that we can only support 256 local
	// variables in scope at one time, so it must be
	// prevented.
	if (compiler->localCount == UINT8_COUNT)
	{
		error(targetParser, "Too many local variables declared in function.");
		return;
	}

	assert(compiler->localCount < static_cast<int>(compiler->locals.size()));
	auto& local = compiler->locals[compiler->localCount++];
	local.name = name;
	local.depth = -1;

	/*for (auto slot = 0; slot < compiler->localCount; ++slot)
        {
            std::cout << "{ " << compiler->locals[slot].name << " }";
        }
        std::cout << '\n';*/
}

const ParseRule& Compilation::getRule(TokenType type) noexcept
{
	auto grouping = [this](bool canAssign) { this->grouping(canAssign); };
	auto unary = [this](bool    canAssign) { this->unary(canAssign); };
	auto binary = [this](bool   canAssign) { this->binary(canAssign); };
	auto number = [this](bool   canAssign) { this->number(canAssign); };
	auto literal = [this](bool  canAssign) { this->literal(canAssign); };
	auto string = [this](bool   canAssign) { this->string(canAssign); };
	auto variable = [this](bool canAssign) { this->variable(canAssign); };
	auto and_ = [this](bool     canAssign) { this->and_(canAssign); };
	auto or_ = [this](bool      canAssign) { this->or_(canAssign); };
	auto call = [this](bool     canAssign) { this->call(canAssign); };

	static ParseRule rls[] =
	{
		{grouping,      call,       Precedence::Call},       // TokenType::LEFT_PAREN
		{nullptr,       nullptr,    Precedence::None},       // TokenType::RIGHT_PAREN
		{nullptr,       nullptr,    Precedence::None},       // TokenType::LEFT_BRACE
		{nullptr,       nullptr,    Precedence::None},       // TokenType::RIGHT_BRACE
		{nullptr,       nullptr,    Precedence::None},       // TokenType::COMMA
		{nullptr,       nullptr,    Precedence::None},       // TokenType::DOT
		{unary,         binary,     Precedence::Term},       // TokenType::MINUS
		{nullptr,       binary,     Precedence::Term},       // TokenType::PLUS
		{nullptr,       nullptr,    Precedence::None},       // TokenType::SEMICOLON
		{nullptr,       binary,     Precedence::Factor},     // TokenType::SLASH
		{nullptr,       binary,     Precedence::Factor},     // TokenType::STAR
		{unary,         nullptr,    Precedence::None},       // TokenType::BANG
		{nullptr,       binary,     Precedence::Equality},   // TokenType::BANG_EQUAL
		{nullptr,       nullptr,    Precedence::None},       // TokenType::EQUAL
		{nullptr,       binary,     Precedence::Equality},   // TokenType::EQUAL_EQUAL
		{nullptr,       binary,     Precedence::Comparison}, // TokenType::GREATER
		{nullptr,       binary,     Precedence::Comparison}, // TokenType::GREATER_EQUAL
		{nullptr,       binary,     Precedence::Comparison}, // TokenType::LESS
		{nullptr,       binary,     Precedence::Comparison}, // TokenType::LESS_EQUAL
		{variable,      nullptr,    Precedence::None},       // TokenType::IDENTIFIER
		{string,        nullptr,    Precedence::None},       // TokenType::STRING
		{number,        nullptr,    Precedence::None},       // TokenType::NUMBER
		{nullptr,       and_,       Precedence::And},        // TokenType::AND
		{nullptr,       nullptr,    Precedence::None},       // TokenType::CLASS
		{nullptr,       nullptr,    Precedence::None},       // TokenType::ELSE
		{literal,       nullptr,    Precedence::None},       // TokenType::FALSE
		{nullptr,       nullptr,    Precedence::None},       // TokenType::FOR
		{nullptr,       nullptr,    Precedence::None},       // TokenType::FUN
		{nullptr,       nullptr,    Precedence::None},       // TokenType::IF
		{literal,       nullptr,    Precedence::None},       // TokenType::NIL
		{nullptr,       or_,        Precedence::Or},         // TokenType::OR
		{nullptr,       nullptr,    Precedence::None},       // TokenType::PRINT
		{nullptr,       nullptr,    Precedence::None},       // TokenType::RETURN
		{nullptr,       nullptr,    Precedence::None},       // TokenType::SUPER
		{nullptr,       nullptr,    Precedence::None},       // TokenType::THIS
		{literal,       nullptr,    Precedence::None},       // TokenType::TRUE
		{nullptr,       nullptr,    Precedence::None},       // TokenType::VAR
		{nullptr,       nullptr,    Precedence::None},       // TokenType::WHILE
		{nullptr,       nullptr,    Precedence::None},       // TokenType::ERROR
		{nullptr,       nullptr,    Precedence::None},       // TokenType::EOF
	};

	return rls[static_cast<int>(type)];
}
