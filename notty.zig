const std = @import("std");

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const allocator = gpa.allocator();

    // Zig 0.15 I/O
    var stdout_buffer: [4096]u8 = undefined;
    var stdout_writer = std.fs.File.stdout().writer(&stdout_buffer);
    const stdout = &stdout_writer.interface;

    // Use raw file for stdin to avoid buffering conflicts with child processes
    const stdin_file = std.fs.File.stdin();

    try stdout.writeAll("Minimal Terminal Emulator\n");
    try stdout.writeAll("Type 'exit' to quit\n\n");
    try stdout.flush();

    var line_buf: [1024]u8 = undefined;

    while (true) {
        try stdout.writeAll("$ ");
        try stdout.flush();

        // Read line manually using raw read
        var len: usize = 0;
        while (len < line_buf.len) {
            var byte: [1]u8 = undefined;
            const n = stdin_file.read(&byte) catch break;
            if (n == 0) break; // EOF
            if (byte[0] == '\n') break;
            line_buf[len] = byte[0];
            len += 1;
        }

        if (len == 0) {
            // Check if we got EOF
            var check: [1]u8 = undefined;
            const n = stdin_file.read(&check) catch break;
            if (n == 0) break;
            continue;
        }

        const trimmed = std.mem.trim(u8, line_buf[0..len], &std.ascii.whitespace);

        if (trimmed.len == 0) continue;
        if (std.mem.eql(u8, trimmed, "exit")) break;

        // Parse command and arguments
        var args: std.ArrayListUnmanaged([]const u8) = .empty;
        defer args.deinit(allocator);

        var iter = std.mem.tokenizeScalar(u8, trimmed, ' ');
        while (iter.next()) |arg| {
            try args.append(allocator, arg);
        }

        if (args.items.len == 0) continue;

        // Execute command
        var child = std.process.Child.init(args.items, allocator);
        child.stdout_behavior = .Inherit;
        child.stderr_behavior = .Inherit;
        child.stdin_behavior = .Inherit;

        const term = child.spawnAndWait() catch |err| {
            try stdout.print("Error executing command: {any}\n", .{err});
            try stdout.flush();
            continue;
        };

        switch (term) {
            .Exited => |code| {
                if (code != 0) {
                    try stdout.print("Command exited with code: {}\n", .{code});
                    try stdout.flush();
                }
            },
            .Signal => |sig| {
                try stdout.print("Command terminated by signal: {}\n", .{sig});
                try stdout.flush();
            },
            .Stopped => |sig| {
                try stdout.print("Command stopped by signal: {}\n", .{sig});
                try stdout.flush();
            },
            .Unknown => |code| {
                try stdout.print("Command exited with unknown code: {}\n", .{code});
                try stdout.flush();
            },
        }
    }

    try stdout.writeAll("Goodbye!\n");
    try stdout.flush();
}