//
// Chunk.
//

// List of VM instructions.

#[repr(u8)]
pub enum Opcode {
    Return,
}

// Structure holding bytecode.
pub struct Chunk {
    pub code: Vec<u8>,
}

impl Chunk {
    // Ctor.
    pub fn new() -> Self {
        Self { code: vec![] }
    }

    // Push the bytecode into the chunk.
    pub fn write(&mut self, byte: u8) {
        self.code.push(byte);
    }

    pub fn write_instruction(&mut self, instruction: Opcode) {
        self.code.push(instruction as u8);
    }

    pub fn simple_instruction(&self, name: &str, offset: i32) -> i32 {
        println!("{} ", name);
        offset + 1
    }

    pub fn disassemble_instruction(&self, offset: i32) -> i32 {
        print!("{:04} ", offset);
        let instruction = *self.code.get(offset as usize).unwrap();
        match instruction {
            instruction if instruction == Opcode::Return as u8 => {
                self.simple_instruction("OP_RETURN", offset)
            }
            _ => {
                println!("Unknown opcode {}", instruction as u32);
                offset + 1
            }
        }
    }

    pub fn disassemble_chunk(&self, name: &str) {
        println!("== {} ==", name);

        let mut offset: i32 = 0;
        while offset < self.code.len() as i32 {
            offset = self.disassemble_instruction(offset);
        }
    }
}

//
// Driver.
//

fn main() {
    let mut chunk = Chunk::new();
    chunk.write_instruction(Opcode::Return);
    chunk.disassemble_chunk("test chunk");
}
