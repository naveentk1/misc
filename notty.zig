const std = @import("std");
const builtin = @import("builtin");
const posix = std.posix;

const Command = struct {
    args: std.ArrayList([]const u8),
    input_file: ?[]const u8 = null,
    output_file: ?[]const u8 = null,
    append_output: bool = false,
    background: bool = false,
};

const Pipeline = struct {
    commands: std.ArrayList(Command),
};

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const allocator = gpa.allocator();

    // Setup signal handler for Ctrl+C
    setupSignalHandler();

    const stdin_file = std.fs.File.stdin();
    const stdout_file = std.fs.File.stdout();

    try stdout_file.writeAll("Enhanced Zig Shell v1.0\n");
    try stdout_file.writeAll("Features: pipes, redirection, background jobs, built-ins\n");
    try stdout_file.writeAll("Type 'help' for commands, 'exit' to quit\n\n");

    var env_map = std.process.EnvMap.init(allocator);
    defer env_map.deinit();

    while (true) {
        try stdout_file.writeAll("zsh$ ");
        
        // Read line using buffered reader with buffer
        var stdin_buf: [4096]u8 = undefined;
        var stdin_reader = stdin_file.reader();
        const line = stdin_reader.readUntilDelimiterOrEof(&stdin_buf, '\n') catch |err| switch (err) {
            error.EndOfStream => break,
            error.StreamTooLong => {
                try stdout_file.writeAll("Error: Input line too long\n");
                continue;
            },
            else => return err,
        } orelse break;

        const trimmed = std.mem.trim(u8, line, &std.ascii.whitespace);
        if (trimmed.len == 0) continue;

        // Parse the command line
        var pipeline = parsePipeline(allocator, trimmed) catch |err| {
            try stdout_file.writer().print("Parse error: {}\n", .{err});
            continue;
        };
        defer freePipeline(allocator, &pipeline);

        if (pipeline.commands.items.len == 0) continue;

        // Check for built-in commands (only if single command, no pipes)
        if (pipeline.commands.items.len == 1) {
            const cmd = &pipeline.commands.items[0];
            if (cmd.args.items.len > 0) {
                const cmd_name = cmd.args.items[0];

                if (std.mem.eql(u8, cmd_name, "exit")) {
                    break;
                } else if (std.mem.eql(u8, cmd_name, "help")) {
                    try printHelp(stdout_file.writer());
                    continue;
                } else if (std.mem.eql(u8, cmd_name, "cd")) {
                    try builtinCd(allocator, cmd.args.items, stdout_file.writer());
                    continue;
                } else if (std.mem.eql(u8, cmd_name, "pwd")) {
                    try builtinPwd(allocator, stdout_file.writer());
                    continue;
                } else if (std.mem.eql(u8, cmd_name, "echo")) {
                    try builtinEcho(cmd.args.items, stdout_file.writer());
                    continue;
                } else if (std.mem.eql(u8, cmd_name, "export")) {
                    try builtinExport(allocator, cmd.args.items, &env_map, stdout_file.writer());
                    continue;
                } else if (std.mem.eql(u8, cmd_name, "env")) {
                    try builtinEnv(&env_map, stdout_file.writer());
                    continue;
                }
            }
        }

        // Execute the pipeline
        executePipeline(allocator, &pipeline, &env_map, stdout_file.writer()) catch |err| {
            try stdout_file.writer().print("Execution error: {}\n", .{err});
        };
    }

    try stdout_file.writeAll("Goodbye!\n");
}

fn setupSignalHandler() void {
    // Install signal handler for SIGINT (Ctrl+C)
    if (builtin.os.tag != .windows) {
        const act = posix.Sigaction{
            .handler = .{ .handler = handleSignal },
            .mask = std.mem.zeroes(posix.sigset_t),
            .flags = 0,
        };
        posix.sigaction(posix.SIG.INT, &act, null);
    }
}

fn handleSignal(sig: c_int) callconv(.c) void {
    _ = sig;
    // Just ignore SIGINT in the shell itself
    // Child processes will still receive it
}

fn parsePipeline(allocator: std.mem.Allocator, line: []const u8) !Pipeline {
    var commands = std.ArrayList(Command).init(allocator);
    errdefer {
        for (commands.items) |*cmd| {
            for (cmd.args.items) |arg| {
                allocator.free(arg);
            }
            cmd.args.deinit();
            if (cmd.input_file) |f| allocator.free(f);
            if (cmd.output_file) |f| allocator.free(f);
        }
        commands.deinit();
    }

    var pipe_iter = std.mem.splitScalar(u8, line, '|');

    while (pipe_iter.next()) |cmd_str| {
        const trimmed_cmd = std.mem.trim(u8, cmd_str, &std.ascii.whitespace);
        if (trimmed_cmd.len == 0) continue;

        var cmd = Command{
            .args = std.ArrayList([]const u8).init(allocator),
        };

        // Check for background job
        var working_str = trimmed_cmd;
        if (std.mem.endsWith(u8, working_str, "&")) {
            cmd.background = true;
            working_str = std.mem.trim(u8, working_str[0 .. working_str.len - 1], &std.ascii.whitespace);
        }

        // Parse tokens handling redirections
        var token_iter = std.mem.tokenizeAny(u8, working_str, &std.ascii.whitespace);
        var redirect_next: ?enum { input, output, append } = null;

        while (token_iter.next()) |token| {
            if (redirect_next) |rtype| {
                switch (rtype) {
                    .input => cmd.input_file = try allocator.dupe(u8, token),
                    .output => cmd.output_file = try allocator.dupe(u8, token),
                    .append => {
                        cmd.output_file = try allocator.dupe(u8, token);
                        cmd.append_output = true;
                    },
                }
                redirect_next = null;
            } else if (std.mem.eql(u8, token, "<")) {
                redirect_next = .input;
            } else if (std.mem.eql(u8, token, ">")) {
                redirect_next = .output;
            } else if (std.mem.eql(u8, token, ">>")) {
                redirect_next = .append;
            } else {
                try cmd.args.append(try allocator.dupe(u8, token));
            }
        }

        try commands.append(cmd);
    }

    return Pipeline{ .commands = commands };
}

fn freePipeline(allocator: std.mem.Allocator, pipeline: *Pipeline) void {
    for (pipeline.commands.items) |*cmd| {
        for (cmd.args.items) |arg| {
            allocator.free(arg);
        }
        cmd.args.deinit();
        if (cmd.input_file) |f| allocator.free(f);
        if (cmd.output_file) |f| allocator.free(f);
    }
    pipeline.commands.deinit();
}

fn executePipeline(allocator: std.mem.Allocator, pipeline: *Pipeline, env_map: *std.process.EnvMap, stdout_writer: anytype) !void {
    const num_commands = pipeline.commands.items.len;
    if (num_commands == 0) return;

    // Create pipes for pipeline
    var pipes = std.ArrayList([2]posix.fd_t).init(allocator);
    defer {
        for (pipes.items) |p| {
            posix.close(p[0]);
            posix.close(p[1]);
        }
        pipes.deinit();
    }

    // Create num_commands - 1 pipes
    var i: usize = 0;
    while (i < num_commands - 1) : (i += 1) {
        const p = try posix.pipe();
        try pipes.append(p);
    }

    // Spawn all commands in the pipeline
    var children = std.ArrayList(std.process.Child).init(allocator);
    defer {
        for (children.items) |*child| {
            _ = child.kill() catch {};
        }
        children.deinit();
    }

    i = 0;
    while (i < num_commands) : (i += 1) {
        const cmd = &pipeline.commands.items[i];
        if (cmd.args.items.len == 0) continue;

        var child = std.process.Child.init(cmd.args.items, allocator);

        // Setup environment variables
        child.env_map = env_map;

        // Setup stdin
        if (i == 0) {
            // First command
            if (cmd.input_file) |file| {
                const fd = try posix.open(file, .{ .ACCMODE = .RDONLY }, 0);
                child.stdin_behavior = .{ .fd = fd };
            } else {
                child.stdin_behavior = .Inherit;
            }
        } else {
            // Read from previous pipe
            child.stdin_behavior = .{ .fd = pipes.items[i - 1][0] };
        }

        // Setup stdout
        if (i == num_commands - 1) {
            // Last command
            if (cmd.output_file) |file| {
                const flags: posix.O = if (cmd.append_output)
                    .{ .ACCMODE = .WRONLY, .CREAT = true, .APPEND = true }
                else
                    .{ .ACCMODE = .WRONLY, .CREAT = true, .TRUNC = true };
                const fd = try posix.open(file, flags, 0o644);
                child.stdout_behavior = .{ .fd = fd };
            } else {
                child.stdout_behavior = .Inherit;
            }
        } else {
            // Write to next pipe
            child.stdout_behavior = .{ .fd = pipes.items[i][1] };
        }

        child.stderr_behavior = .Inherit;

        try child.spawn();
        try children.append(child);
    }

    // Close all pipe fds in parent
    for (pipes.items) |p| {
        posix.close(p[0]);
        posix.close(p[1]);
    }
    pipes.clearRetainingCapacity();

    // Wait for all children (unless background)
    const is_background = pipeline.commands.items[pipeline.commands.items.len - 1].background;

    if (is_background) {
        try stdout_writer.print("[Background job started with PID {}]\n", .{children.items[children.items.len - 1].pid});
    } else {
        for (children.items) |*child| {
            const term = try child.wait();
            switch (term) {
                .Exited => |code| {
                    if (code != 0 and num_commands == 1) {
                        try stdout_writer.print("Command exited with code: {}\n", .{code});
                    }
                },
                else => {},
            }
        }
    }
}

// Built-in commands
fn printHelp(stdout_writer: anytype) !void {
    try stdout_writer.writeAll(
        \\Built-in commands:
        \\  help              - Show this help message
        \\  exit              - Exit the shell
        \\  cd [dir]          - Change directory
        \\  pwd               - Print working directory
        \\  echo [args...]    - Print arguments
        \\  export VAR=value  - Set environment variable
        \\  env               - Show environment variables
        \\
        \\Features:
        \\  cmd1 | cmd2       - Pipe output
        \\  cmd > file        - Redirect output
        \\  cmd >> file       - Append output
        \\  cmd < file        - Redirect input
        \\  cmd &             - Background job
        \\
    );
}

fn builtinCd(_: std.mem.Allocator, args: [][]const u8, stdout_writer: anytype) !void {
    const target_dir = if (args.len > 1)
        args[1]
    else blk: {
        const home = std.posix.getenv("HOME") orelse {
            try stdout_writer.writeAll("cd: HOME not set\n");
            return;
        };
        break :blk home;
    };

    std.posix.chdir(target_dir) catch |err| {
        try stdout_writer.print("cd: {s}: {}\n", .{ target_dir, err });
    };
}

fn builtinPwd(_: std.mem.Allocator, stdout_writer: anytype) !void {
    var buf: [std.fs.max_path_bytes]u8 = undefined;
    const cwd = try std.posix.getcwd(&buf);
    try stdout_writer.print("{s}\n", .{cwd});
}

fn builtinEcho(args: [][]const u8, stdout_writer: anytype) !void {
    for (args[1..], 0..) |arg, i| {
        if (i > 0) try stdout_writer.writeAll(" ");
        try stdout_writer.writeAll(arg);
    }
    try stdout_writer.writeAll("\n");
}

fn builtinExport(allocator: std.mem.Allocator, args: [][]const u8, env_map: *std.process.EnvMap, stdout_writer: anytype) !void {
    if (args.len < 2) {
        try stdout_writer.writeAll("Usage: export VAR=value\n");
        return;
    }

    for (args[1..]) |arg| {
        if (std.mem.indexOf(u8, arg, "=")) |eq_pos| {
            const key = arg[0..eq_pos];
            const value = arg[eq_pos + 1 ..];

            const key_owned = try allocator.dupe(u8, key);
            const value_owned = try allocator.dupe(u8, value);

            if (env_map.fetchPut(key_owned, value_owned)) |old| {
                allocator.free(old.key);
                allocator.free(old.value);
            }

            try std.posix.setenv(key, value, 1);
        } else {
            try stdout_writer.print("export: invalid format: {s}\n", .{arg});
        }
    }
}

fn builtinEnv(env_map: *std.process.EnvMap, stdout_writer: anytype) !void {
    var it = env_map.iterator();
    while (it.next()) |entry| {
        try stdout_writer.print("{s}={s}\n", .{ entry.key_ptr.*, entry.value_ptr.* });
    }
}
