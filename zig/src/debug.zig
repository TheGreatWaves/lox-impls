const Chunk = @import("chunk.zig").Chunk();
const std = @import("std");
const common = @import("common.zig");

fn simpleInstruction(name: []const u8, offset: u32) u32 {
    std.debug.print("{s}\n", .{name});
    return offset + 1;
}

fn disassembleInstruction(chunk: *Chunk, offset: u32) u32 {
    std.debug.print("{d:0>4} ", .{offset});

    const instruction = chunk.code[offset];

    switch (instruction) {
        @intFromEnum(common.OpCode.op_return) => {
            return simpleInstruction("OP_RETURN", offset);
        },
        else => {
            std.debug.print("Unknown opcode {d}\n", .{instruction});
            return offset + 1;
        },
    }
}

pub fn disassembleChunk(chunk: *Chunk, name: []const u8) void {
    std.debug.print("== {s} ==\n", .{name});
    var offset: u32 = 0;

    while (offset < chunk.count) {
        offset = disassembleInstruction(chunk, offset);
    }
}
