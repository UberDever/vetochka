// to run add LD_PRELOAD=/usr/lib/gcc/x86_64-linux-gnu/11/libasan.so

const std = @import("std");
const c = @cImport({
    @cInclude("cells_api.h");
    @cInclude("cells_debug.h");
});
const log = std.log.scoped(.reducer);

const str = []const u8;

pub const CError = error{Unknown};

inline fn cTry(rc: c.error_t) CError!void {
    if (rc >= 0) return;
    return switch (rc) {
        else => error.Unknown,
    };
}

// change to stderr to see things in tests
extern fn vfprintf(stream: *std.c.FILE, format: [*c]const u8, ap: std.builtin.VaList) c_int;
extern var stderr: *std.c.FILE;

fn debug_print(ctx: ?*anyopaque, fmt: [*c]const u8, ...) callconv(.c) void {
    _ = ctx;
    var args: std.builtin.VaList = @cVaStart();
    _ = vfprintf(stderr, fmt, args);
    @cVaEnd(&args);
}

test "smoke memory" {
    std.testing.log_level = .debug;

    const gpa = std.testing.allocator;
    _ = gpa;

    var cells: ?*c.struct_cells_t = null;
    try cTry(c.cells_create(&cells, 64));
    defer c.cells_destroy(&cells);

    const tree_node = c.cells_new_tree0();
    const lhs_ref = c.cells_new_ref4(12345);
    const rhs_n0f = c.cells_new_native0f(-3516);

    var tree_index: usize = 0;
    try cTry(c.cells_alloc_chunk(cells, tree_node.meta.size, &tree_index));
    try cTry(c.cells_write_node(cells, tree_index, tree_node));
    var lhs_index: usize = 0;
    try cTry(c.cells_alloc_chunk(cells, lhs_ref.meta.size, &lhs_index));
    try cTry(c.cells_write_node(cells, lhs_index, lhs_ref));
    var rhs_index: usize = 0;
    try cTry(c.cells_alloc_chunk(cells, rhs_n0f.meta.size, &rhs_index));
    try cTry(c.cells_write_node(cells, rhs_index, rhs_n0f));

    const tree_meta = c.cells_get_node_meta(cells, tree_index);
    try std.testing.expectEqual(c.CELLS_NODE_TYPE_TREE0, tree_meta.type);
    const lhs_meta = c.cells_get_node_meta(cells, lhs_index);
    try std.testing.expectEqual(c.CELLS_NODE_TYPE_REF4, lhs_meta.type);
    const rhs_meta = c.cells_get_node_meta(cells, rhs_index);
    try std.testing.expectEqual(c.CELLS_NODE_TYPE_NATIVE0F, rhs_meta.type);

    const tree_node_out = c.cells_get_node(cells, tree_index, tree_meta);
    try std.testing.expectEqual(c.CELLS_NODE_TYPE_TREE0, tree_node_out.meta.type);
    const lhs_node_out = c.cells_get_node(cells, lhs_index, lhs_meta);
    try std.testing.expectEqual(c.CELLS_NODE_TYPE_REF4, lhs_node_out.meta.type);
    const rhs_node_out = c.cells_get_node(cells, rhs_index, rhs_meta);
    try std.testing.expectEqual(c.CELLS_NODE_TYPE_NATIVE0F, rhs_node_out.meta.type);

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

test "debug view demo" {
    std.testing.log_level = .debug;

    var cells: ?*c.struct_cells_t = null;
    try cTry(c.cells_create(&cells, 128));
    defer c.cells_destroy(&cells);

    // Create a mix of nodes
    const n1 = c.cells_new_tree2();
    var idx1: usize = 0;
    try cTry(c.cells_alloc_chunk(cells, n1.meta.size, &idx1));
    try cTry(c.cells_write_node(cells, idx1, n1));
    try std.testing.expectEqual(c.CELLS_NODE_TYPE_TREE2, c.cells_get_node_meta(cells, idx1).type);

    var payload_data = [_]u8{ 0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE };
    const payload = c.span_byte_t{ .data = &payload_data, .len = payload_data.len };
    const n3 = c.cells_new_native0v(payload);
    var idx3: usize = 0;
    try cTry(c.cells_alloc_chunk(cells, n3.meta.size, &idx3));
    try cTry(c.cells_write_node(cells, idx3, n3));
    try std.testing.expectEqual(c.CELLS_NODE_TYPE_NATIVE0V, c.cells_get_node_meta(cells, idx3).type);

    const n2 = c.cells_new_ref4(1024);
    var idx2: usize = 0;
    try cTry(c.cells_alloc_chunk(cells, n2.meta.size, &idx2));
    try cTry(c.cells_write_node(cells, idx2, n2));
    try std.testing.expectEqual(c.CELLS_NODE_TYPE_REF4, c.cells_get_node_meta(cells, idx2).type);

    // std.debug.print("\n--- Debug View Demo ---\n", .{});
    // c.cells_print_debug_view(cells, debug_print, null);
}
