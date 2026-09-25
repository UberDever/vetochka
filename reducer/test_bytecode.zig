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
    try std.testing.expectEqualSlices(u8, expected, node.node.as.nativev.data[0..node.node.as.nativev.len]);
}

fn expectInt(node: Located, expected: i64) !void {
    try expectType(node, c.CELLS_NODE_TYPE_VALUEF0);
    try std.testing.expectEqual(@as(c.i64, expected), node.node.as.nativef);
}

fn expectNil(node: Located) !void {
    try expectType(node, c.CELLS_NODE_TYPE_DELTA0);
}

/// Proper list `~[a, ~[b, ~[]]]`: return its items, checking the `~[]` terminator.
fn items(cells: ?*c.struct_cells_t, list: Located, out: []Located) ![]Located {
    var cursor = list;
    var count: usize = 0;
    while (cursor.node.header.type.value == c.CELLS_NODE_TYPE_DELTA2) : (count += 1) {
        out[count] = try left(cells, cursor);
        cursor = try right(cells, cursor);
    }
    try expectNil(cursor);
    return out[0..count];
}

/// Tagged node `[{tag}, meta, ...fields]` with empty meta: return the fields.
fn tagged(cells: ?*c.struct_cells_t, node: Located, tag: []const u8, out: []Located) ![]Located {
    const all = try items(cells, node, out);
    try std.testing.expect(all.len >= 2);
    try expectBytes(all[0], tag);
    try expectNil(all[1]);
    return all[2..];
}

const Encoded = struct {
    source: ?*c.struct_source_tree_t = null,
    cells: ?*c.struct_cells_t = null,
    root: Located = undefined,

    fn init(text: []const u8, options: c.struct_bytecode_source_options_t) !Encoded {
        var self = Encoded{};
        errdefer self.deinit();
        try cTry(c.source_ast_create(&self.source, .{ .data = text.ptr, .len = text.len }, c.allocator_libc()));
        try cTry(c.cells_create(&self.cells, 8192));
        var root_index: usize = 0;
        try cTry(c.bytecode_source_encode(.{ .data = text.ptr, .len = text.len }, self.source, options, self.cells, &root_index));
        self.root = try nodeAt(self.cells, root_index);
        return self;
    }

    fn deinit(self: *Encoded) void {
        c.cells_destroy(&self.cells);
        c.source_tree_free(&self.source);
    }

    /// The single statement of a one-statement source block.
    fn statement(self: *Encoded, buf: []Located) !Located {
        const body = try tagged(self.cells, self.root, ":block", buf);
        try std.testing.expectEqual(@as(usize, 1), body.len);
        return body[0];
    }
};

const no_options = c.struct_bytecode_source_options_t{ .source_meta = false };

fn encode(text: []const u8) !Encoded {
    return Encoded.init(text, no_options);
}

fn expectId(cells: ?*c.struct_cells_t, node: Located, name: []const u8) !void {
    var buf: [8]Located = undefined;
    const fields = try tagged(cells, node, ":id", &buf);
    try std.testing.expectEqual(@as(usize, 1), fields.len);
    try expectBytes(fields[0], name);
}

/// `[{@}, meta, f, x]`: return f and x.
fn application(cells: ?*c.struct_cells_t, node: Located) ![2]Located {
    var buf: [8]Located = undefined;
    const fields = try tagged(cells, node, "@", &buf);
    try std.testing.expectEqual(@as(usize, 2), fields.len);
    return .{ fields[0], fields[1] };
}

test "rule 1: identifiers" {
    var e = try encode("x");
    defer e.deinit();
    var buf: [8]Located = undefined;
    try expectId(e.cells, try e.statement(&buf), "x");
}

test "literals lower to themselves" {
    var e = try encode("[42, {text {nested}}]");
    defer e.deinit();
    var buf: [8]Located = undefined;
    var list_buf: [8]Located = undefined;
    const list = try items(e.cells, try e.statement(&buf), &list_buf);
    try std.testing.expectEqual(@as(usize, 2), list.len);
    try expectInt(list[0], 42);
    try expectBytes(list[1], "text {nested}");
}

test "rules 3 and 4: lists are plain nyad lists, parens are erased" {
    var e = try encode("[(x), 1]");
    defer e.deinit();
    var buf: [8]Located = undefined;
    var list_buf: [8]Located = undefined;
    const list = try items(e.cells, try e.statement(&buf), &list_buf);
    try std.testing.expectEqual(@as(usize, 2), list.len);
    try expectId(e.cells, list[0], "x");
    try expectInt(list[1], 1);
}

test "nyads" {
    var e = try encode("[~[], ~[1], ~[1, 2]]");
    defer e.deinit();
    var buf: [8]Located = undefined;
    var list_buf: [8]Located = undefined;
    const list = try items(e.cells, try e.statement(&buf), &list_buf);
    try expectNil(list[0]);
    try expectType(list[1], c.CELLS_NODE_TYPE_DELTA1);
    try expectInt(try left(e.cells, list[1]), 1);
    try expectType(list[2], c.CELLS_NODE_TYPE_DELTA2);
    try expectInt(try right(e.cells, list[2]), 2);
}

test "rule 5: applications are {@} data, curried" {
    var e = try encode("f(1, 2)");
    defer e.deinit();
    var buf: [8]Located = undefined;
    const outer = try application(e.cells, try e.statement(&buf));
    try expectInt(outer[1], 2);
    const inner = try application(e.cells, outer[0]);
    try expectId(e.cells, inner[0], "f");
    try expectInt(inner[1], 1);
}

test "f() is f(~[])" {
    var e = try encode("f()");
    defer e.deinit();
    var buf: [8]Located = undefined;
    const app = try application(e.cells, try e.statement(&buf));
    try expectNil(app[1]);
}

test "rules 9 and 10: f[...] and f{...}" {
    var e = try encode("f[1]{b}");
    defer e.deinit();
    var buf: [8]Located = undefined;
    const outer = try application(e.cells, try e.statement(&buf));
    try expectBytes(outer[1], "b");
    const inner = try application(e.cells, outer[0]);
    var list_buf: [8]Located = undefined;
    const list = try items(e.cells, inner[1], &list_buf);
    try expectInt(list[0], 1);
}

test "rule 6 and loose postfix: labels" {
    var e = try encode("f x: 1");
    defer e.deinit();
    var buf: [8]Located = undefined;
    const app = try application(e.cells, try e.statement(&buf));
    try expectId(e.cells, app[0], "f");
    var label_buf: [8]Located = undefined;
    const label = try tagged(e.cells, app[1], ":label", &label_buf);
    try expectBytes(label[0], "x");
    try expectInt(label[1], 1);
}

test "rule 7: blocks, as a loose postfix" {
    var e = try encode("f do 1; 2 end");
    defer e.deinit();
    var buf: [8]Located = undefined;
    const app = try application(e.cells, try e.statement(&buf));
    var block_buf: [8]Located = undefined;
    const block = try tagged(e.cells, app[1], ":block", &block_buf);
    try std.testing.expectEqual(@as(usize, 2), block.len);
    try expectInt(block[1], 2);
}

test "rule 8: annotations" {
    var e = try encode("@[a, b] x");
    defer e.deinit();
    var buf: [8]Located = undefined;
    var annot_buf: [8]Located = undefined;
    const annot = try tagged(e.cells, try e.statement(&buf), ":annot", &annot_buf);
    var list_buf: [8]Located = undefined;
    const list = try items(e.cells, annot[0], &list_buf);
    try std.testing.expectEqual(@as(usize, 2), list.len);
    try expectId(e.cells, annot[1], "x");
}

test "rule 11: prefix operators" {
    var e = try encode("-x");
    defer e.deinit();
    var buf: [8]Located = undefined;
    var prefix_buf: [8]Located = undefined;
    const prefix = try tagged(e.cells, try e.statement(&buf), ":prefix", &prefix_buf);
    try expectBytes(prefix[0], "-");
    try expectId(e.cells, prefix[1], "x");
}

test "rule 12: mixed infix chains are flat, operators in a list" {
    var e = try encode("a + b * c");
    defer e.deinit();
    var buf: [8]Located = undefined;
    var infix_buf: [8]Located = undefined;
    const infix = try tagged(e.cells, try e.statement(&buf), ":infix", &infix_buf);
    try std.testing.expectEqual(@as(usize, 4), infix.len);
    var ops_buf: [8]Located = undefined;
    const ops = try items(e.cells, infix[0], &ops_buf);
    try expectBytes(ops[0], "+");
    try expectBytes(ops[1], "*");
    try expectId(e.cells, infix[3], "c");
}

test "rule 12: parens keep grouping as nesting" {
    var e = try encode("(a + b) * c");
    defer e.deinit();
    var buf: [8]Located = undefined;
    var infix_buf: [8]Located = undefined;
    const infix = try tagged(e.cells, try e.statement(&buf), ":infix", &infix_buf);
    try std.testing.expectEqual(@as(usize, 3), infix.len);
    var inner_buf: [8]Located = undefined;
    const inner = try tagged(e.cells, infix[1], ":infix", &inner_buf);
    try std.testing.expectEqual(@as(usize, 3), inner.len);
}

test "rule 13: selectors" {
    var e = try encode("base.name");
    defer e.deinit();
    var buf: [8]Located = undefined;
    var sel_buf: [8]Located = undefined;
    const sel = try tagged(e.cells, try e.statement(&buf), ":selector", &sel_buf);
    try expectId(e.cells, sel[0], "base");
    try expectBytes(sel[1], "name");
}

test "rule 2: $ heads an opcode as plain application" {
    var e = try encode("$fn: [x]");
    defer e.deinit();
    var buf: [8]Located = undefined;
    const app = try application(e.cells, try e.statement(&buf));
    try expectId(e.cells, app[0], "$");
    var label_buf: [8]Located = undefined;
    const label = try tagged(e.cells, app[1], ":label", &label_buf);
    try expectBytes(label[0], "fn");
}

test "source positions in meta when asked" {
    var e = try Encoded.init("\n  x", .{ .source_meta = true });
    defer e.deinit();
    var block_buf: [8]Located = undefined;
    const block = try items(e.cells, e.root, &block_buf);
    const statement = block[2];
    var id_buf: [8]Located = undefined;
    const id = try items(e.cells, statement, &id_buf);
    try expectBytes(id[0], ":id");
    var meta_buf: [8]Located = undefined;
    const meta = try items(e.cells, id[1], &meta_buf);
    try std.testing.expectEqual(@as(usize, 2), meta.len);
    var line_buf: [8]Located = undefined;
    const line = try tagged(e.cells, meta[0], ":label", &line_buf);
    try expectBytes(line[0], "line");
    try expectInt(line[1], 2);
    var col_buf: [8]Located = undefined;
    const col = try tagged(e.cells, meta[1], ":label", &col_buf);
    try expectInt(col[1], 3);
}
