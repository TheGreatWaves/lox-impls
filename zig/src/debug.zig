const Chunk = @import("chunk.zig").Chunk;
const std = @import("std");
const OpCode = @import("common.zig").OpCode;
const value = @import("value.zig");

pub const DEBUG = true;

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

var prev_line: u32 = 0;

pub fn disassembleInstruction(chunk: *Chunk, offset: u32) u32 {
    std.debug.print("{d:0>4} ", .{offset});

    const line = chunk.getLine(offset);

    if (offset > 0 and line == prev_line) {
        std.debug.print("   | ", .{});
    } else {
        std.debug.print("{d: >4} ", .{line});
        prev_line = line;
    }

    const instruction = chunk.code[offset];

    switch (instruction) {
        @intFromEnum(OpCode.constant) => {
            return constantInstruction("OP_CONSTANT", chunk, offset);
        },
        @intFromEnum(OpCode.add) => {
            return simpleInstruction("OP_ADD", offset);
        },
        @intFromEnum(OpCode.subtract) => {
            return simpleInstruction("OP_SUBTRACT", offset);
        },
        @intFromEnum(OpCode.multiply) => {
            return simpleInstruction("OP_MULTIPLY", offset);
        },
        @intFromEnum(OpCode.divide) => {
            return simpleInstruction("OP_DIVIDE", offset);
        },
        @intFromEnum(OpCode.negate) => {
            return simpleInstruction("OP_NEGATE", offset);
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
