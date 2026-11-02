#include <crtcli/core.h>
#include <cerrno>
#include <cstdio>
#include <crtcli/environment.h>
#include <spawn.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>
#include <fcntl.h>
namespace crtcli {
ProcessResult execute(const std::vector<std::string>& argv, unsigned timeout_seconds, bool relay, bool host_tool, const std::vector<std::string>& runtime_path) {
    return execute_child(argv, child_environment(host_tool, runtime_path), {}, timeout_seconds, relay);
}
ProcessResult execute_child(const std::vector<std::string>& argv, const ChildEnvironment& child,
                            const fs::path& directory, unsigned timeout_seconds, bool relay) {
    require(!argv.empty(), "empty process argv");
    native_executable(argv.front());
    // posix_spawn is the one process-creation path on every host: CRT's Windows PAL provides it natively, while a
    // fork child needs an opt-in self-relaunch there. Everything is allocated before the call.
    std::vector<char*> raw;
    for (const auto& arg : argv) raw.push_back(const_cast<char*>(arg.c_str()));
    raw.push_back(nullptr);
    std::vector<char*> environment;
    for (const auto& value : child.values) environment.push_back(const_cast<char*>(value.c_str()));
    environment.push_back(nullptr);
    int descriptors[2];
    require(pipe(descriptors) == 0, "cannot create process output pipe");
    fcntl(descriptors[0], F_SETFD, FD_CLOEXEC);
    fcntl(descriptors[1], F_SETFD, FD_CLOEXEC);
    fcntl(descriptors[0], F_SETFL, O_NONBLOCK);
    posix_spawn_file_actions_t actions;
    posix_spawnattr_t attributes;
    require(posix_spawn_file_actions_init(&actions) == 0 && posix_spawnattr_init(&attributes) == 0, "cannot initialize process attributes");
    bool prepared = posix_spawn_file_actions_adddup2(&actions, descriptors[1], STDOUT_FILENO) == 0 &&
                    posix_spawn_file_actions_addclose(&actions, descriptors[0]) == 0 &&
                    posix_spawnattr_setflags(&attributes, POSIX_SPAWN_SETPGROUP) == 0 && posix_spawnattr_setpgroup(&attributes, 0) == 0;
    if (!directory.empty()) prepared = prepared && posix_spawn_file_actions_addchdir_np(&actions, directory.string().c_str()) == 0;
    pid_t pid = -1;
    int launch = prepared ? posix_spawn(&pid, raw[0], &actions, &attributes, raw.data(), environment.data()) : EINVAL;
    posix_spawn_file_actions_destroy(&actions);
    posix_spawnattr_destroy(&attributes);
    if (launch != 0) { close(descriptors[0]); close(descriptors[1]); throw Error("cannot start process: " + argv.front() + " (error " + std::to_string(launch) + ")"); }
    close(descriptors[1]);
    timespec start{}, now{};
    clock_gettime(CLOCK_MONOTONIC, &start);
    int status = 0;
    bool timeout = false;
    bool truncated = false;
    std::string output;
    auto drain = [&]() -> size_t {
        char buffer[4096]; ssize_t count; size_t total = 0;
        // Bound each drain so a flooding child cannot starve timeout checks.
        for (int reads = 0; reads < 64 && (count = read(descriptors[0], buffer, sizeof(buffer))) > 0; ++reads) {
            size_t remaining = 8 * 1024 * 1024 - output.size();
            size_t kept = static_cast<size_t>(count) < remaining ? static_cast<size_t>(count) : remaining;
            truncated |= kept < static_cast<size_t>(count);
            if (relay && kept) std::fwrite(buffer, 1, kept, stderr);
            output.append(buffer, kept);
            total += static_cast<size_t>(count);
        }
        return total;
    };
    for (;;) {
        drain();
        pid_t state = waitpid(pid, &status, WNOHANG);
        if (state == pid) {
            // A completed owned job must not leave background descendants alive.
            // Each launch owns its own process group; never kill by executable name.
            kill(-pid, SIGKILL);
            break;
        }
        if (state < 0 && errno != EINTR) { close(descriptors[0]); throw Error("waitpid failed"); }
        clock_gettime(CLOCK_MONOTONIC, &now);
        if (now.tv_sec - start.tv_sec >= timeout_seconds) {
            timeout = true;
            kill(-pid, SIGKILL);
            kill(pid, SIGKILL);
            while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {}
            break;
        }
        timespec pause{0, 10000000}; nanosleep(&pause, nullptr);
    }
    // The child is gone but its pipe may still hold more than one bounded drain (a Windows pipe buffers far more than 64 KiB).
    while (drain() > 0) {}
    close(descriptors[0]);
    if (truncated && relay) std::fprintf(stderr, "[child stdout truncated at 8 MiB]\n");
    int code = timeout ? 124 : WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
    return {code, timeout, output, truncated};
}
}
