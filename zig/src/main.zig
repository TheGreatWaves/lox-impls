const std = @import("std");
const chunk = @import("chunk.zig");
const OpCode = @import("common.zig").OpCode;
const debug = @import("debug.zig");

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    const allocator = gpa.allocator();
    defer {
        _ = gpa.deinit();
    }

    var c = chunk.Chunk.init(allocator);
    const constant_idx_1 = c.addConstant(1.2);
    const constant_idx_2 = c.addConstant(4.2);
    c.write_opcode(OpCode.constant, 123);
    c.write_u32(constant_idx_1, 123);
    c.write_opcode(OpCode.constant, 123);
    c.write_u32(constant_idx_2, 123);
    c.write_opcode(OpCode.@"return", 124);

    debug.disassembleChunk(&c, "test chunk");

    defer c.free();
}
