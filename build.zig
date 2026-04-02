const std = @import("std");
const fs = std.fs;

const builtin = @import("builtin");
const str = []const u8;

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

const TestSuite = struct {
    b: *std.Build,
    c_core_dir: str,
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,

    fn init(
        b: *std.Build,
        c_core_dir: str,
        target: std.Build.ResolvedTarget,
        optimize: std.builtin.OptimizeMode,
    ) TestSuite {
        return .{
            .b = b,
            .c_core_dir = c_core_dir,
            .target = target,
            .optimize = optimize,
        };
    }

    fn deinit(self: TestSuite) void {
        _ = self;
    }

    fn makeTest(self: *TestSuite, name: []const u8, dependOn: *std.Build.Step.Compile) !*std.Build.Step.Compile {
        const test_exe = self.b.addTest(.{
            .name = name,
            .root_module = self.b.createModule(.{
                .root_source_file = self.b.path(self.b.pathJoin(&.{
                    self.c_core_dir,
                    std.mem.concat(self.b.allocator, u8, &[_][]const u8{ name, ".zig" }) catch unreachable,
                })),
                .target = self.target,
                .optimize = self.optimize,
            }),
        });
        test_exe.root_module.addIncludePath(self.b.path(self.c_core_dir));
        test_exe.root_module.link_libc = true;
        test_exe.root_module.linkLibrary(dependOn);
        test_exe.step.dependOn(&dependOn.step);
        return test_exe;
    }

    fn addTest(self: *TestSuite, name: str, lib: *std.Build.Step.Compile) !struct { *std.Build.Step.Run, *std.Build.Step.Compile } {
        const compile_tests = try self.makeTest(try std.fmt.allocPrint(self.b.allocator, "test_{s}", .{name}), lib);
        const run_tests = self.b.addRunArtifact(compile_tests);
        self.b.step(
            try std.fmt.allocPrint(self.b.allocator, "test-{s}", .{name}),
            try std.fmt.allocPrint(self.b.allocator, "Run {s} tests", .{name}),
        ).dependOn(&run_tests.step);
        return .{ run_tests, compile_tests };
    }

    fn makeCFlags(self: TestSuite, sanitized: bool) !std.ArrayList([]const u8) {
        var flags = std.ArrayList([]const u8).empty;
        try flags.appendSlice(self.b.allocator, &.{
            "-fPIC",
            "--std=c99",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-pedantic-errors",
        });
        if (!sanitized) {
            try flags.appendSlice(self.b.allocator, &.{"-g"});
            try flags.append(self.b.allocator, "-O2");
        } else {
            try flags.appendSlice(self.b.allocator, &.{ "-g", "-fno-omit-frame-pointer", "-fsanitize=address", "-shared-libasan" });
            try flags.append(self.b.allocator, "-O0");
        }
        const project_root = try std.fs.cwd().realpathAlloc(self.b.allocator, ".");
        try flags.appendSlice(self.b.allocator, &.{
            try std.fmt.allocPrint(self.b.allocator,
                \\-DPROJECT_ROOT="{s}"
            , .{try fs.path.join(self.b.allocator, &.{ project_root, self.c_core_dir })}),
            try std.fmt.allocPrint(self.b.allocator,
                \\-DPATH_SEP="{c}"
            , .{std.fs.path.sep}),
        });
        return flags;
    }
};

fn runAndCapture(b: *std.Build, argv: []const []const u8) []const u8 {
    var out_code: u8 = undefined;
    const result = b.runAllowFail(argv, &out_code, std.process.Child.StdIo.Pipe) catch @panic("failed to run tool");
    return std.mem.trim(u8, result, " \r\n\t");
}

// https://ziggit.dev/t/build-system-tricks/3531
pub fn build(b: *std.Build) !void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});
    const sanitize = b.option(bool, "sanitize", "Enable ASan/UBSan-style flags") orelse false;
    const cc = b.option([]const u8, "cc", "C compiler for ASan discovery") orelse "gcc";

    const c_core_dir = "reducer";
    var s = TestSuite.init(b, c_core_dir, target, optimize);
    defer s.deinit();

    const c_core_sources = &.{
        b.pathJoin(&.{ c_core_dir, "vendor_stbds.c" }),
        b.pathJoin(&.{ c_core_dir, "cells_cells.c" }),
        b.pathJoin(&.{ c_core_dir, "cells_debug.c" }),
        b.pathJoin(&.{ c_core_dir, "cells_node.c" }),
        b.pathJoin(&.{ c_core_dir, "bytecode_tree.c" }),
        b.pathJoin(&.{ c_core_dir, "bytecode_text.c" }),
        b.pathJoin(&.{ c_core_dir, "reducer_reducer.c" }),
    };

    const lib = b.addLibrary(.{
        .name = c_core_dir,
        .linkage = .dynamic,
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
        }),
    });

    b.installArtifact(lib);
    var test_compile_targets = std.ArrayList(*std.Build.Step.Compile).empty;
    defer test_compile_targets.deinit(b.allocator);
    var test_run_targets = std.ArrayList(*std.Build.Step.Run).empty;
    defer test_run_targets.deinit(b.allocator);

    {
        const run, const compile = try s.addTest("reducer", lib);
        try test_compile_targets.append(b.allocator, compile);
        try test_run_targets.append(b.allocator, run);
    }
    {
        const run, const compile = try s.addTest("bytecode", lib);
        try test_compile_targets.append(b.allocator, compile);
        try test_run_targets.append(b.allocator, run);
    }

    const test_all_step = b.step("test-all", "Run all unit tests");
    for (test_compile_targets.items) |test_step| {
        test_all_step.dependOn(&test_step.step);
    }

    // https://github.com/zigtools/zls/blob/master/schema.json
    // https://zigtools.org/zls/guides/build-on-save/
    const check = b.step("check", "Check if everything compiles");
    for (test_compile_targets.items) |test_step| {
        check.dependOn(&test_step.step);
    }

    var flags = try s.makeCFlags(sanitize);
    defer flags.deinit(s.b.allocator);

    lib.root_module.addIncludePath(b.path(c_core_dir));
    lib.root_module.addCSourceFiles(.{ .files = c_core_sources, .flags = flags.items });

    if (sanitize) {
        const libasan_so = runAndCapture(b, &.{
            cc,
            "-print-file-name=libasan.so",
        });
        const libasan_preinit = runAndCapture(b, &.{
            cc,
            "-print-file-name=libasan_preinit.o",
        });
        const libasan_dir = std.fs.path.dirname(libasan_so).?;
        lib.root_module.addObjectFile(.{ .cwd_relative = libasan_preinit });
        lib.root_module.addLibraryPath(.{ .cwd_relative = libasan_dir });
        for (test_run_targets.items) |test_step| {
            test_step.setEnvironmentVariable("LD_PRELOAD", libasan_so);
        }

        lib.root_module.linkSystemLibrary("asan", .{});
    }
}
