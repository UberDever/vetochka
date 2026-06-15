const std = @import("std");

const c = @cImport({
    @cInclude("bytecode_api.h");
    @cInclude("cells_api.h");
    @cInclude("source_api.h");
});

const CError = error{Unknown};

fn cTry(result: c.error_t) CError!void {
    if (result < 0) return error.Unknown;
}

const Located = struct {
    index: usize,
    node: c.struct_cells_node_t,
};

fn nodeAt(cells: ?*c.struct_cells_t, index: usize) !Located {
    const header = c.cells_get_node_header(cells, index);
    try std.testing.expect(header.type.value != c.CELLS_NODE_TYPE_INVALID);
    return .{ .index = index, .node = c.cells_get_node(cells, index, header) };
}

fn left(cells: ?*c.struct_cells_t, parent: Located) !Located {
    var index = parent.index;
    var node = c.struct_cells_node_t{};
    try cTry(c.cells_get_left_node(cells, &index, &node));
    return .{ .index = index, .node = node };
}

fn right(cells: ?*c.struct_cells_t, parent: Located) !Located {
    var index = parent.index;
    var node = c.struct_cells_node_t{};
    try cTry(c.cells_get_right_node(cells, &index, &node));
    return .{ .index = index, .node = node };
}

fn expectType(node: Located, expected: u8) !void {
    try std.testing.expectEqual(expected, node.node.header.type.value);
}

fn expectBytes(node: Located, expected: []const u8) !void {
    try expectType(node, c.CELLS_NODE_TYPE_VALUEV0);
    try std.testing.expectEqualSlices(
        u8,
        expected,
        node.node.as.nativev.data[0..node.node.as.nativev.len],
    );
}

test "documented source forms encode" {
    const cases = [_][]const u8{
        "",
        "^; 42; {bytes}; name",
        "[1, name]; (name); @[ann] name",
        "f(1, 2); f(); f[1, 2]; f{bytes}",
        "base.name; f name: 1; f do 1; 2 end",
        "!name; a + b + c",
    };

    for (cases) |text| {
        var source: ?*c.struct_source_tree_t = null;
        try cTry(c.source_ast_create(
            &source,
            .{ .data = text.ptr, .len = text.len },
            c.allocator_libc(),
        ));
        defer c.source_tree_free(&source);

        var cells: ?*c.struct_cells_t = null;
        try cTry(c.cells_create(&cells, 4096));
        defer c.cells_destroy(&cells);

        var root_index: usize = 0;
        try cTry(c.bytecode_source_encode(
            .{ .data = text.ptr, .len = text.len },
            source,
            cells,
            &root_index,
        ));
        try expectType(try nodeAt(cells, root_index), c.CELLS_NODE_TYPE_DELTA2);
    }
}

test "source encodes bootstrap syntax into cells" {
    const text = "{fn}(42)";

    var source: ?*c.struct_source_tree_t = null;
    try cTry(c.source_ast_create(
        &source,
        .{ .data = text.ptr, .len = text.len },
        c.allocator_libc(),
    ));
    defer c.source_tree_free(&source);

    var cells: ?*c.struct_cells_t = null;
    try cTry(c.cells_create(&cells, 1024));
    defer c.cells_destroy(&cells);

    var root_index: usize = 0;
    try cTry(c.bytecode_source_encode(
        .{ .data = text.ptr, .len = text.len },
        source,
        cells,
        &root_index,
    ));

    const block = try nodeAt(cells, root_index);
    try expectType(block, c.CELLS_NODE_TYPE_DELTA2);
    try expectBytes(try left(cells, block), ":block");

    const block_items = try right(cells, block);
    try expectType(block_items, c.CELLS_NODE_TYPE_DELTA2);
    const application = try left(cells, block_items);
    try expectType(application, c.CELLS_NODE_TYPE_APPLY);
    try expectType(try right(cells, block_items), c.CELLS_NODE_TYPE_DELTA0);

    const opcode = try left(cells, application);
    try expectType(opcode, c.CELLS_NODE_TYPE_OP_FN0);
    const argument = try right(cells, application);
    try expectType(argument, c.CELLS_NODE_TYPE_VALUEF0);
    try std.testing.expectEqual(@as(c.i64, 42), argument.node.as.nativef);
}
