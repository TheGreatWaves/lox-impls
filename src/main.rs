use std::{
    io::{self, BufRead, Write},
    process::ExitCode,
};

use clap::{command, Parser};

//
// CLI.
//

#[derive(Parser, Debug)]
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
type Value = f32;

pub fn print_value(value: &Value) {
    print!("{}", value);
}

//
// Chunk.
//

// List of VM instructions.
#[derive(FromPrimitive)]
#[repr(u8)]
pub enum Opcode {
    Constant = 1,
    Add,
    Subtract,
    Multiply,
    Divide,
    Negate,
    Return,
}

/// A chunk is a sequence of bytecode.
pub struct Chunk {
    /// The list of bytecode which represents the program.
    pub code: Vec<u8>,
    /// The list of constants declared.
    pub constants: Vec<Value>,
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
    pub fn add_constant(&mut self, value: Value) -> u8 {
        self.constants.push(value);
        (self.constants.len() - 1) as u8
    }

    /// Print the constant's handle and it's value. Returns the next offset.
    pub fn constant_instruction(&self, name: &str, offset: usize) -> usize {
        let constant_index = self.code[(offset + 1) as usize] as usize;
        print!("{:-16} {:4} '", name, constant_index);
        print_value(&self.constants[constant_index]);
        println!("'");
        return offset + 2;
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

        let byte = self.code[offset as usize];
        let instruction: Option<Opcode> = FromPrimitive::from_u8(byte);

        match instruction {
            Some(Opcode::Constant) => self.constant_instruction("OP_CONSTANT", offset),
            Some(Opcode::Add) => self.simple_instruction("OP_CONSTANT", offset),
            Some(Opcode::Subtract) => self.simple_instruction("OP_SUBTRACT", offset),
            Some(Opcode::Multiply) => self.simple_instruction("OP_MULTIPLY", offset),
            Some(Opcode::Divide) => self.simple_instruction("OP_DIVIDE", offset),
            Some(Opcode::Negate) => self.simple_instruction("OP_NEGATE", offset),
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
// Virtual Machine.
//

enum InterpretResult {
    Ok,
    CompileError,
}

const STACK_MAX: usize = 256;

struct VM {
    // Bytecode chunks.
    chunk: Chunk,

    // Instruction pointer.
    ip: usize,

    // Stack.
    stack: Vec<Value>,
}

impl VM {
    fn new() -> Self {
        Self {
            chunk: Chunk::new(),
            ip: 0,
            stack: Vec::with_capacity(STACK_MAX),
        }
    }

    fn push(&mut self, value: Value) {
        self.stack.push(value);
    }

    fn pop(&mut self) -> Value {
        self.stack.pop().unwrap()
    }

    #[allow(unused_variables)]
    fn interpret(&mut self, source: &str) -> InterpretResult {
        // TODO: Implement me!
        InterpretResult::Ok
    }

    fn read_instruction(&mut self) -> Option<Opcode> {
        FromPrimitive::from_u8(self.read_byte())
    }

    fn read_byte(&mut self) -> u8 {
        let instruction: u8 = self.chunk.code[self.ip];
        self.ip += 1;
        instruction
    }

    fn read_constant(&mut self) -> f32 {
        let idx = self.read_byte() as usize;
        self.chunk.constants[idx]
    }

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
                Some(Opcode::Constant) => {
                    let constant = self.read_constant();
                    self.push(constant);
                }
                Some(Opcode::Add) => {
                    let a = self.pop();
                    let b = self.pop();
                    self.push(a + b);
                }
                Some(Opcode::Subtract) => {
                    let a = self.pop();
                    let b = self.pop();
                    self.push(a - b);
                }
                Some(Opcode::Multiply) => {
                    let a = self.pop();
                    let b = self.pop();
                    self.push(a * b);
                }
                Some(Opcode::Divide) => {
                    let a = self.pop();
                    let b = self.pop();
                    self.push(a / b);
                }
                Some(Opcode::Negate) => {
                    let negated_value = -self.pop();
                    self.push(negated_value);
                }
                Some(Opcode::Return) => {
                    println!("{}", self.pop());
                    return InterpretResult::Ok;
                }
                None => {
                    println!("Invalid opcode found.")
                }
            }
        }
        InterpretResult::CompileError
    }
}

//
// Main driver.
//
fn main() -> ExitCode {
    let args = Args::parse();
    let vm = VM::new();

    if let Some(path) = args.path.as_deref() {
        run_file(vm, &path)
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
    let stdin = io::stdin();
    for line in stdin.lock().lines() {
        print!("> ");
        io::stdout().flush().unwrap();
        let line = line.unwrap();
        match vm.interpret(&line) {
            InterpretResult::CompileError => return ExitCode::from(65),
            _ => {}
        }
    }
    ExitCode::SUCCESS
}
