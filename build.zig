const std = @import("std");

pub fn build(b: *std.Build) !void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});
    const sanitize = b.option(bool, "sanitize", "Enable ASan/UBSan-style flags") orelse false;

    const eval_sources = &.{
        "eval/eval.c",
        "eval/util.c",
        "eval/memory.c",
        "eval/encode.c",
        "eval/native.c",
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

    const lib = b.addLibrary(.{
        .name = "eval",
        .linkage = .dynamic,
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
        }),
    });
    lib.root_module.addCSourceFiles(.{ .files = eval_sources, .flags = flags.items });
    lib.root_module.addIncludePath(b.path("eval"));
    b.installArtifact(lib);
    b.getInstallStep().dependOn(&lib.step);
}
