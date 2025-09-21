const std = @import("std");

const str = []const u8;

const TestDataTag = enum {
    json,
    file_testsuite,
};
const TestData = struct {
    name: str,
    as: ?union(TestDataTag) {
        json: str,
        file_testsuite: struct { name: str, test_fn: *const fn (std.mem.Allocator, TestData) bool },
    },
};
const TestFn = *const fn (std.mem.Allocator, TestData) bool;
const TestCase = struct {
    test_fn: TestFn,
    name: str,
    data: TestData,
};

fn testMemorySmoke(gpa: std.mem.Allocator, _: TestData) bool {
    _ = gpa;
    return true;
}

test "testmain" {
    const gpa = std.testing.allocator;

    var cases = std.ArrayList(TestCase).empty;
    defer cases.deinit(gpa);

    const name = "testMemorySmoke";
    const test_case: TestCase = .{ .test_fn = testMemorySmoke, .name = name, .data = .{
        .name = name,
        .as = null,
    } };
    try cases.append(gpa, test_case);

    var result = true;
    const GREEN = "\x1b[32m";
    const CYAN = "\x1b[36m";
    const RED = "\x1b[31m";
    const RESET = "\x1b[0m";
    for (cases.items) |testcase| {
        std.debug.print("{s}{s}{s}\n", .{ CYAN, testcase.name, RESET });
        if (testcase.test_fn(gpa, testcase.data)) {
            std.debug.print("{s}PASSED{s}\n\n", .{ GREEN, RESET });
        } else {
            std.debug.print("{s}FAILED{s}\n\n", .{ RED, RESET });
            result = false;
        }
    }
    if (!result) {
        std.debug.print("{s}You have failed tests :({s}\n\n", .{ RED, RESET });
    }

    try std.testing.expect(result);
}
