#include "core.h"
#include <cerrno>
#include <cstdio>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>
#include <fcntl.h>
namespace crtcli {
ProcessResult execute(const std::vector<std::string>& argv, unsigned timeout_seconds, bool relay) {
    require(!argv.empty(), "empty process argv");
    native_executable(argv.front());
#if defined(__linux__)
    // Allocate before fork; use only PAL/POSIX calls in the child before execve.
    std::vector<char*> raw;
    for (const auto& arg : argv) raw.push_back(const_cast<char*>(arg.c_str()));
    raw.push_back(nullptr);
    char path[] = "PATH=/nonexistent", locale[] = "LC_ALL=C";
    char* environment[] = {path, locale, nullptr};
    int descriptors[2];
    require(pipe(descriptors) == 0, "cannot create process output pipe");
    fcntl(descriptors[0], F_SETFD, FD_CLOEXEC);
    fcntl(descriptors[1], F_SETFD, FD_CLOEXEC);
    fcntl(descriptors[0], F_SETFL, O_NONBLOCK);
    pid_t pid = fork();
    if (pid < 0) { close(descriptors[0]); close(descriptors[1]); throw Error("fork failed"); }
    if (pid == 0) {
        if (setpgid(0, 0) != 0 || dup2(descriptors[1], STDOUT_FILENO) < 0) _exit(126);
        close(descriptors[0]); close(descriptors[1]);
        execve(raw[0], raw.data(), environment);
        _exit(127);
    }
    close(descriptors[1]);
    setpgid(pid, pid);
    timespec start{}, now{};
    clock_gettime(CLOCK_MONOTONIC, &start);
    int status = 0;
    bool timeout = false;
    bool truncated = false;
    std::string output;
    auto drain = [&]() {
        char buffer[4096]; ssize_t count;
        // Bound each drain so a flooding child cannot starve timeout checks.
        for (int reads = 0; reads < 64 && (count = read(descriptors[0], buffer, sizeof(buffer))) > 0; ++reads) {
            size_t remaining = 8 * 1024 * 1024 - output.size();
            size_t kept = static_cast<size_t>(count) < remaining ? static_cast<size_t>(count) : remaining;
            truncated |= kept < static_cast<size_t>(count);
            if (relay && kept) std::fwrite(buffer, 1, kept, stderr);
            output.append(buffer, kept);
        }
    };
    for (;;) {
        drain();
        pid_t state = waitpid(pid, &status, WNOHANG);
        if (state == pid) break;
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
    drain();
    close(descriptors[0]);
    if (truncated && relay) std::fprintf(stderr, "[child stdout truncated at 8 MiB]\n");
    int code = timeout ? 124 : WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
    return {code, timeout, output, truncated};
#else
    throw Error("process backend currently supports Linux native replay only");
#endif
}
}
