const std = @import("std");
const Allocator = std.mem.Allocator;

pub const OpCode = enum {
    constant,
    add,
    subtract,
    multiply,
    divide,
    negate,
    @"return",
};

// old_size commented out because it is not needed yet and the zig compiler will complain
pub fn reallocate(allocator: Allocator, comptime T: type, pointer: []T, _: u32, new_size: u32) ?[]T {
    if (new_size == 0) {
        allocator.free(pointer);
        return null;
    }

    const result = allocator.realloc(pointer, new_size);
    return result;
}

pub fn grow_capacity(old_capacity: u32) u32 {
    return if (old_capacity < 8) 8 else old_capacity * 2;
}

pub fn grow_array(comptime T: type, pointer: []T, old_count: u32, new_count: u32) []T {
    const type_size = @sizeOf(T);
    return reallocate(T, pointer, type_size * old_count, type_size * new_count);
}
