const std = @import("std");
const chunk = @import("chunk.zig");
const common = @import("common.zig");
const debug = @import("debug.zig");

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    const allocator = gpa.allocator();
    defer {
        _ = gpa.deinit();
    }

    var c = chunk.Chunk().init(allocator);
    c.write(@intFromEnum(common.OpCode.op_return));

    debug.disassembleChunk(&c, "test chunk");

    defer c.free();
}
