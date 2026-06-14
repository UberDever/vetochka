// to run add LD_PRELOAD=/usr/lib/gcc/x86_64-linux-gnu/11/libasan.so

const std = @import("std");
const c = @cImport({
    @cInclude("cells_api.h");
    @cInclude("reducer_api.h");
    @cInclude("bytecode_api.h");
    @cInclude("stdio.h");
});
const log = std.log.scoped(.reducer);

const str = []const u8;

pub const CError = error{Unknown};

inline fn cTry(rc: c.error_t) CError!void {
    if (rc >= 0) return;
    return switch (rc) {
        else => ret: {
            std.debug.print("cTry error: {}\n", .{rc});
            break :ret error.Unknown;
        },
    };
}

fn debug_print(ctx: ?*anyopaque, fmt: [*c]const u8, ...) callconv(.c) void {
    _ = ctx;
    var args: std.builtin.VaList = @cVaStart();
    _ = c.vfprintf(c.stderr, fmt, @ptrCast(&args));
    @cVaEnd(&args);
}

fn append_node(cells: ?*c.struct_cells_t, cursor: *usize, node: c.struct_cells_node_t) !void {
    var index: usize = 0;
    try cTry(c.cells_alloc_chunk(cells, node.header.encoded_size, &index));
    try std.testing.expectEqual(cursor.*, index);
    try cTry(c.cells_write_node(cells, index, node));
    cursor.* += node.header.encoded_size;
}

test "node info projections" {
    var raw: u16 = 0;
    while (raw <= std.math.maxInt(u8)) : (raw += 1) {
        const value: u8 = @intCast(raw);
        if (!c.cells_node_type_t_is_valid_raw(value) or value == c.CELLS_NODE_TYPE_INVALID) continue;
        try std.testing.expect(c.cells_node_type_is_encodable(.{ .value = value }));
    }

    const value1_type = c.cells_node_type_t{ .value = c.CELLS_NODE_TYPE_VALUEV1 };
    try std.testing.expectEqual(@as(i8, 1), c.cells_node_type_get_arity(value1_type));

    const value2_type = c.cells_node_type_with_arity(value1_type, 2);
    try std.testing.expectEqual(c.CELLS_NODE_TYPE_VALUEV2, value2_type.value);

    const op_fn_type = c.cells_node_type_t{ .value = c.CELLS_NODE_TYPE_OP_FN };
    try std.testing.expectEqual(@as(i8, 1), c.cells_node_type_get_arity(op_fn_type));
    try std.testing.expectEqual(
        c.CELLS_NODE_TYPE_INVALID,
        c.cells_node_type_with_arity(op_fn_type, 0).value,
    );
    try std.testing.expectEqual(
        c.CELLS_NODE_TYPE_INVALID,
        c.cells_node_type_with_arity(op_fn_type, 2).value,
    );

    const delta = c.cells_new_node(.{ .value = c.CELLS_NODE_TYPE_DELTA0 });
    try std.testing.expectEqual(c.CELLS_NODE_TYPE_DELTA0, delta.header.type.value);
    const invalid = c.cells_new_node(value1_type);
    try std.testing.expectEqual(c.CELLS_NODE_TYPE_INVALID, invalid.header.type.value);

    var payload_data = [_]u8{ 0xCA, 0xFE };
    var value = c.cells_new_value0v(.{ .data = &payload_data, .len = payload_data.len });
    const encoded_size = value.header.encoded_size;
    try std.testing.expect(c.cells_node_set_arity(&value, 2));
    try std.testing.expectEqual(c.CELLS_NODE_TYPE_VALUEV2, value.header.type.value);
    try std.testing.expectEqual(encoded_size, value.header.encoded_size);

    var reference = c.cells_new_ref(0);
    try std.testing.expect(!c.cells_node_set_arity(&reference, 0));
}

test "stable node byte encoding" {
    var cells: ?*c.struct_cells_t = null;
    try cTry(c.cells_create(&cells, 64));
    defer c.cells_destroy(&cells);

    var cursor: usize = 0;
    try append_node(cells, &cursor, c.cells_new_delta0());
    try append_node(cells, &cursor, c.cells_new_delta1());
    try append_node(cells, &cursor, c.cells_new_delta2());
    try append_node(cells, &cursor, c.cells_new_value0f(0x0102030405060708));
    try append_node(cells, &cursor, c.cells_new_value1f(0));
    try append_node(cells, &cursor, c.cells_new_value2f(-1));

    var empty_payload = [_]u8{};
    var payload_data = [_]u8{ 0xAA, 0xBB };
    try append_node(
        cells,
        &cursor,
        c.cells_new_value0v(.{ .data = &empty_payload, .len = empty_payload.len }),
    );
    try append_node(
        cells,
        &cursor,
        c.cells_new_value1v(.{ .data = &empty_payload, .len = empty_payload.len }),
    );
    try append_node(
        cells,
        &cursor,
        c.cells_new_value2v(.{ .data = &payload_data, .len = payload_data.len }),
    );

    const short_ref_index = cursor;
    try append_node(cells, &cursor, c.cells_new_ref(0x0123));
    const long_ref_index = cursor;
    try append_node(cells, &cursor, c.cells_new_ref(0x2000));

    const expected = [_]u8{
        0x80, 0x81, 0x82,
        0x83, 0x08, 0x07,
        0x06, 0x05, 0x04,
        0x03, 0x02, 0x01,
        0x84, 0x00, 0x00,
        0x00, 0x00, 0x00,
        0x00, 0x00, 0x00,
        0x85, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF,
        0x86, 0x00, 0x87,
        0x00, 0x88, 0x02,
        0xAA, 0xBB, 0x01,
        0x23, 0x40, 0x00,
        0x00, 0x00, 0x00,
        0x00, 0x20, 0x00,
    };
    const span = c.cells_get_span(cells);
    try std.testing.expectEqualSlices(u8, &expected, span.data[0..cursor]);

    const short_header = c.cells_get_node_header(cells, short_ref_index);
    try std.testing.expectEqual(c.CELLS_NODE_TYPE_REF, short_header.type.value);
    try std.testing.expectEqual(@as(usize, 2), short_header.encoded_size);
    const short_ref = c.cells_get_node(cells, short_ref_index, short_header);
    try std.testing.expectEqual(@as(i64, 0x0123), short_ref.as.ref);
    const long_header = c.cells_get_node_header(cells, long_ref_index);
    try std.testing.expectEqual(c.CELLS_NODE_TYPE_REF, long_header.type.value);
    try std.testing.expectEqual(@as(usize, 8), long_header.encoded_size);
    const long_ref = c.cells_get_node(cells, long_ref_index, long_header);
    try std.testing.expectEqual(@as(i64, 0x2000), long_ref.as.ref);

    const invalid_ref = c.cells_new_ref(std.math.maxInt(i64));
    try std.testing.expectEqual(c.CELLS_NODE_TYPE_INVALID, invalid_ref.header.type.value);
}

test "smoke memory" {
    std.testing.log_level = .debug;

    const gpa = std.testing.allocator;
    _ = gpa;

    var cells: ?*c.struct_cells_t = null;
    try cTry(c.cells_create(&cells, 64));
    defer c.cells_destroy(&cells);

    const tree_node = c.cells_new_delta0();
    const lhs_ref = c.cells_new_ref(8191);
    const rhs_n0f = c.cells_new_value0f(-3516);

    var tree_index: usize = 0;
    try cTry(c.cells_alloc_chunk(cells, tree_node.header.encoded_size, &tree_index));
    try cTry(c.cells_write_node(cells, tree_index, tree_node));
    var lhs_index: usize = 0;
    try cTry(c.cells_alloc_chunk(cells, lhs_ref.header.encoded_size, &lhs_index));
    try cTry(c.cells_write_node(cells, lhs_index, lhs_ref));
    var rhs_index: usize = 0;
    try cTry(c.cells_alloc_chunk(cells, rhs_n0f.header.encoded_size, &rhs_index));
    try cTry(c.cells_write_node(cells, rhs_index, rhs_n0f));

    const tree_header = c.cells_get_node_header(cells, tree_index);
    try std.testing.expectEqual(c.CELLS_NODE_TYPE_DELTA0, tree_header.type.value);
    const lhs_header = c.cells_get_node_header(cells, lhs_index);
    try std.testing.expectEqual(c.CELLS_NODE_TYPE_REF, lhs_header.type.value);
    const rhs_header = c.cells_get_node_header(cells, rhs_index);
    try std.testing.expectEqual(c.CELLS_NODE_TYPE_VALUEF0, rhs_header.type.value);

    const tree_node_out = c.cells_get_node(cells, tree_index, tree_header);
    try std.testing.expectEqual(c.CELLS_NODE_TYPE_DELTA0, tree_node_out.header.type.value);
    const lhs_node_out = c.cells_get_node(cells, lhs_index, lhs_header);
    try std.testing.expectEqual(c.CELLS_NODE_TYPE_REF, lhs_node_out.header.type.value);
    const rhs_node_out = c.cells_get_node(cells, rhs_index, rhs_header);
    try std.testing.expectEqual(c.CELLS_NODE_TYPE_VALUEF0, rhs_node_out.header.type.value);

    try cTry(c.cells_node_free(cells, tree_index, tree_header.encoded_size));
    try cTry(c.cells_node_free(cells, lhs_index, lhs_header.encoded_size));
    try cTry(c.cells_node_free(cells, rhs_index, rhs_header.encoded_size));

    const tree_header1 = c.cells_get_node_header(cells, tree_index);
    try std.testing.expectEqual(c.CELLS_NODE_TYPE_INVALID, tree_header1.type.value);
    const lhs_header1 = c.cells_get_node_header(cells, lhs_index);
    try std.testing.expectEqual(c.CELLS_NODE_TYPE_INVALID, lhs_header1.type.value);
    const rhs_header1 = c.cells_get_node_header(cells, rhs_index);
    try std.testing.expectEqual(c.CELLS_NODE_TYPE_INVALID, rhs_header1.type.value);
}

test "debug view demo" {
    std.testing.log_level = .debug;

    var cells: ?*c.struct_cells_t = null;
    try cTry(c.cells_create(&cells, 128));
    defer c.cells_destroy(&cells);

    // Create a mix of nodes
    const n1 = c.cells_new_delta2();
    var idx1: usize = 0;
    try cTry(c.cells_alloc_chunk(cells, n1.header.encoded_size, &idx1));
    try cTry(c.cells_write_node(cells, idx1, n1));
    try std.testing.expectEqual(c.CELLS_NODE_TYPE_DELTA2, c.cells_get_node_header(cells, idx1).type.value);

    var payload_data = [_]u8{ 0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE };
    const payload = c.span_byte_t{ .data = &payload_data, .len = payload_data.len };
    const n3 = c.cells_new_value0v(payload);
    var idx3: usize = 0;
    try cTry(c.cells_alloc_chunk(cells, n3.header.encoded_size, &idx3));
    try cTry(c.cells_write_node(cells, idx3, n3));
    try std.testing.expectEqual(c.CELLS_NODE_TYPE_VALUEV0, c.cells_get_node_header(cells, idx3).type.value);

    const n2 = c.cells_new_ref(1024);
    var idx2: usize = 0;
    try cTry(c.cells_alloc_chunk(cells, n2.header.encoded_size, &idx2));
    try cTry(c.cells_write_node(cells, idx2, n2));
    try std.testing.expectEqual(c.CELLS_NODE_TYPE_REF, c.cells_get_node_header(cells, idx2).type.value);

    // std.debug.print("\n--- Debug View Demo ---\n", .{});
    // c.cells_print_debug_view(cells, debug_print, null);
}

test "eval smoke" {
    std.testing.log_level = .debug;

    var cells: ?*c.struct_cells_t = null;
    try cTry(c.cells_create(&cells, 256));
    defer c.cells_destroy(&cells);

    var reducer: ?*c.struct_reducer_t = null;
    try cTry(c.reducer_create(&reducer, cells));
    defer c.reducer_free(&reducer);

    defer {
        const err = c.reducer_get_error(reducer);
        if (err != null) {
            std.debug.print("{s}", .{err});
        }
    }

    {
        // rule 0.a
        c.reducer_push_to_stack(reducer, c.REDUCER_APPLY_TOKEN);

        const delta0 = c.cells_new_delta0();
        var index_out: usize = 0;
        try cTry(c.cells_alloc_chunk(cells, delta0.header.encoded_size, &index_out));
        try cTry(c.cells_write_node(cells, index_out, delta0));
        c.reducer_push_to_stack(reducer, index_out);

        try cTry(c.cells_alloc_chunk(cells, delta0.header.encoded_size, &index_out));
        try cTry(c.cells_write_node(cells, index_out, delta0));
        c.reducer_push_to_stack(reducer, index_out);

        while (true) {
            const res = c.reducer_step(reducer);
            try cTry(res);
            if (res == c.REDUCER_DONE) break;
        }
        try std.testing.expectEqual(true, c.reducer_has_result(reducer));

        const result = c.reducer_get_result(reducer);
        var root_node = c.cells_node_t{};
        var result1 = result;
        try cTry(c.cells_dereference_node(cells, &result1, &root_node));
        try std.testing.expectEqual(c.CELLS_NODE_TYPE_DELTA1, root_node.header.type.value);
        var left_node = c.cells_node_t{};
        var result2 = result;
        try cTry(c.cells_get_left_node(cells, &result2, &left_node));
        try std.testing.expectEqual(c.CELLS_NODE_TYPE_DELTA0, left_node.header.type.value);
    }
    {
        // rule 0.b
        c.reducer_push_to_stack(reducer, c.REDUCER_APPLY_TOKEN);

        const delta1 = c.cells_new_delta1();
        const delta0 = c.cells_new_delta0();
        var index_out: usize = 0;
        try cTry(c.cells_alloc_chunk(cells, delta1.header.encoded_size + delta0.header.encoded_size, &index_out));
        try cTry(c.cells_write_node(cells, index_out, delta1));
        try cTry(c.cells_write_node(cells, index_out + delta1.header.encoded_size, delta0));
        c.reducer_push_to_stack(reducer, index_out);

        try cTry(c.cells_alloc_chunk(cells, delta0.header.encoded_size, &index_out));
        try cTry(c.cells_write_node(cells, index_out, delta0));
        c.reducer_push_to_stack(reducer, index_out);

        while (true) {
            const res = c.reducer_step(reducer);
            try cTry(res);
            if (res == c.REDUCER_DONE) break;
        }
        try std.testing.expectEqual(true, c.reducer_has_result(reducer));

        const result = c.reducer_get_result(reducer);
        var root_node = c.cells_node_t{};
        var result1 = result;
        try cTry(c.cells_dereference_node(cells, &result1, &root_node));
        try std.testing.expectEqual(c.CELLS_NODE_TYPE_DELTA2, root_node.header.type.value);
        var left_node = c.cells_node_t{};
        var result2 = result;
        try cTry(c.cells_get_left_node(cells, &result2, &left_node));
        try std.testing.expectEqual(c.CELLS_NODE_TYPE_DELTA0, left_node.header.type.value);
        var right_node = c.cells_node_t{};
        var result3 = result;
        try cTry(c.cells_get_right_node(cells, &result3, &right_node));
        try std.testing.expectEqual(c.CELLS_NODE_TYPE_DELTA0, right_node.header.type.value);
    }
    {
        // rule 1
        var b: ?*c.bytecode_tree_builder_t = null;
        try cTry(c.bytecode_tree_builder_create(&b));
        defer c.bytecode_tree_builder_destroy(&b);
        _ = c.bytecode_new_node2(
            b,
            c.cells_new_delta2(),
            c.bytecode_new_node0(b, c.cells_new_delta0()),
            c.bytecode_new_node2(
                b,
                c.cells_new_delta2(),
                c.bytecode_new_node0(b, c.cells_new_delta0()),
                c.bytecode_new_node0(b, c.cells_new_delta0()),
            ),
        );
        var root_redex: usize = undefined;
        try cTry(c.bytecode_tree_builder_build(b, cells, &root_redex));

        c.bytecode_tree_builder_reset(b);
        _ = c.bytecode_new_node2(
            b,
            c.cells_new_delta2(),
            c.bytecode_new_node0(b, c.cells_new_delta0()),
            c.bytecode_new_node0(b, c.cells_new_delta0()),
        );
        var root_arg: usize = undefined;
        try cTry(c.bytecode_tree_builder_build(b, cells, &root_arg));

        c.reducer_push_to_stack(reducer, c.REDUCER_APPLY_TOKEN);
        c.reducer_push_to_stack(reducer, root_redex);
        c.reducer_push_to_stack(reducer, root_arg);

        while (true) {
            const res = c.reducer_step(reducer);
            try cTry(res);
            if (res == c.REDUCER_DONE) break;
        }
        try std.testing.expectEqual(true, c.reducer_has_result(reducer));

        const result = c.reducer_get_result(reducer);
        var root_node = c.cells_node_t{};
        var result1 = result;
        try cTry(c.cells_dereference_node(cells, &result1, &root_node));
        try std.testing.expectEqual(c.CELLS_NODE_TYPE_DELTA2, root_node.header.type.value);
        var left_node = c.cells_node_t{};
        var result2 = result;
        try cTry(c.cells_get_left_node(cells, &result2, &left_node));
        try std.testing.expectEqual(c.CELLS_NODE_TYPE_DELTA0, left_node.header.type.value);
        var right_node = c.cells_node_t{};
        var result3 = result;
        try cTry(c.cells_get_right_node(cells, &result3, &right_node));
        try std.testing.expectEqual(c.CELLS_NODE_TYPE_DELTA0, right_node.header.type.value);
    }
    {
        // rule 2
        var b: ?*c.bytecode_tree_builder_t = null;
        try cTry(c.bytecode_tree_builder_create(&b));
        defer c.bytecode_tree_builder_destroy(&b);
        _ = c.bytecode_new_node2(
            b,
            c.cells_new_delta2(),
            c.bytecode_new_node1(
                b,
                c.cells_new_delta1(),
                c.bytecode_new_node0(b, c.cells_new_delta0()),
            ),
            c.bytecode_new_node0(b, c.cells_new_delta0()),
        );
        var root_redex: usize = undefined;
        try cTry(c.bytecode_tree_builder_build(b, cells, &root_redex));

        c.bytecode_tree_builder_reset(b);
        _ = c.bytecode_new_node0(b, c.cells_new_delta0());
        var root_arg: usize = undefined;
        try cTry(c.bytecode_tree_builder_build(b, cells, &root_arg));

        c.reducer_push_to_stack(reducer, c.REDUCER_APPLY_TOKEN);
        c.reducer_push_to_stack(reducer, root_redex);
        c.reducer_push_to_stack(reducer, root_arg);
        while (true) {
            const res = c.reducer_step(reducer);
            try cTry(res);
            if (res == c.REDUCER_DONE) break;
        }
        try std.testing.expectEqual(true, c.reducer_has_result(reducer));

        const result = c.reducer_get_result(reducer);
        var root_node = c.cells_node_t{};
        var result1 = result;
        try cTry(c.cells_dereference_node(cells, &result1, &root_node));
        try std.testing.expectEqual(c.CELLS_NODE_TYPE_DELTA2, root_node.header.type.value);
        var left_node = c.cells_node_t{};
        var result2 = result;
        try cTry(c.cells_get_left_node(cells, &result2, &left_node));
        try std.testing.expectEqual(c.CELLS_NODE_TYPE_DELTA0, left_node.header.type.value);
        var right_node = c.cells_node_t{};
        var result3 = result;
        try cTry(c.cells_get_right_node(cells, &result3, &right_node));
        try std.testing.expectEqual(c.CELLS_NODE_TYPE_DELTA1, right_node.header.type.value);
        var right_left_node = c.cells_node_t{};
        var result4 = result3;
        try cTry(c.cells_get_left_node(cells, &result4, &right_left_node));
        try std.testing.expectEqual(c.CELLS_NODE_TYPE_DELTA0, right_left_node.header.type.value);
    }
    {
        // rule 3a
        var b: ?*c.bytecode_tree_builder_t = null;
        try cTry(c.bytecode_tree_builder_create(&b));
        defer c.bytecode_tree_builder_destroy(&b);
        _ = c.bytecode_new_node2(
            b,
            c.cells_new_delta2(),
            c.bytecode_new_node2(
                b,
                c.cells_new_delta2(),
                c.bytecode_new_node0(b, c.cells_new_delta0()),
                c.bytecode_new_node0(b, c.cells_new_delta0()),
            ),
            c.bytecode_new_node0(b, c.cells_new_delta0()),
        );
        var root_redex: usize = undefined;
        try cTry(c.bytecode_tree_builder_build(b, cells, &root_redex));

        c.bytecode_tree_builder_reset(b);
        _ = c.bytecode_new_node0(b, c.cells_new_delta0());
        var root_arg: usize = undefined;
        try cTry(c.bytecode_tree_builder_build(b, cells, &root_arg));

        c.reducer_push_to_stack(reducer, c.REDUCER_APPLY_TOKEN);
        c.reducer_push_to_stack(reducer, root_redex);
        c.reducer_push_to_stack(reducer, root_arg);
        while (true) {
            const res = c.reducer_step(reducer);
            try cTry(res);
            if (res == c.REDUCER_DONE) break;
        }
        try std.testing.expectEqual(true, c.reducer_has_result(reducer));

        const result = c.reducer_get_result(reducer);
        var root_node = c.cells_node_t{};
        var result1 = result;
        try cTry(c.cells_dereference_node(cells, &result1, &root_node));
        try std.testing.expectEqual(c.CELLS_NODE_TYPE_DELTA0, root_node.header.type.value);
    }
    {
        // rule 3b
        var b: ?*c.bytecode_tree_builder_t = null;
        try cTry(c.bytecode_tree_builder_create(&b));
        defer c.bytecode_tree_builder_destroy(&b);
        _ = c.bytecode_new_node2(
            b,
            c.cells_new_delta2(),
            c.bytecode_new_node2(
                b,
                c.cells_new_delta2(),
                c.bytecode_new_node0(b, c.cells_new_delta0()),
                c.bytecode_new_node0(b, c.cells_new_delta0()),
            ),
            c.bytecode_new_node0(b, c.cells_new_delta0()),
        );
        var root_redex: usize = undefined;
        try cTry(c.bytecode_tree_builder_build(b, cells, &root_redex));

        c.bytecode_tree_builder_reset(b);
        _ = c.bytecode_new_node1(
            b,
            c.cells_new_delta1(),
            c.bytecode_new_node0(b, c.cells_new_delta0()),
        );
        var root_arg: usize = undefined;
        try cTry(c.bytecode_tree_builder_build(b, cells, &root_arg));

        c.reducer_push_to_stack(reducer, c.REDUCER_APPLY_TOKEN);
        c.reducer_push_to_stack(reducer, root_redex);
        c.reducer_push_to_stack(reducer, root_arg);
        while (true) {
            const res = c.reducer_step(reducer);
            try cTry(res);
            if (res == c.REDUCER_DONE) break;
        }
        try std.testing.expectEqual(true, c.reducer_has_result(reducer));

        const result = c.reducer_get_result(reducer);
        var root_node = c.cells_node_t{};
        var result1 = result;
        try cTry(c.cells_dereference_node(cells, &result1, &root_node));
        try std.testing.expectEqual(c.CELLS_NODE_TYPE_DELTA1, root_node.header.type.value);
        var left_node = c.cells_node_t{};
        var result2 = result;
        try cTry(c.cells_get_left_node(cells, &result2, &left_node));
        try std.testing.expectEqual(c.CELLS_NODE_TYPE_DELTA0, left_node.header.type.value);
    }
    {
        // rule 3c
        var b: ?*c.bytecode_tree_builder_t = null;
        try cTry(c.bytecode_tree_builder_create(&b));
        defer c.bytecode_tree_builder_destroy(&b);
        _ = c.bytecode_new_node2(
            b,
            c.cells_new_delta2(),
            c.bytecode_new_node2(
                b,
                c.cells_new_delta2(),
                c.bytecode_new_node0(b, c.cells_new_delta0()),
                c.bytecode_new_node0(b, c.cells_new_delta0()),
            ),
            c.bytecode_new_node0(b, c.cells_new_delta0()),
        );
        var root_redex: usize = undefined;
        try cTry(c.bytecode_tree_builder_build(b, cells, &root_redex));

        c.bytecode_tree_builder_reset(b);
        _ = c.bytecode_new_node2(
            b,
            c.cells_new_delta2(),
            c.bytecode_new_node0(b, c.cells_new_delta0()),
            c.bytecode_new_node0(b, c.cells_new_delta0()),
        );
        var root_arg: usize = undefined;
        try cTry(c.bytecode_tree_builder_build(b, cells, &root_arg));

        c.reducer_push_to_stack(reducer, c.REDUCER_APPLY_TOKEN);
        c.reducer_push_to_stack(reducer, root_redex);
        c.reducer_push_to_stack(reducer, root_arg);
        while (true) {
            const res = c.reducer_step(reducer);
            try cTry(res);
            if (res == c.REDUCER_DONE) break;
        }
        try std.testing.expectEqual(true, c.reducer_has_result(reducer));

        const result = c.reducer_get_result(reducer);
        var root_node = c.cells_node_t{};
        var result1 = result;
        try cTry(c.cells_dereference_node(cells, &result1, &root_node));
        try std.testing.expectEqual(c.CELLS_NODE_TYPE_DELTA2, root_node.header.type.value);
        var left_node = c.cells_node_t{};
        var result2 = result;
        try cTry(c.cells_get_left_node(cells, &result2, &left_node));
        try std.testing.expectEqual(c.CELLS_NODE_TYPE_DELTA0, left_node.header.type.value);
        var right_node = c.cells_node_t{};
        var result3 = result;
        try cTry(c.cells_get_right_node(cells, &result3, &right_node));
        try std.testing.expectEqual(c.CELLS_NODE_TYPE_DELTA0, right_node.header.type.value);
    }
    {
        // not, true, false
        var b: ?*c.bytecode_tree_builder_t = null;
        try cTry(c.bytecode_tree_builder_create(&b));
        defer c.bytecode_tree_builder_destroy(&b);
        _ = c.bytecode_new_node2(
            b,
            c.cells_new_delta2(),
            c.bytecode_new_node2(
                b,
                c.cells_new_delta2(),
                c.bytecode_new_node1(
                    b,
                    c.cells_new_delta1(),
                    c.bytecode_new_node0(b, c.cells_new_delta0()),
                ),
                c.bytecode_new_node2(
                    b,
                    c.cells_new_delta2(),
                    c.bytecode_new_node0(b, c.cells_new_delta0()),
                    c.bytecode_new_node0(b, c.cells_new_delta0()),
                ),
            ),
            c.bytecode_new_node0(b, c.cells_new_delta0()),
        );
        var not_program_redex: usize = undefined;
        try cTry(c.bytecode_tree_builder_build(b, cells, &not_program_redex));
        c.bytecode_tree_builder_reset(b);

        _ = c.bytecode_new_node0(b, c.cells_new_delta0());
        var false_redex: usize = undefined;
        try cTry(c.bytecode_tree_builder_build(b, cells, &false_redex));
        c.bytecode_tree_builder_reset(b);

        _ = c.bytecode_new_node1(
            b,
            c.cells_new_delta1(),
            c.bytecode_new_node0(b, c.cells_new_delta0()),
        );
        var true_redex: usize = undefined;
        try cTry(c.bytecode_tree_builder_build(b, cells, &true_redex));
        c.bytecode_tree_builder_reset(b);

        {
            // not false => true
            c.reducer_push_to_stack(reducer, c.REDUCER_APPLY_TOKEN);
            c.reducer_push_to_stack(reducer, not_program_redex);
            c.reducer_push_to_stack(reducer, false_redex);
            while (true) {
                const res = c.reducer_step(reducer);
                try cTry(res);
                if (res == c.REDUCER_DONE) break;
            }
            try std.testing.expectEqual(true, c.reducer_has_result(reducer));

            const result = c.reducer_get_result(reducer);
            var root_node = c.cells_node_t{};
            var result1 = result;
            try cTry(c.cells_dereference_node(cells, &result1, &root_node));
            try std.testing.expectEqual(c.CELLS_NODE_TYPE_DELTA1, root_node.header.type.value);
            var left_node = c.cells_node_t{};
            var result2 = result;
            try cTry(c.cells_get_left_node(cells, &result2, &left_node));
            try std.testing.expectEqual(c.CELLS_NODE_TYPE_DELTA0, left_node.header.type.value);
        }
        {
            // not true => false
            c.reducer_push_to_stack(reducer, c.REDUCER_APPLY_TOKEN);
            c.reducer_push_to_stack(reducer, not_program_redex);
            c.reducer_push_to_stack(reducer, true_redex);
            while (true) {
                const res = c.reducer_step(reducer);
                try cTry(res);
                if (res == c.REDUCER_DONE) break;
            }
            try std.testing.expectEqual(true, c.reducer_has_result(reducer));
            const result = c.reducer_get_result(reducer);
            var root_node = c.cells_node_t{};
            var result1 = result;
            try cTry(c.cells_dereference_node(cells, &result1, &root_node));
            try std.testing.expectEqual(c.CELLS_NODE_TYPE_DELTA0, root_node.header.type.value);
        }
    }
}
