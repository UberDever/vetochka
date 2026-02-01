const std = @import("std");
const c = @cImport(
    @cInclude("cells_api.h"),
);

const str = []const u8;

test "smoke memory" {
    const gpa = std.testing.allocator;
    _ = gpa;
}
