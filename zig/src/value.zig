const Allocator = @import("std").mem.Allocator;
const mem = @import("mem.zig");
const std = @import("std");

pub const Value = f64;

pub const ValueArray = struct {
    count: u32,
    capacity: u32,
    values: []Value,
    allocator: Allocator,

    const Self = @This();

    pub fn init(
        allocator: Allocator,
    ) Self {
        return Self{
            .count = 0,
            .capacity = 0,
            .values = &[_]Value{},
            .allocator = allocator,
        };
    }

    pub fn write(self: *Self, value: Value) void {
        if (self.capacity < self.count + 1) {
            const old_capacity = self.capacity;
            self.capacity = mem.grow_capacity(old_capacity);
            self.values = mem.grow_array(self.allocator, Value, self.values, old_capacity, self.capacity);
        }

        self.values[self.count] = value;
        self.count += 1;
    }

    pub fn free(self: *Self) void {
        mem.free_array(self.allocator, Value, self.values, self.capacity);
    }
};

pub fn printValue(value: Value) void {
    std.debug.print("{d}", .{value});
}
