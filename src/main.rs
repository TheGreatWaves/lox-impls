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
#[repr(u8)]
pub enum Opcode {
    Constant,
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
    pub fn simple_instruction(&self, name: &str, offset: i32) -> i32 {
        println!("{} ", name);
        offset + 1
    }

    /// Push a constant into the constant vector, return the index which the constant resides.
    pub fn add_constant(&mut self, value: Value) -> u8 {
        self.constants.push(value);
        (self.constants.len() - 1) as u8
    }

    /// Print the constant's handle and it's value. Returns the next offset.
    pub fn constant_instruction(&self, name: &str, offset: i32) -> i32 {
        let constant_index = self.code[(offset + 1) as usize] as usize;
        print!("{:-16} {:4} '", name, constant_index);
        print_value(&self.constants[constant_index]);
        println!("'");
        return offset + 2;
    }

    /// Dump the instruction's information.
    pub fn disassemble_instruction(&self, offset: i32) -> i32 {
        print!("{:04} ", offset);

        let offset_index = offset as usize;
        if offset_index > 0 && self.lines[offset_index] == self.lines[offset_index - 1] {
            print!("   | ")
        } else {
            print!("{:4} ", self.lines[offset_index]);
        }

        let instruction = self.code[offset as usize];
        match instruction {
            instruction if instruction == Opcode::Return as u8 => {
                self.simple_instruction("OP_RETURN", offset)
            }
            instruction if instruction == Opcode::Constant as u8 => {
                self.constant_instruction("OP_CONSTANT", offset)
            }
            _ => {
                println!("Unknown opcode {}", instruction as u32);
                offset + 1
            }
        }
    }

    /// For debugging. Dumps the program's instructions.
    pub fn disassemble_chunk(&self, name: &str) {
        println!("== {} ==", name);

        let mut offset: i32 = 0;
        while offset < self.code.len() as i32 {
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

struct VM {
    // Bytecode chunks.
    chunk: Chunk,

    // Instruction pointer.
    ip: usize,
}

impl VM {
    fn new() -> Self {
        Self {
            chunk: Chunk::new(),
            ip: 0,
        }
    }

    fn read_instruction(&mut self) -> Opcode {
        unsafe { ::std::mem::transmute(self.read_byte()) }
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

    fn run(&mut self) -> InterpretResult {
        while self.ip < self.chunk.code.len() {
            match self.read_instruction() {
                Opcode::Constant => {
                    let constant = self.read_constant();
                    println! {"Constant: {}", constant};
                }
                Opcode::Return => {
                    return InterpretResult::Ok;
                }
            }
        }
        InterpretResult::CompileError
    }
}

//
// Main driver.
//
fn main() {
    let mut vm = VM::new();

    // Add a new constant, retrieve the index the constant was written to.
    let constant_index = vm.chunk.add_constant(1.2);

    // Now we will have `<OPCODE_CONSTANT> <CONSTANT_INDEX>`.
    vm.chunk.write_instruction(Opcode::Constant, 123);
    vm.chunk.write(constant_index, 123);

    vm.chunk.write_instruction(Opcode::Return, 123);

    match vm.run() {
        InterpretResult::Ok => println!("All ok!"),
        InterpretResult::CompileError => println!("Compile error"),
    }

    // NOTE: For debugging.
    vm.chunk.disassemble_chunk("test chunk");
}
