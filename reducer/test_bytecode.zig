const std = @import("std");
const c = @cImport({
    @cInclude("cells_api.h");
    @cInclude("reducer_api.h");
    @cInclude("bytecode_impl.h");
    @cInclude("stdio.h");
    @cInclude("string.h");
});

const stbds = @cImport({
    @cDefine("STB_DS_IMPLEMENTATION", {});
    @cInclude("vendor/stb_ds.h");
});

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

fn bytecode_read(
    src: [*c]const u8,
    cells: *c.struct_cells_t,
    applications_stack: ?[*]usize,
) CError!void {
    const src_len = c.strlen(src);
    var result = c.bytecode_reading_result_t{};
    try cTry(c.bytecode_text_read(c.span_cbyte_t{
        .data = src,
        .len = src_len,
    }, cells, applications_stack, &result));
}

fn bytecode_write(
    dst: c.span_byte_t,
    cells: *c.struct_cells_t,
    applications_stack: ?[*]usize,
) CError!void {
    try cTry(c.bytecode_text_write(cells, applications_stack, dst));
}

test "empty" {
    var cells: ?*c.struct_cells_t = null;
    try cTry(c.cells_create(&cells, 256));
    defer c.cells_destroy(&cells);

    const applications_stack: ?[*]usize = null;
    defer {
        if (applications_stack != null) {
            stbds.stbds_arrfreef(applications_stack);
        }
    }
    const buf = std.testing.allocator.alloc(u8, 1024) catch unreachable;
    defer std.testing.allocator.free(buf);

    try bytecode_read("", cells.?, applications_stack);
    try bytecode_write(c.span_byte_t{
        .data = buf.ptr,
        .len = buf.len,
    }, cells.?, applications_stack);
}
