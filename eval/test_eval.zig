const std = @import("std");
const c = @cImport(
    @cInclude("api.h"),
);

const str = []const u8;

test "smoke memory" {
    const gpa = std.testing.allocator;
    _ = gpa;

    var cells: ?*c.allocator_t = undefined;
    _ = c.eval_cells_init(&cells, 10);
    defer {
        _ = c.eval_cells_free(&cells);
    }

    var idx: u64 = 0;

    _ = c.eval_cells_set(cells, idx, 0);
    idx += 1;
    _ = c.eval_cells_set(cells, idx, 1);
    idx += 1;
    _ = c.eval_cells_set(cells, idx, 2);
    idx += 1;
    _ = c.eval_cells_set(cells, idx, 3);
    idx += 1;
    _ = c.eval_cells_set_word(cells, idx - 1, 0xDEADBEEF);

    try std.testing.expect(c.eval_cells_get(cells, 0) == 0);
    try std.testing.expect(c.eval_cells_get(cells, 1) == 1);
    try std.testing.expect(c.eval_cells_get(cells, 2) == 2);
    try std.testing.expect(c.eval_cells_get(cells, 3) == 3);
    var word: i64 = 0;
    const err = c.eval_cells_get_word(cells, 3, &word);
    try std.testing.expect(err != -1);
    try std.testing.expect(word == 0xDEADBEEF);
}
