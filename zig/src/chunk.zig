const Allocator = @import("std").mem.Allocator;
const mem = @import("mem.zig");

pub fn Chunk() type {
    return struct {
        count: u32,
        capacity: u32,
        code: []u8,
        allocator: Allocator,

        const Self = @This();

        pub fn init(
            allocator: Allocator,
        ) Self {
            return Self{
                .count = 0,
                .capacity = 0,
                .code = &[_]u8{},
                .allocator = allocator,
            };
        }

        pub fn write(self: *Self, byte: u8) void {
            if (self.capacity < self.count + 1) {
                const old_capacity = self.capacity;
                self.capacity = mem.grow_capacity(old_capacity);
                self.code = mem.grow_array(self.allocator, u8, self.code, old_capacity, self.capacity);
            }

            self.code[self.count] = byte;
            self.count += 1;
        }

        pub fn free(self: *Self) void {
            mem.free_array(self.allocator, u8, self.code, self.capacity);
        }
    };
}
