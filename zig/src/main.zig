const std = @import("std");
const chunk = @import("chunk.zig");
const vm = @import("vm.zig");
const OpCode = @import("common.zig").OpCode;
const debug = @import("debug.zig");

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    const allocator = gpa.allocator();
    defer {
        _ = gpa.deinit();
    }

    var v = vm.VM.init(allocator);
    defer v.free();

    var c = chunk.Chunk.init(allocator);
    defer c.free();

    const constant_idx_1 = c.addConstant(1.2);
    const constant_idx_2 = c.addConstant(4.2);

    c.write_opcode(OpCode.constant, 1);
    c.write_u32(constant_idx_1, 1);
    c.write_opcode(OpCode.constant, 2);
    c.write_u32(constant_idx_2, 2);
    c.write_opcode(OpCode.@"return", 2);
    c.write_opcode(OpCode.@"return", 3);
    c.write_opcode(OpCode.constant, 4);
    c.write_u32(constant_idx_2, 4);
    c.write_opcode(OpCode.@"return", 5);
    c.write_opcode(OpCode.@"return", 5);
    c.write_opcode(OpCode.@"return", 5);
    c.write_opcode(OpCode.@"return", 6);
    c.write_opcode(OpCode.@"return", 6);

    _ = v.interpret(&c);
}
