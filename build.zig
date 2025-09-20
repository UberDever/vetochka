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
}

// https://ziggit.dev/t/build-system-tricks/3531
pub fn build(b: *std.Build) !void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});
    const sanitize = b.option(bool, "sanitize", "Enable ASan/UBSan-style flags") orelse false;

    const eval_dir = "eval";

    const eval_sources = &.{
        b.pathJoin(&.{ eval_dir, "eval.c" }),
        b.pathJoin(&.{ eval_dir, "util.c" }),
        b.pathJoin(&.{ eval_dir, "memory.c" }),
        b.pathJoin(&.{ eval_dir, "encode.c" }),
        b.pathJoin(&.{ eval_dir, "native.c" }),
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
        try flags.appendSlice(b.allocator, &.{
            "-g",
            "-fno-omit-frame-pointer",
            "-fsanitize=address,leak,undefined",
        });
    }
    try flags.append(b.allocator, if (sanitize) "-O0" else "-O2");

    const project_root = try std.fs.cwd().realpathAlloc(b.allocator, ".");
    const cfg_text = b.fmt(
        \\#pragma once
        \\#define PROJECT_ROOT "{s}"
        \\#define PATH_SEP "/"
    , .{try std.fs.path.join(b.allocator, &.{ project_root, eval_dir })});
    const wf = b.addWriteFiles();
    const cfg = wf.add("config.h", cfg_text);
    const install_cfg = b.addInstallFileWithDir(cfg, .prefix, "config.h");

    const lib = b.addLibrary(.{
        .name = eval_dir,
        .linkage = .dynamic,
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
        }),
    });
    lib.root_module.addCSourceFiles(.{ .files = eval_sources, .flags = flags.items });
    lib.root_module.addIncludePath(b.path(eval_dir));
    lib.root_module.addIncludePath(wf.getDirectory());

    const test_sources = &.{"eval/test_eval.c"};
    const test_exe = b.addExecutable(.{ .name = try std.fmt.allocPrint(b.allocator, "test_{s}", .{eval_dir}), .root_module = b.createModule(.{ .target = target, .optimize = optimize, .sanitize_c = .full, .link_libc = true }) });

    test_exe.root_module.addCSourceFiles(.{ .files = test_sources, .flags = flags.items });
    test_exe.root_module.addIncludePath(b.path(eval_dir));
    test_exe.root_module.addIncludePath(wf.getDirectory());
    test_exe.root_module.linkLibrary(lib);

    switch (target.result.os.tag) {
        .linux => test_exe.addRPath(b.path("$ORIGIN/../lib")),
        .macos => test_exe.addRPath(b.path("@executable_path/../lib")),
        else => {}, // On Windows, the DLL must be next to the exe or on PATH
    }

    lib.step.dependOn(&wf.step);
    lib.step.dependOn(&wf.step);
    test_exe.step.dependOn(&install_cfg.step);
    test_exe.step.dependOn(&lib.step);

    b.installArtifact(lib);
    b.installArtifact(test_exe);
}
