const Allocator = @import("std").mem.Allocator;
const Chunk = @import("chunk.zig").Chunk;
const OpCode = @import("common.zig").OpCode;
const value = @import("value.zig");
const std = @import("std");
const debug = @import("debug.zig");

const InterpretResult = enum {
    ok,
    compile_error,
    runtime_error,
};

pub const VM = struct {
    chunk: ?*Chunk,
    ip: u8,
    allocator: Allocator,

    const Self = @This();

    pub fn init(allocator: Allocator) Self {
        return Self{
            .chunk = null,
            .allocator = allocator,
            .ip = 0,
        };
    }

    pub fn interpret(self: *Self, chunk: *Chunk) InterpretResult {
        self.chunk = chunk;
        self.ip = 0;
        return self.run();
    }

    fn read_byte(self: *Self) u8 {
        const byte = self.chunk.?.code[self.ip];
        self.ip += 1;
        return byte;
    }

    fn read_constant(self: *Self) value.Value {
        const constant_idx = self.read_byte();
        return self.chunk.?.constants.values[constant_idx];
    }

    fn run(self: *Self) InterpretResult {
        while (true) {
            if (debug.DEBUG) {
                _ = debug.disassembleInstruction(self.chunk.?, self.ip);
            }

            const instruction: u8 = self.read_byte();

            switch (instruction) {
                @intFromEnum(OpCode.@"return") => return InterpretResult.ok,
                @intFromEnum(OpCode.constant) => {
                    const constant = self.read_constant();
                    value.printValue(constant);
                    std.debug.print("\n", .{});
                },
                else => {},
            }
        }
    }

    pub fn free(_: *Self) void {}
};
