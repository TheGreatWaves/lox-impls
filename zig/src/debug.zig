const Chunk = @import("chunk.zig").Chunk;
const std = @import("std");
const OpCode = @import("common.zig").OpCode;
const value = @import("value.zig");

fn simpleInstruction(name: []const u8, offset: u32) u32 {
    std.debug.print("{s}\n", .{name});
    return offset + 1;
}

fn constantInstruction(name: []const u8, chunk: *Chunk, offset: u32) u32 {
    const constant = chunk.code[offset + 1];
    std.debug.print("{s: <16} {d: >4} '", .{ name, constant });
    value.printValue(chunk.constants.values[constant]);
    std.debug.print("'\n", .{});
    return offset + 2;
}

fn disassembleInstruction(chunk: *Chunk, offset: u32) u32 {
    std.debug.print("{d:0>4} ", .{offset});

    const instruction = chunk.code[offset];

    switch (instruction) {
        @intFromEnum(OpCode.constant) => {
            return constantInstruction("OP_CONSTANT", chunk, offset);
        },
        @intFromEnum(OpCode.@"return") => {
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
