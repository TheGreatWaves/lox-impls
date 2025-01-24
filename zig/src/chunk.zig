const Allocator = @import("std").mem.Allocator;
const OpCode = @import("common.zig").OpCode;
const Value = @import("value.zig").Value;
const ValueArray = @import("value.zig").ValueArray;
const mem = @import("mem.zig");
const std = @import("std");

pub const Chunk = struct {
    count: u32,
    capacity: u32,
    code: []u8,

    line_count: u32,
    line_capacity: u32,
    lines: []u32,
    line: u32,
    constants: ValueArray,
    allocator: Allocator,

    const Self = @This();

    pub fn init(
        allocator: Allocator,
    ) Self {
        return Self{
            .count = 0,
            .capacity = 0,
            .code = &[_]u8{},
            .line_count = 0,
            .line_capacity = 0,
            .lines = &[_]u32{},
            .line = 0,
            .constants = ValueArray.init(allocator),
            .allocator = allocator,
        };
    }

    fn write(self: *Self, byte: u8, line: u32) void {
        if (self.capacity < self.count + 1) {
            const old_capacity = self.capacity;
            self.capacity = mem.grow_capacity(old_capacity);
            self.code = mem.grow_array(self.allocator, u8, self.code, old_capacity, self.capacity);
        }

        if (self.line != line) {
            if (self.line_capacity < self.count + 2) {
                const old_line_capacity = self.line_capacity;
                self.line_capacity = mem.grow_capacity(old_line_capacity);
                self.lines = mem.grow_array(self.allocator, u32, self.lines, old_line_capacity, self.line_capacity);
            }
            self.lines[self.line_count] = line;
            self.lines[self.line_count + 1] = 1;
            self.line_count += 2;
            self.line = line;
        } else {
            self.lines[self.line_count - 1] += 1;
        }

        self.code[self.count] = byte;
        self.count += 1;
    }

    pub fn write_u32(self: *Self, value: u32, line: u32) void {
        self.write(@truncate(value), line);
    }

    pub fn write_opcode(self: *Self, opcode: OpCode, line: u32) void {
        self.write(@intFromEnum(opcode), line);
    }

    pub fn addConstant(self: *Self, value: Value) u32 {
        self.constants.write(value);
        return self.constants.count - 1;
    }

    pub fn free(self: *Self) void {
        mem.free_array(self.allocator, u8, self.code, self.capacity);
        mem.free_array(self.allocator, u32, self.lines, self.line_capacity);
        self.constants.free();
    }
};
