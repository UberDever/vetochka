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

test "source grammar smokes" {
    const cases = [_][]const u8{
        "",
        "^; Δ; 0; 42; {text {nested}}; name?; ~ * & value",
        \\@[doc]
        \\!value.field(1, 2,)[index]{payload}
        ,
        \\[
        \\  (grouped),
        \\  item,
        \\]
        \\f()
        \\left :: right
        ,
        "call label: -value other: @[meta] target",
        "task do first; nested do value end; last end",
        \\first
        \\second ...
        \\  + third
        \\;; line comment
        \\#| outer #| nested |# comment |#
        \\#; discarded(1, 2) ^
        ,
    };

    for (cases, 0..) |text, index| {
        expectParses(text) catch |err| {
            std.debug.print("source smoke case {d} failed:\n{s}\n", .{ index, text });
            return err;
        };
    }
}

test "source formatters smoke" {
    std.testing.log_level = .debug;

    const text =
        \\@[trace] run args: [1, 2,] do
        \\  value.field()
        \\  result = value + 1
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
    // std.debug.print("{s}\n", .{canonical.bytes()});

    var sexpr = try Output.init();
    defer sexpr.deinit();
    try cTry(c.source_tree_format_sexpr(
        .{ .data = text.ptr, .len = text.len },
        parsed.tree,
        &sexpr.data,
    ));
    try std.testing.expect(std.mem.startsWith(u8, sexpr.bytes(), "(source"));
    try std.testing.expect(std.mem.indexOf(u8, sexpr.bytes(), "(annotation") != null);
    // std.debug.print("{s}\n", .{sexpr.bytes()});
}
