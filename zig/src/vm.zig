const Chunk = @import("chunk.zig").Chunk;
const OpCode = @import("common.zig").OpCode;
const Value = @import("value.zig").Value;
const printValue = @import("value.zig").printValue;
const std = @import("std");
const debug = @import("debug.zig");

pub const InterpretResult = enum {
    ok,
    compile_error,
    runtime_error,
};

const BinaryOp = enum {
    add,
    subtract,
    multiply,
    divide,
};

const STACK_MAX = 256;

pub const VM = struct {
    chunk: ?*Chunk,
    ip: u8,
    stack: [STACK_MAX]Value,
    sp: u8,

    const Self = @This();

    pub fn init() Self {
        var vm = Self{
            .chunk = null,
            .ip = 0, // Instruction pointer starts at 0.
            .stack = std.mem.zeroes([STACK_MAX]Value),
            .sp = 0,
        };

        vm.reset_stack();

        return vm;
    }

    pub fn interpret(self: *Self, chunk: *Chunk) InterpretResult {
        self.chunk = chunk;
        self.ip = 0;
        return self.run();
    }

    pub fn push(self: *Self, value: Value) void {
        self.stack[self.sp] = value;
        self.sp += 1;
    }

    pub fn pop(self: *Self) Value {
        self.sp -= 1;
        return self.stack[self.sp];
    }

    pub fn stack_top(self: *Self) *Value {
        return &self.stack[self.sp - 1];
    }

    fn read_byte(self: *Self) u8 {
        const byte = self.chunk.?.code[self.ip];
        self.ip += 1;
        return byte;
    }

    fn read_constant(self: *Self) Value {
        const constant_idx = self.read_byte();
        return self.chunk.?.constants.values[constant_idx];
    }

    fn reset_stack(self: *Self) void {
        self.sp = 0;
    }

    fn binary_op(self: *Self, comptime binary_op_type: BinaryOp) void {
        const b = @as(f64, self.pop());
        switch (binary_op_type) {
            BinaryOp.add => {
                self.stack_top().* += b;
            },
            BinaryOp.subtract => {
                self.stack_top().* -= b;
            },
            BinaryOp.multiply => {
                self.stack_top().* *= b;
            },
            BinaryOp.divide => {
                self.stack_top().* /= b;
            },
        }
    }

    fn run(self: *Self) InterpretResult {
        while (true) {
            if (debug.DEBUG) {
                std.debug.print("          ", .{});
                for (0..self.sp) |p| {
                    std.debug.print("[ ", .{});
                    printValue(self.stack[p]);
                    std.debug.print(" ]", .{});
                }
                std.debug.print("\n", .{});
                _ = debug.disassembleInstruction(self.chunk.?, self.ip);
            }

            const instruction: u8 = self.read_byte();

            switch (instruction) {
                @intFromEnum(OpCode.@"return") => {
                    printValue(self.pop());
                    std.debug.print("\n", .{});
                    return InterpretResult.ok;
                },
                @intFromEnum(OpCode.constant) => {
                    const constant = self.read_constant();
                    self.push(constant);
                },
                @intFromEnum(OpCode.add) => {
                    self.binary_op(BinaryOp.add);
                },
                @intFromEnum(OpCode.subtract) => {
                    self.binary_op(BinaryOp.subtract);
                },
                @intFromEnum(OpCode.multiply) => {
                    self.binary_op(BinaryOp.multiply);
                },
                @intFromEnum(OpCode.divide) => {
                    self.binary_op(BinaryOp.divide);
                },
                @intFromEnum(OpCode.negate) => {
                    self.stack[self.sp - 1] *= -1.0;
                },
                else => {},
            }
        }
    }

    pub fn free(_: *Self) void {}
};
