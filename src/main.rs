use std::{
    fmt,
    io::{self, BufRead, Write},
    mem,
    process::ExitCode,
};

use clap::command;

//
// CLI.
//

#[derive(clap::Parser, Debug)]
#[command(version, about,long_about = None)]
struct Args {
    // Source code file path. If not specifed, REPL mode will start.
    #[arg(short, long)]
    path: Option<String>,
}

use log::error;
use num_derive::FromPrimitive;
use num_traits::FromPrimitive;

//
// Value.
//
#[derive(Clone, Copy)]
enum Value {
    Bool(bool),
    Nil,
    Number(f32),
}

impl Value {
    #[allow(dead_code)]
    fn is_bool(&self) -> bool {
        matches!(*self, Value::Bool(_))
    }

    #[allow(dead_code)]
    fn is_number(&self) -> bool {
        matches!(*self, Value::Number(_))
    }

    #[allow(dead_code)]
    fn is_nil(&self) -> bool {
        matches!(*self, Value::Nil)
    }

    #[allow(dead_code)]
    fn as_bool(&self) -> bool {
        match *self {
            Value::Bool(value) => value,
            _ => unreachable!(),
        }
    }

    fn as_number(&self) -> f32 {
        match *self {
            Value::Number(value) => value,
            _ => unreachable!(),
        }
    }

    fn is_falsey(&self) -> bool {
        match *self {
            Value::Bool(value) => !value,
            Value::Nil => true,
            _ => false,
        }
    }

    fn is_equal(&self, other: &Value) -> bool {
        match (*self, *other) {
            (Value::Bool(a), Value::Bool(b)) => a == b,
            (Value::Nil, Value::Nil) => true,
            (Value::Number(a), Value::Number(b)) => a == b,
            _ => false,
        }
    }
}

impl fmt::Display for Value {
    // This trait requires `fmt` with this exact signature.
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match *self {
            Value::Bool(v) => write!(f, "{}", v),
            Value::Nil => write!(f, "nil"),
            Value::Number(v) => write!(f, "{}", v),
        }
    }
}

fn print_value(value: &Value) {
    match value {
        Value::Bool(v) => println!("{}", v),
        Value::Nil => println!("nil"),
        Value::Number(v) => println!("{}", v),
    }
}

//
// Chunk.
//

// List of VM instructions.
#[derive(FromPrimitive)]
#[repr(u8)]
pub enum Opcode {
    Constant = 1,
    Nil,   // These three value area added here because it's better for performance.
    True,  // By doing so, we don't need to create another look up table,
    False, // and we can save an additional byte (no need for Opcode::Constant).
    Equal,
    Greater,
    Less,
    Add,
    Subtract,
    Multiply,
    Divide,
    Not,
    Negate,
    Return,
}

// Precedence table. From lowest to highest.
#[derive(FromPrimitive, Clone, Copy)]
#[repr(u8)]
pub enum Precedence {
    None = 1,
    Assignment, // =
    Or,         // or
    And,        // and
    Equality,   // == !=
    Comparison, // < > <= >=
    Term,       // + -
    Factor,     // * /
    Unary,      // ! -
    Call,       // . ()
    Primary,
}

/// A chunk is a sequence of bytecode.
#[derive(Default)]
pub struct Chunk {
    /// The list of bytecode which represents the program.
    pub code: Vec<u8>,
    /// The list of constants declared.
    constants: Vec<Value>,
    /// The line numbers for each bytecode.
    pub lines: Vec<i32>,
}

impl Chunk {
    /// Returns a newly intialized chunk.
    pub fn new() -> Self {
        Self {
            code: vec![],
            constants: vec![],
            lines: vec![],
        }
    }

    /// Write a byte into the chunk.
    pub fn write(&mut self, byte: u8, line: i32) {
        self.code.push(byte);
        self.lines.push(line);
    }

    /// Write an instruction into the chunk.
    pub fn write_instruction(&mut self, instruction: Opcode, line: i32) {
        self.write(instruction as u8, line);
    }

    // TODO: I should probably move this out.
    /// Print instruction name and return the next offset.
    pub fn simple_instruction(&self, name: &str, offset: usize) -> usize {
        println!("{} ", name);
        offset + 1
    }

    /// Push a constant into the constant vector, return the index which the constant resides.
    fn add_constant(&mut self, value: Value) -> u16 {
        self.constants.push(value);
        (self.constants.len() - 1) as u16
    }

    /// Print the constant's handle and it's value. Returns the next offset.
    pub fn constant_instruction(&self, name: &str, offset: usize) -> usize {
        let constant_index = self.code[offset + 1] as usize;
        print!("{:-16} {:4} '", name, constant_index);
        print_value(&self.constants[constant_index]);
        println!("'");
        offset + 2
    }

    /// Dump the instruction's information.
    pub fn disassemble_instruction(&self, offset: usize) -> usize {
        print!("{:04} ", offset);

        let offset_index = offset;
        if offset_index > 0 && self.lines[offset_index] == self.lines[offset_index - 1] {
            print!("   | ")
        } else {
            print!("{:4} ", self.lines[offset_index]);
        }

        let byte = self.code[offset];
        let instruction: Option<Opcode> = FromPrimitive::from_u8(byte);

        match instruction {
            Some(Opcode::Greater) => self.simple_instruction("OP_GREATER", offset),
            Some(Opcode::Less) => self.simple_instruction("OP_GREATER", offset),
            Some(Opcode::Equal) => self.simple_instruction("OP_EQUAL", offset),
            Some(Opcode::True) => self.simple_instruction("OP_TRUE", offset),
            Some(Opcode::False) => self.simple_instruction("OP_FALSE", offset),
            Some(Opcode::Nil) => self.simple_instruction("OP_NIL", offset),
            Some(Opcode::Constant) => self.constant_instruction("OP_CONSTANT", offset),
            Some(Opcode::Add) => self.simple_instruction("OP_ADD", offset),
            Some(Opcode::Subtract) => self.simple_instruction("OP_SUBTRACT", offset),
            Some(Opcode::Multiply) => self.simple_instruction("OP_MULTIPLY", offset),
            Some(Opcode::Divide) => self.simple_instruction("OP_DIVIDE", offset),
            Some(Opcode::Negate) => self.simple_instruction("OP_NEGATE", offset),
            Some(Opcode::Not) => self.simple_instruction("OP_NOT", offset),
            Some(Opcode::Return) => self.simple_instruction("OP_RETURN", offset),
            None => {
                println!("Unknown opcode {}", byte);
                offset + 1
            }
        }
    }

    /// For debugging. Dumps the program's instructions.
    pub fn disassemble_chunk(&self, name: &str) {
        println!("== {} ==", name);

        let mut offset: usize = 0;
        while offset < self.code.len() {
            offset = self.disassemble_instruction(offset);
        }
    }
}

//
// Token.
//
#[derive(Clone, Copy, PartialEq, Eq)]
enum TokenKind {
    // Single-character tokens.
    LeftParen,
    RightParen,
    LeftBrace,
    RightBrace,
    Comma,
    Dot,
    Minus,
    Plus,
    Semicolon,
    Slash,
    Star,
    // One or two character tokens.
    Bang,
    BangEqual,
    Equal,
    EqualEqual,
    Greater,
    GreaterEqual,
    Less,
    LessEqual,
    // Literals.
    Identifier,
    String,
    Number,
    // Keywords.
    And,
    Class,
    Else,
    False,
    For,
    Fun,
    If,
    Nil,
    Or,
    Print,
    Return,
    Super,
    This,
    True,
    Var,
    While,

    Error,
    Eof,
}

#[derive(Clone, Copy)]
struct Token<'a> {
    kind: TokenKind,
    start: usize,
    length: usize,
    line: usize,
    source: &'a str,
}

impl<'a> Token<'a> {
    fn lexeme(&self) -> &'a str {
        &self.source[self.start..self.start + self.length]
    }

    fn new(tty: TokenKind, start: usize, length: usize, line: usize, source: &'a str) -> Self {
        Self {
            kind: tty,
            start,
            length,
            line,
            source,
        }
    }

    fn dummy() -> Self {
        Token::new(TokenKind::Eof, 0, 0, 0, "")
    }
}

//
// Scanner.
//
struct Scanner<'a> {
    current: usize,
    line: usize,
    source: &'a str,
    start: usize,
}

impl<'a> Scanner<'a> {
    // Creates a new [`Scanner`].
    fn new(source: &'a str) -> Self {
        Self {
            current: 0,
            line: 1,
            start: 0,
            source,
        }
    }

    fn skip_whitespace(&mut self) {
        loop {
            let c: char = self.peek();
            match c {
                // General whitespace.
                ' ' | '\r' | '\t' => {
                    self.advance();
                }
                // Handle newline.
                '\n' => {
                    self.line += 1;
                    self.advance();
                }
                // Handle comments.
                '/' => {
                    if self.peek_offset(1) == '/' {
                        // Ignore until newline or scanner exhausted.
                        while self.peek() != '\n' && !self.is_at_end() {
                            self.advance();
                        }
                    } else {
                        return;
                    }
                }
                _ => return,
            }
        }
    }

    // Scan the next token.
    fn scan_token(&mut self) -> Token<'a> {
        self.skip_whitespace();
        self.start = self.current;

        if self.is_at_end() {
            return self.make_token(TokenKind::Eof);
        }

        let c = self.advance();

        if c.is_ascii_digit() {
            return self.number();
        } else if c.is_ascii_alphabetic() || c == '_' {
            return self.identifer();
        }

        match c {
            '(' => return self.make_token(TokenKind::LeftParen),
            ')' => return self.make_token(TokenKind::RightParen),
            '{' => return self.make_token(TokenKind::LeftBrace),
            '}' => return self.make_token(TokenKind::RightBrace),
            ';' => return self.make_token(TokenKind::Semicolon),
            ',' => return self.make_token(TokenKind::Comma),
            '.' => return self.make_token(TokenKind::Dot),
            '-' => return self.make_token(TokenKind::Minus),
            '+' => return self.make_token(TokenKind::Plus),
            '/' => return self.make_token(TokenKind::Slash),
            '*' => return self.make_token(TokenKind::Star),
            '!' => {
                if self.match_char('=') {
                    return self.make_token(TokenKind::BangEqual);
                } else {
                    return self.make_token(TokenKind::Bang);
                }
            }
            '=' => {
                if self.match_char('=') {
                    return self.make_token(TokenKind::EqualEqual);
                } else {
                    return self.make_token(TokenKind::Equal);
                }
            }
            '<' => {
                if self.match_char('=') {
                    return self.make_token(TokenKind::LessEqual);
                } else {
                    return self.make_token(TokenKind::Less);
                }
            }
            '>' => {
                if self.match_char('=') {
                    return self.make_token(TokenKind::GreaterEqual);
                } else {
                    return self.make_token(TokenKind::Greater);
                }
            }
            '"' => {
                return self.string();
            }
            _ => {}
        }

        return self.error_token("Unexpected character.");
    }

    // Advance to the next character.
    fn advance(&mut self) -> char {
        let c = self.peek();
        self.current += 1;
        c
    }

    fn peek_offset(&self, idx: usize) -> char {
        self.source.chars().nth(self.current + idx).unwrap_or('\0')
    }

    fn peek(&self) -> char {
        self.peek_offset(0)
    }

    fn match_char(&mut self, expected: char) -> bool {
        if self.is_at_end() || self.peek() != expected {
            false
        } else {
            self.current += 1;
            true
        }
    }

    // Returns true when the scanner is exhausted.
    fn is_at_end(&self) -> bool {
        self.current >= self.source.len()
    }

    // Create a new token of given kind.
    fn make_token(&self, tty: TokenKind) -> Token<'a> {
        Token::new(
            tty,
            self.start,
            self.current - self.start,
            self.line,
            self.source,
        )
    }

    // Create a new error token with the specific message.
    fn error_token(&self, message: &'static str) -> Token<'a> {
        Token {
            kind: TokenKind::Error,
            start: self.start,
            length: self.current - self.start,
            line: self.line,
            source: message,
        }
    }

    fn string(&mut self) -> Token<'a> {
        while self.peek() != '"' && !self.is_at_end() {
            if self.peek() == '\n' {
                self.line += 1;
            }
            _ = self.advance();
        }

        if self.peek() != '"' {
            self.error_token("Unterminated string")
        } else {
            _ = self.advance();
            self.make_token(TokenKind::String)
        }
    }

    fn number(&mut self) -> Token<'a> {
        while self.peek().is_ascii_digit() {
            _ = self.advance();
        }

        // Handle fraction.
        if self.peek() == '.' {
            self.advance();

            while self.peek().is_ascii_digit() {
                _ = self.advance();
            }
        }

        self.make_token(TokenKind::Number)
    }

    fn identifer(&mut self) -> Token<'a> {
        while self.peek().is_ascii_alphanumeric() || self.peek() == '_' {
            self.advance();
        }
        self.make_token(self.identifer_type())
    }

    fn identifer_type(&self) -> TokenKind {
        match self.source.chars().nth(self.start).unwrap() {
            'a' => return self.check_keyword(1, "nd", TokenKind::And),
            'c' => return self.check_keyword(1, "lass", TokenKind::Class),
            'e' => return self.check_keyword(1, "lse", TokenKind::Else),
            'f' => {
                if self.current - self.start > 1 {
                    match self.source.chars().nth(self.start + 1).unwrap() {
                        'a' => return self.check_keyword(2, "lse", TokenKind::False),
                        'o' => return self.check_keyword(2, "r", TokenKind::For),
                        'u' => return self.check_keyword(2, "n", TokenKind::Fun),
                        _ => {}
                    }
                }
            }
            't' => {
                if self.current - self.start > 1 {
                    match self.source.chars().nth(self.start + 1).unwrap() {
                        'h' => return self.check_keyword(2, "is", TokenKind::This),
                        'r' => return self.check_keyword(2, "ue", TokenKind::True),
                        _ => {}
                    }
                }
            }
            'i' => return self.check_keyword(1, "f", TokenKind::If),
            'n' => return self.check_keyword(1, "il", TokenKind::Nil),
            'o' => return self.check_keyword(1, "r", TokenKind::Or),
            'p' => return self.check_keyword(1, "rint", TokenKind::Print),
            'r' => return self.check_keyword(1, "eturn", TokenKind::Return),
            's' => return self.check_keyword(1, "uper", TokenKind::Super),
            'v' => return self.check_keyword(1, "ar", TokenKind::Var),
            'w' => return self.check_keyword(1, "hile", TokenKind::While),
            _ => {}
        }

        TokenKind::Identifier
    }

    fn check_keyword(&self, offset: usize, expected: &str, kind: TokenKind) -> TokenKind {
        let keyword_found =
            &self.source[self.start + offset..self.start + offset + expected.len()] == expected;

        let length_matches = (self.current - self.start) == expected.len() + offset;

        if keyword_found && length_matches {
            kind
        } else {
            TokenKind::Identifier
        }
    }
}

//
// The parser.
//
struct Parser<'a> {
    scanner: Scanner<'a>,
    current: Token<'a>,
    previous: Token<'a>,
    had_error: bool,
    chunk: Chunk,

    // Flag for sane error reporting.
    // Resync the state of the parser.
    panic: bool,
}

impl<'a> Parser<'a> {
    fn new(source: &'a str) -> Self {
        Self {
            scanner: Scanner::new(source),
            current: Token::dummy(),
            previous: Token::dummy(),
            had_error: false,
            panic: false,
            chunk: Chunk::new(),
        }
    }

    fn advance(&mut self) {
        self.previous = self.current;

        loop {
            self.current = self.scanner.scan_token();
            if self.current.kind != TokenKind::Error {
                break;
            }

            let lexeme = self.current.lexeme();
            self.report_error_at_current(lexeme);
        }
    }

    fn consume(&mut self, kind: TokenKind, message: &str) {
        let got_expected = self.current.kind == kind;

        if got_expected {
            self.advance();
        } else {
            self.report_error_at_current(message);
        }
    }

    fn report_error_at(&mut self, token: Token, message: &str) {
        if self.panic {
            return;
        }

        self.panic = true;

        eprint!("[line {}] Error", token.line);

        if token.kind == TokenKind::Eof {
            eprint!(" at end");
        } else if token.kind == TokenKind::Error {
            // Do nothing.
        } else {
            eprint!(" at {}", token.lexeme());
        }

        // Print error message
        eprintln!(": {}", message);

        self.had_error = true;
    }

    fn report_error_at_current(&mut self, message: &str) {
        let token = self.current;
        self.report_error_at(token, message);
    }

    #[allow(dead_code)]
    fn report_error(&mut self, message: &str) {
        let token = self.previous;
        self.report_error_at(token, message);
    }

    #[allow(dead_code)]
    fn emit_byte(&mut self, byte: u8) {
        self.chunk.write(byte, self.previous.line as i32);
    }

    fn emit_opcode(&mut self, opcode: Opcode) {
        self.emit_byte(opcode as u8);
    }

    #[allow(dead_code)]
    fn emit_bytes(&mut self, byte1: u8, byte2: u8) {
        self.emit_byte(byte1);
        self.emit_byte(byte2);
    }

    #[allow(dead_code)]
    fn end(&mut self) {
        self.emit_opcode(Opcode::Return);

        if cfg!(debug_assetions) && !self.had_error {
            self.chunk.disassemble_chunk("code");
        }
    }

    fn expression(&mut self) {
        self.parse_precedence(Precedence::Assignment);
    }

    #[allow(dead_code)]
    // Convert lexeme into numerical value, then push the value into the constant array.
    fn number(&mut self) {
        let value: f32 = self.previous.lexeme().parse::<f32>().unwrap();
        self.emit_constant(Value::Number(value));
    }

    // Note: This function assumes that the '(' has already been consumed.
    fn grouping(&mut self) {
        self.expression();
        self.consume(TokenKind::RightParen, "Expect ')' after expression.");
    }

    fn unary(&mut self) {
        let token_type: TokenKind = self.previous.kind;

        // Collect / compile the operand.
        self.parse_precedence(Precedence::Unary);

        // Emit the instruction based on the token type.
        if TokenKind::Minus == token_type {
            self.emit_opcode(Opcode::Negate);
        } else if token_type == TokenKind::Bang {
            self.emit_opcode(Opcode::Not);
        }
    }

    fn binary(&mut self) {
        let operator_type = self.previous.kind;

        let rule = self.get_rule(operator_type);

        self.parse_precedence(Precedence::from_u8(rule.precedence as u8 + 1).unwrap());

        match operator_type {
            TokenKind::Plus => self.emit_opcode(Opcode::Add),
            TokenKind::Minus => self.emit_opcode(Opcode::Subtract),
            TokenKind::Star => self.emit_opcode(Opcode::Multiply),
            TokenKind::Slash => self.emit_opcode(Opcode::Divide),
            TokenKind::BangEqual => self.emit_bytes(Opcode::Equal as u8, Opcode::Not as u8),
            TokenKind::EqualEqual => self.emit_opcode(Opcode::Equal),
            TokenKind::Greater => self.emit_opcode(Opcode::Greater),
            TokenKind::GreaterEqual => self.emit_bytes(Opcode::Less as u8, Opcode::Not as u8),
            TokenKind::Less => self.emit_opcode(Opcode::Less),
            TokenKind::LessEqual => self.emit_bytes(Opcode::Greater as u8, Opcode::Not as u8),
            _ => unreachable!(),
        }
    }

    // Emit constant pushes two things onto the stack.
    // - Opcode::Constant
    // - The index which the constant lives in the constant array.
    #[allow(dead_code)]
    fn emit_constant(&mut self, value: Value) {
        let constant = self.make_constant(value);
        self.emit_bytes(Opcode::Constant as u8, constant);
    }

    // Creates a new constant and return the index which it lives inside the value array.
    #[allow(dead_code)]
    fn make_constant(&mut self, value: Value) -> u8 {
        let constant = self.chunk.add_constant(value);

        if constant > std::u8::MAX as u16 {
            self.report_error("Too many constants in one chunk.");
            0
        } else {
            constant as u8
        }
    }

    fn get_rule(&mut self, operator_type: TokenKind) -> ParseRule {
        let empty_rule = ParseRule {
            prefix: None,
            infix: None,
            precedence: Precedence::None,
        };
        match operator_type {
            TokenKind::LeftParen => ParseRule {
                prefix: Some(Box::new(|this| this.grouping())),
                precedence: Precedence::None,
                ..empty_rule
            },
            TokenKind::RightParen => empty_rule,
            TokenKind::LeftBrace => empty_rule,
            TokenKind::RightBrace => empty_rule,
            TokenKind::Comma => empty_rule,
            TokenKind::Dot => empty_rule,
            TokenKind::Minus => ParseRule {
                prefix: Some(Box::new(|this| this.unary())),
                infix: Some(Box::new(|this| this.binary())),
                precedence: Precedence::Term,
            },
            TokenKind::Plus => ParseRule {
                infix: Some(Box::new(|this| this.binary())),
                precedence: Precedence::Term,
                ..empty_rule
            },
            TokenKind::Semicolon => empty_rule,
            TokenKind::Slash => ParseRule {
                infix: Some(Box::new(|this| this.binary())),
                precedence: Precedence::Factor,
                ..empty_rule
            },
            TokenKind::Star => ParseRule {
                infix: Some(Box::new(|this| this.binary())),
                precedence: Precedence::Factor,
                ..empty_rule
            },
            TokenKind::Bang => ParseRule {
                prefix: Some(Box::new(|this| this.unary())),
                ..empty_rule
            },
            TokenKind::BangEqual => ParseRule {
                prefix: Some(Box::new(|this| this.binary())),
                precedence: Precedence::Equality,
                ..empty_rule
            },
            TokenKind::Equal => empty_rule,
            TokenKind::EqualEqual => ParseRule {
                infix: Some(Box::new(|this| this.binary())),
                precedence: Precedence::Equality,
                ..empty_rule
            },
            TokenKind::Greater => ParseRule {
                infix: Some(Box::new(|this| this.binary())),
                precedence: Precedence::Comparison,
                ..empty_rule
            },
            TokenKind::GreaterEqual => ParseRule {
                infix: Some(Box::new(|this| this.binary())),
                precedence: Precedence::Comparison,
                ..empty_rule
            },
            TokenKind::Less => ParseRule {
                infix: Some(Box::new(|this| this.binary())),
                precedence: Precedence::Comparison,
                ..empty_rule
            },
            TokenKind::LessEqual => ParseRule {
                infix: Some(Box::new(|this| this.binary())),
                precedence: Precedence::Comparison,
                ..empty_rule
            },
            TokenKind::Identifier => empty_rule,
            TokenKind::String => empty_rule,
            TokenKind::Number => ParseRule {
                prefix: Some(Box::new(|this| this.number())),
                ..empty_rule
            },
            TokenKind::And => empty_rule,
            TokenKind::Class => empty_rule,
            TokenKind::Else => empty_rule,
            TokenKind::False => ParseRule {
                prefix: Some(Box::new(|this| this.literal())),
                ..empty_rule
            },
            TokenKind::For => empty_rule,
            TokenKind::Fun => empty_rule,
            TokenKind::If => empty_rule,
            TokenKind::Nil => ParseRule {
                prefix: Some(Box::new(|this| this.literal())),
                ..empty_rule
            },
            TokenKind::Or => empty_rule,
            TokenKind::Print => empty_rule,
            TokenKind::Return => empty_rule,
            TokenKind::Super => empty_rule,
            TokenKind::This => empty_rule,
            TokenKind::True => ParseRule {
                prefix: Some(Box::new(|this| this.literal())),
                ..empty_rule
            },
            TokenKind::Var => empty_rule,
            TokenKind::While => empty_rule,
            TokenKind::Error => empty_rule,
            TokenKind::Eof => empty_rule,
        }
    }

    fn parse_precedence(&mut self, precedence: Precedence) {
        self.advance();

        // Retrieve the prefix rule for the given token kind.
        // We expect this to return a valid rule because if it
        // does not then we have an incorrect first token.
        // For instance, an expression can not start with 'else' or '}'.
        // dbg!(self.previous.lexeme());
        let prefix_rule = self.get_rule(self.previous.kind).prefix;

        if let Some(rule) = prefix_rule {
            rule(self);
        } else {
            self.report_error("Expect expression.");
            return;
        }

        while (precedence as u8) <= (self.get_rule(self.current.kind).precedence as u8) {
            self.advance();
            let infix_rule = self.get_rule(self.previous.kind).infix.unwrap();
            infix_rule(self);
        }
    }

    fn literal(&mut self) {
        match self.previous.kind {
            TokenKind::True => self.emit_opcode(Opcode::True),
            TokenKind::False => self.emit_opcode(Opcode::False),
            TokenKind::Nil => self.emit_opcode(Opcode::Nil),
            _ => unreachable!(),
        }
    }
}

type ParseFn = Box<dyn Fn(&mut Parser)>;

//
// Parse rule.
//
struct ParseRule {
    prefix: Option<ParseFn>,
    infix: Option<ParseFn>,
    precedence: Precedence,
}

//
// The compiler.
//
struct Compiler<'a> {
    parser: Parser<'a>,
}

impl<'a> Compiler<'a> {
    fn new(source: &'a str) -> Self {
        Self {
            parser: Parser::new(source),
        }
    }

    fn compile(&mut self) -> Option<Chunk> {
        self.parser.advance();
        self.parser.expression();
        self.parser
            .consume(TokenKind::Eof, "Expected end of expression.");
        self.parser.end();

        if self.parser.had_error {
            None
        } else {
            Some(mem::take(&mut self.parser.chunk))
        }
    }
}

//
// Virtual Machine.
//

// The InterpretResult enum symbolises the state of the compiler result.
enum InterpretResult {
    Ok,
    CompileError,
    RuntimeError,
}

// The max size of the stack.
const STACK_MAX: usize = 256;

// The virtual machine (VM) is responsible for interpreting bytecode chunks and mutating internal state accordingly.
struct VM {
    // Bytecode chunks.
    chunk: Chunk,

    // Instruction pointer.
    ip: usize,

    // Stack.
    stack: Vec<Value>,
}

impl VM {
    // Return a new virtual machine instance.
    fn new(chunk: Chunk) -> Self {
        Self {
            chunk,
            ip: 0,
            stack: Vec::with_capacity(STACK_MAX),
        }
    }

    // Push a new value onto the stack.
    fn push(&mut self, value: Value) {
        self.stack.push(value);
    }

    // Pop and return value from the stack.
    fn pop(&mut self) -> Value {
        self.stack.pop().unwrap()
    }

    // Interpret source code. Return Interpret result which symbolizes the success state.
    #[allow(unused_variables)]
    fn interpret(&mut self, source: &str) -> InterpretResult {
        let mut compiler = Compiler::new(source);

        let chunk = compiler.compile();

        if chunk.is_none() {
            return InterpretResult::CompileError;
        }

        // Take the compiled chunk.
        self.chunk = chunk.unwrap();

        self.ip = 0;

        self.run(false)
    }

    // Interpret the next byte as an opcode.
    fn read_instruction(&mut self) -> Option<Opcode> {
        FromPrimitive::from_u8(self.read_byte())
    }

    // Read the current byte and increment onto the next.
    fn read_byte(&mut self) -> u8 {
        let instruction: u8 = self.chunk.code[self.ip];
        self.ip += 1;
        instruction
    }

    // Read the byte as the value used to index into the constants array.
    fn read_constant(&mut self) -> &Value {
        let idx = self.read_byte() as usize;
        &self.chunk.constants[idx]
    }

    // Main run loop. Interpret all byte code and mutate internal state.
    #[allow(dead_code)]
    fn run(&mut self, debug: bool) -> InterpretResult {
        while self.ip < self.chunk.code.len() {
            if debug {
                print!("          ");
                self.stack.iter().for_each(|&slot| print!("[ {} ]", slot));
                println!();
                self.chunk.disassemble_instruction(self.ip);
            }
            match self.read_instruction() {
                Some(Opcode::Equal) => {
                    let b = self.pop();
                    let a = self.pop();
                    self.push(Value::Bool(a.is_equal(&b)))
                }
                Some(Opcode::Greater) => {
                    if !self.peek(0).is_number() || !self.peek(1).is_number() {
                        self.runtime_error("Operands must be numbers.");
                        return InterpretResult::RuntimeError;
                    }
                    let b = self.pop().as_number();
                    let a = self.pop().as_number();
                    self.push(Value::Bool(a > b));
                }
                Some(Opcode::Less) => {
                    if !self.peek(0).is_number() || !self.peek(1).is_number() {
                        self.runtime_error("Operands must be numbers.");
                        return InterpretResult::RuntimeError;
                    }
                    let b = self.pop().as_number();
                    let a = self.pop().as_number();
                    self.push(Value::Bool(a < b));
                }
                Some(Opcode::Not) => {
                    let value = self.pop();
                    self.push(Value::Bool(value.is_falsey()));
                }
                Some(Opcode::False) => self.push(Value::Bool(false)),
                Some(Opcode::True) => self.push(Value::Bool(true)),
                Some(Opcode::Nil) => self.push(Value::Nil),
                Some(Opcode::Constant) => {
                    let constant = *self.read_constant();
                    self.push(constant);
                }
                Some(Opcode::Add) => {
                    if !self.peek(0).is_number() || !self.peek(1).is_number() {
                        self.runtime_error("Operands must be numbers.");
                        return InterpretResult::RuntimeError;
                    }
                    let b = self.pop().as_number();
                    let a = self.pop().as_number();
                    self.push(Value::Number(a + b));
                }
                Some(Opcode::Subtract) => {
                    if !self.peek(0).is_number() || !self.peek(1).is_number() {
                        self.runtime_error("Operands must be numbers.");
                        return InterpretResult::RuntimeError;
                    }
                    let b = self.pop().as_number();
                    let a = self.pop().as_number();
                    self.push(Value::Number(a - b));
                }
                Some(Opcode::Multiply) => {
                    if !self.peek(0).is_number() || !self.peek(1).is_number() {
                        self.runtime_error("Operands must be numbers.");
                        return InterpretResult::RuntimeError;
                    }
                    let a = self.pop().as_number();
                    let b = self.pop().as_number();
                    self.push(Value::Number(a * b));
                }
                Some(Opcode::Divide) => {
                    if !self.peek(0).is_number() || !self.peek(1).is_number() {
                        self.runtime_error("Operands must be numbers.");
                        return InterpretResult::RuntimeError;
                    }
                    let a = self.pop().as_number();
                    let b = self.pop().as_number();
                    self.push(Value::Number(a / b));
                }
                Some(Opcode::Negate) => {
                    if self.peek(0).is_number() {
                        self.runtime_error("Operand must be a number.");
                    }
                    let negated_value = -self.pop().as_number();
                    self.push(Value::Number(negated_value));
                }
                Some(Opcode::Return) => {
                    print_value(&self.pop());
                    return InterpretResult::Ok;
                }
                None => {
                    println!("Invalid opcode found.")
                }
            }
        }
        InterpretResult::CompileError
    }

    fn peek(&self, offset: usize) -> &Value {
        &self.stack[offset]
    }

    fn runtime_error(&mut self, message: &str) {
        eprintln!("{}", message);
        let instruction = self.ip;
        let line = self.chunk.lines[instruction];
        eprintln!("[line {}] in script", line);
    }
}

//
// Main driver.
//
fn main() -> ExitCode {
    let args = <Args as clap::Parser>::parse();
    let vm = VM::new(Chunk::new());

    if let Some(path) = args.path.as_deref() {
        run_file(vm, path)
    } else {
        run_repl(vm)
    }
}

//
// Run file.
//
fn run_file(mut vm: VM, path: &str) -> ExitCode {
    let file_source = std::fs::read_to_string(path);

    if let Ok(file_source) = file_source {
        let result = vm.interpret(&file_source);

        match result {
            InterpretResult::CompileError => ExitCode::from(65),
            InterpretResult::Ok => ExitCode::SUCCESS,
            InterpretResult::RuntimeError => ExitCode::from(70),
        }
    } else {
        // File not found.
        error!("File at path not found: {}", path);
        io::stdout().flush().unwrap();
        ExitCode::from(74)
    }
}

//
// REPL.
//
fn run_repl(mut vm: VM) -> ExitCode {
    let _ = vm;
    print!("> ");
    io::stdout().flush().unwrap();

    'l: loop {
        let stdin = io::stdin();
        for line in stdin.lock().lines() {
            let line = line.unwrap();
            if let InterpretResult::CompileError = vm.interpret(&line) {
                break 'l;
            }
            print!("> ");
            io::stdout().flush().unwrap();
        }
    }
    ExitCode::SUCCESS
}

#[cfg(test)]
mod tests {
    use super::*;

    // Testing scanner.

    #[test]
    fn test_scanner_basic() {
        let source = "(";
        let mut scanner = Scanner::new(source);

        let token = scanner.scan_token();

        assert!(token.kind == TokenKind::LeftParen);
    }

    #[test]
    fn test_scanner() {
        let source = "({;,.-+/*})";
        let mut scanner = Scanner::new(source);

        let mut idx = 0;
        let expected = [
            TokenKind::LeftParen,
            TokenKind::LeftBrace,
            TokenKind::Semicolon,
            TokenKind::Comma,
            TokenKind::Dot,
            TokenKind::Minus,
            TokenKind::Plus,
            TokenKind::Slash,
            TokenKind::Star,
            TokenKind::RightBrace,
            TokenKind::RightParen,
        ];
        while !scanner.is_at_end() {
            let token = scanner.scan_token();

            assert!(token.kind == expected[idx]);

            idx += 1;
        }
    }
}
