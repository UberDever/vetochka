const std = @import("std");
const fs = std.fs;

const builtin = @import("builtin");

comptime {
    const required_zig = "0.15.1";
    const current_zig = builtin.zig_version;
    const min_zig = std.SemanticVersion.parse(required_zig) catch unreachable;
    if (current_zig.order(min_zig) == .lt) {
        const error_message =
            \\Sorry, it looks like your version of zig is too old. :-(
            \\
            \\Vetochka requires development build {}
            \\
            \\Please download a development ("master") build from
            \\
            \\https://ziglang.org/download/
            \\
            \\
        ;
        @compileError(std.fmt.comptimePrint(error_message, .{min_zig}));
    }
    if (@sizeOf(usize) != 8) @compileError("need 64-bit pointers");
    if (@sizeOf(isize) != 8) @compileError("need 64-bit pointers");
}

// https://ziggit.dev/t/build-system-tricks/3531
pub fn build(b: *std.Build) !void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});
    const sanitize = b.option(bool, "sanitize", "Enable ASan/UBSan-style flags") orelse false;

    const c_core_dir = "reducer";

    const c_core_sources = &.{
        b.pathJoin(&.{ c_core_dir, "vendor_stbds.c" }),
        b.pathJoin(&.{ c_core_dir, "cells_cells.c" }),
        b.pathJoin(&.{ c_core_dir, "cells_debug.c" }),
        b.pathJoin(&.{ c_core_dir, "bytecode_node.c" }),
        b.pathJoin(&.{ c_core_dir, "bytecode_tree.c" }),
        b.pathJoin(&.{ c_core_dir, "reducer_reducer.c" }),
    };

    var flags = std.ArrayList([]const u8).empty;
    defer flags.deinit(b.allocator);
    try flags.appendSlice(b.allocator, &.{
        "-fPIC",
        "--std=c99",
        "-Wall",
        "-Wextra",
        "-Werror",
    });
    if (sanitize) {
        try flags.appendSlice(b.allocator, &.{ "-g", "-fno-omit-frame-pointer", "-fsanitize=address", "-shared-libasan" });
    }
    try flags.append(b.allocator, if (sanitize) "-O0" else "-O2");

    const project_root = try std.fs.cwd().realpathAlloc(b.allocator, ".");
    try flags.appendSlice(b.allocator, &.{
        b.fmt(
            \\-DPROJECT_ROOT="{s}"
        , .{b.pathJoin(&.{ project_root, c_core_dir })}),
        b.fmt(
            \\-DPATH_SEP="{c}"
        , .{std.fs.path.sep}),
    });

    const lib = b.addLibrary(.{
        .name = c_core_dir,
        .linkage = .dynamic,
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
        }),
    });
    lib.root_module.addCSourceFiles(.{ .files = c_core_sources, .flags = flags.items });
    if (sanitize) {
        // Add ASan linker flags for the shared library
        lib.root_module.addLibraryPath(.{ .cwd_relative = "/usr/lib/gcc/x86_64-linux-gnu/11" });
        lib.root_module.addObjectFile(.{ .cwd_relative = "/usr/lib/gcc/x86_64-linux-gnu/11/libasan_preinit.o" });
        lib.root_module.linkSystemLibrary("asan", .{});
    }
    lib.root_module.addIncludePath(b.path(c_core_dir));

    const test_name = try std.fmt.allocPrint(b.allocator, "test_{s}", .{c_core_dir});
    const test_exe = b.addTest(.{
        .name = test_name,
        .root_module = b.createModule(
            .{
                .root_source_file = b.path(
                    b.pathJoin(&.{
                        c_core_dir,
                        std.mem.concat(b.allocator, u8, &[_][]const u8{ test_name, ".zig" }) catch unreachable,
                    }),
                ),
                .target = target,
                .optimize = optimize,
            },
        ),
    });
    test_exe.root_module.addIncludePath(b.path(c_core_dir));
    test_exe.root_module.link_libc = true;
    test_exe.root_module.linkLibrary(lib);
    test_exe.step.dependOn(&lib.step);

    b.installArtifact(lib);

    const test_step = b.step("test", "Run all unit tests");
    const run_tests = b.addRunArtifact(test_exe);
    test_step.dependOn(&run_tests.step);
}
