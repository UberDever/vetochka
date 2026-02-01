const std = @import("std");
const c = @cImport(
    @cInclude("cells_api.h"),
);
const log = std.log.scoped(.reducer);

const str = []const u8;

test "smoke memory" {
    std.testing.log_level = .info;
    const gpa = std.testing.allocator;
    _ = gpa;

    log.info("all good", .{});
}
