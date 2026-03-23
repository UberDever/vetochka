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

test "smoke memory" {
    std.testing.log_level = .debug;

    const gpa = std.testing.allocator;
    _ = gpa;

    var cells: ?*c.struct_cells_t = null;
    try cTry(c.cells_create(&cells, 64));
    defer c.cells_destroy(&cells);

    const tree_node = c.cells_new_delta0();
    const lhs_ref = c.cells_new_ref2(8191);
    const rhs_n0f = c.cells_new_value0f(-3516);

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
    try std.testing.expectEqual(c.CELLS_NODE_TYPE_DELTA0, tree_meta.type.value);
    const lhs_meta = c.cells_get_node_meta(cells, lhs_index);
    try std.testing.expectEqual(c.CELLS_NODE_TYPE_REF2, lhs_meta.type.value);
    const rhs_meta = c.cells_get_node_meta(cells, rhs_index);
    try std.testing.expectEqual(c.CELLS_NODE_TYPE_VALUEF0, rhs_meta.type.value);

    const tree_node_out = c.cells_get_node(cells, tree_index, tree_meta);
    try std.testing.expectEqual(c.CELLS_NODE_TYPE_DELTA0, tree_node_out.meta.type.value);
    const lhs_node_out = c.cells_get_node(cells, lhs_index, lhs_meta);
    try std.testing.expectEqual(c.CELLS_NODE_TYPE_REF2, lhs_node_out.meta.type.value);
    const rhs_node_out = c.cells_get_node(cells, rhs_index, rhs_meta);
    try std.testing.expectEqual(c.CELLS_NODE_TYPE_VALUEF0, rhs_node_out.meta.type.value);

    try cTry(c.cells_node_free(cells, tree_index, tree_meta.size));
    try cTry(c.cells_node_free(cells, lhs_index, lhs_meta.size));
    try cTry(c.cells_node_free(cells, rhs_index, rhs_meta.size));

    const tree_meta1 = c.cells_get_node_meta(cells, tree_index);
    try std.testing.expectEqual(c.CELLS_NODE_TYPE_INVALID, tree_meta1.type.value);
    const lhs_meta1 = c.cells_get_node_meta(cells, lhs_index);
    try std.testing.expectEqual(c.CELLS_NODE_TYPE_INVALID, lhs_meta1.type.value);
    const rhs_meta1 = c.cells_get_node_meta(cells, rhs_index);
    try std.testing.expectEqual(c.CELLS_NODE_TYPE_INVALID, rhs_meta1.type.value);
}

test "debug view demo" {
    std.testing.log_level = .debug;

    var cells: ?*c.struct_cells_t = null;
    try cTry(c.cells_create(&cells, 128));
    defer c.cells_destroy(&cells);

    // Create a mix of nodes
    const n1 = c.cells_new_delta2();
    var idx1: usize = 0;
    try cTry(c.cells_alloc_chunk(cells, n1.meta.size, &idx1));
    try cTry(c.cells_write_node(cells, idx1, n1));
    try std.testing.expectEqual(c.CELLS_NODE_TYPE_DELTA2, c.cells_get_node_meta(cells, idx1).type.value);

    var payload_data = [_]u8{ 0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE };
    const payload = c.span_byte_t{ .data = &payload_data, .len = payload_data.len };
    const n3 = c.cells_new_value0v(payload);
    var idx3: usize = 0;
    try cTry(c.cells_alloc_chunk(cells, n3.meta.size, &idx3));
    try cTry(c.cells_write_node(cells, idx3, n3));
    try std.testing.expectEqual(c.CELLS_NODE_TYPE_VALUEV0, c.cells_get_node_meta(cells, idx3).type.value);

    const n2 = c.cells_new_ref2(1024);
    var idx2: usize = 0;
    try cTry(c.cells_alloc_chunk(cells, n2.meta.size, &idx2));
    try cTry(c.cells_write_node(cells, idx2, n2));
    try std.testing.expectEqual(c.CELLS_NODE_TYPE_REF2, c.cells_get_node_meta(cells, idx2).type.value);

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
        try cTry(c.cells_alloc_chunk(cells, delta0.meta.size, &index_out));
        try cTry(c.cells_write_node(cells, index_out, delta0));
        c.reducer_push_to_stack(reducer, index_out);

        try cTry(c.cells_alloc_chunk(cells, delta0.meta.size, &index_out));
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
        try std.testing.expectEqual(c.CELLS_NODE_TYPE_DELTA1, root_node.meta.type.value);
        var left_node = c.cells_node_t{};
        var result2 = result;
        try cTry(c.cells_get_left_node(cells, &result2, &left_node));
        try std.testing.expectEqual(c.CELLS_NODE_TYPE_DELTA0, left_node.meta.type.value);
    }
    {
        // rule 0.b
        c.reducer_push_to_stack(reducer, c.REDUCER_APPLY_TOKEN);

        const delta1 = c.cells_new_delta1();
        const delta0 = c.cells_new_delta0();
        var index_out: usize = 0;
        try cTry(c.cells_alloc_chunk(cells, delta1.meta.size + delta0.meta.size, &index_out));
        try cTry(c.cells_write_node(cells, index_out, delta1));
        try cTry(c.cells_write_node(cells, index_out + delta1.meta.size, delta0));
        c.reducer_push_to_stack(reducer, index_out);

        try cTry(c.cells_alloc_chunk(cells, delta0.meta.size, &index_out));
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
        try std.testing.expectEqual(c.CELLS_NODE_TYPE_DELTA2, root_node.meta.type.value);
        var left_node = c.cells_node_t{};
        var result2 = result;
        try cTry(c.cells_get_left_node(cells, &result2, &left_node));
        try std.testing.expectEqual(c.CELLS_NODE_TYPE_DELTA0, left_node.meta.type.value);
        var right_node = c.cells_node_t{};
        var result3 = result;
        try cTry(c.cells_get_right_node(cells, &result3, &right_node));
        try std.testing.expectEqual(c.CELLS_NODE_TYPE_DELTA0, right_node.meta.type.value);
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
        try std.testing.expectEqual(c.CELLS_NODE_TYPE_DELTA2, root_node.meta.type.value);
        var left_node = c.cells_node_t{};
        var result2 = result;
        try cTry(c.cells_get_left_node(cells, &result2, &left_node));
        try std.testing.expectEqual(c.CELLS_NODE_TYPE_DELTA0, left_node.meta.type.value);
        var right_node = c.cells_node_t{};
        var result3 = result;
        try cTry(c.cells_get_right_node(cells, &result3, &right_node));
        try std.testing.expectEqual(c.CELLS_NODE_TYPE_DELTA0, right_node.meta.type.value);
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
        try std.testing.expectEqual(c.CELLS_NODE_TYPE_DELTA2, root_node.meta.type.value);
        var left_node = c.cells_node_t{};
        var result2 = result;
        try cTry(c.cells_get_left_node(cells, &result2, &left_node));
        try std.testing.expectEqual(c.CELLS_NODE_TYPE_DELTA0, left_node.meta.type.value);
        var right_node = c.cells_node_t{};
        var result3 = result;
        try cTry(c.cells_get_right_node(cells, &result3, &right_node));
        try std.testing.expectEqual(c.CELLS_NODE_TYPE_DELTA1, right_node.meta.type.value);
        var right_left_node = c.cells_node_t{};
        var result4 = result3;
        try cTry(c.cells_get_left_node(cells, &result4, &right_left_node));
        try std.testing.expectEqual(c.CELLS_NODE_TYPE_DELTA0, right_left_node.meta.type.value);
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
        try std.testing.expectEqual(c.CELLS_NODE_TYPE_DELTA0, root_node.meta.type.value);
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
        try std.testing.expectEqual(c.CELLS_NODE_TYPE_DELTA1, root_node.meta.type.value);
        var left_node = c.cells_node_t{};
        var result2 = result;
        try cTry(c.cells_get_left_node(cells, &result2, &left_node));
        try std.testing.expectEqual(c.CELLS_NODE_TYPE_DELTA0, left_node.meta.type.value);
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
        try std.testing.expectEqual(c.CELLS_NODE_TYPE_DELTA2, root_node.meta.type.value);
        var left_node = c.cells_node_t{};
        var result2 = result;
        try cTry(c.cells_get_left_node(cells, &result2, &left_node));
        try std.testing.expectEqual(c.CELLS_NODE_TYPE_DELTA0, left_node.meta.type.value);
        var right_node = c.cells_node_t{};
        var result3 = result;
        try cTry(c.cells_get_right_node(cells, &result3, &right_node));
        try std.testing.expectEqual(c.CELLS_NODE_TYPE_DELTA0, right_node.meta.type.value);
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
            try std.testing.expectEqual(c.CELLS_NODE_TYPE_DELTA1, root_node.meta.type.value);
            var left_node = c.cells_node_t{};
            var result2 = result;
            try cTry(c.cells_get_left_node(cells, &result2, &left_node));
            try std.testing.expectEqual(c.CELLS_NODE_TYPE_DELTA0, left_node.meta.type.value);
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
            try std.testing.expectEqual(c.CELLS_NODE_TYPE_DELTA0, root_node.meta.type.value);
        }
    }
}
