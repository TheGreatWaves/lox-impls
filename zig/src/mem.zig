const std = @import("std");
const Allocator = std.mem.Allocator;

fn reallocate(allocator: Allocator, comptime T: type, pointer: []T, _: u32, new_size: u32) []T {
    if (new_size == 0) {
        allocator.free(pointer);
        return &[_]T{};
    }

    const result = allocator.realloc(pointer, new_size) catch {
        std.os.linux.exit(1);
    };
    return result;
}

pub fn grow_capacity(old_capacity: u32) u32 {
    return if (old_capacity < 8) 8 else old_capacity * 2;
}

pub fn grow_array(allocator: Allocator, comptime T: type, pointer: []T, old_count: u32, new_count: u32) []T {
    return reallocate(allocator, T, pointer, old_count, new_count);
}

pub fn free_array(allocator: Allocator, comptime T: type, pointer: []T, old_count: u32) void {
    _ = reallocate(allocator, T, pointer, @sizeOf(T) * old_count, 0);
}
