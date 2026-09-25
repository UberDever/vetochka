const std = @import("std");

const c = @cImport({
    @cInclude("source_api.h");
});

const CError = error{Unknown};

fn cTry(result: c.error_t) CError!void {
    if (result < 0) return error.Unknown;
}

const Parsed = struct {
    text: []const u8,
    tree: ?*c.struct_source_tree_t = null,

    fn init(text: []const u8) !Parsed {
        var parsed = Parsed{ .text = text };
        try cTry(c.source_ast_create(
            &parsed.tree,
            .{ .data = text.ptr, .len = text.len },
            c.allocator_libc(),
        ));
        return parsed;
    }

    fn deinit(self: *Parsed) void {
        c.source_tree_free(&self.tree);
    }
};

const Output = struct {
    data: c.struct_da_byte_t,

    fn init() !Output {
        var output: Output = undefined;
        var allocator = c.allocator_libc();
        try cTry(c.domain_da_byte_init(&output.data, &allocator));
        return output;
    }

    fn deinit(self: *Output) void {
        c.domain_da_byte_free(&self.data);
    }

    fn bytes(self: *const Output) []const u8 {
        const span = c.domain_da_byte_get_span(&self.data);
        return span.data[0..span.len];
    }
};

fn expectParses(text: []const u8) !void {
    var parsed = try Parsed.init(text);
    defer parsed.deinit();

    try std.testing.expect(c.source_tree_get_count(parsed.tree) >= 2);
    const root = c.source_tree_get_node(parsed.tree, 0);
    try std.testing.expectEqual(c.SOURCE_NODE_TYPE_SOURCE, root.type.value);
}

fn expectRejects(text: []const u8) !void {
    var tree: ?*c.struct_source_tree_t = null;
    const result = c.source_ast_create(&tree, .{ .data = text.ptr, .len = text.len }, c.allocator_libc());
    defer c.source_tree_free(&tree);
    try std.testing.expect(result < 0);
}

test "source grammar accepts the spec forms" {
    const cases = [_][]const u8{
        "",
        "0; 42; {text {nested}}; name?; foo'; x-y",
        "~[]; ~[x]; ~[x, y]; ~ [x]",
        "@[doc, more] !value.field(1, 2,)[index]{payload}",
        \\[
        \\  (grouped),
        \\  item,
        \\]
        \\f()
        \\left :: right
        ,
        "call label: -value other: @[meta] target",
        "task do first; nested do value end; last end",
        "x: 1; do a end; [x: 2, y: 4]; (x: (y: 1))",
        "f do: 1 end: 2",
        "$fn: [x] do x end to: 42; $ fn: [] do 1 end",
        "$ns: {core} op: {add} x: 1 y: 2",
        "a + b * c - d; !!x; -(x); x - y; a ^ b; ^x",
        "f(x)(y).z{bytes}[w]",
        \\first
        \\second ...
        \\  + third
        \\;; line comment
        \\#| outer #| nested |# comment |#
        \\#; discarded(1, 2)
        \\last
        ,
        // layout: parens and brackets are inactive, a nested do-block is active again
        \\f(a
        \\  + b)
        \\g(do
        \\  one
        \\  two
        \\end)
        ,
    };

    for (cases, 0..) |text, index| {
        expectParses(text) catch |err| {
            std.debug.print("source case {d} failed:\n{s}\n", .{ index, text });
            return err;
        };
    }
}

test "source grammar rejects what the spec forbids" {
    const cases = [_][]const u8{
        "x -y", // prefix operator after an operand: no juxtaposition
        "1-2", // operator glued to a literal
        "f(x)-y", // operator glued to `)`
        "x=", // operator glued to an identifier
        "f (a)", // tight postfix must be glued
        "f x", // no juxtaposition
        "x : y", // free lone colon
        "a . b", // free dot
        "~[1, 2, 3]", // nyads have at most two children
        "~[1,]",
        "$", // `$` must head a labeled expression
        "$ x",
        "@x", // `@` only opens `@[`
        "^", // an operator with no operands
        "\u{394}", // Δ: the lexer is ASCII-only for now, task 20260924-123410
        "007",
        \\@[doc]
        \\target
        , // annotation `]` ends an expression: ASI inserts `;`
    };

    for (cases, 0..) |text, index| {
        expectRejects(text) catch |err| {
            std.debug.print("rejection case {d} parsed:\n{s}\n", .{ index, text });
            return err;
        };
    }
}

test "source formatters smoke" {
    const text =
        \\@[trace] run args: [1, 2,] do
        \\  value.field()
        \\  result = !value + 1
        \\  $fn: [x] do x end
        \\end
    ;
    var parsed = try Parsed.init(text);
    defer parsed.deinit();

    var canonical = try Output.init();
    defer canonical.deinit();
    try cTry(c.source_tree_format_canonical(
        .{ .data = text.ptr, .len = text.len },
        parsed.tree,
        &canonical.data,
    ));
    try std.testing.expect(std.mem.indexOf(u8, canonical.bytes(), "@[trace]") != null);
    try std.testing.expect(std.mem.indexOf(u8, canonical.bytes(), "value.field()") != null);
    try std.testing.expect(std.mem.indexOf(u8, canonical.bytes(), "!value + 1") != null);
    try std.testing.expect(std.mem.indexOf(u8, canonical.bytes(), "$fn: [x]") != null);

    // The canonical form parses again.
    var reparsed = try Parsed.init(canonical.bytes());
    defer reparsed.deinit();

    var sexpr = try Output.init();
    defer sexpr.deinit();
    try cTry(c.source_tree_format_sexpr(
        .{ .data = text.ptr, .len = text.len },
        parsed.tree,
        &sexpr.data,
    ));
    try std.testing.expect(std.mem.startsWith(u8, sexpr.bytes(), "(source"));
    try std.testing.expect(std.mem.indexOf(u8, sexpr.bytes(), "(annotation") != null);
    try std.testing.expect(std.mem.indexOf(u8, sexpr.bytes(), "(op-prefix") != null);
}
