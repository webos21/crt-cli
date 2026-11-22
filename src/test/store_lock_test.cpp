// Store lock (include/crtcli/store_lock.h): mutual exclusion across processes, and the earlier directory layout.
// The stress case re-runs this executable as worker processes that compete for one lock file. Each worker enters the
// critical section many times and checks, with an exclusive marker file, that no other worker is inside. This is the
// regression for the lock defects traced on Windows/arm64 on 2026-11-22: fatal create/remove contention errors and
// stolen live locks (two holders at once). It takes a few seconds; CRT_CLI_TEST_LOCK_ITERATIONS and
// CRT_CLI_TEST_LOCK_WORKERS scale it.
#include <crtcli/store_lock.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <spawn.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
using namespace crtcli;
namespace {
size_t checks = 0;
void check(bool condition, const std::string& message) { require(condition, "test failed: " + message); ++checks; }
std::string error_of(const std::function<void()>& action) { try { action(); } catch (const Error& e) { return e.what(); } return {}; }
long number_from_env(const char* name, long fallback) {
    const char* value = std::getenv(name);
    return value ? std::strtol(value, nullptr, 10) : fallback;
}

// Worker: returns 0 when every critical section was exclusive, 2 on a violation, 1 on any other failure.
int worker(const fs::path& directory, long iterations) {
    try {
        const auto marker = directory / "inside";
        long violations = 0;
        for (long i = 0; i < iterations; ++i) {
            Lock lock(directory / "work.lock", 300);
            int fd = open(marker.string().c_str(), O_CREAT | O_EXCL | O_WRONLY, 0644);
            if (fd < 0) { ++violations; continue; }  // another holder is inside: the lock did not exclude
            close(fd);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            std::error_code ignored;
            fs::remove(marker, ignored);
        }
        return violations ? 2 : 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "lock worker failed: %s\n", error.what());
        return 1;
    }
}
}  // namespace

int main(int argc, char** argv) {
    try {
        if (argc == 4 && std::string(argv[1]) == "--worker") return worker(argv[2], std::strtol(argv[3], nullptr, 10));
        const fs::path scratch = fs::temp_directory_path() / ("crtcli-store-lock-test-" + std::to_string(current_pid()));
        fs::remove_all(scratch);
        fs::create_directories(scratch);

        // ---- one process: acquire, release, busy -------------------------------------------------------------------------
        {
            const auto path = scratch / "basic" / "a.lock";
            {
                Lock first(path, 1);
                check(fs::is_regular_file(path), "the lock is a file");
                // Windows locks are mandatory: the held file cannot be read through another handle, so the PID is not checked here.
            }
            Lock again(path, 1);
            check(true, "a released lock is acquired again");
            again.release();
            again.release();
            check(true, "release is idempotent");
        }

        // ---- an earlier directory-style lock ----------------------------------------------------------------------------
        {
            const auto live = scratch / "legacy" / "live.lock";
            fs::create_directories(live);
            write_text(live / "owner", std::to_string(current_pid()) + "\n");
            check(error_of([&] { Lock lock(live, 1); }).find("in progress") != std::string::npos, "a live owner's directory lock is waited on, never stolen");
            check(fs::exists(live / "owner"), "the live directory lock is untouched");

            const auto fresh = scratch / "legacy" / "fresh.lock";
            fs::create_directories(fresh);  // no owner yet: its creator may still be writing it
            check(error_of([&] { Lock lock(fresh, 1); }).find("in progress") != std::string::npos, "a young ownerless directory lock is not stolen");

            const auto dead = scratch / "legacy" / "dead.lock";
            fs::create_directories(dead);
            write_text(dead / "owner", "4194000\n");
            { Lock lock(dead, 5); check(fs::is_regular_file(dead), "a dead owner's directory lock is replaced by a lock file"); }
        }

        // ---- many processes, one lock ---------------------------------------------------------------------------------------
        {
            const long workers = number_from_env("CRT_CLI_TEST_LOCK_WORKERS", 4);
            const long iterations = number_from_env("CRT_CLI_TEST_LOCK_ITERATIONS", 150);
            const auto directory = scratch / "stress";
            fs::create_directories(directory);
            auto self = fs::canonical(argv[0]).string();
            std::string mode = "--worker", target = directory.string(), count = std::to_string(iterations);
            char* worker_argv[] = {self.data(), mode.data(), target.data(), count.data(), nullptr};
            std::vector<pid_t> children(static_cast<size_t>(workers), -1);
            std::vector<int> launch_errors(static_cast<size_t>(workers), 0);
            for (size_t i = 0; i < children.size(); ++i) launch_errors[i] = posix_spawn(&children[i], self.c_str(), nullptr, nullptr, worker_argv, environ);
            std::string results;
            bool all_clean = true;
            for (size_t i = 0; i < children.size(); ++i) {
                int code = -1;
                if (launch_errors[i] == 0) {
                    int status;
                    pid_t waited;
                    do { waited = waitpid(children[i], &status, 0); } while (waited < 0 && errno == EINTR);
                    if (waited == children[i] && WIFEXITED(status)) code = WEXITSTATUS(status);
                }
                results += (i ? "/" : "") + std::to_string(code);
                all_clean = all_clean && code == 0;
            }
            check(all_clean, std::to_string(workers) + " processes x " + std::to_string(iterations) +
                             " acquisitions are exclusive and error-free (exit " + results + ")");
            check(!fs::exists(directory / "inside"), "no worker is left inside the critical section");
        }
        std::error_code ignored;
        fs::remove_all(scratch, ignored);
        std::printf("store-lock-test: %zu checks passed\n", checks);
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "store-lock-test: %s\n", error.what());
        return 1;
    }
}
