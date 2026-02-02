// to run add LD_PRELOAD=/usr/lib/gcc/x86_64-linux-gnu/11/libasan.so

const std = @import("std");
const c = @cImport(
    @cInclude("cells_api.h"),
);
const log = std.log.scoped(.reducer);

const str = []const u8;

pub const CError = error{Unknown};

inline fn cTry(rc: c_int) CError!void {
    if (rc >= 0) return;
    return switch (rc) {
        else => error.Unknown,
    };
}

test "smoke memory" {
    std.testing.log_level = .info;

    const gpa = std.testing.allocator;
    _ = gpa;

    var cells: ?*c.struct_cells_t = null;
    try cTry(c.cells_init(&cells, 64));
    defer c.cells_free(&cells);

    const tree_node = c.cells_new_tree0();
    const lhs_ref = c.cells_new_ref8(12345);
    const rhs_n0f = c.cells_new_native0f(-3516);

    var tree_index: usize = 0;
    try cTry(c.cells_alloc_node(cells, tree_node.meta.size, &tree_index));
    try cTry(c.cells_write_node(cells, tree_index, tree_node));
    var lhs_index: usize = 0;
    try cTry(c.cells_alloc_node(cells, lhs_ref.meta.size, &lhs_index));
    try cTry(c.cells_write_node(cells, lhs_index, lhs_ref));
    var rhs_index: usize = 0;
    try cTry(c.cells_alloc_node(cells, rhs_n0f.meta.size, &rhs_index));
    try cTry(c.cells_write_node(cells, rhs_index, rhs_n0f));

    const tree_meta = c.cells_get_node_meta(cells, tree_index);
    try std.testing.expectEqual(c.CELLS_NODE_TYPE_TREE0, tree_meta.type);
    const lhs_meta = c.cells_get_node_meta(cells, lhs_index);
    try std.testing.expectEqual(c.CELLS_NODE_TYPE_REF8, lhs_meta.type);
    const rhs_meta = c.cells_get_node_meta(cells, rhs_index);
    try std.testing.expectEqual(c.CELLS_NODE_TYPE_NATIVE0F, rhs_meta.type);

    const tree_node_out = c.cells_get_node(cells, tree_index, tree_meta);
    try std.testing.expectEqual(c.CELLS_NODE_TYPE_TREE0, tree_node_out.meta.type);
    const lhs_node_out = c.cells_get_node(cells, lhs_index, lhs_meta);
    try std.testing.expectEqual(c.CELLS_NODE_TYPE_REF8, lhs_node_out.meta.type);
    const rhs_node_out = c.cells_get_node(cells, rhs_index, rhs_meta);
    try std.testing.expectEqual(c.CELLS_NODE_TYPE_NATIVE0F, rhs_node_out.meta.type);

    log.info("da nodes {any}\n {any}\n {any}", .{ tree_node_out, lhs_node_out, rhs_node_out });

    try cTry(c.cells_node_free(cells, tree_index, tree_meta.size));
    try cTry(c.cells_node_free(cells, lhs_index, lhs_meta.size));
    try cTry(c.cells_node_free(cells, rhs_index, rhs_meta.size));

    const tree_meta1 = c.cells_get_node_meta(cells, tree_index);
    try std.testing.expectEqual(c.CELLS_NODE_TYPE_INVALID, tree_meta1.type);
    const lhs_meta1 = c.cells_get_node_meta(cells, lhs_index);
    try std.testing.expectEqual(c.CELLS_NODE_TYPE_INVALID, lhs_meta1.type);
    const rhs_meta1 = c.cells_get_node_meta(cells, rhs_index);
    try std.testing.expectEqual(c.CELLS_NODE_TYPE_INVALID, rhs_meta1.type);
}
