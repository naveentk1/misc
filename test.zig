const std = @import("std");
const mem = std.mem;

const Builtin = enum {
    cd,
    exit,
    unknown,

    pub fn fromString(s: []const u8) Builtin {
        if (mem.eql(u8, s, "cd")) return .cd;
        if (mem.eql(u8, s, "exit")) return .exit;
        return .unknown;
    }
};

pub fn main() !void {
    // 1. Memory Management
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const allocator = gpa.allocator();

    // 2. Setup I/O (Zig 0.15.x Style)
    // We need explicit buffers.
    var stdin_buf: [4096]u8 = undefined;
    var stdout_buf: [4096]u8 = undefined;

    // A. Create the Buffered Implementation
    // These structs handle the actual memory/buffering but don't have 'print'
    var stdin_impl = std.fs.File.stdin().reader(&stdin_buf);
    var stdout_impl = std.fs.File.stdout().writer(&stdout_buf);

    // B. Get the Interface
    // The '.interface' field gives us the generic Reader/Writer with methods like 'print' and 'readUntil...'
    const stdin = &stdin_impl.interface;
    const stdout = &stdout_impl.interface;

    var input_buffer: [1024]u8 = undefined;

    while (true) {
        var cwd_buf: [1024]u8 = undefined;
        const cwd = try std.process.getCwd(&cwd_buf);

        // Use 'stdout' (the interface) to print
        try stdout.print("\x1b[32m{s}\x1b[0m \x1b[36mzigsh>\x1b[0m ", .{cwd});
        
        // Use 'stdout_impl' (the implementation) to flush the buffer
        try stdout_impl.flush();

        // Use 'stdin' (the interface) to read
        const raw_input = try stdin.readUntilDelimiterOrEof(&input_buffer, '\n');
        if (raw_input == null) break;

        const input = mem.trim(u8, raw_input.?, " \r\t");
        if (input.len == 0) continue;

        var args = std.ArrayList([]const u8).init(allocator);
        defer args.deinit();

        var iterator = mem.splitScalar(u8, input, ' ');
        while (iterator.next()) |arg| {
            if (arg.len > 0) try args.append(arg);
        }

        if (args.items.len == 0) continue;

        const command = args.items[0];
        const cmd_type = Builtin.fromString(command);

        switch (cmd_type) {
            .cd => {
                if (args.items.len < 2) {
                    try stdout.print("cd: missing argument\n", .{});
                } else {
                    const path = args.items[1];
                    std.os.chdir(path) catch |err| {
                        try stdout.print("cd: {s}: {s}\n", .{ path, @errorName(err) });
                    };
                }
                try stdout_impl.flush();
            },
            .exit => {
                try stdout.print("Goodbye!\n", .{});
                try stdout_impl.flush();
                break;
            },
            .unknown => {
                try stdout_impl.flush(); // Clear buffer before child process takes over
                
                var child_proc = std.process.Child.init(args.items, allocator);
                
                _ = child_proc.spawnAndWait() catch |err| {
                    if (err == error.FileNotFound) {
                        // Ignore errors on print to avoid crashing the shell loop
                        stdout.print("zigsh: command not found: {s}\n", .{command}) catch {};
                    } else {
                        stdout.print("zigsh: error executing command: {s}\n", .{@errorName(err)}) catch {};
                    }
                    stdout_impl.flush() catch {};
                };
            },
        }
    }
}