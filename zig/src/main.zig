const std = @import("std");
const chunk = @import("chunk.zig");
const vm = @import("vm.zig");
const OpCode = @import("common.zig").OpCode;
const debug = @import("debug.zig");
const Allocator = std.mem.Allocator;

fn interpret() !void {}

fn repl() !void {
    var buffer: [1024]u8 = undefined;
    const reader = std.io.getStdIn().reader();
    const writer = std.io.getStdOut().writer();

    while (true) {
        try writer.print("> ", .{});
        _ = try reader.readUntilDelimiter(&buffer, '\n');
    }
}

fn runFile(path: [*:0]const u8) void {
    _ = path;
}

fn readFile(allocator: Allocator, path: [*:0]const u8) ![]u8 {
    return try std.fs.cwd().readFileAlloc(allocator, path, 16384);
}

pub fn main() !void {
    // var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    // const allocator = gpa.allocator();
    // defer gpa.deinit();

    var v = vm.VM.init();
    defer v.free();

    const argv = std.os.argv;
    const argc = argv.len;

    if (argc == 1) {
        try repl();
    } else if (argc == 2) {
        runFile(argv[1][0.. :0]);
    } else {
        std.debug.print("Usage: clox [path]\n", .{});
        std.process.exit(64);
    }
}
